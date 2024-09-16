/*
 * Copyright (c) 2016 MariaDB Corporation Ab
 * Copyright (c) 2023 MariaDB plc, Finnish Branch
 *
 * Use of this software is governed by the Business Source License included
 * in the LICENSE.TXT file and at www.mariadb.com/bsl11.
 *
 * Change Date: 2027-09-09
 *
 * On the date above, in accordance with the Business Source License, use
 * of this software will be governed by version 2 or later of the General
 * Public License.
 */

#define MXB_MODULE_NAME "masking"
#include "maskingfilter.hh"

#include <maxscale/json_api.hh>
#include <maxscale/modulecmd.hh>
#include <maxscale/paths.hh>
#include <maxscale/utils.hh>

#include "maskingrules.hh"

using std::shared_ptr;
using std::string;

namespace
{

char VERSION_STRING[] = "V1.0.0";

/**
 * Implement "call command masking reload ..."
 *
 * @param pArgs  The arguments of the command.
 *
 * @return True, if the command was handled.
 */
bool masking_command_reload(const ModuleCmdArgs& pArgs, json_t** output)
{
    mxb_assert(pArgs.size() == 1);
    mxb_assert(pArgs[0].type == mxs::modulecmd::ArgType::FILTER);

    const MXS_FILTER_DEF* pFilterDef = pArgs[0].filter;
    mxb_assert(pFilterDef);
    MaskingFilter* pFilter = reinterpret_cast<MaskingFilter*>(filter_def_get_instance(pFilterDef));

    bool rv = false;
    MXS_EXCEPTION_GUARD(rv = pFilter->reload());

    if (!rv)
    {
        MXB_ERROR("Could not reload the rules.");
    }

    return rv;
}
}

//
// Global symbols of the Module
//

extern "C" MXS_MODULE* MXS_CREATE_MODULE()
{
    std::vector<ModuleCmdArgDesc> reload_argv =
    {
        {mxs::modulecmd::ArgType::FILTER, mxs::modulecmd::ARG_NAME_MATCHES_DOMAIN, "Masking name"}
    };

    modulecmd_register_command(MXB_MODULE_NAME,
                               "reload",
                               mxs::modulecmd::CmdType::WRITE,
                               masking_command_reload,
                               reload_argv,
                               "Reload masking filter rules");

    static MXS_MODULE info =
    {
        mxs::MODULE_INFO_VERSION,
        MXB_MODULE_NAME,
        mxs::ModuleType::FILTER,
        mxs::ModuleStatus::GA,
        MXS_FILTER_VERSION,
        "A masking filter that is capable of masking/obfuscating returned column values.",
        "V1.0.0",
        RCAP_TYPE_STMT_INPUT | RCAP_TYPE_STMT_OUTPUT,
        &mxs::FilterApi<MaskingFilter>::s_api,
        NULL,   /* Process init. */
        NULL,   /* Process finish. */
        NULL,   /* Thread init. */
        NULL,   /* Thread finish. */
    };

    static bool populated = false;

    if (!populated)
    {
        MaskingFilterConfig::populate(info);
        populated = true;
    }

    return &info;
}

//
// MaskingFilter
//

MaskingFilter::MaskingFilter(const char* zName)
    : m_config(zName, *this)
{
    MXB_NOTICE("Masking filter [%s] created.", m_config.name().c_str());
}

MaskingFilter::~MaskingFilter()
{
}

// static
std::unique_ptr<mxs::Filter> MaskingFilter::create(const char* zName)
{
    return std::make_unique<MaskingFilter>(zName);
}

std::shared_ptr<mxs::FilterSession> MaskingFilter::newSession(MXS_SESSION* pSession, SERVICE* pService)
{
    return std::make_shared<MaskingFilterSession>(pSession, pService, this);
}

// static
json_t* MaskingFilter::diagnostics() const
{
    return NULL;
}

// static
uint64_t MaskingFilter::getCapabilities() const
{
    return RCAP_TYPE_STMT_INPUT | RCAP_TYPE_STMT_OUTPUT;
}

bool MaskingFilter::reload()
{
    bool rval = m_config.reload_rules();
    const auto& cnf = config();

    if (rval)
    {
        MXB_NOTICE("Rules for masking filter '%s' were reloaded from '%s'.",
                   m_config.name().c_str(), cnf.rules.c_str());
    }
    else
    {
        MXB_ERROR("Rules for masking filter '%s' could not be reloaded from '%s'.",
                  m_config.name().c_str(), cnf.rules.c_str());
    }

    return rval;
}
