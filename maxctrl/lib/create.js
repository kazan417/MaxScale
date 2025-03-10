/*
 * Copyright (c) 2016 MariaDB Corporation Ab
 * Copyright (c) 2023 MariaDB plc, Finnish Branch
 *
 * Use of this software is governed by the Business Source License included
 * in the LICENSE.TXT file and at www.mariadb.com/bsl11.
 *
 * Change Date: 2027-04-10
 *
 * On the date above, in accordance with the Business Source License, use
 * of this software will be governed by version 2 or later of the General
 * Public License.
 */
const {
  maxctrl,
  _,
  warning,
  parseValue,
  doRequest,
  getJson,
  OK,
  helpMsg,
  paramToRelationship,
  targetToRelationships,
  setMonitorRelationship,
} = require("./common.js");
const fs = require("fs");
const os = require("os");
const tar = require("tar-stream");

function isKV(val) {
  return typeof val == "string" && val.indexOf("=") != -1;
}

// Converts a key=value string into an object
function to_obj(obj, value) {
  var pos = value.indexOf("=");
  _.set(obj, value.slice(0, pos), parseValue(value.slice(pos + 1)));
  return obj;
}

function checkName(name) {
  if (name.match(/[^a-zA-Z0-9_.~-]/)) {
    warning("The name '" + name + "' contains URL-unsafe characters.");
  }
}

function validateParams(argv, params) {
  var rval = null;
  params.forEach((value) => {
    if (!isKV(value)) {
      rval = "Not a key-value parameter: " + value;
    }
  });

  return rval;
}

async function readAllEndpoints(host, log_lines) {
  var endpoints = [
    { endpoint: "maxscale", name: "maxscale" },
    { endpoint: "servers", name: "servers" },
    { endpoint: "services", name: "services" },
    { endpoint: "monitors", name: "monitors" },
    { endpoint: "listeners", name: "listeners" },
    { endpoint: "filters", name: "filters" },
    { endpoint: "sessions", name: "sessions" },
    { endpoint: "users", name: "users" },
    { endpoint: "maxscale/modules", name: "modules" },
    { endpoint: "maxscale/query_classifier", name: "query_classifier" },
    { endpoint: "maxscale/threads", name: "threads" },
    { endpoint: "maxscale/logs/data", name: "logs", options: "?page[size]=" + log_lines },
    { endpoint: "maxscale/debug/server_diagnostics", name: "server_diagnostics" },
  ];

  var data = {};

  for (const e of endpoints) {
    data[e.name] = await getJson(host, e.endpoint + (e.options ? e.options : ""));
  }

  return data;
}

function readAndSanitizeFile(filename) {
  var data = fs.readFileSync(filename);

  if (filename.endsWith(".cnf")) {
    data = String(data).replace(/(password|passwd)\S*=.*/g, "$1=*****");
  }

  return data;
}

async function createSupportReport(host, argv) {
  if (!host.match(/127.0.0.1|localhost/)) {
    return error(
      "The --archive option must be used on the MaxScale server and the --hosts must point to localhost or 127.0.0.1."
    );
  }

  var output = process.stdout;

  if (argv.file) {
    var outfile = argv.file;

    if (!outfile.endsWith(".tar")) {
      outfile += ".tar";
    }

    if (fs.existsSync(outfile)) {
      return error(`File already exists: ${outfile}`);
    }

    output = fs.createWriteStream(outfile);
  }

  const prefix = "report";

  // Read the data from the REST-API as well as the cpuinfo and meminfo from the local machine.
  var data = await readAllEndpoints(host, argv.lines);
  var pack = tar.pack();
  pack.entry({ name: prefix + "/maxctrl-report.json" }, JSON.stringify(data, null, 2));
  pack.entry({ name: prefix + "/meminfo" }, fs.readFileSync("/proc/meminfo"));
  pack.entry({ name: prefix + "/cpuinfo" }, fs.readFileSync("/proc/cpuinfo"));

  // Collect the files from the default locations. If they do not exist, they are not
  // included in the tarball.
  const static_files = ["/etc/maxscale.cnf", "/var/log/maxscale/maxscale.log", "/etc/os-release"];
  const static_dirs = ["/etc/maxscale.cnf.d/", "/var/lib/maxscale/maxscale.cnf.d/"];

  for (let f of static_files) {
    if (fs.existsSync(f)) {
      pack.entry({ name: prefix + f }, readAndSanitizeFile(f));
    }
  }

  for (let dir of static_dirs) {
    if (fs.existsSync(dir)) {
      for (let f of fs.readdirSync(dir)) {
        const file_in_dir = dir + "/" + f;
        if (fs.existsSync(file_in_dir)) {
          pack.entry({ name: prefix + file_in_dir }, readAndSanitizeFile(file_in_dir));
        }
      }
    }
  }

  pack.pipe(output);
  // If we're piping to stdout, return an empty string so we don't write any extra data to it.
  return argv.file ? OK() : "";
}

exports.command = "create <command>";
exports.desc = "Create objects";
exports.handler = function () {};
exports.builder = function (yargs) {
  yargs
    // Create server
    .command(
      "server <name> <params...>",
      "Create a new server",
      function (yargs) {
        return yargs
          .epilog(
            `
The configuration parameters for the created object must be passed as 'key=value' pairs.
For example, to create a server that listens on port 3306 at 192.168.0.123, the following
command can be used:

    create server MyServer address=192.168.0.123 port=3306


To use the server in a service or a monitor, use the --services or --monitors options
or use the 'link' commands.


The legacy syntax where the address and port are given as the second and third arguments
has been deprecated in MaxScale 24.02.

Legacy syntax: create server <name> <host|socket> [port] [params...]

If the first two arguments are not 'key=value' pairs, the command is interpreted with
the legacy syntax. In this mode, if the <host|socket> argument is an absolute path,
the server will use a local UNIX domain socket connection. In this case the third [port]
argument is ignored.
`
          )
          .usage("Usage: create server <name> [params...]")
          .group(["services", "monitors"], "Create server options:")
          .option("services", {
            describe: "Link the created server to these services",
            type: "array",
          })
          .option("monitors", {
            describe: "Link the created server to these monitors",
            type: "array",
          });
      },
      function (argv) {
        maxctrl(argv, function (host) {
          var server = {
            data: {
              id: String(argv.name),
              type: "servers",
              attributes: {
                parameters: {},
              },
            },
          };

          if (argv.services) {
            for (let i = 0; i < argv.services.length; i++) {
              _.set(server, "data.relationships.services.data[" + i + "]", {
                id: String(argv.services[i]),
                type: "services",
              });
            }
          }

          if (argv.monitors) {
            for (let i = 0; i < argv.monitors.length; i++) {
              _.set(server, "data.relationships.monitors.data[" + i + "]", {
                id: String(argv.monitors[i]),
                type: "monitors",
              });
            }
          }

          var params = argv.params;

          if (!isKV(params[0])) {
            const host = params[0];
            const port = params.length == 1 ? null : argv.params[1];
            params = params.slice(2);

            if (host[0] == "/") {
              server.data.attributes.parameters.socket = host;
            } else {
              server.data.attributes.parameters.address = host;
              server.data.attributes.parameters.port = port;
            }
          }

          var err = validateParams(argv, params);
          var extra_params = params.reduce(to_obj, {});
          Object.assign(server.data.attributes.parameters, extra_params);

          checkName(argv.name);

          if (err) {
            return Promise.reject(err);
          }

          return doRequest(host, "servers", { method: "POST", data: server });
        });
      }
    )

    // Create monitor
    .command(
      "monitor <name> <module> [params...]",
      "Create a new monitor",
      function (yargs) {
        return yargs
          .epilog(
            "The list of servers given with the --servers option should not " +
              "contain any servers that are already monitored by another monitor. " +
              "The last argument to this command is a list of key=value parameters " +
              "given as the monitor parameters. " +
              "The redundant option parameters have been deprecated in MaxScale 22.08."
          )
          .usage("Usage: create monitor <name> <module> [params...]")
          .group(["servers"], "Create monitor options:")
          .option("servers", {
            describe:
              "Link the created monitor to these servers. All non-option arguments " +
              "after --servers are interpreted as server names e.g. `--servers srv1 srv2 srv3`.",
            type: "array",
          });
      },
      function (argv) {
        var monitor = {
          data: {
            id: String(argv.name),
            attributes: {
              module: argv.module,
            },
          },
        };

        var err = false;

        err = validateParams(argv, argv.params);
        var params = argv.params.reduce(to_obj, {});

        if (params.servers) {
          paramToRelationship(monitor, params.servers.split(","), "servers");
          delete params.servers;
        }

        monitor.data.attributes.parameters = params;

        if (argv.servers) {
          for (let i = 0; i < argv.servers.length; i++) {
            _.set(monitor, "data.relationships.servers.data[" + i + "]", {
              id: String(argv.servers[i]),
              type: "servers",
            });
          }
        }

        maxctrl(argv, function (host) {
          checkName(argv.name);

          if (err) {
            return Promise.reject(err);
          }
          return doRequest(host, "monitors", { method: "POST", data: monitor });
        });
      }
    )

    // Create service
    .command(
      "service <name> <router> <params...>",
      "Create a new service",
      function (yargs) {
        return yargs
          .epilog(
            "The last argument to this command is a list of key=value parameters " +
              "given as the service parameters. If the --servers, --services or " +
              "--filters options are used, they must be defined after the service parameters. " +
              "The --cluster option is mutually exclusive with the --servers and --services options." +
              "\n\nNote that the `user` and `password` parameters must be defined."
          )
          .usage("Usage: service <name> <router> <params...>")
          .group(["servers", "filters", "services", "cluster"], "Create service options:")
          .option("servers", {
            describe:
              "Link the created service to these servers. All non-option arguments " +
              "after --servers are interpreted as server names e.g. `--servers srv1 srv2 srv3`.",
            type: "array",
          })
          .option("services", {
            describe:
              "Link the created service to these services. All non-option arguments " +
              "after --services are interpreted as service names e.g. `--services svc1 svc2 svc3`.",
            type: "array",
          })
          .option("cluster", {
            describe: "Link the created service to this cluster (i.e. a monitor)",
            type: "string",
          })
          .option("filters", {
            describe:
              "Link the created service to these filters. All non-option arguments " +
              "after --filters are interpreted as filter names e.g. `--filters f1 f2 f3`.",
            type: "array",
          });
      },
      function (argv) {
        maxctrl(argv, async function (host) {
          checkName(argv.name);

          let err = validateParams(argv, argv.params);
          if (err) {
            return Promise.reject(err);
          }

          var service = {
            data: {
              id: String(argv.name),
              attributes: {
                router: argv.router,
              },
            },
          };

          var params = argv.params.reduce(to_obj, {});

          if (params.servers) {
            await paramToRelationship(service, params.servers.split(","), "servers");
            delete params.servers;
          }

          if (params.targets) {
            await targetToRelationships(host, service, params.targets.split(","));
            delete params.targets;
          }

          if (params.cluster) {
            await setMonitorRelationship(service, params.cluster, "monitors");
            delete params.cluster;
          }

          if (params.filters) {
            await paramToRelationship(service, params.filters.split("|"), "filters");
            delete params.filters;
          }

          service.data.attributes.parameters = params;

          if (argv.servers) {
            for (let i = 0; i < argv.servers.length; i++) {
              _.set(service, "data.relationships.servers.data[" + i + "]", {
                id: String(argv.servers[i]),
                type: "servers",
              });
            }
          }

          if (argv.services) {
            for (let i = 0; i < argv.services.length; i++) {
              _.set(service, "data.relationships.services.data[" + i + "]", {
                id: String(argv.services[i]),
                type: "services",
              });
            }
          }

          if (argv.cluster) {
            _.set(service, "data.relationships.monitors.data[0]", {
              id: String(argv.cluster),
              type: "monitors",
            });
          }

          if (argv.filters) {
            for (let i = 0; i < argv.filters.length; i++) {
              _.set(service, "data.relationships.filters.data[" + i + "]", {
                id: String(argv.filters[i]),
                type: "filters",
              });
            }
          }

          return doRequest(host, "services", { method: "POST", data: service });
        });
      }
    )

    // Create filter
    .command(
      "filter <name> <module> [params...]",
      "Create a new filter",
      function (yargs) {
        return yargs
          .epilog(
            "The last argument to this command is a list of key=value parameters " +
              "given as the filter parameters."
          )
          .usage("Usage: filter <name> <module> [params...]");
      },
      function (argv) {
        maxctrl(argv, function (host) {
          checkName(argv.name);

          var filter = {
            data: {
              id: String(argv.name),
              attributes: {
                module: argv.module,
              },
            },
          };

          var err = validateParams(argv, argv.params);
          if (err) {
            return Promise.reject(err);
          }
          filter.data.attributes.parameters = argv.params.reduce(to_obj, {});

          return doRequest(host, "filters", { method: "POST", data: filter });
        });
      }
    )

    // Create listener
    .command(
      "listener <service> <name> <port> [params...]",
      "Create a new listener",
      function (yargs) {
        return yargs
          .epilog(
            "The new listener will be taken into use immediately. " +
              "The last argument to this command is a list of key=value parameters " +
              "given as the listener parameters. These parameters override any parameters " +
              "set via command line options: e.g. using `protocol=mariadb` will override " +
              "the `--protocol=cdc` option. " +
              "The redundant option parameters have been deprecated in MaxScale 22.08."
          )
          .usage("Usage: create listener <service> <name> <port> [params...]");
      },
      function (argv) {
        maxctrl(argv, function (host) {
          checkName(argv.name);

          if (!Number.isInteger(argv.port) || argv.port <= 0) {
            return Promise.reject("'" + argv.port + "' is not a valid value for port");
          }

          var err = validateParams(argv, argv.params);

          if (err) {
            return Promise.reject(err);
          }

          var listener = {
            data: {
              id: String(argv.name),
              type: "listeners",
              attributes: {
                parameters: {
                  port: argv.port,
                },
              },
              relationships: {
                services: {
                  data: [{ id: String(argv.service), type: "services" }],
                },
              },
            },
          };

          var extra_params = argv.params.reduce(to_obj, {});
          Object.assign(listener.data.attributes.parameters, extra_params);

          return doRequest(host, "listeners", { method: "POST", data: listener });
        });
      }
    )
    .command(
      "user <name> <passwd>",
      "Create a new network user",
      function (yargs) {
        return yargs
          .epilog(
            "By default the created " +
              "user will have read-only privileges. To make the user an " +
              "administrative user, use the `--type=admin` option. " +
              "Basic users can only perform `list` and `show` commands."
          )
          .usage("Usage: create user <name> <password>")
          .group(["type"], "Create user options:")
          .option("type", {
            describe: "Type of user to create",
            type: "string",
            default: "basic",
            choices: ["admin", "basic"],
          });
      },
      function (argv) {
        var user = {
          data: {
            id: String(argv.name),
            type: "inet",
            attributes: {
              password: String(argv.passwd),
              account: argv.type,
            },
          },
        };

        maxctrl(argv, function (host) {
          return doRequest(host, "users/inet", { method: "POST", data: user });
        });
      }
    )
    .command(
      "report [file]",
      "Create a diagnostic report and save it into a file. If no file is given, the report is written to stdout.",
      function (yargs) {
        return yargs
          .epilog(
            `
The generated report contains the state of all the objects in MaxScale
as well as all other required information needed to diagnose problems.

If the --archive option is used, a tarball report will be generated that
will contain additional information about MaxScale and the system it is
running on. The contents of the following files and directories will be added
to the archive:

  /etc/maxscale.cnf
  /etc/maxscale.cnf.d/
  /var/lib/maxscale/maxscale.cnf.d/
  /var/log/maxscale.log
  /proc/meminfo
  /proc/cpuinfo

All password parameters in any .cnf files will be sanitized and replaced with
placeholders but other information like IP addresses and usernames are
unmodified.

The output of the --archive mode can be piped directory to gzip to make
a compressed tarball by omitting the filename:

  maxctrl create report --archive | gzip > my-report.tar.gz
`
          )
          .group(["lines", "archive"], "Report options:")
          .option("lines", {
            describe: "How many lines of logs to collect",
            type: "number",
            default: 1000,
          })
          .option("archive", {
            describe: "Collect all relevant files and create a report tarball.",
            type: "boolean",
            default: false,
          })
          .wrap(null)
          .usage("Usage: create report [file]");
      },
      function (argv) {
        maxctrl(argv, async function (host) {
          if (argv.archive) {
            return createSupportReport(host, argv);
          }

          var data = await readAllEndpoints(host, argv.lines);

          if (argv.file) {
            fs.writeFileSync(argv.file, JSON.stringify(data, null, 2));
            return OK();
          } else {
            return JSON.stringify(data, null, 2);
          }
        });
      }
    )
    .usage("Usage: create <command>")
    .help()
    .wrap(null)
    .demandCommand(1, helpMsg);
};
