//
// Created by YANHAI on 2019/5/5.
//

/**
 * \file
 *
 * \author yanhai
 *
 * Utility code for dealing with a pidfile.
 */

#include <iostream>
#include <cstring>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <cinttypes>
#include <csignal>

#include "util-pidfile.h"

Pidfile::Pidfile(const char *file) {
    pid_filename = file;
}

Pidfile::Pidfile(const std::string &file) {
    pid_filename = file;
}

int Pidfile::PidfileSetPidfile(const char *file) {
    pid_filename = file;
    return 0;
}

int Pidfile::PidfileSetPidfile(const std::string &file) {
    pid_filename = file;
    return 0;
}

int Pidfile::PidfileCreate() {
    if (!pid_filename.empty())
        return PidfileCreate(pid_filename.c_str());
//    else {
//        LogError("pid create error: pid filename is null");
//        return -1;
//    }
    return 0;
}

/**
 * \brief Write a pid file (used at the startup)
 *        This commonly needed by the init scripts
 *
 * \param pointer to the name of the pid file to write (optarg)
 *
 * \retval 0 if succes
 * \retval -1 on failure
 */
int Pidfile::PidfileCreate(const char *pidfile) {
    int pidfd = 0;
    char val[16];

    int len = snprintf(val, sizeof(val), "%" PRIuMAX "\n", (uintmax_t) getpid());
    if (len <= 0) {
        printf("Pid error (%s)\n", strerror(errno));
        return (-1);
    }

    pidfd = open(pidfile, O_CREAT | O_TRUNC | O_NOFOLLOW | O_WRONLY, 0644);
    if (pidfd < 0) {
        printf("ERROR: unable to set pidfile '%s': %s\n",
               pidfile,
               strerror(errno));
        return (-1);
    }

    ssize_t r = write(pidfd, val, (unsigned int) len);
    if (r == -1) {
        printf("ERROR: unable to write pidfile: %s\n", strerror(errno));
        close(pidfd);
        return (-1);
    } else if ((size_t) r != len) {
        printf("ERROR: unable to write pidfile: wrote %" PRIdMAX" of %" PRIuMAX" bytes.\n",
               (intmax_t) r, (uintmax_t) len);
        close(pidfd);
        return (-1);
    }

    close(pidfd);
    return (0);
}

void Pidfile::PidfileRemove() {
    if (!pid_filename.empty())
        PidfileRemove(pid_filename.c_str());
//    else {
//        LogError("pid file remove error: pid filename is null");
//    }
}

/**
 * \brief Remove the pid file (used at the startup)
 *
 * \param pointer to the name of the pid file to write (optarg)
 */
void Pidfile::PidfileRemove(const char *pid_filename) {
    if (pid_filename != nullptr) {
        /* we ignore the result, the user may have removed the file already. */
        (void) unlink(pid_filename);
    }
}

int Pidfile::PidfileTestRunning() {
    if (!pid_filename.empty())
        return PidfileTestRunning(pid_filename.c_str());
//    else {
//        LogError("pid file remove error: pid filename is null");
//        return -1;
//    }
    return 0;
}

/**
 * \brief Check a pid file (used at the startup)
 *        This commonly needed by the init scripts
 *
 * \param pointer to the name of the pid file to write (optarg)
 *
 * \retval 0 if succes
 * \retval -1 on failure
 */
int Pidfile::PidfileTestRunning(const char *pid_filename) {
    if (access(pid_filename, F_OK) == 0) {
        /* Check if the existing process is still alive. */
        pid_t pidv;
        FILE *pf;

        pf = fopen(pid_filename, "r");
        if (pf == nullptr) {
            printf("pid file '%s' exists and can not be read. Aborting!\n",
                   pid_filename);
            return -1;
        }

        if (fscanf(pf, "%d", &pidv) == 1 && kill(pidv, 0) == 0) {
            fclose(pf);
            printf("pid file '%s' exists. Is " PROG_NAME" already running? Aborting!\n",
                   pid_filename);
            return -1;
        }

        fclose(pf);
    }
    return 0;
}

int Pidfile::PidfileTestCreate() {
    if (PidfileTestRunning() != 0)
        return -1;

    return PidfileCreate();
}

int Pidfile::PidfileTestCreate(const char *pid_filename) {
    if (PidfileTestRunning(pid_filename) != 0)
        return -1;

    return PidfileCreate(pid_filename);
}
