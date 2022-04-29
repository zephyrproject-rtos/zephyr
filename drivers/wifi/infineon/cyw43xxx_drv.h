/*
 * Copyright (c) 2022 Cypress Semiconductor Corporation (an Infineon company) or
 * an affiliate of Cypress Semiconductor Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * \brief This function returns pointer type to handle instance
 *        of whd interface (whd_interface_t) which allocated in
 *        Zephyr CYW43xxx driver (drivers/wifi/infineon/cy43xxx_drv.c)
 */

whd_interface_t cyw43xx_get_whd_interface(void);
