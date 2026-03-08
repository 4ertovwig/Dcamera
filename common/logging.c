/*
 * This file is part of Dcamera project.
 * See the LICENSE file in the root directory for full license terms.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <stdbool.h>
#include <pthread.h>
#include <sys/time.h>

#include "logging.h"

static pthread_mutex_t LastTimeMutex = PTHREAD_MUTEX_INITIALIZER;
static int debug_buf_size = 1024;

LogLevel debuglvl = INFO;
extern bool ColorOutput;

    static const char *loglvl2str(LogLevel lvl)
    {
        switch (lvl)
        {
        case SYMBOL:
            return "[SYMBOL]";
        case EMPTY:
            return "[EMPTY]";
        case INFO:
            return "[INFO]";
        case DEBUG:
            return "[DEBUG]";
        case WARNING:
            return "[WARNING]";
        case ERROR:
            return "[ERROR]";
        case FATAL:
            return "[FATAL]";
        case DEVEL:
            return "[DEVEL]";
        }
    }

    static void log_line(const int lvl, const char *logBuf)
    {
        static struct timeval last_time = {0, 0};
        struct timeval new_time = {0, 0};
        struct timeval tmp;
        pthread_t thread_id;
        int delta;

        (void)pthread_mutex_lock(&LastTimeMutex);
        gettimeofday(&new_time, NULL);
        if (0 == last_time.tv_sec)
            last_time = new_time;

        tmp.tv_sec = new_time.tv_sec - last_time.tv_sec;
        tmp.tv_usec = new_time.tv_usec - last_time.tv_usec;
        if (tmp.tv_usec < 0)
        {
            tmp.tv_sec--;
            tmp.tv_usec += 1000000;
        }
        if (tmp.tv_sec < 100)
            delta = tmp.tv_sec * 1000000 + tmp.tv_usec;
        else
            delta = 99999999;

        last_time = new_time;

        const char *color_pfx = "", *color_sfx = "";

        if (ColorOutput)
        {
            color_sfx = "\33[0m";
            switch (lvl)
            {
                case ERROR:
                    color_pfx = "\033[01;31m"; // bright + Red
                    break;

                case WARNING:
                    color_pfx = "\033[33m"; /// Yellow
                    break;

                case INFO:
                    color_pfx = "\033[32m"; // Green
                    break;
            }
        }

        const char* logstr = "";
        if (lvl == DEVEL)
        {
            logstr = "%s %.8d [%u] %s%s%s";
            thread_id = pthread_self();
            printf(logstr, loglvl2str(lvl), delta, thread_id, color_pfx, logBuf, color_sfx);
        }
        else if (lvl == SYMBOL)
        {
            printf("%s", logBuf);
        }
        else
        {
            logstr = "%s %.8d %s%s%s";
            printf(logstr, loglvl2str(lvl), delta, color_pfx, logBuf, color_sfx);

        }

        fflush(stdout);
        (void)pthread_mutex_unlock(&LastTimeMutex);
    }

    void log_set_lvl(LogLevel lvl)
    {
        (void)pthread_mutex_lock(&LastTimeMutex);
        debuglvl = lvl;
        (void)pthread_mutex_unlock(&LastTimeMutex);
    }

    void log_msg(LogLevel lvl, const char *fmt, ...)
    {
        if (lvl > debuglvl)
            return;

        char logBuf[debug_buf_size];
        memset(logBuf, 0, debug_buf_size);
        va_list arglist;
        va_start(arglist, fmt);
        vsnprintf(logBuf, sizeof(logBuf), fmt, arglist);
        va_end(arglist);

        log_line(lvl, logBuf);
    }

    void log_symbol(LogLevel lvl, char fmt)
    {
        if (lvl > debuglvl)
            return;

        char logBuf[1] = {fmt};
        log_line(lvl, logBuf);
    }
