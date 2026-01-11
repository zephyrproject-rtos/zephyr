/*
 * Copyright (c) 2025 STMicroelectronics
 * Derived from soc/st/stm32/common/stm32_backup_domain.h
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef GD32_BACKUP_DOMAIN_H_
#define GD32_BACKUP_DOMAIN_H_

#ifdef __cplusplus
extern "C" {
#endif

#ifdef CONFIG_GD32_BACKUP_PROTECTION
void gd32_backup_domain_enable_access(void);
void gd32_backup_domain_disable_access(void);
#else
static inline void gd32_backup_domain_enable_access(void) { }
static inline void gd32_backup_domain_disable_access(void) { }
#endif

#ifdef __cplusplus
}
#endif

#endif /* GD32_BACKUP_DOMAIN_H_ */
