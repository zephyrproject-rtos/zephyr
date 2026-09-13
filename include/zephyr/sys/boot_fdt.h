/*
 * Copyright (c) 2026 EPAM Systems
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Boot firmware device tree helpers.
 */

#ifndef ZEPHYR_INCLUDE_ZEPHYR_SYS_BOOT_FDT_H_
#define ZEPHYR_INCLUDE_ZEPHYR_SYS_BOOT_FDT_H_

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Validate and copy a boot firmware device tree blob.
 *
 * @kconfig_dep{CONFIG_BOOT_FDT}
 *
 * This helper validates the FDT header magic and total-size field, checks that
 * the blob fits in the caller-provided storage, and copies the blob into that
 * storage. The caller owns the destination buffer and decides how long the
 * copied blob remains available.
 *
 * @param dst Destination buffer for the copied FDT.
 * @param dst_size Destination buffer size in bytes.
 * @param src Source FDT pointer.
 * @param fdt_size Output for the FDT size in bytes. Set on success and when
 *		   returning -ENOSPC.
 *
 * @retval 0 If the FDT was validated and copied.
 * @retval -EINVAL If an argument is invalid or @p src is not a valid FDT.
 * @retval -ENOSPC If @p dst is too small for the FDT.
 */
int sys_boot_fdt_copy(uint8_t *dst, size_t dst_size, const uint8_t *src,
		      uint32_t *fdt_size);

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_ZEPHYR_SYS_BOOT_FDT_H_ */
