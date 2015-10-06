/** @file
 * @brief Dining philosophers header file.
 *
 * Collects the includes and defines needed to implement the dining philosophers
 * example.
 */

/*
 * Copyright (c) 2012-2014 Wind River Systems, Inc.
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

/* Needed includes. */

#if defined(CONFIG_STDOUT_CONSOLE)
  #include <stdio.h>
#else
  #include <misc/printk.h>
#endif

/**
 * @def N_PHILOSOPHERS
 * @brief Defines the number of philosophers.
 *
 * @details Multiple tasks do printfs and they may conflict.
 * Uses puts() instead of printf() to avoid conflicts.
 */

#define N_PHILOSOPHERS	6

#if defined(CONFIG_STDOUT_CONSOLE)
  #define PRINTF(...) {char output[256]; sprintf(output, __VA_ARGS__); puts(output);}
#else
  #define PRINTF(...) printk(__VA_ARGS__)
#endif
