//
// Created by YANHAI on 2019/5/6.
//

#include <iostream>
#include "worker.h"

using namespace std;

int Worker::_worker_threads = 0;

Worker::Worker()
{
    _worker_threads++;
    snprintf(name, sizeof(name), "W-%02d", _worker_threads);
//    queue = FileQueueNew<File>();
    cout << "worker()" << endl;
}

Worker::~Worker()
{
    cout << "~worker()" << endl;
    while (true) {
//        File *f = FileDequeue(queue);
//        if (f == nullptr)
//            break;
//        cout << "Dequeue hash: " << f->hash << endl;
//        free(f);
        cout << "hello world" << endl;
        break;
    };
//    FileQueueDestroy(queue);
//    queue = nullptr;
}

int Worker::init()
{
    cout << "Worker thread init: " << name << endl;
    return 0;
}

int Worker::loop()
{
    cout << "Worker thread loop: " << name << endl;
//    File *f = (File *)malloc(sizeof(File));
//    if (f != nullptr) {
//        f->hash = num;
//        FileEnqueue(queue, f);
//        cout << "Enqueue hash: " << f->hash << endl;
//        num++;
//    }
    return 0;
}

void Worker::exitPrintStats()
{
    cout << "Worker thread exit print stats: " << name << endl;
}

int Worker::deinit()
{
    cout << "Worker thread deinit: " << name << endl;
    return 0;
}

ThreadVars* Worker::create() {
    // std::nothrow 保证new失败时，返回空指针，默认则是抛出异常
    ThreadVars *tv = new(nothrow) Worker;
    if (tv == nullptr) {
        cout << "thread create malloc failed" << endl;
        return nullptr;
    }

    tv->sleep = 3;
    cout << tv->name << endl;

    return tv;
}
