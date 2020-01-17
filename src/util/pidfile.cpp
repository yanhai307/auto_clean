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

#include <cstring>
#include <sys/types.h>
#include <fcntl.h>
#include <unistd.h>
#include <cinttypes>
#include <csignal>

#include <util/log.h>
#include "pidfile.h"

Pidfile::Pidfile(const std::string &file)
{
    pid_filename = file;
}

Pidfile &Pidfile::operator=(const std::string &file)
{
    pid_filename = file;
    return *this;
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
int Pidfile::create(const char *pidfile)
{
    int pidfd = 0;
    char val[16];

    if (pidfile == nullptr) {
        if (pid_filename.empty()) {
            spdlog::warn("Pid filename not set");
            return -1;
        }
        pidfile = pid_filename.c_str();
    }

    int len = snprintf(val, sizeof(val), "%" PRIuMAX "\n", (uintmax_t) getpid());
    if (len <= 0) {
        spdlog::warn("Pid error ({})", strerror(errno));
        return (-1);
    }

    pidfd = open(pidfile, O_CREAT | O_TRUNC | O_NOFOLLOW | O_WRONLY, 0644);
    if (pidfd < 0) {
        spdlog::error("unable to set pidfile '{}': {}", pidfile, strerror(errno));
        return (-1);
    }

    ssize_t r = write(pidfd, val, (unsigned int) len);
    if (r == -1) {
        spdlog::error("unable to write pidfile: {}", strerror(errno));
        close(pidfd);
        return (-1);
    } else if ((size_t) r != len) {
        spdlog::error("unable to write pidfile: wrote {} of {} bytes.", r, len);
        close(pidfd);
        return (-1);
    }

    close(pidfd);
    return (0);
}

/**
 * \brief Remove the pid file (used at the startup)
 *
 * \param pointer to the name of the pid file to write (optarg)
 */
void Pidfile::remove(const char *pidfile)
{
    if (pidfile == nullptr) {
        if (pid_filename.empty()) {
            spdlog::warn("Pid filename not set");
            return;
        }
        pidfile = pid_filename.c_str();
    }

    if (pidfile != nullptr) {
        /* we ignore the result, the user may have removed the file already. */
        (void) unlink(pidfile);
    }
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
int Pidfile::testRunning(const char *pidfile)
{
    if (pidfile == nullptr) {
        if (pid_filename.empty()) {
            spdlog::warn("pid filename not set");
            return -1;
        }
        pidfile = pid_filename.c_str();
    }

    if (access(pidfile, F_OK) == 0) {
        /* Check if the existing process is still alive. */
        pid_t pidv;
        FILE *pf;

        pf = fopen(pidfile, "r");
        if (pf == nullptr) {
            spdlog::error("pid file '{}' exists and can not be read, Aborting!", pidfile);
            return -1;
        }

        if (fscanf(pf, "%d", &pidv) == 1 && kill(pidv, 0) == 0) {
            fclose(pf);
            spdlog::error("pid file '{}' exists. Is {} already running? Aborting!", pidfile, PROG_NAME);
            return -1;
        }

        fclose(pf);
    }
    return 0;
}

int Pidfile::testCreate(const char *pidfile)
{
    if (pidfile == nullptr) {
        if (pid_filename.empty()) {
            spdlog::warn("pid filename not set");
            return -1;
        }
        pidfile = pid_filename.c_str();
    }

    if (testRunning(pidfile) != 0)
        return -1;

    return create(pidfile);
}

int Pidfile::testCreate()
{
    if (testRunning(pid_filename.c_str()) != 0)
        return -1;
    return create(pid_filename.c_str());
}
