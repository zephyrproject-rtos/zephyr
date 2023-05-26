/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef NSI_COMMON_SRC_INCL_NSI_MAIN_H
#define NSI_COMMON_SRC_INCL_NSI_MAIN_H

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Terminate the execution of the native simulator
 *
 * exit_code: Requested exit code to the shell
 *            Note that other components may have requested a different
 *            exit code which may have precedence if it was !=0
 */
void nsi_exit(int exit_code);

#ifdef __cplusplus
}
#endif

#endif /* NSI_COMMON_SRC_INCL_NSI_MAIN_H */
