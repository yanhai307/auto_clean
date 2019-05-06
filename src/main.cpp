//
// Created by YANHAI on 2019/5/5.
//

#include <iostream>
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

static void SignalHandlerSigint(__attribute__((unused)) int sig) {
    sigint_count = 1;
}

static void SignalHandlerSigterm(__attribute__((unused)) int sig) {
    sigterm_count = 1;
}

void printVersion() {
    cout << "This is " << PROG_NAME << " version " << PROG_VER << " " << xstr(REVISION) << endl;
}

int main() {
    Pidfile p;

    signal(SIGINT, SignalHandlerSigint);
    signal(SIGTERM, SignalHandlerSigterm);

    printVersion();

    ConfInit();
    if (ConfYamlLoadFile(DEFAULT_CONFIG) != 0) {
        /* Error already displayed. */
        return 1;
    }

    p.PidfileSetPidfile(DEFAULT_PIDFILE);
    if (p.PidfileTestCreate() != 0)
        return 1;

    // do other
    // for test Conf yaml
    char *conf_val;
    if ((ConfGet("key1.key2", &conf_val)) == 1) {
        cout << "key1.key2: " << conf_val << endl;
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
    p.PidfileRemove();
    ConfDeInit();
    return 0;
}
