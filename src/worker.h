//
// Created by YANHAI on 2019/5/6.
//

#pragma once

#include "util-threads.h"

class Worker: public ThreadVars {
public:
    Worker();
    ~Worker();

    virtual int init();
    virtual int loop();
    virtual void exitPrintStats();
    virtual int deinit();

    static ThreadVars * create();

    static int worker_threads () {
        return _worker_threads;
    };

private:
//    char *path;
    static int _worker_threads;
//    struct FileQueue<File> *queue;
//    int num;
};


