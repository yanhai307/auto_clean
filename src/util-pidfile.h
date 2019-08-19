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

    Pidfile(const std::string &);

    ~Pidfile() = default;

    Pidfile &operator=(const std::string &);

    int create(const char * = nullptr);

    void remove(const char * = nullptr);

    int testRunning(const char * = nullptr);

    int testCreate(const char * = nullptr);

private:
    std::string pid_filename;
};

