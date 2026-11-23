/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdarg.h>

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/util.h>

/* Set while RF calibration runs, see blob_printf(). */
static volatile bool bflb_rf_cal_active;

extern int __real_rfc_init(uint32_t xtal);
extern void rf_pri_init_calib_mem(void);

#ifdef CONFIG_BT_BFLB_BL60X
extern void bflb_ble_irq_setup(void);

int bflb_rf_init(void)
{
	bflb_ble_irq_setup();
	return 0;
}
#endif

/* RF calibration, called by the BLE and WiFi blobs. With WiFi, some
 * calibration paths poll ADC registers with tight timing and preempting
 * between register accesses leaves the MAC/PHY bus wedged: run it with
 * IRQs locked and pre-init the cal memory.
 */
int __wrap_rfc_init(uint32_t xtal)
{
	unsigned int key = 0U;
	int r;

	bflb_rf_cal_active = true;
	if (IS_ENABLED(CONFIG_WIFI_BFLB)) {
		rf_pri_init_calib_mem();
		key = irq_lock();
	}
	r = __real_rfc_init(xtal);
	if (IS_ENABLED(CONFIG_WIFI_BFLB)) {
		irq_unlock(key);
	}
	bflb_rf_cal_active = false;

	return r;
}

/* The PHY library logs with bare printf; route it to the logging subsystem,
 * and drop it entirely while calibration runs because the output latency
 * there breaks PHY timing.
 */
__printf_like(1, 2) int blob_printf(const char *fmt, ...)
{
#ifdef CONFIG_LOG
	va_list ap;

	if (bflb_rf_cal_active) {
		return 0;
	}

	va_start(ap, fmt);
	log_generic(LOG_LEVEL_INF, fmt, ap);
	va_end(ap);
#else
	ARG_UNUSED(fmt);
#endif
	return 0;
}
