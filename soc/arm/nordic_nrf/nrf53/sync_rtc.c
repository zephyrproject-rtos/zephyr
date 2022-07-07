/*
 * Copyright (c) 2021 Nordic Semiconductor ASA.
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <nrfx_dppi.h>
#include <hal/nrf_ipc.h>
#include <helpers/nrfx_gppi.h>
#include <zephyr/drivers/timer/nrf_rtc_timer.h>
#include <zephyr/drivers/ipm.h>
#include <zephyr/drivers/mbox.h>
#include <zephyr/logging/log_ctrl.h>
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(sync_rtc, CONFIG_SYNC_RTC_LOG_LEVEL);

/* Arbitrary delay is used needed to handle cases when offset between cores is
 * small and rtc synchronization process might not handle events on time.
 * Setting high value prolongs synchronization process but setting too low may
 * lead synchronization failure if offset between cores is small and/or there
 * are significant interrupt handling latencies.
 */
#define RTC_SYNC_ARBITRARY_DELAY 100

static uint32_t sync_cc;
static int32_t nrf53_sync_offset = -EBUSY;

union rtc_sync_channels {
	uint32_t raw;
	struct {
		uint8_t ppi;
		uint8_t rtc;
		uint8_t ipc_out;
		uint8_t ipc_in;
	} ch;
};

/* Algorithm for establishing RTC offset on the network side.
 *
 * Assumptions:
 * APP starts first thus its RTC is ahead. Only network will need to adjust its
 * time. Because APP will capture the offset but NET needs it, algorithm
 * consists of two stages: Getting offset on APP side, passing this offset to
 * NET core. To keep it simple and independent from IPM protocols, value is passed
 * using just IPC, PPI and RTC.
 *
 * 1st stage:
 * APP: setup PPI connection from IPC_RECEIVE to RTC CAPTURE, enable interrupt
 *      IPC received.
 * NET: setup RTC CC for arbitrary offset from now, setup PPI from RTC_COMPARE to IPC_SEND
 *      Record value set to CC.
 *
 * When APP will capture the value it needs to be passed to NET since it will be
 * capable of calculating the offset since it know what counter value corresponds
 * to the value captured on APP side.
 *
 * 2nd stage:
 * APP: Sets Compare event for value = 2 * captured value + arbitrary offset
 * NET: setup PPI from IPC_RECEIVE to RTC CAPTURE
 *
 * When NET RTC captures IPC event it takes CC value and knowing CC value previously
 * used by NET and arbitrary offset (which is the same on APP and NET) is able
 * to calculate exact offset between RTC counters.
 *
 * Note, arbitrary delay is used to accommodate for the case when NET-APP offset
 * is small enough that interrupt latency would impact it. NET-APP offset depends
 * on when NET core is reset and time when RTC system clock is initialized.
 */

/* Setup or clear connection from IPC_RECEIVE to RTC_CAPTURE
 *
 * @param channels Details about channels
 * @param setup If true connection is setup, else it is cleared.
 */
static void ppi_ipc_to_rtc(union rtc_sync_channels channels, bool setup)
{
	nrf_ipc_event_t ipc_evt = nrf_ipc_receive_event_get(channels.ch.ipc_in);
	uint32_t task_addr = z_nrf_rtc_timer_capture_task_address_get(channels.ch.rtc);

	if (setup) {
		nrfx_gppi_task_endpoint_setup(channels.ch.ppi, task_addr);
		nrf_ipc_publish_set(NRF_IPC, ipc_evt, channels.ch.ppi);
	} else {
		nrfx_gppi_task_endpoint_clear(channels.ch.ppi, task_addr);
		nrf_ipc_publish_clear(NRF_IPC, ipc_evt);
	}
}

/* Setup or clear connection from RTC_COMPARE to IPC_SEND
 *
 * @param channels Details about channels
 * @param setup If true connection is setup, else it is cleared.
 */
static void ppi_rtc_to_ipc(union rtc_sync_channels channels, bool setup)
{
	uint32_t evt_addr = z_nrf_rtc_timer_compare_evt_address_get(channels.ch.rtc);
	nrf_ipc_task_t ipc_task = nrf_ipc_send_task_get(channels.ch.ipc_out);

	if (setup) {
		nrf_ipc_subscribe_set(NRF_IPC, ipc_task, channels.ch.ppi);
		nrfx_gppi_event_endpoint_setup(channels.ch.ppi, evt_addr);
	} else {
		nrfx_gppi_event_endpoint_clear(channels.ch.ppi, evt_addr);
		nrf_ipc_subscribe_clear(NRF_IPC, ipc_task);
	}
}

/* Free DPPI and RTC channels */
static void free_resources(union rtc_sync_channels channels)
{
	nrfx_err_t err;

	nrfx_gppi_channels_disable(BIT(channels.ch.ppi));

	z_nrf_rtc_timer_chan_free(channels.ch.rtc);

	err = nrfx_dppi_channel_free(channels.ch.ppi);
	__ASSERT_NO_MSG(err == NRFX_SUCCESS);
}

int z_nrf_rtc_timer_nrf53net_offset_get(void)
{
	if (!IS_ENABLED(CONFIG_SOC_NRF5340_CPUNET)) {
		return -ENOSYS;
	}

	return nrf53_sync_offset;
}

static void rtc_cb(int32_t id, uint64_t cc_value, void *user_data)
{
	ARG_UNUSED(id);
	ARG_UNUSED(cc_value);

	union rtc_sync_channels channels;

	channels.raw = (uint32_t)user_data;
	ppi_rtc_to_ipc(channels, false);
	if (IS_ENABLED(CONFIG_SOC_NRF5340_CPUAPP)) {
		/* APP: Synchronized completed */
		free_resources(channels);
	} else {
		/* Compare event generated, reconfigure PPI and wait for
		 * IPC event from APP.
		 */
		ppi_ipc_to_rtc(channels, true);
	}
}

static log_timestamp_t sync_rtc_timestamp_get(void)
{
	return (log_timestamp_t)(sys_clock_tick_get() + nrf53_sync_offset);
}

static void remote_callback(void *user_data)
{
	extern const struct log_link *log_link_ipc_get_link(void);

	union rtc_sync_channels channels;
	uint32_t cc;

	channels.raw = (uint32_t)user_data;

	cc = z_nrf_rtc_timer_compare_read(channels.ch.rtc);

	/* Clear previous task,event */
	ppi_ipc_to_rtc(channels, false);

	if (IS_ENABLED(CONFIG_SOC_NRF5340_CPUAPP)) {
		/* Setup new connection from RTC to IPC and set RTC to a new
		 * interval that contains captured offset.
		 */
		ppi_rtc_to_ipc(channels, true);

		z_nrf_rtc_timer_set(channels.ch.rtc, cc + cc + RTC_SYNC_ARBITRARY_DELAY,
				    rtc_cb, (void *)channels.raw);
	} else {
		/* Synchronization completed */
		free_resources(channels);
		nrf53_sync_offset = cc - RTC_SYNC_ARBITRARY_DELAY - 2 * sync_cc;
		if (IS_ENABLED(CONFIG_NRF53_SYNC_RTC_LOG_TIMESTAMP)) {
			uint32_t offset_us =
				(uint64_t)nrf53_sync_offset * 1000000 /
				sys_clock_hw_cycles_per_sec();

			log_set_timestamp_func(sync_rtc_timestamp_get,
					       sys_clock_hw_cycles_per_sec());
			LOG_INF("Updated timestamp to synchronized RTC by %d ticks (%dus)",
					nrf53_sync_offset, offset_us);
		}
	}
}

static void mbox_callback(const struct device *dev, uint32_t channel,
			  void *user_data, struct mbox_msg *data)
{
	struct mbox_channel ch;
	int err;

	mbox_init_channel(&ch, dev, channel);
	err = mbox_set_enabled(&ch, false);

	(void)err;
	__ASSERT_NO_MSG(err == 0);

	remote_callback(user_data);
}

static void ipm_callback(const struct device *ipmdev, void *user_data,
			 uint32_t id, volatile void *data)
{
	int err = ipm_set_enabled(ipmdev, false);

	(void)err;
	__ASSERT_NO_MSG(err == 0);

	remote_callback(user_data);
}

static int mbox_rx_init(void *user_data)
{
	const struct device *dev;
	struct mbox_channel channel;
	int err;

	dev = COND_CODE_1(CONFIG_MBOX, (DEVICE_DT_GET(DT_NODELABEL(mbox))), (NULL));
	if (dev == NULL) {
		return -ENODEV;
	}

	mbox_init_channel(&channel, dev, CONFIG_NRF53_SYNC_RTC_IPM_IN);

	err = mbox_register_callback(&channel, mbox_callback, user_data);
	if (err < 0) {
		return err;
	}

	return mbox_set_enabled(&channel, true);
}

static int ipm_rx_init(void *user_data)
{
	const struct device *ipm_dev;

	ipm_dev = device_get_binding("IPM_" STRINGIFY(CONFIG_NRF53_SYNC_RTC_IPM_IN));
	if (ipm_dev == NULL) {
		return -ENODEV;
	}

	ipm_register_callback(ipm_dev, ipm_callback, user_data);

	return ipm_set_enabled(ipm_dev, true);
}

/* Setup RTC synchronization. */
static int sync_rtc_setup(const struct device *unused)
{
	ARG_UNUSED(unused);

	nrfx_err_t err;
	union rtc_sync_channels channels;
	int32_t sync_rtc_ch;
	int rv;

	err = nrfx_dppi_channel_alloc(&channels.ch.ppi);
	if (err != NRFX_SUCCESS) {
		rv = -ENODEV;
		goto bail;
	}

	sync_rtc_ch = z_nrf_rtc_timer_chan_alloc();
	if (sync_rtc_ch < 0) {
		nrfx_dppi_channel_free(channels.ch.ppi);
		rv = sync_rtc_ch;
		goto bail;
	}

	channels.ch.rtc = (uint8_t)sync_rtc_ch;
	channels.ch.ipc_out = CONFIG_NRF53_SYNC_RTC_IPM_OUT;
	channels.ch.ipc_in = CONFIG_NRF53_SYNC_RTC_IPM_IN;

	rv = IS_ENABLED(CONFIG_MBOX) ? mbox_rx_init((void *)channels.raw) :
				       ipm_rx_init((void *)channels.raw);
	if (rv < 0) {
		goto bail;
	}

	nrfx_gppi_channels_enable(BIT(channels.ch.ppi));

	if (IS_ENABLED(CONFIG_SOC_NRF5340_CPUAPP)) {
		ppi_ipc_to_rtc(channels, true);
	} else {
		ppi_rtc_to_ipc(channels, true);

		uint32_t key = irq_lock();

		sync_cc = z_nrf_rtc_timer_read() + RTC_SYNC_ARBITRARY_DELAY;
		z_nrf_rtc_timer_set(channels.ch.rtc, sync_cc, rtc_cb, (void *)channels.raw);
		irq_unlock(key);
	}

bail:
	if (rv != 0) {
		LOG_ERR("Failed synchronized RTC setup (err: %d)", rv);
	}

	return rv;
}

#if defined(CONFIG_MBOX_INIT_PRIORITY)
BUILD_ASSERT(CONFIG_NRF53_SYNC_RTC_INIT_PRIORITY > CONFIG_MBOX_INIT_PRIORITY,
		"RTC Sync must be initialized after MBOX driver.");
#endif

SYS_INIT(sync_rtc_setup, POST_KERNEL, CONFIG_NRF53_SYNC_RTC_INIT_PRIORITY);
