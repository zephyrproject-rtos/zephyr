/* Bluetooth: Mesh Generic OnOff, Generic Level, Lighting & Vendor Models
 *
 * Copyright (c) 2018 Vikrant More
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _SMP_SVR_H
#define _SMP_SVR_H

extern struct k_timer smp_svr_timer;

void smp_svr_init(void);
void smp_svr(void);

#endif
