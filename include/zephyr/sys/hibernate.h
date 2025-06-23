/*
 * Copyright (c) 2025 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

 #ifndef ZEPHYR_INCLUDE_SYS_HIBERNATE_H_
 #define ZEPHYR_INCLUDE_SYS_HIBERNATE_H_
 
 #include <zephyr/toolchain.h>
 
 #ifdef __cplusplus
 extern "C" {
 #endif
 
 /**
  * @defgroup sys_hibernate System hibernate
  * @ingroup os_services
  * @{
  */
 
 /** @cond INTERNAL_HIDDEN */
 
 /**
  * @brief System hibernate hook.
  *
  * This function needs to be implemented in platform code. It must only
  * perform a context save and immediate power off of the system
  */
 void z_sys_hibernate(void);
 
 /** @endcond */
 
 /**
  * @brief Perform a system hibernate.
  *
  * This function will perform a context sane and immediate power off of the system.
  * It is the responsibility of the caller to ensure that the system is in a safe state to
  * be powered off. Any required wake up sources must be enabled before calling
  * this function.
  *
  * @kconfig{CONFIG_HIBERNATE} needs to be enabled to use this API.
  */
 void sys_hibernate(void);
 
 /** @} */
 
 #ifdef __cplusplus
 }
 #endif
 
 #endif /* ZEPHYR_INCLUDE_SYS_HIBERNATE_H_ */
 