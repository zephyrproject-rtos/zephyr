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
 * @brief Like nsi_exit(), do all cleanup required to terminate the
 *        execution of the native_simulator, but instead of exiting,
 *        return to the caller what would have been passed to exit()
 *
 * @param[in] exit_code: Requested exit code to the shell
 *            Note that other components may have requested a different
 *            exit code which may have precedence if it was !=0
 *
 * @returns Code which would have been passed to exit()
 */
int nsi_exit_inner(int exit_code);

/**
 * @brief Terminate the execution of the native simulator
 *
 * @param[in] exit_code: Requested exit code to the shell
 *            Note that other components may have requested a different
 *            exit code which may have precedence if it was !=0
 */
void nsi_exit(int exit_code);

#ifdef __cplusplus
}
#endif

#endif /* NSI_COMMON_SRC_INCL_NSI_MAIN_H */
