/*
 * Copyright (c) 2016-2019 Nordic Semiconductor ASA
 * Copyright (c) 2016 Vinayak Kariappa Chettimada
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <soc.h>
#include <drivers/clock_control.h>
#include <drivers/clock_control/nrf_clock_control.h>
#include "nrf_clock_calibration.h"
#include <logging/log.h>
#include <hal/nrf_power.h>

LOG_MODULE_REGISTER(clock_control, CONFIG_CLOCK_CONTROL_LOG_LEVEL);

/* Helper logging macros which prepends device name to the log. */
#define CLOCK_LOG(lvl, dev, ...) \
	LOG_##lvl("%s: " GET_ARG1(__VA_ARGS__), dev->config->name \
			COND_CODE_0(NUM_VA_ARGS_LESS_1(__VA_ARGS__),\
					(), (, GET_ARGS_LESS_1(__VA_ARGS__))))
#define ERR(dev, ...) CLOCK_LOG(ERR, dev, __VA_ARGS__)
#define WRN(dev, ...) CLOCK_LOG(WRN, dev, __VA_ARGS__)
#define INF(dev, ...) CLOCK_LOG(INF, dev, __VA_ARGS__)
#define DBG(dev, ...) CLOCK_LOG(DBG, dev, __VA_ARGS__)

/* returns true if clock stopping or starting can be performed. If false then
 * start/stop will be deferred and performed later on by handler owner.
 */
typedef bool (*nrf_clock_handler_t)(struct device *dev);

/* Clock instance structure */
struct nrf_clock_control {
	sys_slist_t list;	/* List of users requesting callback */
	u8_t ref;		/* Users counter */
	bool started;		/* Indicated that clock is started */
};

/* Clock instance static configuration */
struct nrf_clock_control_config {
	nrf_clock_handler_t start_handler; /* Called before start */
	nrf_clock_handler_t stop_handler; /* Called before stop */
	nrf_clock_event_t started_evt;	/* Clock started event */
	nrf_clock_task_t start_tsk;	/* Clock start task */
	nrf_clock_task_t stop_tsk;	/* Clock stop task */
};

static void clkstarted_handle(struct device *dev);

/* Return true if given event has enabled interrupt and is triggered. Event
 * is cleared.
 */
static bool clock_event_check_and_clean(nrf_clock_event_t evt, u32_t intmask)
{
	bool ret = nrf_clock_event_check(NRF_CLOCK, evt) &&
			nrf_clock_int_enable_check(NRF_CLOCK, intmask);

	if (ret) {
		nrf_clock_event_clear(NRF_CLOCK, evt);
	}

	return ret;
}

static void clock_irqs_disable(void)
{
	nrf_clock_int_disable(NRF_CLOCK,
			(NRF_CLOCK_INT_HF_STARTED_MASK |
			 NRF_CLOCK_INT_LF_STARTED_MASK |
			 COND_CODE_1(CONFIG_USB_NRFX,
				(NRF_POWER_INT_USBDETECTED_MASK |
				 NRF_POWER_INT_USBREMOVED_MASK |
				 NRF_POWER_INT_USBPWRRDY_MASK),
				(0))));
}

static void clock_irqs_enable(void)
{
	nrf_clock_int_enable(NRF_CLOCK,
			(NRF_CLOCK_INT_HF_STARTED_MASK |
			 NRF_CLOCK_INT_LF_STARTED_MASK |
			 COND_CODE_1(CONFIG_USB_NRFX,
				(NRF_POWER_INT_USBDETECTED_MASK |
				 NRF_POWER_INT_USBREMOVED_MASK |
				 NRF_POWER_INT_USBPWRRDY_MASK),
				(0))));
}

static enum clock_control_status get_status(struct device *dev,
					    clock_control_subsys_t sys)
{
	struct nrf_clock_control *data = dev->driver_data;

	if (data->started) {
		return CLOCK_CONTROL_STATUS_ON;
	}

	if (data->ref > 0) {
		return CLOCK_CONTROL_STATUS_STARTING;
	}

	return CLOCK_CONTROL_STATUS_OFF;
}

static int clock_stop(struct device *dev, clock_control_subsys_t sub_system)
{
	const struct nrf_clock_control_config *config =
						dev->config->config_info;
	struct nrf_clock_control *data = dev->driver_data;
	int err = 0;
	int key;

	key = irq_lock();
	if (data->ref == 0) {
		err = -EALREADY;
		goto out;
	}
	data->ref--;
	if (data->ref == 0) {
		bool do_stop;

		DBG(dev, "Stopping");
		sys_slist_init(&data->list);

		do_stop =  (config->stop_handler) ?
				config->stop_handler(dev) : true;

		if (do_stop) {
			nrf_clock_task_trigger(NRF_CLOCK, config->stop_tsk);
			/* It may happen that clock is being stopped when it
			 * has just been started and start is not yet handled
			 * (due to irq_lock). In that case after stopping the
			 * clock, started event is cleared to prevent false
			 * interrupt being triggered.
			 */
			nrf_clock_event_clear(NRF_CLOCK, config->started_evt);
		}

		data->started = false;
	}

out:
	irq_unlock(key);

	return err;
}

static bool is_in_list(sys_slist_t *list, sys_snode_t *node)
{
	sys_snode_t *item = sys_slist_peek_head(list);

	do {
		if (item == node) {
			return true;
		}

		item = sys_slist_peek_next(item);
	} while (item);

	return false;
}

static void list_append(sys_slist_t *list, sys_snode_t *node)
{
	int key;

	key = irq_lock();
	sys_slist_append(list, node);
	irq_unlock(key);
}

static struct clock_control_async_data *list_get(sys_slist_t *list)
{
	struct clock_control_async_data *async_data;
	sys_snode_t *node;
	int key;

	key = irq_lock();
	node = sys_slist_get(list);
	irq_unlock(key);
	async_data = CONTAINER_OF(node,
		struct clock_control_async_data, node);

	return async_data;
}

static int clock_async_start(struct device *dev,
			     clock_control_subsys_t sub_system,
			     struct clock_control_async_data *data)
{
	const struct nrf_clock_control_config *config =
						dev->config->config_info;
	struct nrf_clock_control *clk_data = dev->driver_data;
	int key;
	u8_t ref;

	__ASSERT_NO_MSG((data == NULL) ||
			((data != NULL) && (data->cb != NULL)));

	/* if node is in the list it means that it is scheduled for
	 * the second time.
	 */
	if ((data != NULL)
	    && is_in_list(&clk_data->list, &data->node)) {
		return -EBUSY;
	}

	key = irq_lock();
	ref = ++clk_data->ref;
	__ASSERT_NO_MSG(clk_data->ref > 0);
	irq_unlock(key);

	if (data) {
		bool already_started;

		clock_irqs_disable();
		already_started = clk_data->started;
		if (!already_started) {
			list_append(&clk_data->list, &data->node);
		}
		clock_irqs_enable();

		if (already_started) {
			data->cb(dev, data->user_data);
		}
	}

	if (ref == 1) {
		bool do_start;

		do_start =  (config->start_handler) ?
				config->start_handler(dev) : true;
		if (do_start) {
			DBG(dev, "Triggering start task");
			nrf_clock_task_trigger(NRF_CLOCK,
					       config->start_tsk);
		} else {
			/* If external start_handler indicated that clcok is
			 * still running (it may happen in case of LF RC clock
			 * which was requested to be stopped during ongoing
			 * calibration (clock will not be stopped in that case)
			 * and requested to be started before calibration is
			 * completed. In that case clock is still running and
			 * we can notify enlisted requests.
			 */
			clkstarted_handle(dev);
		}
	}

	return 0;
}

static int clock_start(struct device *dev, clock_control_subsys_t sub_system)
{
	return clock_async_start(dev, sub_system, NULL);
}

/* Note: this function has public linkage, and MUST have this
 * particular name.  The platform architecture itself doesn't care,
 * but there is a test (tests/kernel/arm_irq_vector_table) that needs
 * to find it to it can set it in a custom vector table.  Should
 * probably better abstract that at some point (e.g. query and reset
 * it by pointer at runtime, maybe?) so we don't have this leaky
 * symbol.
 */
void nrf_power_clock_isr(void *arg);

static int hfclk_init(struct device *dev)
{
	IRQ_CONNECT(DT_INST_0_NORDIC_NRF_CLOCK_IRQ_0,
		    DT_INST_0_NORDIC_NRF_CLOCK_IRQ_0_PRIORITY,
		    nrf_power_clock_isr, 0, 0);

	irq_enable(DT_INST_0_NORDIC_NRF_CLOCK_IRQ_0);

	nrf_clock_lf_src_set(NRF_CLOCK, CLOCK_CONTROL_NRF_K32SRC);

	if (IS_ENABLED(CONFIG_CLOCK_CONTROL_NRF_K32SRC_RC_CALIBRATION)) {
		z_nrf_clock_calibration_init(dev);
	}

	clock_irqs_enable();
	sys_slist_init(&((struct nrf_clock_control *)dev->driver_data)->list);

	return 0;
}

static int lfclk_init(struct device *dev)
{
	sys_slist_init(&((struct nrf_clock_control *)dev->driver_data)->list);
	return 0;
}

static const struct clock_control_driver_api clock_control_api = {
	.on = clock_start,
	.off = clock_stop,
	.async_on = clock_async_start,
	.get_status = get_status,
};

static struct nrf_clock_control hfclk;
static const struct nrf_clock_control_config hfclk_config = {
	.start_tsk = NRF_CLOCK_TASK_HFCLKSTART,
	.started_evt = NRF_CLOCK_EVENT_HFCLKSTARTED,
	.stop_tsk = NRF_CLOCK_TASK_HFCLKSTOP
};

DEVICE_AND_API_INIT(clock_nrf5_m16src,
		    DT_INST_0_NORDIC_NRF_CLOCK_LABEL  "_16M",
		    hfclk_init, &hfclk, &hfclk_config, PRE_KERNEL_1,
		    CONFIG_KERNEL_INIT_PRIORITY_DEVICE,
		    &clock_control_api);


static struct nrf_clock_control lfclk;
static const struct nrf_clock_control_config lfclk_config = {
	.start_tsk = NRF_CLOCK_TASK_LFCLKSTART,
	.started_evt = NRF_CLOCK_EVENT_LFCLKSTARTED,
	.stop_tsk = NRF_CLOCK_TASK_LFCLKSTOP,
	.start_handler =
		IS_ENABLED(CONFIG_CLOCK_CONTROL_NRF_K32SRC_RC_CALIBRATION) ?
			z_nrf_clock_calibration_start : NULL,
	.stop_handler =
		IS_ENABLED(CONFIG_CLOCK_CONTROL_NRF_K32SRC_RC_CALIBRATION) ?
			z_nrf_clock_calibration_stop : NULL
};

DEVICE_AND_API_INIT(clock_nrf5_k32src,
		    DT_INST_0_NORDIC_NRF_CLOCK_LABEL  "_32K",
		    lfclk_init, &lfclk, &lfclk_config, PRE_KERNEL_1,
		    CONFIG_KERNEL_INIT_PRIORITY_DEVICE,
		    &clock_control_api);

static void clkstarted_handle(struct device *dev)
{
	struct clock_control_async_data *async_data;
	struct nrf_clock_control *data = dev->driver_data;

	DBG(dev, "Clock started");
	data->started = true;

	while ((async_data = list_get(&data->list)) != NULL) {
		async_data->cb(dev, async_data->user_data);
	}
}

#if defined(CONFIG_USB_NRFX)
static bool power_event_check_and_clean(nrf_power_event_t evt, u32_t intmask)
{
	bool ret = nrf_power_event_check(NRF_POWER, evt) &&
			nrf_power_int_enable_check(NRF_POWER, intmask);

	if (ret) {
		nrf_power_event_clear(NRF_POWER, evt);
	}

	return ret;
}
#endif

static void usb_power_isr(void)
{
#if defined(CONFIG_USB_NRFX)
	extern void usb_dc_nrfx_power_event_callback(nrf_power_event_t event);

	if (power_event_check_and_clean(NRF_POWER_EVENT_USBDETECTED,
					NRF_POWER_INT_USBDETECTED_MASK)) {
		usb_dc_nrfx_power_event_callback(NRF_POWER_EVENT_USBDETECTED);
	}

	if (power_event_check_and_clean(NRF_POWER_EVENT_USBPWRRDY,
					NRF_POWER_INT_USBPWRRDY_MASK)) {
		usb_dc_nrfx_power_event_callback(NRF_POWER_EVENT_USBPWRRDY);
	}

	if (power_event_check_and_clean(NRF_POWER_EVENT_USBREMOVED,
					NRF_POWER_INT_USBREMOVED_MASK)) {
		usb_dc_nrfx_power_event_callback(NRF_POWER_EVENT_USBREMOVED);
	}
#endif
}

void nrf_power_clock_isr(void *arg)
{
	ARG_UNUSED(arg);

	if (clock_event_check_and_clean(NRF_CLOCK_EVENT_HFCLKSTARTED,
					NRF_CLOCK_INT_HF_STARTED_MASK)) {
		struct device *hfclk_dev = DEVICE_GET(clock_nrf5_m16src);
		struct nrf_clock_control *data = hfclk_dev->driver_data;

		/* Check needed due to anomaly 201:
		 * HFCLKSTARTED may be generated twice.
		 */
		if (!data->started) {
			clkstarted_handle(hfclk_dev);
		}
	}

	if (clock_event_check_and_clean(NRF_CLOCK_EVENT_LFCLKSTARTED,
					NRF_CLOCK_INT_LF_STARTED_MASK)) {
		struct device *lfclk_dev = DEVICE_GET(clock_nrf5_k32src);

		if (IS_ENABLED(
			CONFIG_CLOCK_CONTROL_NRF_K32SRC_RC_CALIBRATION)) {
			z_nrf_clock_calibration_lfclk_started(lfclk_dev);
		}
		clkstarted_handle(lfclk_dev);
	}

	usb_power_isr();

	if (IS_ENABLED(CONFIG_CLOCK_CONTROL_NRF_K32SRC_RC_CALIBRATION)) {
		z_nrf_clock_calibration_isr();
	}
}

#ifdef CONFIG_USB_NRFX
void nrf5_power_usb_power_int_enable(bool enable)
{
	u32_t mask;

	mask = NRF_POWER_INT_USBDETECTED_MASK |
	       NRF_POWER_INT_USBREMOVED_MASK |
	       NRF_POWER_INT_USBPWRRDY_MASK;

	if (enable) {
		nrf_power_int_enable(NRF_POWER, mask);
		irq_enable(DT_INST_0_NORDIC_NRF_CLOCK_IRQ_0);
	} else {
		nrf_power_int_disable(NRF_POWER, mask);
	}
}
#endif
