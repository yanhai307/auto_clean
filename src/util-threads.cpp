//
// Created by YANHAI on 2019/5/6.
//


#include <iostream>
#include <unistd.h>
#include "util-threads.h"


/* root of the threadvars list */
//ThreadVars *tv_root = { nullptr };

using namespace std;

ThreadVars *ThreadVars::tv_root = nullptr;


ThreadVars::ThreadVars()
{
    t = 0;
    flags = 0;
    // name = "";

    /** slot functions */
    sleep = 0;     // seconds
//    cout << "ThreadVars()" << endl;
    initMC();
}

ThreadVars::~ThreadVars()
{
//    cout << "~ThreadVars()" << endl;
    deinitMC();
}

/**
 * \brief Initializes the mutex and condition variables for this TV
 *
 * It can be used by a thread to control a wait loop that can also be
 * influenced by other threads.
 *
 * \param tv Pointer to a TV instance
 */
void ThreadVars::initMC()
{
    if ( (ctrl_mutex = (CtrlMutex *)malloc(sizeof(CtrlMutex))) == nullptr) {
        cout << "Fatal error encountered in TmThreadInitMC. Exiting..." << endl;
        exit(EXIT_FAILURE);
    }

    if (CtrlMutexInit(ctrl_mutex, nullptr) != 0) {
        cout << "Error initializing the tv->m mutex" << endl;
        exit(EXIT_FAILURE);
    }

    if ( (ctrl_cond = (CtrlCondT *)malloc(sizeof(CtrlCondT))) == nullptr) {
        cout << "Fatal error encountered in TmThreadInitMC. Exiting..." << endl;
        exit(EXIT_FAILURE);
    }

    if (CtrlCondInit(ctrl_cond, nullptr) != 0) {
        cout << "Error initializing the tv->cond condition variable" << endl;
        exit(EXIT_FAILURE);
    }
}

void ThreadVars::deinitMC()
{
    if (ctrl_mutex) {
        CtrlMutexDestroy(ctrl_mutex);
        free(ctrl_mutex);
    }
    if (ctrl_cond) {
        CtrlCondDestroy(ctrl_cond);
        free(ctrl_cond);
    }
}

void ThreadVars::ctrlCondTimedwait(time_t t)
{
    struct timespec cond_time;

    cond_time.tv_sec = time(nullptr) + t + 1;
    cond_time.tv_nsec = 0;

    //sleep(sleep);
    CtrlMutexLock(ctrl_mutex);
    CtrlCondTimedwait(ctrl_cond, ctrl_mutex, &cond_time);
    CtrlMutexUnlock(ctrl_mutex);
}


