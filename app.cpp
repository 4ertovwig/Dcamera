/*
 * This file is part of Dcamera project.
 * See the LICENSE file in the root directory for full license terms.
 */
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/file.h>
#include <fcntl.h>
#include <signal.h>

#include "common/logging.h"
#include "sync.h"


#define DCAMERA_PID_DIR     "/tmp"
#define RUN_PID             "/dcamera.pid"
#define DCAMERA_RUN_PID		DCAMERA_PID_DIR RUN_PID

// For video thread stopping
extern SynchronizeData syncData;

void createPidFile()
{
    int mode = S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH;
    int fd = open(DCAMERA_RUN_PID, O_RDWR | O_CREAT, mode);
    pid_t pid = getpid();
	if (fd == -1)
    {
		LOG_FATAL("Cannot open or reopen " DCAMERA_RUN_PID ": %s",
            strerror(errno));

        if (kill(pid, SIGKILL) != 0)
        {
            LOG_FATAL("Kill dcamera with pid: %d\n", pid);
        }
        exit(EXIT_SUCCESS);
    }

    if (flock(fd, LOCK_EX | LOCK_NB) != 0)
    {
        LOG_FATAL("Kill dcamera with pid: %d\n", pid);
        exit(EXIT_SUCCESS);
    }
}

void cleanPidFile()
{
	int ret = remove(DCAMERA_RUN_PID);
	if (ret != 0)
		LOG_ERROR("Cannot remove " DCAMERA_RUN_PID ": %s. Bad situatuion...",
            strerror(errno));
}

void setVideoStopFlag()
{
    syncData.stopStream = true;
}

void errorInfo()
{
    LOG_FATAL("Internal error:\n"
        "       video device not founded.\n"
        "       1) If you use libuvc:\n"
        "       See 'libusb' ouput and udev rules if use libuvc.\n"
        "       Try to sudo chmod 666 /dev/bus/usb/${bus_number}/${device_number}\n"
        "       For example:\n"
        "       $ lsusb\n"
        "       Bus 001 Device 007: ID 046d:0826 Logitech, Inc. HD Webcam C525\n"
        "       ...\n"
        "       2) If you use v4l2:\n"
        "       video4linux framework may not available in your kernel\n"
        "       or try:\n"
        "       $ modprobe -r uvcvideo && modprobe uvcvideo"
        "       3) If you are using a wayland backend, wayland compositor(sway, weston...) must be running\n"
        "       4) Error in caca_set_display_driver with backend x11 or slang...\n"
        "       Yor libcaca library must be supported x11 or slang backend. Try to build libcaca with --enable-x11 or --enable-slang\r");
}
