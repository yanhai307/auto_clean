//
// Created by YANHAI on 2019/5/5.
//

#include <iostream>
#include "util-pidfile.h"
#include "conf-yaml-loader.h"
#include "conf.h"
#include "util-disk.h"
#include "util-file.h"

using namespace std;

/* we need this to stringify the defines which are supplied at compiletime see:
   http://gcc.gnu.org/onlinedocs/gcc-3.4.1/cpp/Stringification.html#Stringification */
#define xstr(s) str(s)
#define str(s) #s

void printVersion() {
    cout << "This is " << PROG_NAME << " version " << PROG_VER << " " << xstr(REVISION) << endl;
}

int main() {
    Pidfile p;

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
    if ((ConfGet("key1.key2", &conf_val)) == 1)
    {
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

    p.PidfileRemove();
    ConfDeInit();
    return 0;
}
