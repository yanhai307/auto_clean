//
// Created by YANHAI on 2019/5/5.
//

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#include "util-file.h"

File::File(const char *name)
        : _name(name),
          _hash(0),
          _time(0),
          _size(0),
          lnext(nullptr),
          lprev(nullptr) {

    struct stat s;
    if (stat(name, &s) == 0) {
        if (S_ISREG(s.st_mode)) {
            _time = STAT_TIME(&s).tv_sec;
            _size = s.st_size;
        }
    }
}
