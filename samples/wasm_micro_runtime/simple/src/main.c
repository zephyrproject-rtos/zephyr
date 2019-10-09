/*
 * Copyright (C) 2019 Intel Corporation.  All rights reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <stdlib.h>
#include <string.h>
#include "bh_platform.h"
#include "wasm_assert.h"
#include "wasm_log.h"
#include "wasm_platform_log.h"
#include "wasm_thread.h"
#include "wasm_export.h"
#include "wasm_memory.h"
#include "bh_memory.h"

#if 0
#define CONFIG_GLOBAL_HEAP_BUF_SIZE 131072
#define CONFIG_APP_STACK_SIZE 8192
#define CONFIG_APP_HEAP_SIZE 8192
#define CONFIG_MAIN_THREAD_STACK_SIZE 4096
#endif

int iwasm_main(void);

void main(void)
{
    iwasm_main();
    for (;;) {
        k_sleep(1000);
    }
}
