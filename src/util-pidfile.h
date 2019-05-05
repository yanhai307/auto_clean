//
// Created by YANHAI on 2019/5/5.
//

/**
 * \file
 *
 * \author yanhai
 */

#pragma once

#include <string>

class Pidfile {
public:
    Pidfile() = default;
    Pidfile(const char *);
    Pidfile(const std::string &);
    ~Pidfile() = default;

    int PidfileSetPidfile(const char *);
    int PidfileSetPidfile(const std::string &);

    int PidfileCreate();
    int PidfileCreate(const char *);

    void PidfileRemove();
    void PidfileRemove(const char *);

    int PidfileTestRunning();
    int PidfileTestRunning(const char *pid_filename);

    int PidfileTestCreate();
    int PidfileTestCreate(const char *pid_filename);

private:
    std::string pid_filename;
};

