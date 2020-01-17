//
// Created by YANHAI on 2019/5/6.
//

#pragma once

#include "util-threads.h"

class Manager : public ThreadVars {
protected:
    Manager();

public:
    ~Manager();

    virtual int init();

    virtual int loop();

    virtual void exitPrintStats();

    virtual int deinit();

    static ThreadVars *create();
};
