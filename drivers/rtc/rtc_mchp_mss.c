/*
 * Copyright (c) 2026 Microchip Technology Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/rtc.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/sys_io.h>
#include <zephyr/sys/util.h>
#include "rtc_utils.h"

#define DT_DRV_COMPAT microchip_rtc_mss

LOG_MODULE_REGISTER(rtc_mchp_mss, CONFIG_RTC_LOG_LEVEL);

#define MSS_RTC_RESET_ENABLED DT_ANY_INST_HAS_PROP_STATUS_OKAY(resets)

#if MSS_RTC_RESET_ENABLED
#include <zephyr/drivers/reset.h>
#endif

#define CONTROL_REG        0x00u
#define MODE_REG           0x04u
#define PRESCALER_REG      0x08u
#define ALARM_LOWER_REG    0x0cu
#define ALARM_UPPER_REG    0x10u
#define COMPARE_LOWER_REG  0x14u
#define COMPARE_UPPER_REG  0x18u
#define DATETIME_LOWER_REG 0x20u
#define DATETIME_UPPER_REG 0x24u

#define CONTROL_START_BIT      BIT(0)
#define CONTROL_STOP_BIT       BIT(1)
#define CONTROL_ALARM_ON_BIT   BIT(2)
#define CONTROL_ALARM_OFF_BIT  BIT(3)
#define CONTROL_RESET_BIT      BIT(4)
#define CONTROL_UPLOAD_BIT     BIT(5)
#define CONTROL_DOWNLOAD_BIT   BIT(6)
#define CONTROL_MATCH_BIT      BIT(7)
#define CONTROL_WAKEUP_CLR_BIT BIT(8)
#define CONTROL_WAKEUP_SET_BIT BIT(9)

#define MODE_CLK_MODE_MASK        0x00000001u
#define MODE_WAKEUP_EN_MASK       0x00000002u
#define MODE_WAKEUP_RESET_MASK    0x00000004u
#define MODE_WAKEUP_CONTINUE_MASK 0x00000008u

#define MAX_PRESCALAR_COUNT   0x03FFFFFFu
#define MSS_RTC_BINARY_MODE   0u
#define MSS_RTC_CALENDAR_MODE 1u

#define DATETIME_UPPER_MASK 0x3FFFFFFFu
#define ALARM_UPPER_MASK    0x0000FFFFu

#define CALCULATION_MASK_YEAR    0xFF0000000000u
#define CALCULATION_MASK_MONTH   0x0F00000000u
#define CALCULATION_MASK_DAY     0x1F000000u
#define CALCULATION_MASK_HOUR    0x1F0000u
#define CALCULATION_MASK_MIN     0x3F00u
#define CALCULATION_MASK_SEC     0x3Fu
#define CALCULATION_MASK_WEEKDAY 0x07000000000000u

#define RTC_ALARM_COUNT   DT_PROP(DT_NODELABEL(rtc), alarms_count)
#define RTC_ALARM_PENDING (1)

/* Supported fields for time validation (seconds through year) */
#define RTC_TIME_SUPPORTED_MASK                                                                    \
	(RTC_ALARM_TIME_MASK_SECOND | RTC_ALARM_TIME_MASK_MINUTE | RTC_ALARM_TIME_MASK_HOUR |      \
	 RTC_ALARM_TIME_MASK_MONTHDAY | RTC_ALARM_TIME_MASK_MONTH | RTC_ALARM_TIME_MASK_YEAR)

#ifdef CONFIG_RTC_ALARM
#define RTC_ALARM_SUPPORTED_MASK RTC_TIME_SUPPORTED_MASK
#endif

struct rtc_mchp_mss_dev_config {
	uint32_t clock_freq;
	uintptr_t base_address;
	uint32_t rtc_irq_base;
	uint32_t prescaler;
	void (*irq_config_func)(const struct device *dev);

#ifdef CONFIG_RTC_ALARM
	uint8_t alarms_count;
#endif

#if MSS_RTC_RESET_ENABLED
	struct reset_dt_spec reset_spec;
#endif
};

#ifdef CONFIG_RTC_ALARM
struct rtc_mchp_mss_data_cb {
	bool is_alarm_pending;
	rtc_alarm_callback alarm_cb;
	void *alarm_user_data;
};
#endif

struct rtc_mchp_mss_dev_data {
	struct k_sem lock;
#ifdef CONFIG_RTC_ALARM
	struct rtc_mchp_mss_data_cb alarms[RTC_ALARM_COUNT];
#endif
};

/*
 * Transform the structure rtc_time into uint64_t to be written in the registers.
 *
 * Note: tm_year is stored as years-since-1900 per POSIX convention. The hardware
 * year register field is 8 bits, so valid range is 0-255 (years 1900-2155).
 */
static inline uint64_t rtc_mchp_tm_to_time(const struct rtc_time *timeptr)
{
	uint64_t timr = 0u;

	timr = (uint64_t)((timeptr->tm_wday + 1) & 0x07);
	timr = (timr << 8) | (timeptr->tm_year & 0xFF);
	timr = (timr << 8) | ((timeptr->tm_mon + 1) & 0x0F);
	timr = (timr << 8) | (timeptr->tm_mday & 0x1F);
	timr = (timr << 8) | (timeptr->tm_hour & 0x1F);
	timr = (timr << 8) | (timeptr->tm_min & 0x3F);
	timr = (timr << 8) | (timeptr->tm_sec & 0x3F);

	return timr;
}

/* Transform the value read from registers uint64_t into a rtc_time struct */
static inline void rtc_mchp_time_to_tm(uint64_t timer, struct rtc_time *timeptr)
{
	timeptr->tm_sec = timer & CALCULATION_MASK_SEC;
	timeptr->tm_min = (timer & CALCULATION_MASK_MIN) >> 8;
	timeptr->tm_hour = (timer & CALCULATION_MASK_HOUR) >> 16;
	timeptr->tm_mday = (timer & CALCULATION_MASK_DAY) >> 24;
	timeptr->tm_mon = ((timer & CALCULATION_MASK_MONTH) >> 32) - 1;
	timeptr->tm_year = (timer & CALCULATION_MASK_YEAR) >> 40;
	timeptr->tm_wday = ((timer & CALCULATION_MASK_WEEKDAY) >> 48) - 1;

	/* There is no nano second support for RTC */
	timeptr->tm_nsec = 0;
	/* There is no day of the year support for RTC */
	timeptr->tm_yday = -1;
}

#ifdef CONFIG_RTC_ALARM
/*
 * Build compare register values from the alarm mask.
 *
 * Each field that is selected in the mask gets its corresponding bits set
 * in the compare registers (indicating the hardware should compare that field).
 * Fields not in the mask get zeroed (hardware ignores them for comparison).
 */
static void rtc_mchp_mss_build_compare_from_mask(uint16_t mask, uint32_t *compare_lower,
						 uint32_t *compare_upper)
{
	*compare_lower = 0;
	*compare_upper = 0;

	if (mask & RTC_ALARM_TIME_MASK_SECOND) {
		*compare_lower |= (uint32_t)CALCULATION_MASK_SEC;
	}
	if (mask & RTC_ALARM_TIME_MASK_MINUTE) {
		*compare_lower |= (uint32_t)((CALCULATION_MASK_MIN) & 0xFFFFFFFF);
	}
	if (mask & RTC_ALARM_TIME_MASK_HOUR) {
		*compare_lower |= (uint32_t)((CALCULATION_MASK_HOUR) & 0xFFFFFFFF);
	}
	if (mask & RTC_ALARM_TIME_MASK_MONTHDAY) {
		*compare_lower |= (uint32_t)((CALCULATION_MASK_DAY) & 0xFFFFFFFF);
	}
	if (mask & RTC_ALARM_TIME_MASK_MONTH) {
		*compare_upper |= (uint32_t)((CALCULATION_MASK_MONTH) >> 32);
	}
	if (mask & RTC_ALARM_TIME_MASK_YEAR) {
		*compare_upper |= (uint32_t)((CALCULATION_MASK_YEAR) >> 32);
	}
}
#endif

static int rtc_mchp_mss_set_time(const struct device *dev, const struct rtc_time *timeptr)
{
	const struct rtc_mchp_mss_dev_config *cfg = dev->config;
	struct rtc_mchp_mss_dev_data *data = dev->data;
	uint64_t timer;

	if (timeptr == NULL) {
		LOG_ERR("RTC set time failed: time pointer is NULL");
		return -EINVAL;
	}

	if (rtc_utils_validate_rtc_time(timeptr, RTC_TIME_SUPPORTED_MASK) == false) {
		LOG_ERR("RTC time parameters are invalid");
		return -EINVAL;
	}

	timer = rtc_mchp_tm_to_time(timeptr);

	k_sem_take(&data->lock, K_FOREVER);

	sys_write32((uint32_t)timer, cfg->base_address + DATETIME_LOWER_REG);
	sys_write32((uint32_t)(timer >> 32) & DATETIME_UPPER_MASK,
		    cfg->base_address + DATETIME_UPPER_REG);

	sys_write32(CONTROL_UPLOAD_BIT, cfg->base_address + CONTROL_REG);

	k_sem_give(&data->lock);

	return 0;
}

static int rtc_mchp_mss_get_time(const struct device *dev, struct rtc_time *timeptr)
{
	const struct rtc_mchp_mss_dev_config *cfg = dev->config;
	struct rtc_mchp_mss_dev_data *data = dev->data;
	uint64_t timer, timeupper;

	if (timeptr == NULL) {
		LOG_ERR("RTC get time failed: time pointer is NULL");
		return -EINVAL;
	}

	k_sem_take(&data->lock, K_FOREVER);

	/* Trigger a download to snapshot running counter into datetime registers */
	sys_write32(CONTROL_DOWNLOAD_BIT, cfg->base_address + CONTROL_REG);

	timer = sys_read32(cfg->base_address + DATETIME_LOWER_REG);
	timeupper = sys_read32(cfg->base_address + DATETIME_UPPER_REG) & DATETIME_UPPER_MASK;
	timer = timer | (timeupper << 32);

	k_sem_give(&data->lock);

	rtc_mchp_time_to_tm(timer, timeptr);

	return 0;
}

#ifdef CONFIG_RTC_ALARM
static int rtc_mchp_mss_get_alarm_supported_fields(const struct device *dev, uint16_t id,
						   uint16_t *mask)
{
	ARG_UNUSED(id);

	*mask = RTC_ALARM_SUPPORTED_MASK;

	return 0;
}

static int rtc_mchp_mss_alarm_is_pending(const struct device *dev, uint16_t id)
{
	const struct rtc_mchp_mss_dev_config *cfg = dev->config;
	struct rtc_mchp_mss_dev_data *data = dev->data;
	int retval = 0;

	if (id >= cfg->alarms_count) {
		LOG_ERR("RTC Alarm id is out of range");
		return -EINVAL;
	}

	/* Lock interrupts to ensure atomic check-and-clear of pending flag */
	unsigned int key = irq_lock();

	if (data->alarms[id].is_alarm_pending == true) {
		retval = RTC_ALARM_PENDING;
		data->alarms[id].is_alarm_pending = false;
	}

	irq_unlock(key);

	return retval;
}

static int rtc_mchp_mss_set_alarm_time(const struct device *dev, uint16_t id, uint16_t mask,
				       const struct rtc_time *timeptr)
{
	const struct rtc_mchp_mss_dev_config *cfg = dev->config;
	struct rtc_mchp_mss_dev_data *data = dev->data;

	uint32_t mode;
	uint64_t timer;
	uint16_t supported_mask;
	uint32_t compare_lower, compare_upper;

	rtc_mchp_mss_get_alarm_supported_fields(dev, 0, &supported_mask);

	/* Check if the mask, alarm and time have valid formats */
	if ((mask & ~supported_mask) != 0) {
		LOG_ERR("Invalid RTC alarm mask");
		return -EINVAL;
	}
	if (id >= cfg->alarms_count) {
		LOG_ERR("RTC Alarm id is out of range");
		return -EINVAL;
	}
	if ((timeptr == NULL) && (mask != 0)) {
		LOG_ERR("No pointer is provided to set RTC alarm");
		return -EINVAL;
	}
	if ((timeptr != NULL) && (rtc_utils_validate_rtc_time(timeptr, supported_mask) == false)) {
		LOG_ERR("Invalid RTC time provided");
		return -EINVAL;
	}

	k_sem_take(&data->lock, K_FOREVER);

	if (mask == 0) {
		/* Disable alarm when mask is zero */
		sys_write32(CONTROL_ALARM_OFF_BIT, cfg->base_address + CONTROL_REG);
		k_sem_give(&data->lock);
		return 0;
	}

	timer = rtc_mchp_tm_to_time(timeptr);

	/* Write the time in the alarm register */
	sys_write32((uint32_t)timer, cfg->base_address + ALARM_LOWER_REG);
	sys_write32((uint32_t)(timer >> 32) & ALARM_UPPER_MASK,
		    cfg->base_address + ALARM_UPPER_REG);

	/* Build compare registers from mask to select which fields to compare */
	rtc_mchp_mss_build_compare_from_mask(mask, &compare_lower, &compare_upper);
	sys_write32(compare_lower, cfg->base_address + COMPARE_LOWER_REG);
	sys_write32(compare_upper, cfg->base_address + COMPARE_UPPER_REG);

	/* Set wakeup/alarm mode: enable wakeup, continue counting, calendar mode */
	mode = sys_read32(cfg->base_address + MODE_REG);
	mode |= MODE_WAKEUP_EN_MASK | MODE_WAKEUP_CONTINUE_MASK | MODE_CLK_MODE_MASK;

	sys_write32(mode, cfg->base_address + MODE_REG);

	/* Enable the alarm */
	sys_write32(CONTROL_ALARM_ON_BIT, cfg->base_address + CONTROL_REG);

	k_sem_give(&data->lock);

	return 0;
}

static int rtc_mchp_mss_get_alarm_time(const struct device *dev, uint16_t id, uint16_t *mask,
				       struct rtc_time *timeptr)
{
	const struct rtc_mchp_mss_dev_config *cfg = dev->config;
	struct rtc_mchp_mss_dev_data *data = dev->data;

	uint64_t timer, timeupper;
	uint16_t supported_mask;

	/* Check if the alarm ID is within the valid range */
	if (id >= cfg->alarms_count) {
		LOG_ERR("RTC Alarm id is out of range");
		return -EINVAL;
	}

	/* Check if the time pointer is provided */
	if (timeptr == NULL) {
		LOG_ERR("No pointer is provided to get RTC alarm");
		return -EINVAL;
	}

	rtc_mchp_mss_get_alarm_supported_fields(dev, 0, &supported_mask);

	k_sem_take(&data->lock, K_FOREVER);

	timer = sys_read32(cfg->base_address + ALARM_LOWER_REG);
	timeupper = sys_read32(cfg->base_address + ALARM_UPPER_REG) & ALARM_UPPER_MASK;

	timer = timer | (timeupper << 32);

	k_sem_give(&data->lock);

	rtc_mchp_time_to_tm(timer, timeptr);

	/*
	 * The MSS RTC uses compare registers as the mask mechanism.
	 * Return the full supported mask since we can't easily reconstruct
	 * the original API mask from hardware compare register values.
	 */
	*mask = supported_mask;

	return 0;
}

static int rtc_mchp_mss_set_alarm_callback(const struct device *dev, uint16_t id,
					   rtc_alarm_callback callback, void *user_data)
{
	struct rtc_mchp_mss_dev_data *data = dev->data;
	const struct rtc_mchp_mss_dev_config *cfg = dev->config;

	/* Check if the alarm ID is within the valid range */
	if (id >= cfg->alarms_count) {
		LOG_ERR("RTC Alarm id is out of range");
		return -EINVAL;
	}

	/* Lock interrupts to ensure atomic setting of callback */
	unsigned int key = irq_lock();

	data->alarms[id].alarm_cb = callback;
	data->alarms[id].alarm_user_data = user_data;

	irq_unlock(key);

	return 0;
}

static void rtc_mchp_mss_isr(const struct device *dev)
{
	struct rtc_mchp_mss_dev_data *data = dev->data;
	const struct rtc_mchp_mss_dev_config *cfg = dev->config;
	uint32_t ctrl = sys_read32(cfg->base_address + CONTROL_REG);

	/* Check if a match occurred */
	if (ctrl & CONTROL_MATCH_BIT) {
		for (uint8_t id = 0; id < RTC_ALARM_COUNT; id++) {
			if (data->alarms[id].alarm_cb != NULL) {
				data->alarms[id].is_alarm_pending = false;
				data->alarms[id].alarm_cb(dev, id,
							  data->alarms[id].alarm_user_data);
			} else {
				data->alarms[id].is_alarm_pending = true;
			}
		}
	}

	/* Clear both the match bit and wakeup to acknowledge the interrupt */
	sys_write32(CONTROL_WAKEUP_CLR_BIT | CONTROL_MATCH_BIT, cfg->base_address + CONTROL_REG);
}
#endif

/*
 * Initializes the RTC hardware by deasserting reset, stopping
 * the RTC, setting the prescaler, enabling the clock calendar mode, and
 * starting the RTC. If RTC alarm configuration is enabled, it also configures
 * the IRQ for the RTC peripheral.
 */
static int rtc_mchp_mss_init(const struct device *dev)
{
	const struct rtc_mchp_mss_dev_config *cfg = dev->config;
	struct rtc_mchp_mss_dev_data *data = dev->data;
	uint32_t pres = cfg->prescaler;
	uint32_t aux;

	/* Initialize the semaphore for thread safety */
	k_sem_init(&data->lock, 1, 1);

#if MSS_RTC_RESET_ENABLED
	/* Deassert reset first, before any register access */
	if (cfg->reset_spec.dev != NULL) {
		(void)reset_line_deassert_dt(&cfg->reset_spec);
	}
#endif

	if (pres > MAX_PRESCALAR_COUNT) {
		LOG_ERR("Prescaler value 0x%08x exceeds maximum 0x%08x", pres, MAX_PRESCALAR_COUNT);
		return -EINVAL;
	}

	/* Stop the RTC device before modifying configuration */
	aux = CONTROL_STOP_BIT;
	sys_write32(aux, cfg->base_address + CONTROL_REG);

	/* Reset the RTC device */
	aux = CONTROL_RESET_BIT;
	sys_write32(aux, cfg->base_address + CONTROL_REG);

	/* Set the prescaler for the RTC peripheral */
	sys_write32(pres, cfg->base_address + PRESCALER_REG);

	/* Set the mode: binary counter or date & time counter */
	sys_write32(MODE_CLK_MODE_MASK, cfg->base_address + MODE_REG);

	/* Reset the alarm and compare registers to a known value */
	sys_write32(0, cfg->base_address + ALARM_LOWER_REG);
	sys_write32(0, cfg->base_address + ALARM_UPPER_REG);
	sys_write32(0, cfg->base_address + COMPARE_LOWER_REG);
	sys_write32(0, cfg->base_address + COMPARE_UPPER_REG);

#ifdef CONFIG_RTC_ALARM
	/* Enable alarm functionality */
	sys_write32(CONTROL_ALARM_ON_BIT, cfg->base_address + CONTROL_REG);
#endif
	/* Start device - RTC running */
	sys_write32(CONTROL_START_BIT, cfg->base_address + CONTROL_REG);

#ifdef CONFIG_RTC_ALARM
	/* In case of alarm, RTC will continue to count after match */
	aux = sys_read32(cfg->base_address + MODE_REG);
	aux |= MODE_WAKEUP_CONTINUE_MASK;
	sys_write32(aux, cfg->base_address + MODE_REG);

	/* Initialize the alarm's data */
	for (uint8_t i = 0; i < RTC_ALARM_COUNT; i++) {
		data->alarms[i].alarm_cb = NULL;
		data->alarms[i].alarm_user_data = NULL;
		data->alarms[i].is_alarm_pending = false;
	}

	/* Configure the IRQ for the RTC peripheral */
	cfg->irq_config_func(dev);
#endif
	return 0;
}

static DEVICE_API(rtc, rtc_mchp_mss_api) = {
	.set_time = rtc_mchp_mss_set_time,
	.get_time = rtc_mchp_mss_get_time,
#ifdef CONFIG_RTC_ALARM
	.alarm_get_supported_fields = rtc_mchp_mss_get_alarm_supported_fields,
	.alarm_is_pending = rtc_mchp_mss_alarm_is_pending,
	.alarm_set_time = rtc_mchp_mss_set_alarm_time,
	.alarm_get_time = rtc_mchp_mss_get_alarm_time,
	.alarm_set_callback = rtc_mchp_mss_set_alarm_callback,
#endif
};

/* Defines the RTC interrupt configurations. */
#ifdef CONFIG_RTC_ALARM
#define RTC_MCHP_MSS_CONNECT(n, m)                                                                 \
	IRQ_CONNECT(DT_INST_IRQN_BY_IDX(n, m), DT_INST_IRQ_BY_IDX(n, m, priority),                 \
		    rtc_mchp_mss_isr, DEVICE_DT_INST_GET(n), 0);                                   \
	irq_enable(DT_INST_IRQN_BY_IDX(n, m));
#endif

/* RTC driver configuration structure for instance n */
#define RTC_MCHP_MSS_CONFIG_DEFN(n)                                                                \
	static const struct rtc_mchp_mss_dev_config rtc_mchp_mss_dev_config_##n = {                \
		.base_address = DT_INST_REG_ADDR(n),                                               \
		.prescaler = DT_INST_PROP(n, prescaler),                                           \
		.clock_freq = DT_INST_PROP(n, clock_frequency),                                    \
		.rtc_irq_base = DT_INST_IRQN(n),                                                   \
		IF_ENABLED(CONFIG_RTC_ALARM, ( \
			.alarms_count = DT_INST_PROP(n, alarms_count), \
			.irq_config_func = rtc_mchp_mss_irq_config_##n, \
		))                                                   \
				    COND_CODE_1(MSS_RTC_RESET_ENABLED,                             \
			(.reset_spec = RESET_DT_SPEC_INST_GET(n),), ()) }
/* Define and initialize the RTC device. */
#define RTC_MCHP_MSS_DEVICE_INIT(n)                                                                \
	static struct rtc_mchp_mss_dev_data rtc_mchp_mss_dev_data_##n;                             \
	IF_ENABLED(CONFIG_RTC_ALARM, ( \
	static void rtc_mchp_mss_irq_config_##n(const struct device *dev) \
	{ \
		RTC_MCHP_MSS_CONNECT(n, 0); \
		RTC_MCHP_MSS_CONNECT(n, 1); \
	} \
	))                                                           \
	RTC_MCHP_MSS_CONFIG_DEFN(n);                                                               \
	DEVICE_DT_INST_DEFINE(n, rtc_mchp_mss_init, NULL, &rtc_mchp_mss_dev_data_##n,              \
			      &rtc_mchp_mss_dev_config_##n, POST_KERNEL, CONFIG_RTC_INIT_PRIORITY, \
			      &rtc_mchp_mss_api);

DT_INST_FOREACH_STATUS_OKAY(RTC_MCHP_MSS_DEVICE_INIT);
