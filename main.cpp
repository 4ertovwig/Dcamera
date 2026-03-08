/*
 * This file is part of Dcamera project.
 * See the LICENSE file in the root directory for full license terms.
 */
#include <stdlib.h>
#include <signal.h>
#include <string.h>

#ifdef VIDEO_BACKEND_UVC
#include "video/uvc_stream.h"
#else
#include "video/v4l_stream.h"
#endif

#if defined (FRONTEND_QT)
#include "QtFrontend/main.h"
#elif defined (FRONTEND_X11)
#include "X11Frontend/main.h"
#elif defined (FRONTEND_WAYLAND)
#include "WaylandFrontend/main.h"
#elif defined (FRONTEND_CACA)
#include "LibcacaFrontend/main.h"
#endif

#include "crash_dump/crash_handler.h"
#include "common/logging.h"

extern void createPidFile();
extern void cleanPidFile();
extern void errorInfo();
extern void setVideoStopFlag();
#if CRASH_DUMP
extern void crash_handler(int sig, siginfo_t *sig_info, void *context);
extern void crash_handler_sigaction(struct sigaction *act);
#endif

extern CamParams camParams;

int main(int argc, char *argv[])
{
    struct sigaction act;
    memset(&act, 0, sizeof(struct sigaction));

#ifdef CRASH_DUMP
    //crash_handler_sigaction(&act);
    act.sa_sigaction = crash_handler;
    act.sa_flags = SA_SIGINFO;

    sigaction(SIGABRT, &act, nullptr);
    sigaction(SIGSEGV, &act, nullptr);
    sigaction(SIGBUS, &act, nullptr);
    sigaction(SIGFPE, &act, nullptr);
    sigaction(SIGILL, &act, nullptr);
#endif

    atexit(cleanPidFile);
    at_quick_exit(cleanPidFile);
    at_quick_exit(errorInfo);
    createPidFile();

    if (parse_args(argc, argv, &camParams))
        return EXIT_FAILURE;

    LOG_INFO("DCamera running...\n");

    return run(argc, argv);
}
