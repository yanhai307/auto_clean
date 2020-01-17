//
// Created by YANHAI on 2019/5/5.
//

#pragma once

#include <cstring>

using namespace std;

class Disk {
public:
    Disk(const std::string &path, short threshold)
            : _path(path), _used_threshold(threshold)
    {
        _total = 0;
        _mount_point = "";
    }

    const char *MountPoint();

    off_t totalBytes();

    short usedPercentage();

    off_t deleteBytes();

private:
    int setMountPoint();

    int GetShellCmdRetVal(const char *cmd, char *result, size_t result_len);

    int GetTotal();

private:
    string _path;
    string _mount_point;
    off_t _total;
    short _used_threshold;
};
