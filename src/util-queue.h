//
// Created by YANHAI on 2019/5/6.
//

#pragma once

#include <cstdint>
#include "util-threads.h"


template <typename T>
class Queue {
/** Spinlocks or Mutex for the queues. */
//#define QLOCK_SPIN
//#define QLOCK_MUTEX

#ifdef QLOCK_SPIN
    #ifdef QLOCK_MUTEX
        #error Cannot enable both FQLOCK_SPIN and QLOCK_MUTEX
    #endif
#endif

#ifdef QLOCK_SPIN
    #define QLOCK_INIT(q) SpinInit(&(q)->s, 0)
    #define QLOCK_DESTROY(q) SpinDestroy(&(q)->s)
    #define QLOCK_LOCK(q) SpinLock(&(q)->s)
    #define QLOCK_TRYLOCK(q) SpinTrylock(&(q)->s)
    #define QLOCK_UNLOCK(q) SpinUnlock(&(q)->s)
#elif defined QLOCK_MUTEX
    #define QLOCK_INIT(q) MutexInit(&(q)->m, NULL)
    #define QLOCK_DESTROY(q) MutexDestroy(&(q)->m)
    #define QLOCK_LOCK(q) MutexLock(&(q)->m)
    #define QLOCK_TRYLOCK(q) MutexTrylock(&(q)->m)
    #define QLOCK_UNLOCK(q) MutexUnlock(&(q)->m)
#else
#define QLOCK_INIT(q)
#define QLOCK_DESTROY(q)
#define QLOCK_LOCK(q)
#define QLOCK_TRYLOCK(q)
#define QLOCK_UNLOCK(q)
#endif

public:
    Queue() {
        top = nullptr;
        bot = nullptr;
        len = 0;
        QLOCK_INIT(this);
    }
    ~Queue() {
        // TODO free all data
        QLOCK_DESTROY(this);
    }

    void enqueue(T *f) {
        QLOCK_LOCK(this);

        /* more flows in queue */
        if (top != NULL) {
            f->lnext = top;
            top->lprev = f;
            top = f;
            /* only flow */
        } else {
            top = f;
            bot = f;
        }
        len++;
        QLOCK_UNLOCK(this);
    }

    T *dequeue() {
        QLOCK_LOCK(this);

        T *f = bot;
        if (f == NULL) {
            QLOCK_UNLOCK(this);
            return NULL;
        }

        /* more packets in queue */
        if (bot->lprev != NULL) {
            bot = bot->lprev;
            bot->lnext = NULL;
            /* just the one we remove, so now empty */
        } else {
            top = NULL;
            bot = NULL;
        }

        if (len > 0)
            len--;

        f->lnext = NULL;
        f->lprev = NULL;

        QLOCK_UNLOCK(this);
        return f;
    }

private:
    T *top;
    T *bot;
    uint32_t len;
#ifdef QLOCK_MUTEX
    Mutex m;
#elif defined QLOCK_SPIN
    Spinlock s;
#endif
};
