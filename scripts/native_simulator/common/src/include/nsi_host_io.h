/*
 * Copyright 2026 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef NSI_COMMON_SRC_INCL_NSI_HOST_IO_H
#define NSI_COMMON_SRC_INCL_NSI_HOST_IO_H

#ifdef __cplusplus
extern "C" {
#endif

int nsi_host_open_read(const char *pathname);
int nsi_host_open_write_truncate(const char *pathname);

#ifdef __cplusplus
}
#endif

#endif /* NSI_COMMON_SRC_INCL_NSI_HOST_IO_H */
