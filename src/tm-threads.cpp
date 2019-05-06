//
// Created by YANHAI on 2019/5/6.
//

#include <iostream>
#include "tm-threads.h"

using namespace std;


void *TmThreads::loop(void *td)
{
    /* block usr2.  usr2 to be handled by the main thread only */
    //UtilSignalBlock(SIGUSR2);

    ThreadVars *tv = (ThreadVars *)td;
    char run = 1;
    int r = 0;

    /* Set the thread name */
    if (SetThreadName(tv->name) < 0) {
        cout << "Unable to set thread name" << endl;
    }
    cout << "init: " << tv->name << endl;

    tv->setFlag(THV_INIT_DONE);

    while(run) {
        if (tv->checkFlag(THV_PAUSE)) {
            tv->setFlag(THV_PAUSED);
            tv->testThreadUnPaused();
            tv->unsetFlag(THV_PAUSED);
        }

        r = tv->loop();
        cout << "loop: " << tv->name << endl;

        if (r != 0 || tv->checkFlag(THV_KILL)) {
            run = 0;
        }

        tv->ctrlCondTimedwait(tv->sleep);
    }

    tv->setFlag(THV_RUNNING_DONE);

    //TmThreadWaitForFlag(tv, THV_DEINIT);

    cout << "end: " << tv->name << endl;
    tv->setFlag(THV_CLOSED);
    pthread_exit(nullptr);
//    return nullptr;

//error:
//    pthread_exit((void *) -1);
//    return nullptr;
}

void TmThreads::kill(ThreadVars *tv)
{
    /* set the thread flag informing the thread that it needs to be
     * terminated */
    tv->setFlag(THV_KILL);
    tv->setFlag(THV_DEINIT);

    /* to be sure, signal more */
    int cnt = 0;
    while (true) {
        if (tv->checkFlag(THV_CLOSED)) {
            cout << "signalled the thread " << cnt << " times" << endl;
            break;
        }

        cnt++;

        if (tv->ctrl_cond != nullptr ) {
            pthread_cond_broadcast(tv->ctrl_cond);
        }

        usleep(100);
    }

    /* join it */
    pthread_join(tv->t, nullptr);
    //LogDebug("thread %s stopped", tv->name);
}

void TmThreads::kills()
{
    ThreadVars *tv = nullptr;
    tv = ThreadVars::tv_root;

    while (tv) {
        kill(tv);
        tv = tv->next;
    }
}

void TmThreads::clear(ThreadVars *tv)
{
    cout << "Freeing thread '" << tv->name << "'." << endl;
    delete tv;
}

void TmThreads::clears()
{
    ThreadVars *tv = nullptr;
    ThreadVars *ptv = nullptr;

    tv = ThreadVars::tv_root;

    while (tv) {
        ptv = tv;
        tv = tv->next;
        clear(ptv);
    }
    ThreadVars::tv_root = nullptr;
}

/**
 * \brief Appends this TV to tv_root based on its type
 *
 * \param type holds the type this TV belongs to.
 */
void TmThreads::append(ThreadVars *tv)
{
    if (ThreadVars::tv_root == nullptr) {
        ThreadVars::tv_root = tv;
        tv->next = nullptr;
        tv->prev = nullptr;

        return;
    }

    ThreadVars *t = ThreadVars::tv_root;

    while (t) {
        if (t->next == nullptr) {
            t->next = tv;
            tv->prev = t;
            tv->next = nullptr;
            break;
        }

        t = t->next;
    }
}

/**
 * \brief Spawns a thread associated with the ThreadVars instance tv
 *
 * \retval 0 on success and -1 on failure
 */
int TmThreads::spawn(ThreadVars *tv)
{
    pthread_attr_t attr;

    /* Initialize and set thread detached attribute */
    pthread_attr_init(&attr);

    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);

    int rc = pthread_create(&tv->t, &attr, loop, (void *) tv);
    if (rc) {
        cout << "ERROR; return code from pthread_create() is " << rc << endl;
        return -1;
    }

    tv->waitForFlag(THV_INIT_DONE | THV_RUNNING_DONE);

    append(tv);
    return 0;
}
