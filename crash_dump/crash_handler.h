/*
 * This file is part of Dcamera project.
 * See the LICENSE file in the root directory for full license terms.
 */
#pragma once

#include <signal.h>

#ifdef __cplusplus
extern "C"
{
#endif

//
void crash_handler_sigaction(struct sigaction *act);

// This function is called when the application crashes and a fork occurs and a gdb call occurs.
void crash_handler(int sig, siginfo_t* sig_info, void* context);

#ifdef __cplusplus
}
#endif
