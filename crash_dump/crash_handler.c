/*
 * This file is part of Dcamera project.
 * See the LICENSE file in the root directory for full license terms.
 */
#define _GNU_SOURCE
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <time.h>
#include <limits.h>
#include <string.h>
#include <stdlib.h>
#include <fcntl.h>

#include "crash_handler.h"
#include "../common/logging.h"

// itoa has problem with include
static char* itoa(char* p, int n)
{
    char* b = p;
    do
    {
        *p = '0' + n % 10;
        n /= 10;
        ++p;
    }
    while (n > 0);

    char* const res = p;
    *p-- = '\0';

    while (p > b)
    {
        char c = *b;
        *b = *p;
        *p = c;
        ++b;
        --p;
    }

    return res;
}

void crash_handler_sigaction(struct sigaction *act)
{
	act->sa_flags = 0;
	sigemptyset(&act->sa_mask);
	sigaddset(&act->sa_mask, SIGABRT);
	sigaddset(&act->sa_mask, SIGSEGV);
	sigaddset(&act->sa_mask, SIGBUS);
	sigaddset(&act->sa_mask, SIGFPE);
	sigaddset(&act->sa_mask, SIGILL);    
}

static const char* find_gdb_path()
{
    const char* common_paths[] = {
        "/usr/bin/gdb",
        "/usr/local/bin/gdb",
        "/bin/gdb",
        "/sbin/gdb",
        "/opt/local/bin/gdb",
        "/usr/pkg/bin/gdb",
        NULL
    };
    for (int i = 0; common_paths[i]; ++i)
    {
        if (access(common_paths[i], X_OK) == 0)
            return common_paths[i];
    }
    return NULL;
}

void crash_handler(int sig, siginfo_t* sig_info, void* context)
{
    struct timespec now;
    clock_gettime(CLOCK_REALTIME, &now);

    const pid_t self_pid = getpid();

    // find gdb in popular paths
    const char *gdbpath = find_gdb_path();
    if (!gdbpath)
    {
        LOG_ERROR("Can't find gdb in system...\n");
        exit(EXIT_FAILURE);
    }

    // Create a child process for the gdb
    pid_t pid = fork();
    if (pid > 0)
    {
        LOG_DEBUG("Stop dcamera\n");
        // Suspend the process to ensure all threads are closest to where they were when the crash occurred
        kill(self_pid, SIGSTOP);

        // Wait for the gdb to finish
        int status = 0;
        waitpid(pid, &status, 0);
    }
    else
    {
        LOG_DEBUG("Crash handler process spawn...\n");
        // gdb process id
        char pid_str[32] = {0};
        itoa(pid_str, self_pid);

        // Redirect debugger console to a file
        char full_file_name[PATH_MAX] = {0};
        int p = 0;
        char *name_dir = get_current_dir_name();
        strlcpy(full_file_name + p, name_dir, sizeof(full_file_name) - 1);
        p += strlen(name_dir);
        size_t len_file = strlen(full_file_name);
        full_file_name[len_file] = '/';
        full_file_name[len_file + 1] = '\0';
        p += 1;
        char *name_stacktrace = "stacktrace.";
        strlcpy(full_file_name + p, name_stacktrace, sizeof(full_file_name) - 1);
        p += strlen(name_stacktrace);
        struct tm *timeinfo = localtime(&now.tv_sec);
        char timebuf[80];
        strftime(timebuf, sizeof(timebuf), "%Y-%m-%d %H:%M:%S", timeinfo);
        strlcpy(full_file_name + p, timebuf, sizeof(full_file_name) - 1);
        p += strlen(timebuf);
        len_file = strlen(full_file_name);
        full_file_name[len_file] = '.';
        full_file_name[len_file + 1] = '\0';
        p += 1;
        strlcpy(full_file_name + p, pid_str, sizeof(full_file_name) - 1);
        p += strlen(pid_str);
        strlcpy(full_file_name + p, ".txt", sizeof(full_file_name) - 1);
        p += 5;
        int console_fd = open(full_file_name, O_WRONLY | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH);
        if (console_fd != -1)
        {
            dup2(console_fd, STDOUT_FILENO);
            dup2(console_fd, STDERR_FILENO);
            close(console_fd);
        }

        len_file = strlen(full_file_name);
        // Setup debugger command line arguments
        char pid_arg[sizeof("--eval-command=attach ") + sizeof(pid_str)] = "--eval-command=attach ";
        char kill_arg[sizeof("--eval-command=shell kill -9 ") + sizeof(pid_str)] = "--eval-command=shell kill -9 ";
        char* logging_arg = malloc(sizeof("--eval-command=set logging file ") + len_file);
        strcpy(logging_arg, "--eval-command=set logging file ");
        const char *const args[] = {
                gdbpath,
                "--batch",
                "--eval-command=set pagination off",
                pid_arg,
                "--eval-command=info thread",
                logging_arg,
                "--eval-command=set logging on",
                "--eval-command=thread apply all bt",
                "--eval-command=set logging off",
                "--eval-command=kill",
                kill_arg, // this kill command is needed to kill the process even if gdb fails to attach to it
                NULL
        };
        
		strcpy(pid_arg + sizeof("--eval-command=attach ") - 1, pid_str);
        strcpy(logging_arg + sizeof("--eval-command=set logging file ") - 1, full_file_name);
		strcpy(kill_arg + sizeof("--eval-command=shell kill -9 ") - 1, pid_str);

        char *gdb_env[] = {NULL};
        execve(gdbpath, args, &gdb_env[0]);

        free(logging_arg);
        free(name_dir);
        LOG_DEBUG("Stack trace file %s\n", full_file_name);
        // If executing the debugger failed, kill the crashed process and terminate
        kill(self_pid, SIGKILL);
        exit(-1);
    }
}
