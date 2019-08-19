//
// Created by YANHAI on 2019/5/6.
//

#pragma once

#include <cstring>
#include <string>
#include <cinttypes>
#include <cctype>

#include <unistd.h>
#include <pthread.h>
#include <sys/prctl.h>


using namespace std;

#define THREAD_NAME_LEN 16

/* mutex */
#define Mutex                               pthread_mutex_t
#define MutexAttr                           pthread_mutexattr_t
#define MutexInit(mut, mutattr)             pthread_mutex_init(mut, mutattr)
#define MutexLock(mut)                      pthread_mutex_lock(mut)
#define MutexTrylock(mut)                   pthread_mutex_trylock(mut)
#define MutexUnlock(mut)                    pthread_mutex_unlock(mut)
#define MutexDestroy                        pthread_mutex_destroy
#define MUTEX_INITIALIZER                   PTHREAD_MUTEX_INITIALIZER

/* rwlocks */
#define RWLock                              pthread_rwlock_t
#define RWLockInit(rwl, rwlattr)            pthread_rwlock_init(rwl, rwlattr)
#define RWLockWRLock(rwl)                   pthread_rwlock_wrlock(rwl)
#define RWLockRDLock(rwl)                   pthread_rwlock_rdlock(rwl)
#define RWLockTryWRLock(rwl)                pthread_rwlock_trywrlock(rwl)
#define RWLockTryRDLock(rwl)                pthread_rwlock_tryrdlock(rwl)
#define RWLockUnlock(rwl)                   pthread_rwlock_unlock(rwl)
#define RWLockDestroy                       pthread_rwlock_destroy

/* conditions */
#define CondT                               pthread_cond_t
#define CondInit                            pthread_cond_init
#define CondSignal                          pthread_cond_signal
#define CondDestroy                         pthread_cond_destroy
#define CondWait(cond, mut)                 pthread_cond_wait(cond, mut)

/* ctrl mutex */
#define CtrlMutex                           pthread_mutex_t
#define CtrlMutexAttr                       pthread_mutexattr_t
#define CtrlMutexInit(mut, mutattr)         pthread_mutex_init(mut, mutattr)
#define CtrlMutexLock(mut)                  pthread_mutex_lock(mut)
#define CtrlMutexTrylock(mut)               pthread_mutex_trylock(mut)
#define CtrlMutexUnlock(mut)                pthread_mutex_unlock(mut)
#define CtrlMutexDestroy                    pthread_mutex_destroy

/* ctrl conditions */
#define CtrlCondT                           pthread_cond_t
#define CtrlCondInit                        pthread_cond_init
#define CtrlCondSignal                      pthread_cond_signal
#define CtrlCondTimedwait                   pthread_cond_timedwait
#define CtrlCondWait                        pthread_cond_wait
#define CtrlCondDestroy                     pthread_cond_destroy

/* spinlocks */
#define Spinlock                            pthread_spinlock_t
#define SpinLock(spin)                      pthread_spin_lock(spin)
#define SpinTrylock(spin)                   pthread_spin_trylock(spin)
#define SpinUnlock(spin)                    pthread_spin_unlock(spin)
#define SpinInit(spin, spin_attr)           pthread_spin_init(spin, spin_attr)
#define SpinDestroy(spin)                   pthread_spin_destroy(spin)


size_t strlcpy(char *dst, const char *src, size_t siz);

/**
 * \brief Set the threads name
 */
#define SetThreadName(n) ({ \
    char tname[THREAD_NAME_LEN + 1] = ""; \
    if (strlen(n) > THREAD_NAME_LEN) \
        cout << "Thread name is too long, truncating it..." << endl; \
    strlcpy(tname, n, THREAD_NAME_LEN); \
    int ret = 0; \
    if ((ret = prctl(PR_SET_NAME, tname, 0, 0, 0)) < 0) \
        cout << "Error setting thread name '" << tname << "': " << strerror(errno) << endl; \
    ret; \
})


/** Thread flags set and read by threads to control the threads */
#define THV_USE       1 /** thread is in use */
#define THV_INIT_DONE (1 << 1) /** thread initialization done */
#define THV_PAUSE     (1 << 2) /** signal thread to pause itself */
#define THV_PAUSED    (1 << 3) /** the thread is paused atm */
#define THV_KILL      (1 << 4) /** thread has been asked to cleanup and exit */
#define THV_FAILED    (1 << 5) /** thread has encountered an error and failed */
#define THV_CLOSED    (1 << 6) /** thread done, should be joinable */
/* used to indicate the thread is going through de-init.  Introduced as more
 * of a hack for solving stream-timeout-shutdown.  Is set by the main thread. */
#define THV_DEINIT    (1 << 7)
#define THV_RUNNING_DONE (1 << 8) /** thread has completed running and is entering
                                   * the de-init phase */

/** signal thread's capture method to create a fake packet to force through
 *  the engine. This is to force timely handling of maintenance taks like
 *  rule reloads even if no packets are read by the capture method. */
#define THV_CAPTURE_INJECT_PKT (1<<11)

/** Thread flags set and read by threads, to control the threads, when they
 *  encounter certain conditions like failure */
#define THV_RESTART_THREAD 0x01 /** restart the thread */
#define THV_ENGINE_EXIT 0x02 /** shut the engine down gracefully */

/** Maximum no of times a thread can be restarted */
#define THV_MAX_RESTARTS 50

class ThreadVars
{
    friend class TmThreads;
public:
    ThreadVars();
    virtual ~ThreadVars();

    /**
     * \brief Check if a thread flag is set.
     *
     * \retval 1 flag is set.
     * \retval 0 flag is not set.
     */
    int checkFlag(uint16_t flag) {
        return (flags & flag) ? 1 : 0;
    }

    /**
     * \brief Set a thread flag.
     */
    void setFlag(uint16_t flag) {
        flags |= flag;
    }

    /**
     * \brief Unset a thread flag.
     */
    void unsetFlag(uint16_t flag) {
        flags &= ~flag;
    }

    /**
     * \brief Waits till the specified flag(s) is(are) set.  We don't bother if
     *        the kill flag has been set or not on the thread.
     *
     * \param tv Pointer to the TV instance.
     */
    void waitForFlag(uint16_t flags) {
        while (!checkFlag(flags)) {
            usleep(100);
        }
    }

    /**
     * \brief Tests if the thread represented in the arg has been unpaused or not.
     *
     *        The function would return if the thread tv has been unpaused or if the
     *        kill flag for the thread has been set.
     *
     * \param tv Pointer to the TV instance.
     */
    void testThreadUnPaused() {
        while (checkFlag(THV_PAUSE)) {
            usleep(100);

            if (checkFlag(THV_KILL))
                break;
        }
    }

    void initMC();
    void deinitMC();
    void ctrlCondTimedwait(time_t t);

    virtual int init() = 0;
    virtual int loop() = 0;
    virtual void exitPrintStats() = 0;
    virtual int deinit() = 0;

public:
    pthread_t t;
    char name[16];

    unsigned int flags;
    unsigned int sleep;     // seconds

private:
    CtrlMutex *ctrl_mutex;
    CtrlCondT *ctrl_cond;

    ThreadVars *next;
    ThreadVars *prev;
    static ThreadVars *tv_root;
};
