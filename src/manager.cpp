//
// Created by YANHAI on 2019/5/6.
//

#include <iostream>
#include "manager.h"

using namespace std;

Manager::Manager()
{
    name = "Manager";
    sleep = 3;
//    queue = FileQueueNew<File>();
//    cout << "worker()" << endl;
}

Manager::~Manager()
{
//    cout << "~Manager()" << endl;
//    while (true) {
//        File *f = FileDequeue(queue);
//        if (f == nullptr)
//            break;
//        cout << "Dequeue hash: " << f->hash << endl;
//        free(f);
//        cout << "hello world" << endl;
//        break;
//    };
//    FileQueueDestroy(queue);
//    queue = nullptr;
}

int Manager::init()
{
//    cout << "Manager thread init: " << name << endl;
    return 0;
}

int Manager::loop()
{
//    cout << "Manager thread loop: " << name << endl;
//    File *f = (File *)malloc(sizeof(File));
//    if (f != nullptr) {
//        f->hash = num;
//        FileEnqueue(queue, f);
//        cout << "Enqueue hash: " << f->hash << endl;
//        num++;
//    }
    return 0;
}

void Manager::exitPrintStats()
{
//    cout << "Manager thread exit print stats: " << name << endl;
}

int Manager::deinit()
{
//    cout << "Manager thread deinit: " << name << endl;
    return 0;
}

ThreadVars *Manager::create()
{
    return new Manager;
}
