//
// Created by YANHAI on 2019/5/5.
//

#include <iostream>
#include <getopt.h>
#include "util-pidfile.h"
#include "conf-yaml-loader.h"
#include "conf.h"
#include "util-disk.h"
#include "util-file.h"
#include "tm-threads.h"
#include "worker.h"
#include "manager.h"

using namespace std;

/* we need this to stringify the defines which are supplied at compiletime see:
   http://gcc.gnu.org/onlinedocs/gcc-3.4.1/cpp/Stringification.html#Stringification */
#define xstr(s) str(s)
#define str(s) #s

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

void printVersion()
{
    cout << "This is " << PROG_NAME << " version " << PROG_VER << " " << xstr(REVISION) << endl;
}

void usage(const char *progname)
{
#ifdef REVISION
    printf("%s %s (rev %s)\n", PROG_NAME, PROG_VER, xstr(REVISION));
#elif defined BREVISION
    printf("This is %s version %s %s\n", PROG_NAME, PROG_VER, BREVISION);
#else
    printf("%s %s\n", PROG_NAME, PROG_VER);
#endif
    printf("USAGE: %s [OPTIONS]\n\n", progname);
    printf("\t-c <path>                            : path to configuration file\n");
    printf("\t--help                               : display this help and exit\n");
//    printf("\t--version                            : output version information and exit\n");
}


static int parse_command_line(int argc, char **argv, string &config_file)
{
    int opt;

    struct option long_opts[] = {
            {"help", no_argument, nullptr, 0},
//            {"version", no_argument, nullptr, 0},
    };

    int option_index = 0;

    char short_opts[] = "c:";

    while ((opt = getopt_long(argc, argv, short_opts, long_opts, &option_index)) != -1) {
        switch (opt) {
            case 0:
                if (strcmp((long_opts[option_index]).name, "help") == 0) {
                    usage(argv[0]);
                    return 1;
                } else if (strcmp((long_opts[option_index]).name, "version") == 0) {
                    // print_version();
                    return 1;
                }
                break;
            case 'c':
                config_file = optarg;
                break;
            default:
                usage(argv[0]);
                return 1;
        }
    }

    if (config_file.empty()) {
        usage(argv[0]);
        return 1;
    }

    return 0;
}

int main(int argc, char **argv)
{
    string config_file = DEFAULT_CONFIG;
    Pidfile p;

    signal(SIGINT, SignalHandlerSigint);
    signal(SIGTERM, SignalHandlerSigterm);

    printVersion();

    if (parse_command_line(argc, argv, config_file) != 0)
        return 1;

    ConfInit();
    if (ConfYamlLoadFile(config_file.c_str()) != 0) {
        /* Error already displayed. */
        return 1;
    }

    p = DEFAULT_PIDFILE;
    if (p.testCreate() != 0)
        return 1;

    // do other
    // for test Conf yaml
    char *conf_val;
    if ((ConfGet("logging.default-log-level", &conf_val)) == 1) {
        cout << "logging.default-log-level: " << conf_val << endl;
    }

    // for test class Disk
    Disk d("/var/log", 3);
    cout << "/var/log mount point is: " << d.MountPoint() << endl;
    cout << "/var/log total: " << d.totalBytes() << endl;
    cout << "/var/log used: " << d.usedPercentage() << endl;
    cout << "/var/log delete bytes: " << d.deleteBytes() << endl;

    // for test class File
    const char *filename = "/root/hello.txt";
    File f(filename);
    cout << filename << ": time: " << f.time() << endl;
    cout << filename << ": size: " << f.size() << endl;

    // for test util-threads
    for (int i = 0; i < 1; i++) {
        ThreadVars *tv = Worker::create();
        if (tv == nullptr) {
            cout << "TmThreadsCreate failed" << endl;
            continue;
        }

        if (TmThreads::spawn(tv) != 0) {
            cout << "TmThreadSpawn failed" << endl;
            delete tv;
        }
    }

    // test manager thread
    {
        ThreadVars *tv = Manager::create();
        if (tv == nullptr) {
            cout << "TmThreadsCreate failed" << endl;
        } else {
            if (TmThreads::spawn(tv) != 0) {
                cout << "TmThreadSpawn failed" << endl;
                delete tv;
            }
        }
    }

    cout << "all " << Worker::worker_threads()
         << " clean processing threads, " << 0
         << " management threads initialized, engine started." << endl;
    while (true) {
        if (sigint_count > 0 ||
            sigterm_count > 0) {
            cout << "Signal Received. Stopping engine" << endl;
            break;
        }

        usleep(100 * 1000);
    }

    // stop all threads
    TmThreads::kills();
    TmThreads::clears();

    // clear pid file and config
    p.remove();
    ConfDeInit();
    return 0;
}
