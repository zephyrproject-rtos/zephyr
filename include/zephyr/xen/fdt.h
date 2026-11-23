/*
 * Copyright (c) 2026 EPAM Systems
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Xen boot device tree access.
 */

#ifndef ZEPHYR_INCLUDE_ZEPHYR_XEN_FDT_H_
#define ZEPHYR_INCLUDE_ZEPHYR_XEN_FDT_H_

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Get the copied Xen boot device tree.
 *
 * @kconfig_dep{CONFIG_XEN_FDT}
 *
 * The returned pointer refers to the Zephyr-owned copy made from the Xen boot
 * firmware arguments during early boot.
 *
 * @param fdt Output for the copied FDT pointer.
 * @param fdt_size Output for the copied FDT size in bytes.
 *
 * @retval 0 If the copied FDT is available.
 * @retval -EINVAL If @p fdt or @p fdt_size is NULL.
 * @retval -ENOENT If no valid FDT copy has been recorded.
 */
int xen_fdt_get(const uint8_t **fdt, uint32_t *fdt_size);

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_ZEPHYR_XEN_FDT_H_ */
