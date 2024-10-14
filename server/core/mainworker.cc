/*
 * Copyright (c) 2018 MariaDB Corporation Ab
 * Copyright (c) 2023 MariaDB plc, Finnish Branch
 *
 * Use of this software is governed by the Business Source License included
 * in the LICENSE.TXT file and at www.mariadb.com/bsl11.
 *
 * Change Date: 2025-09-12
 *
 * On the date above, in accordance with the Business Source License, use
 * of this software will be governed by version 2 or later of the General
 * Public License.
 */

#include <maxscale/mainworker.hh>

#include <signal.h>
#include <vector>
#include <fcntl.h>
#ifdef HAVE_SYSTEMD
#include <systemd/sd-daemon.h>
#endif
#include <maxscale/cn_strings.hh>
#include <maxscale/config.hh>
#include <maxscale/routingworker.hh>

#include "internal/admin.hh"
#include "internal/configmanager.hh"
#include "internal/http_sql.hh"
#include "internal/listener.hh"
#include "internal/modules.hh"
#include "internal/monitormanager.hh"
#include "internal/service.hh"

namespace
{

static struct ThisUnit
{
    maxscale::MainWorker* pMain = nullptr;
    int64_t               clock_ticks;
} this_unit;

thread_local struct ThisThread
{
    maxscale::MainWorker* pMain = nullptr;
} this_thread;
}

namespace maxscale
{

MainWorker::MainWorker(mxb::WatchdogNotifier* pNotifier)
    : mxb::WatchedWorker(pNotifier)
    , m_callable(this)
{
    mxb_assert(!this_unit.pMain);

    this_unit.pMain = this;
    // Actually, pMain should be set in pre_run() and cleared in post_run(),
    // but when set here and cleared in the destructor, we will appear to be
    // running in the MainWorker also when MainWorker::run() has returned, which
    // is conceptually ok as the main worker runs in the main thread that stays
    // around until the program ends.
    this_thread.pMain = this;
}

MainWorker::~MainWorker()
{
    m_callable.cancel_dcalls();

    mxb_assert(this_unit.pMain);
    this_thread.pMain = nullptr;
    this_unit.pMain = nullptr;
}

// static
bool MainWorker::created()
{
    return this_unit.pMain ? true : false;
}

// static
MainWorker* MainWorker::get()
{
    return this_unit.pMain;
}

// static
int64_t MainWorker::ticks()
{
    return mxb::atomic::load(&this_unit.clock_ticks, mxb::atomic::RELAXED);
}

// static
bool MainWorker::is_main_worker()
{
    return this_thread.pMain != nullptr;
}

void MainWorker::update_rebalancing()
{
    mxb_assert(is_main_worker());

    // MainWorker must be running
    if (get_current() == nullptr)
    {
        return;
    }

    const auto& config = mxs::Config::get();

    std::chrono::milliseconds period = config.rebalance_period.get();

    if (m_rebalancing_dc == 0 && period != 0ms)
    {
        // If the rebalancing delayed call is not currently active and the
        // period is now != 0, then we order a delayed call.
        order_balancing_dc();
    }
    else if (m_rebalancing_dc != 0 && period == 0ms)
    {
        // If the rebalancing delayed call is currently active and the
        // period is now 0, then we cancel the call, effectively shutting
        // down the rebalancing.
        m_callable.cancel_dcall(m_rebalancing_dc);
        m_rebalancing_dc = 0;
    }
}

bool MainWorker::pre_run()
{
    bool rval = false;

    if (pipe2(m_signal_pipe, O_NONBLOCK | O_CLOEXEC) == -1)
    {
        MXB_ERROR("Failed to create pipe for signals: %d, %s", errno, mxb_strerror(errno));
    }
    else
    {
        add_pollable(EPOLLIN, &m_signal_handler);

        m_callable.dcall(100ms, &MainWorker::inc_ticks);
        update_rebalancing();

        if (modules_thread_init())
        {
            if (qc_thread_init(QC_INIT_SELF))
            {
                rval = true;
                // Disable qcc in the main worker. This affects no other workers.
                // QC needed in MainWorker due to "maxctrl classify".
                qc_use_local_cache(false);
            }
            else
            {
                modules_thread_finish();
            }
        }

        if (rval)
        {
            const auto& auto_tune = Config::get().auto_tune;

            if (!auto_tune.empty())
            {
                if (auto_tune.size() == 1 && auto_tune.front() == CN_ALL)
                {
                    const auto& server_dependencies = Service::specification()->server_dependencies();

                    for (const auto* pDependency : server_dependencies)
                    {
                        m_tunables.insert(pDependency->parameter().name());
                    }
                }
                else
                {
                    for (const auto& parameter : auto_tune)
                    {
                        m_tunables.insert(parameter);
                    }
                }

                MXB_NOTICE("The following parameters will be auto tuned: %s",
                           mxb::join(m_tunables, ", ", "'").c_str());

                m_callable.dcall(5s, [this]() {
                    check_dependencies_dc();
                    return true;
                });
            }
            else
            {
                MXB_INFO("No 'auto_tune' parameters specified, no auto tuning will be performed.");
            }
        }
        else
        {
            close(m_signal_pipe[0]);
            close(m_signal_pipe[1]);
            m_signal_pipe[0] = -1;
            m_signal_pipe[1] = -1;
        }
    }

    return rval;
}

void MainWorker::post_run()
{
    mxb_assert(m_signal_pipe[0] != -1 && m_signal_pipe[1] != -1);
    remove_pollable(&m_signal_handler);
    close(m_signal_pipe[0]);
    close(m_signal_pipe[1]);

    // Clearing the storage right after the main loop returns guarantees that both the MainWorker and the
    // RoutingWorkers are alive when stored data is destroyed. Without this, the destruction of filters is
    // delayed until the MainWorker is destroyed which is something that must be avoided. All objects in
    // MaxScale should be destroyed before the workers are destroyed.
    m_storage.clear();

    qc_thread_end(QC_INIT_SELF);
    modules_thread_finish();
}

// static
bool MainWorker::inc_ticks(Worker::Callable::Action action)
{
    if (action == Callable::EXECUTE)
    {
        mxb::atomic::add(&this_unit.clock_ticks, 1, mxb::atomic::RELAXED);
    }

    return true;
}

bool MainWorker::balance_workers(BalancingApproach approach, int threshold)
{
    bool rebalanced = false;

    const auto& config = mxs::Config::get();

    if (threshold == -1)
    {
        threshold = config.rebalance_threshold.get();
    }

    RoutingWorker::collect_worker_load(config.rebalance_window.get());

    std::chrono::milliseconds period = config.rebalance_period.get();

    mxb::TimePoint now = epoll_tick_now();

    if (approach == BALANCE_UNCONDITIONALLY
        || now - m_last_rebalancing >= period)
    {
        rebalanced = RoutingWorker::balance_workers(threshold);
        m_last_rebalancing = now;
    }

    return rebalanced;
}

bool MainWorker::balance_workers_dc()
{
    balance_workers(BALANCE_ACCORDING_TO_PERIOD);

    return true;
}

void MainWorker::order_balancing_dc()
{
    mxb_assert(m_rebalancing_dc == 0);

    m_rebalancing_dc = m_callable.dcall(1000ms, &MainWorker::balance_workers_dc, this);
}

void MainWorker::check_dependencies_dc()
{
    auto services = Service::get_all();

    for (auto* pService : services)
    {
        pService->check_server_dependencies(m_tunables);
    }
}

// static
void MainWorker::start_shutdown()
{
    auto func = []() {
            // Stop all monitors and listeners to prevent any state changes during shutdown and to prevent the
            // creation of new sessions. Stop the REST API to prevent any conflicting changes from being
            // executed while we're shutting down.
            MonitorManager::stop_all_monitors();
            if (mxs::Config::get().admin_enabled)
            {
                mxs_admin_shutdown();
                // Stop cleanup-thread only after rest-api is shut down, so that no queries are active.
                HttpSql::stop_cleanup();
            }
            mxs::ConfigManager::get()->stop_sync();
            Listener::stop_all();
            Service::shutdown();

            // The RoutingWorkers proceed with the shutdown on their own. Once all sessions have closed, they
            // will exit the event loop.
            mxs::RoutingWorker::start_shutdown();

            // Wait until RoutingWorkers have stopped before proceeding with MainWorker shudown
            auto self = MainWorker::get();
            self->m_callable.dcall(100ms, &MainWorker::wait_for_shutdown, self);
        };

    MainWorker::get()->execute(func, EXECUTE_QUEUED);
}

bool MainWorker::wait_for_shutdown()
{
    bool again = true;

    if (RoutingWorker::shutdown_complete())
    {
        shutdown();
        again = false;
    }

    return again;
}

uint32_t MainWorker::SignalHandler::handle_poll_events(mxb::Worker* worker,
                                                       uint32_t events,
                                                       Pollable::Context context)
{
    mxb_assert(this_unit.pMain == worker);
    static_cast<MainWorker*>(worker)->read_signal_from_pipe();
    return events;
}

int MainWorker::SignalHandler::poll_fd() const
{
    // Can't use polling_worker() here: the value is not set when this functions is first called.
    return this_unit.pMain->m_signal_pipe[0];
}

bool MainWorker::execute_signal_safe(void (* func)(void))
{
    intptr_t val = (intptr_t)func;
    int rc = write(m_signal_pipe[1], &val, sizeof(val));
    return rc == sizeof(val);
}

void MainWorker::read_signal_from_pipe()
{
    intptr_t val;

    while (read(m_signal_pipe[0], &val, sizeof(val)) == sizeof(val))
    {
        ((void (*)(void)) val)();
    }
}
}

int64_t mxs_clock()
{
    return mxs::MainWorker::ticks();
}
