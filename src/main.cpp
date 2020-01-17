//
// Created by YANHAI on 2019/5/5.
//

#include <iostream>
#include "util/cmdline.hpp"
#include "util/log.h"
#include "util/config.h"
#include "util/pidfile.h"
#include "util-disk.h"
#include "util-file.h"
#include "tm-threads.h"
#include "worker.h"
#include "manager.h"

using namespace std;

volatile sig_atomic_t sigint_count = 0;
volatile sig_atomic_t sigterm_count = 0;

static void SignalHandlerSigint(__attribute__((unused)) int sig)
{
    sigint_count = 1;
}

static void SignalHandlerSigterm(__attribute__((unused)) int sig)
{
    sigterm_count = 1;
}

void _print_version()
{
    spdlog::info("This is {} version {}", PROG_NAME, PROG_VER);
}

static void parse_command_line(int argc, char **argv, string &config_file)
{
    cmdline::parser args;
    args.add<string>("config", 'c', "directory to configuration file", false,
                     DEFAULT_CONFIG);
    args.add<string>("level", 'l', "set logger level", false, "info",
                     cmdline::oneof<string>("debug", "info", "warning", "error",
                                            "critical", "off"));
    args.add("nolog", 0, "not log to file");
    args.add("version", 'V', "output version information and exit");
    args.set_program_name(argv[0]);

    args.parse_check(argc, argv);

    if (args.exist("version")) {
        _print_version();
        exit(0);
    }

    if (args.exist("nolog"))
        init_logger(args.get<string>("level"));
    else
        init_logger(args.get<string>("level"), "/var/log/auto_clean.log");

    // load config
    if (!Config::instance().load_config(args.get<string>("config")))
        exit(EXIT_FAILURE);
}

int main(int argc, char **argv)
{
    string config_file = DEFAULT_CONFIG;

    signal(SIGINT, SignalHandlerSigint);
    signal(SIGTERM, SignalHandlerSigterm);

    SetThreadName("main");
    parse_command_line(argc, argv, config_file);
    _print_version();

    if (Pidfile::instance().testCreate() != 0)
        return 1;

    // start worker threads
    for (auto &WorkConfig: Config::instance()["clean"]) {
        if (!WorkConfig["enabled"].asBool()) {
            spdlog::warn("worker thread {} is disabled", WorkConfig["name"].asCString());
            continue;
        }
        ThreadVars *tv = Worker::create(WorkConfig);
        if (TmThreads::spawn(tv) != 0) {
            spdlog::error("TmThreadSpawn failed");
            delete tv;
        }
    }

    // start manager thread
    ThreadVars *tv = Manager::create();
    if (TmThreads::spawn(tv) != 0) {
        cout << "TmThreadSpawn failed" << endl;
        delete tv;
    }

    spdlog::info("all {} clean processing threads, 0 management threads initialized, engine started.",
                 Worker::worker_threads());
    while (true) {
        if (sigint_count > 0 ||
            sigterm_count > 0) {
            spdlog::info("Signal Received. Stopping engine");
            break;
        }

        usleep(100 * 1000);
    }

    // stop all threads
    TmThreads::kills();
    TmThreads::clears();

    // clear pid file and config
    Pidfile::instance().remove();
    return 0;
}
