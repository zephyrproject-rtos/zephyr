/* profiler.h - Profiler code for flushing data on UART */

/*
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef PROFILER_H
#define PROFILER_H

extern void prof_flush(void);

#if defined(CONFIG_CONSOLE_SHELL) && defined(CONFIG_KERNEL_EVENT_LOGGER_DYNAMIC)
extern int shell_cmd_prof(int argc, char *argv[]);
#define PROF_CMD { "prof", shell_cmd_prof, "<start|stop|cfg> [cfg1] [cfg2]" }
#else
#define PROF_CMD { "prof", NULL }
#endif

#endif /* PROFILER_H */

