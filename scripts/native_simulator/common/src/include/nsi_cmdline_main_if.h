/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief API to the native simulator expects any integrating program has for handling
 *        command line argument parsing
 */

#ifndef NATIVE_SIMULATOR_COMMON_SRC_NSI_CMDLINE_MAIN_IF_H
#define NATIVE_SIMULATOR_COMMON_SRC_NSI_CMDLINE_MAIN_IF_H

#ifdef __cplusplus
extern "C" {
#endif

void nsi_handle_cmd_line(int argc, char *argv[]);
void nsi_register_extra_args(int argc, char *argv[]);

#ifdef __cplusplus
}
#endif

#endif /* NATIVE_SIMULATOR_COMMON_SRC_NSI_CMDLINE_MAIN_IF_H */
