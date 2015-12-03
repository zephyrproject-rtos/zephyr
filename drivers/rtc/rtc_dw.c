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
#include <clock_control.h>
#include "board.h"

#include "rtc_dw.h"

#define CLK_RTC_DIV_DEF_MASK (0xFFFFFF83)
#define CCU_RTC_CLK_DIV_EN (2)

#ifdef RTC_DW_INT_MASK
static inline void _rtc_dw_int_unmask(void)
{
	sys_write32(sys_read32(RTC_DW_INT_MASK) & INT_UNMASK_IA,
						RTC_DW_INT_MASK);
}
#else
#define _rtc_dw_int_unmask()
#endif

#ifdef CONFIG_RTC_DW_CLOCK_GATE
static inline void _rtc_dw_clock_config(struct device *dev)
{
	char *drv = CONFIG_RTC_DW_CLOCK_GATE_DRV_NAME;
	struct device *clk;

	clk = device_get_binding(drv);
	if (clk) {
		struct rtc_dw_runtime *context = dev->driver_data;

		context->clock = clk;
	}
}

static inline void _rtc_dw_clock_on(struct device *dev)
{
	struct rtc_dw_dev_config *config = dev->config->config_info;
	struct rtc_dw_runtime *context = dev->driver_data;

	clock_control_on(context->clock, config->clock_data);
}

static inline void _rtc_dw_clock_off(struct device *dev)
{
	struct rtc_dw_dev_config *config = dev->config->config_info;
	struct rtc_dw_runtime *context = dev->driver_data;

	clock_control_off(context->clock, config->clock_data);
}
#else
#define _rtc_dw_clock_config(...)
#define _rtc_dw_clock_on(...)
#define _rtc_dw_clock_off(...)
#endif

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
static void rtc_dw_enable(struct device *dev)
{
	_rtc_dw_clock_on(dev);
}

/**
 *  @brief   Function to disable clock gating for the RTC
 *  @return  N/A
 */
static void rtc_dw_disable(struct device *dev)
{
	_rtc_dw_clock_off(dev);
}

/**
 *  @brief   RTC alarm ISR
 *
 *  calls a user defined callback
 *
 *  @return  N/A
 */
void rtc_dw_isr(void *arg)
{
	struct device *dev = arg;
	struct rtc_dw_dev_config *rtc_dev = dev->config->config_info;
	struct rtc_dw_runtime *context = dev->driver_data;

	/*  Disable RTC interrupt */
	sys_clear_bit(rtc_dev->base_address + RTC_CCR, 0);

	if (context->rtc_dw_cb_fn) {
		context->rtc_dw_cb_fn(dev);
	}

	/* clear interrupt */
	sys_read32(rtc_dev->base_address + RTC_EOI);
}

/**
 * @brief Sets an RTC alarm
 * @param alarm_val Alarm value
 * @return 0 on success
 */
static int rtc_dw_set_alarm(struct device *dev, const uint32_t alarm_val)
{
	struct rtc_dw_dev_config *rtc_dev = dev->config->config_info;

	sys_set_bit(rtc_dev->base_address + RTC_CCR, 0);

	sys_write32(alarm_val, rtc_dev->base_address + RTC_CMR);

	return DEV_OK;
}

/**
 *  @brief   Function to configure the RTC
 *  @param   config  pointer to a RTC configuration structure
 *  @return  0 on success
 */
static int rtc_dw_set_config(struct device *dev, struct rtc_config *config)
{
	struct rtc_dw_dev_config *rtc_dev = dev->config->config_info;
	struct rtc_dw_runtime *context = dev->driver_data;

	/*  Set RTC divider - 32768 / 32.768 khz = 1 second.  */
	rtc_dw_set_div(RTC_DIVIDER);

	/* set initial RTC value */
	sys_write32(config->init_val, rtc_dev->base_address + RTC_CLR);

	/* clear any pending interrupts */
	sys_read32(rtc_dev->base_address + RTC_EOI);

	context->rtc_dw_cb_fn = config->cb_fn;
	if (config->alarm_enable) {
		rtc_dw_set_alarm(dev, config->alarm_val);
	} else {
		sys_clear_bit(rtc_dev->base_address + RTC_CCR, 0);
	}

	return DEV_OK;
}

/**
 * @brief Read current RTC value
 * @return current rtc value
 */
static uint32_t rtc_dw_read(struct device *dev)
{
	struct rtc_dw_dev_config *rtc_dev = dev->config->config_info;

	return sys_read32(rtc_dev->base_address + RTC_CCVR);
}

static struct rtc_driver_api funcs = {
	.set_config = rtc_dw_set_config,
	.read = rtc_dw_read,
	.enable = rtc_dw_enable,
	.disable = rtc_dw_disable,
	.set_alarm = rtc_dw_set_alarm,
};

/* IRQ_CONFIG needs the flags variable declared by IRQ_CONNECT_STATIC */
IRQ_CONNECT_STATIC(rtc, CONFIG_RTC_DW_IRQ,
		   CONFIG_RTC_DW_IRQ_PRI, rtc_dw_isr, 0, 0);

int rtc_dw_init(struct device *dev)
{
	IRQ_CONFIG(rtc, CONFIG_RTC_DW_IRQ);
	irq_enable(CONFIG_RTC_DW_IRQ);

	_rtc_dw_int_unmask();

	_rtc_dw_clock_config(dev);

	dev->driver_api = &funcs;

	return DEV_OK;
}

struct rtc_dw_runtime rtc_runtime;

struct rtc_dw_dev_config rtc_dev = {
	.base_address = CONFIG_RTC_DW_BASE_ADDR,
#ifdef CONFIG_RTC_DW_CLOCK_GATE
	.clock_data = UINT_TO_POINTER(CONFIG_RTC_DW_CLOCK_GATE_SUBSYS),
#endif
};

DECLARE_DEVICE_INIT_CONFIG(rtc, CONFIG_RTC_DW_DRV_NAME,
			   &rtc_dw_init, &rtc_dev);

SYS_DEFINE_DEVICE(rtc, &rtc_runtime, SECONDARY,
		  CONFIG_KERNEL_INIT_PRIORITY_DEVICE);

struct device *rtc_dw_isr_dev = SYS_GET_DEVICE(rtc);
