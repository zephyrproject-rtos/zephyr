/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @brief Header containing address/offets and functions for writing
 * the FICR fields of the OTP memory on nRF7002 device
 */

#ifndef __OTP_PROG_H_
#define __OTP_PROG_H_

#include <stdio.h>
#include <stdlib.h>

int write_otp_memory(unsigned int otp_addr, unsigned int *write_val);
int read_otp_memory(unsigned int otp_addr, unsigned int *read_val, int len);
unsigned int check_protection(unsigned int *buff, unsigned int off1, unsigned int off2,
					unsigned int off3, unsigned int off4);

#endif /* __OTP_PROG_H_ */
