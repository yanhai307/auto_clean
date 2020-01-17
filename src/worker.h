//
// Created by YANHAI on 2019/5/6.
//

#pragma once

#include <list>
#include <json/json.h>
#include "util-threads.h"
#include "util-file.h"
#include "util-disk.h"

class Worker : public ThreadVars {
protected:
    Worker(Json::Value &config);

public:
    ~Worker();

    virtual int init();

    virtual int loop();

    virtual void exitPrintStats();

    virtual int deinit();

    static ThreadVars *create(Json::Value &config);

    static int worker_threads()
    {
        return _worker_threads;
    };

private:
    Json::Value &config;
    unsigned long timeout;  // file timeout , will deleted

    static int _worker_threads;
    FileCtx *file;
    Disk *disk;
};
