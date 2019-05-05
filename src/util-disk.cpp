//
// Created by YANHAI on 2019/5/5.
//

#include <iostream>
#include <cstring>
#include "util-disk.h"

using namespace std;

int Disk::setMountPoint() {
    int r;
    char cmd[1024];
    char buffer[4096];

    snprintf(cmd, sizeof(cmd), "df %s | grep -v '^Filesystem' | awk '{print $6}'", _path);
    r = GetShellCmdRetVal(cmd, buffer, sizeof(buffer));
    if (r != 0) {
        return -1;
    }

    _mount_point = buffer;
    return 0;
}

const char *Disk::MountPoint() {
    if (_mount_point.empty()) {
        setMountPoint();
        if (_mount_point.empty())
            return nullptr;
    }

    return _mount_point.c_str();
}

off_t Disk::totalBytes() {
    if (_total == 0) {
        if (GetTotal() != 1) {
            printf("get total failed\n");
        }
    }
    return _total;
}

int Disk::GetTotal() {
    int r;
    char cmd[1024];
    char buffer[1024];

    snprintf(cmd, sizeof(cmd), "df --block-size=1 %s | grep -v '^Filesystem' | awk '{print $2}'", _path);
    r = GetShellCmdRetVal(cmd, buffer, sizeof(buffer));
    if (r != 0) {
        return 0;
    }

    _total = atol(buffer);
    return 1;
}

short Disk::usedPercentage() {
    int r;
    char cmd[1024];
    char buffer[1024];

    snprintf(cmd, sizeof(cmd), "df %s | grep -v '^Filesystem' | awk '{print $5}'", _path);
    r = GetShellCmdRetVal(cmd, buffer, sizeof(buffer));
    if (r != 0) {
        return 0;
    }

    return atoi(buffer);
}

off_t Disk::deleteBytes() {
    short delete_percent;
    double delete_bytes;

    short used = usedPercentage();
    if (used < _used_threshold) {
        return 0;
    }

    delete_percent = used - _used_threshold;
    delete_bytes = _total * 1.0 * (delete_percent + (short) 1) / 100;
    return (off_t) delete_bytes;
}

int Disk::GetShellCmdRetVal(const char *cmd, char *result, size_t result_len) {
    char buf_ps[1024];
    char ps[1024] = {0};
    FILE *ptr;
    size_t len = 0;
    size_t copy_len = 0;
    uint8_t flg = 0;

    strcpy(ps, cmd);
    if ((ptr = popen(ps, "r")) != nullptr) {
        while (fgets(buf_ps, 1024, ptr) != nullptr) {
            len = strlen(buf_ps);
            len = min(len, result_len - copy_len - 1);
            if (result_len - copy_len - 1 < len) { // result buffer will full
                len = result_len - copy_len - 1;
                flg = 1;
            }
            if (len > 0) {
                memcpy(result + copy_len, buf_ps, len);
                copy_len += len;
                result[copy_len] = '\0';
            }
            if (flg)
                break;
        }
        pclose(ptr);
    } else {
        printf("popen %s error\n", ps);
        return -1;
    }

    return 0;
}
