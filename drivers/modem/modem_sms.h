/** @file
 * @brief Modem SMS for SMS common structure.
 *
 * Modem SMS handling for modem driver.
 */

/*
 * Copyright (c) 2021 John Lange <john@y2038.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_MODEM_MODEM_SMS_H_
#define ZEPHYR_INCLUDE_DRIVERS_MODEM_MODEM_SMS_H_

#define SMS_PHONE_MAX_LEN          16
#define SMS_TIME_MAX_LEN           26

#ifndef CONFIG_SMS_IN_MSG_MAX_LEN
#define CONFIG_SMS_IN_MSG_MAX_LEN  168
#endif

#ifndef CONFIG_SMS_OUT_MSG_MAX_LEN
#define CONFIG_SMS_OUT_MSG_MAX_LEN 168
#endif

/*
 * All fields in sms_out and sms_in are NULL terminated
 */

struct sms_out {
        char phone[SMS_PHONE_MAX_LEN];
        char msg  [CONFIG_SMS_OUT_MSG_MAX_LEN];
};

struct sms_in {
        char phone[SMS_PHONE_MAX_LEN];
        char time [SMS_TIME_MAX_LEN];
        char msg  [CONFIG_SMS_IN_MSG_MAX_LEN];
};

typedef int (*send_sms_func_t)(void *obj, const struct sms_out *sms);
typedef int (*recv_sms_func_t)(void *obj,       struct sms_in  *sms, k_timeout_t timeout);

enum io_ctl {
	SMS_SEND,
	SMS_RECV,
};

#endif /* ZEPHYR_INCLUDE_DRIVERS_MODEM_MODEM_SMS_H_ */
