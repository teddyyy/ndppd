// ndppd - NDP Proxy Daemon
// Copyright (C) 2011  Daniel Adolfsson <daniel@priv.nu>
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.
#include <cstdlib>
#include <csignal>

#include <iostream>
#include <fstream>
#include <memory>

#include <getopt.h>
#include <sys/time.h>

#include <sys/stat.h>
#include <unistd.h>

#include "ndppd.hpp"
#include "config.hpp"
#include "socket.hpp"

using namespace ndppd;

static int daemonize()
{
    pid_t pid = fork();

    if (pid < 0)
        return -1;

    if (pid > 0)
        exit(0);

    umask(0);

    pid_t sid = setsid();

    if (sid < 0)
        return -1;

    if (chdir("/") < 0)
        return -1;

    close(STDIN_FILENO);
    close(STDOUT_FILENO);
    close(STDERR_FILENO);

    return 0;
}

static void configure(std::shared_ptr<config> config)
{

}

static bool running = true;

static void exit_ndppd(int sig)
{
    logger::error() << "Shutting down...";
    running = 0;
}

int main(int argc, char* argv[], char* env[])
{
    signal(SIGINT, exit_ndppd);
    signal(SIGTERM, exit_ndppd);

    std::string config_path("/etc/ndppd.conf");
    std::string pidfile;
    std::string verbosity;
    bool daemon = false;

    while (1) {
        int c, opt;

        static struct option long_options[] =
        {
            { "config",     1, 0, 'c' },
            { "daemon",     0, 0, 'd' },
            { "verbose",    1, 0, 'v' },
            { 0, 0, 0, 0}
        };

        c = getopt_long(argc, argv, "c:dp:v", long_options,& opt);

        if (c == -1)
            break;

        switch (c) {
        case 'c':
            config_path = optarg;
            break;

        case 'd':
            daemon = true;
            break;

        case 'p':
            pidfile = optarg;
            break;

        case 'v':
            logger::verbosity(logger::verbosity() + 1);
            /*if (optarg) {
                if (!logger::verbosity(optarg))
                    logger::error() << "Unknown verbosity level '" << optarg << "'";
            } else {
                logger::max_pri(LOG_INFO);
            }*/
            break;
        }
    }

    logger::notice()
        << "ndppd (NDP Proxy Daemon) version " NDPPD_VERSION << logger::endl
        << "Using configuration file '" << config_path << "'";

    // Load configuration.

    if (daemon) {
        logger::syslog(true);

        if (daemonize() < 0) {
            logger::error() << "Failed to daemonize process";
            return 1;
        }
    }

    //if (!configure(cf))
    //    return -1;

    if (!pidfile.empty()) {
        std::ofstream pf;
        pf.open(pidfile.c_str(), std::ios::out | std::ios::trunc);
        pf << getpid() << std::endl;
        pf.close();
    }

    // Time stuff.

    timeval t1 = {};
    gettimeofday(&t1, nullptr);

#ifdef WITH_ND_NETLINK
    netlink_setup();
#endif

    while (running) {
        socket::poll_all();

        timeval t2 = {};
        gettimeofday(&t2, nullptr);

        auto elapsed_time =
            ((t2.tv_sec  - t1.tv_sec)*   1000) +
            ((t2.tv_usec - t1.tv_usec) / 1000);

        t1 = t2;
    }

#ifdef WITH_ND_NETLINK
    netlink_teardown();
#endif

    logger::notice() << "Bye";

    return 0;
}

