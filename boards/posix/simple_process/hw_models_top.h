/*
 * Copyright (c) 2017 Oticon A/S
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _SIMPLE_PROCESS_HW_MODELS_H
#define _SIMPLE_PROCESS_HW_MODELS_H

#include <stdint.h>
#include <inttypes.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef uint64_t hwtime_t;
#define NEVER UINT64_MAX
#define PRItime PRIu64

void hwm_main_loop(void);
void hwm_init(void);
void hwm_cleanup(void);
void hwm_set_end_of_time(hwtime_t new_end_of_time);
hwtime_t hwm_get_time(void);


#ifdef __cplusplus
}
#endif

#endif /* _SIMPLE_PROCESS_HW_MODELS_H */
