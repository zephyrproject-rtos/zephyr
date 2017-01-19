/* nrf51-pm.h Power Management for nrf51 chip */

/*
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

int nrf51_allow_sleep(void);
int nrf51_wakeup(void);
int nrf51_init(struct device *dev);
