//
// Created by YANHAI on 2019/5/5.
//

#include <cstring>
#include "util-path.h"

using namespace std;

/**
 *  \brief Check if a path is absolute
 *
 *  \param path string with the path
 *
 *  \retval true absolute
 *  \retval false not absolute
 */
bool Path::isAbsolute() {

    if (strlen(_path) > 1 && _path[0] == '/') {
        return true;
    }

#if (defined OS_WIN32 || defined __CYGWIN__)
    if (strlen(path) > 2) {
        if (isalpha((unsigned char)path[0]) && path[1] == ':') {
            return true;
        }
    }
#endif

    return false;
}

