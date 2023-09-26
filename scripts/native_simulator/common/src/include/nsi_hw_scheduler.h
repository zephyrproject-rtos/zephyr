/*
 * Copyright (c) 2017 Oticon A/S
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef NSI_COMMON_SRC_INCL_HW_SCHEDULER_H
#define NSI_COMMON_SRC_INCL_HW_SCHEDULER_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define NSI_NEVER UINT64_MAX

/* API intended for the native simulator specific embedded drivers: */
uint64_t nsi_hws_get_time(void);

/* Internal APIs to the native_simulator and its HW models: */
void nsi_hws_init(void);
void nsi_hws_cleanup(void);
void nsi_hws_one_event(void);
void nsi_hws_set_end_of_time(uint64_t new_end_of_time);
void nsi_hws_find_next_event(void);
uint64_t nsi_hws_get_next_event_time(void);

#ifdef __cplusplus
}
#endif

#endif /* NSI_COMMON_SRC_INCL_HW_SCHEDULER_H */
