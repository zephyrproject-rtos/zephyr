/*  NVS: non volatile storage in flash
 *
 * Copyright (c) 2018 Laczen
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef __NVS_PRIV_H_
#define __NVS_PRIV_H_

#ifdef __cplusplus
extern "C" {
#endif

#define SYS_LOG_DOMAIN "fs/nvs"
#define SYS_LOG_LEVEL CONFIG_NVS_LOG_LEVEL
#include <logging/sys_log.h>

#define NVS_MAX_LEN 0x7ffd

/*
 * Special id values
 */
#define NVS_ID_EMPTY		0xFFFF
#define NVS_ID_JUMP		0x0000

/*
 * Status return values
 */
#define NVS_STATUS_NOSPACE 1

#define NVS_MIN_WRITE_BLOCK_SIZE 4

#ifdef __cplusplus
}
#endif

#endif /* __NVS_PRIV_H_ */
