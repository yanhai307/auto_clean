//
// Created by YANHAI on 2019/5/5.
//

#pragma once

#include <cstdint>
#include <sys/param.h>
#include <cstring>
#include <string>

using namespace std;

#define DL_ATIME    // 依据文件创建时间删除
//#define DL_CTIME    // 依据文件访问时间删除
//#define DL_MTIME    // 依据文件修改时间删除

#ifdef DL_ATIME
#define STAT_TIME(s) ((s)->st_atim)
#elif defined DL_CTIME
#define STAT_TIME(s) ((s)->st_ctim)
#elif defined DL_MTIME
#define STAT_TIME(s) ((s)->st_mtim)
#else
#error Enable DL_ATIME or DL_CTIME or DL_MTIME
#endif

class File {
public:
    File(const char *name);

    File(const char *name, time_t t, off_t size)
            : _name(name),
              _time(t),
              _size(size),
              _hash(0),
              lnext(nullptr),
              lprev(nullptr) {
    }

    void hash(uint32_t hash) {
        _hash = hash;
    }

    uint32_t hash() {
        return _hash;
    }

    time_t time() {
        return _time;
    }

    off_t size() {
        return _size;
    }

    const char *name() {
        return _name.c_str();
    }

    uint32_t name_len() {
        return _name.size();
    }

private:
    uint32_t _hash;
    const string _name;
//    uint32_t _name_len;
    off_t _size;    // bytes
    time_t _time;

    class File *lnext;

    class File *lprev;
};
