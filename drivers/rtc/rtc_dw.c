/*
 * Copyright (c) 2015 Intel Corporation.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <nanokernel.h>
#include <sys_io.h>
#include <stdio.h>
#include <rtc.h>
#include <init.h>
#include "board.h"

#include "rtc_dw.h"

#define CLK_RTC_DIV_DEF_MASK (0xFFFFFF83)
#define CCU_RTC_CLK_DIV_EN (2)

static void (*rtc_dw_cb_fn)(void);

static void rtc_dw_set_div(const enum clk_rtc_div div)
{
	/* set default division mask */
	uint32_t reg =
		sys_read32(CLOCK_SYSTEM_CLOCK_CONTROL) & CLK_RTC_DIV_DEF_MASK;
	reg |= (div << CCU_RTC_CLK_DIV_OFFSET);
	sys_write32(reg, CLOCK_SYSTEM_CLOCK_CONTROL);
	/* CLK Div en bit must be written from 0 -> 1 to apply new value */
	sys_set_bit(CLOCK_SYSTEM_CLOCK_CONTROL, CCU_RTC_CLK_DIV_EN);
}

/**
 *  @brief   Function to enable clock gating for the RTC
 *  @return  N/A
 */
static void rtc_dw_enable(void)
{
	sys_set_bit(CLOCK_PERIPHERAL_BASE_ADDR, 11);
	sys_set_bit(CLOCK_PERIPHERAL_BASE_ADDR, 1);
}

/**
 *  @brief   Function to disable clock gating for the RTC
 *  @return  N/A
 */
static void rtc_dw_disable(void)
{
	sys_clear_bit(CLOCK_PERIPHERAL_BASE_ADDR, 11);
}

/**
 *  @brief   RTC alarm ISR
 *
 *  calls a user defined callback
 *
 *  @return  N/A
 */
void rtc_dw_isr(void)
{
	/*  Disable RTC interrupt */
	sys_clear_bit(RTC_BASE_ADDR + RTC_CCR, 0);

	if (rtc_dw_cb_fn) {
		(*rtc_dw_cb_fn)();
	}

	/* clear interrupt */
	sys_read32(RTC_BASE_ADDR + RTC_EOI);
}
IRQ_CONNECT_STATIC(rtc, INT_RTC_IRQ, CONFIG_RTC_IRQ_PRI, rtc_dw_isr, 0, 0);

/**
 * @brief Sets an RTC alarm
 * @param alarm_val Alarm value
 * @return 0 on success
 */
static int rtc_dw_set_alarm(const uint32_t alarm_val)
{
	sys_set_bit(RTC_BASE_ADDR + RTC_CCR, 0);

	sys_write32(alarm_val, RTC_BASE_ADDR + RTC_CMR);

	return DEV_OK;
}

/**
 *  @brief   Function to configure the RTC
 *  @param   config  pointer to a RTC configuration structure
 *  @return  0 on success
 */
static int rtc_dw_set_config(struct rtc_config *config)
{
	/*  Set RTC divider - 32768 / 32.768 khz = 1 second.  */
	rtc_dw_set_div(RTC_DIVIDER);

	/* set initial RTC value */
	sys_write32(config->init_val, RTC_BASE_ADDR + RTC_CLR);

	/* clear any pending interrupts */
	sys_read32(RTC_BASE_ADDR + RTC_EOI);

	rtc_dw_cb_fn = config->cb_fn;
	if (config->alarm_enable) {
		rtc_dw_set_alarm(config->alarm_val);
	} else {
		sys_clear_bit(RTC_BASE_ADDR + RTC_CCR, 0);
	}

	return DEV_OK;
}

/**
 * @brief Read current RTC value
 * @return current rtc value
 */
static uint32_t rtc_dw_read(void)
{
	return sys_read32(RTC_BASE_ADDR + RTC_CCVR);
}

static struct rtc_driver_api funcs = {
	.set_config = rtc_dw_set_config,
	.read = rtc_dw_read,
	.enable = rtc_dw_enable,
	.disable = rtc_dw_disable,
	.set_alarm = rtc_dw_set_alarm,
};

int rtc_dw_init(struct device *dev)
{
	IRQ_CONFIG(rtc, INT_RTC_IRQ);
	irq_enable(INT_RTC_IRQ);
	SCSS_INTERRUPT->int_rtc_mask = INT_UNMASK_IA;
	dev->driver_api = &funcs;
	return DEV_OK;
}

struct rtc_dw_dev_config rtc_dev = {
	.base_address = RTC_BASE_ADDR,
};

#ifdef CONFIG_RTC_DW
DECLARE_DEVICE_INIT_CONFIG(rtc, RTC_DRV_NAME, &rtc_dw_init, &rtc_dev);

SYS_DEFINE_DEVICE(rtc, NULL, SECONDARY, CONFIG_KERNEL_INIT_PRIORITY_DEVICE);
#endif
