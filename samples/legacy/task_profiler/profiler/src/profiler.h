/* profiler.h - Profiler code for flushing data on UART */

/*
 * Copyright (c) 2016 Intel Corporation
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef PROFILER_H
#define PROFILER_H

extern void prof_flush(void);

#if defined(CONFIG_CONSOLE_HANDLER_SHELL) && defined(CONFIG_KERNEL_EVENT_LOGGER_DYNAMIC)
extern int shell_cmd_prof(int argc, char *argv[]);
#define PROF_CMD { "prof", shell_cmd_prof, "<start|stop|cfg> [cfg1] [cfg2]" }
#else
#define PROF_CMD { "prof", NULL }
#endif

#endif /* PROFILER_H */

