/*
 * Copyright (c) 2016-2019 Nordic Semiconductor ASA
 * Copyright (c) 2016 Vinayak Kariappa Chettimada
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <soc.h>
#include <sys/onoff.h>
#include <drivers/clock_control.h>
#include <drivers/clock_control/nrf_clock_control.h>
#include "nrf_clock_calibration.h"
#include <logging/log.h>
#include <hal/nrf_power.h>
#include <shell/shell.h>

LOG_MODULE_REGISTER(clock_control, CONFIG_CLOCK_CONTROL_LOG_LEVEL);

#define DT_DRV_COMPAT nordic_nrf_clock


#define CTX_ONOFF		BIT(6)
#define CTX_API			BIT(7)
#define CTX_MASK (CTX_ONOFF | CTX_API)

#define STATUS_MASK		0x7
#define GET_STATUS(flags)	(flags & STATUS_MASK)
#define GET_CTX(flags)		(flags & CTX_MASK)

/* Used only by HF clock */
#define HF_USER_BT		BIT(0)
#define HF_USER_GENERIC		BIT(1)

/* Helper logging macros which prepends subsys name to the log. */
#ifdef CONFIG_LOG
#define CLOCK_LOG(lvl, dev, subsys, ...) \
	LOG_##lvl("%s: " GET_ARG_N(1, __VA_ARGS__), \
		get_sub_config(dev, (enum clock_control_nrf_type)subsys)->name \
		COND_CODE_0(NUM_VA_ARGS_LESS_1(__VA_ARGS__),\
				(), (, GET_ARGS_LESS_N(1, __VA_ARGS__))))
#else
#define CLOCK_LOG(...)
#endif

#define ERR(dev, subsys, ...) CLOCK_LOG(ERR, dev, subsys, __VA_ARGS__)
#define WRN(dev, subsys, ...) CLOCK_LOG(WRN, dev, subsys, __VA_ARGS__)
#define INF(dev, subsys, ...) CLOCK_LOG(INF, dev, subsys, __VA_ARGS__)
#define DBG(dev, subsys, ...) CLOCK_LOG(DBG, dev, subsys, __VA_ARGS__)

/* Clock subsys structure */
struct nrf_clock_control_sub_data {
	clock_control_cb_t cb;
	void *user_data;
	uint32_t flags;
};

typedef void (*clk_ctrl_func_t)(void);

/* Clock subsys static configuration */
struct nrf_clock_control_sub_config {
	clk_ctrl_func_t start;		/* Clock start function */
	clk_ctrl_func_t stop;		/* Clock stop function */
#ifdef CONFIG_LOG
	const char *name;
#endif
};

struct nrf_clock_control_data {
	struct onoff_manager mgr[CLOCK_CONTROL_NRF_TYPE_COUNT];
	struct nrf_clock_control_sub_data subsys[CLOCK_CONTROL_NRF_TYPE_COUNT];
};

struct nrf_clock_control_config {
	struct nrf_clock_control_sub_config
					subsys[CLOCK_CONTROL_NRF_TYPE_COUNT];
};

static atomic_t hfclk_users;
static uint64_t hf_start_tstamp;
static uint64_t hf_stop_tstamp;

/* Return true if given event has enabled interrupt and is triggered. Event
 * is cleared.
 */
static bool clock_event_check_and_clean(nrf_clock_event_t evt, uint32_t intmask)
{
	bool ret = nrf_clock_event_check(NRF_CLOCK, evt) &&
			nrf_clock_int_enable_check(NRF_CLOCK, intmask);

	if (ret) {
		nrf_clock_event_clear(NRF_CLOCK, evt);
	}

	return ret;
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

static struct nrf_clock_control_sub_data *get_sub_data(struct device *dev,
					      enum clock_control_nrf_type type)
{
	struct nrf_clock_control_data *data = dev->data;

	return &data->subsys[type];
}

static const struct nrf_clock_control_sub_config *get_sub_config(
					struct device *dev,
					enum clock_control_nrf_type type)
{
	const struct nrf_clock_control_config *config =
						dev->config;

	return &config->subsys[type];
}

static struct onoff_manager *get_onoff_manager(struct device *dev,
					enum clock_control_nrf_type type)
{
	struct nrf_clock_control_data *data = dev->data;

	return &data->mgr[type];
}

DEVICE_DECLARE(clock_nrf);

struct onoff_manager *z_nrf_clock_control_get_onoff(clock_control_subsys_t sys)
{
	return get_onoff_manager(DEVICE_GET(clock_nrf),
				(enum clock_control_nrf_type)sys);
}

static enum clock_control_status get_status(struct device *dev,
					    clock_control_subsys_t subsys)
{
	enum clock_control_nrf_type type = (enum clock_control_nrf_type)subsys;

	__ASSERT_NO_MSG(type < CLOCK_CONTROL_NRF_TYPE_COUNT);

	return GET_STATUS(get_sub_data(dev, type)->flags);
}

static int set_off_state(uint32_t *flags, uint32_t ctx)
{
	int err = 0;
	int key = irq_lock();
	uint32_t current_ctx = GET_CTX(*flags);

	if ((current_ctx != 0) && (current_ctx != ctx)) {
		err = -EPERM;
	} else {
		*flags = CLOCK_CONTROL_STATUS_OFF;
	}

	irq_unlock(key);

	return err;
}

static int set_starting_state(uint32_t *flags, uint32_t ctx)
{
	int err = 0;
	int key = irq_lock();
	uint32_t current_ctx = GET_CTX(*flags);

	if ((*flags & (STATUS_MASK)) == CLOCK_CONTROL_STATUS_OFF) {
		*flags = CLOCK_CONTROL_STATUS_STARTING | ctx;
	} else if (current_ctx != ctx) {
		err = -EPERM;
	} else {
		err = -EBUSY;
	}

	irq_unlock(key);

	return err;
}

static void set_on_state(uint32_t *flags)
{
	int key = irq_lock();

	*flags = CLOCK_CONTROL_STATUS_ON | GET_CTX(*flags);
	irq_unlock(key);
}

static void clkstarted_handle(struct device *dev,
			      enum clock_control_nrf_type type)
{
	struct nrf_clock_control_sub_data *sub_data = get_sub_data(dev, type);
	clock_control_cb_t callback = sub_data->cb;
	void *user_data = sub_data->user_data;

	sub_data->cb = NULL;
	set_on_state(&sub_data->flags);
	DBG(dev, type, "Clock started");

	if (callback) {
		callback(dev, (clock_control_subsys_t)type, user_data);
	}
}

static inline void anomaly_132_workaround(void)
{
#if (CONFIG_NRF52_ANOMALY_132_DELAY_US - 0)
	static bool once;

	if (!once) {
		k_busy_wait(CONFIG_NRF52_ANOMALY_132_DELAY_US);
		once = true;
	}
#endif
}

static void lfclk_start(void)
{

	if (IS_ENABLED(CONFIG_NRF52_ANOMALY_132_WORKAROUND)) {
		anomaly_132_workaround();
	}

	nrf_clock_task_trigger(NRF_CLOCK, NRF_CLOCK_TASK_LFCLKSTART);
}

static void lfclk_stop(void)
{
	if (IS_ENABLED(CONFIG_CLOCK_CONTROL_NRF_K32SRC_RC_CALIBRATION)) {
		z_nrf_clock_calibration_lfclk_stopped();
	}

	nrf_clock_event_clear(NRF_CLOCK, NRF_CLOCK_EVENT_LFCLKSTARTED);
	nrf_clock_task_trigger(NRF_CLOCK, NRF_CLOCK_TASK_LFCLKSTOP);
}

static void hfclk_start(void)
{
	if (IS_ENABLED(CONFIG_CLOCK_CONTROL_NRF_SHELL)) {
		hf_start_tstamp = k_uptime_get();
	}

	nrf_clock_task_trigger(NRF_CLOCK, NRF_CLOCK_TASK_HFCLKSTART);
}

static void hfclk_stop(void)
{
	if (IS_ENABLED(CONFIG_CLOCK_CONTROL_NRF_SHELL)) {
		hf_stop_tstamp = k_uptime_get();
	}

	nrf_clock_event_clear(NRF_CLOCK, NRF_CLOCK_EVENT_HFCLKSTARTED);
	nrf_clock_task_trigger(NRF_CLOCK, NRF_CLOCK_TASK_HFCLKSTOP);
}

static uint32_t *get_hf_flags(void)
{
	struct nrf_clock_control_data *data = DEVICE_GET(clock_nrf)->data;

	return &data->subsys[CLOCK_CONTROL_NRF_TYPE_HFCLK].flags;
}

static void generic_hfclk_start(void)
{
	nrf_clock_hfclk_t type;
	bool already_started = false;
	int key = irq_lock();

	hfclk_users |= HF_USER_GENERIC;
	if (hfclk_users & HF_USER_BT) {
		(void)nrf_clock_is_running(NRF_CLOCK, NRF_CLOCK_DOMAIN_HFCLK,
					   &type);
		if (type == NRF_CLOCK_HFCLK_HIGH_ACCURACY) {
			already_started = true;
			/* Set on state in case clock interrupt comes and we
			 * want to avoid handling that.
			 */
			set_on_state(get_hf_flags());
		}
	}

	irq_unlock(key);

	if (already_started) {
		/* Clock already started by z_nrf_clock_bt_ctlr_hf_request */
		clkstarted_handle(DEVICE_GET(clock_nrf),
				  CLOCK_CONTROL_NRF_TYPE_HFCLK);
		return;
	}

	hfclk_start();
}

static void generic_hfclk_stop(void)
{
	if (atomic_and(&hfclk_users, ~HF_USER_GENERIC) & HF_USER_BT) {
		/* bt still requesting the clock. */
		return;
	}

	hfclk_stop();
}


void z_nrf_clock_bt_ctlr_hf_request(void)
{
	if (atomic_or(&hfclk_users, HF_USER_BT) & HF_USER_GENERIC) {
		/* generic request already activated clock. */
		return;
	}

	hfclk_start();
}

void z_nrf_clock_bt_ctlr_hf_release(void)
{
	if (atomic_and(&hfclk_users, ~HF_USER_BT) & HF_USER_GENERIC) {
		/* generic still requesting the clock. */
		return;
	}

	hfclk_stop();
}

static int stop(struct device *dev, clock_control_subsys_t subsys, uint32_t ctx)
{
	enum clock_control_nrf_type type = (enum clock_control_nrf_type)subsys;
	struct nrf_clock_control_sub_data *subdata = get_sub_data(dev, type);
	int err;

	__ASSERT_NO_MSG(type < CLOCK_CONTROL_NRF_TYPE_COUNT);

	err = set_off_state(&subdata->flags, ctx);
	if (err < 0) {
		return err;
	}

	get_sub_config(dev, type)->stop();

	return 0;
}

static int api_stop(struct device *dev, clock_control_subsys_t subsys)
{
	return stop(dev, subsys, CTX_API);
}

static int async_start(struct device *dev, clock_control_subsys_t subsys,
			struct clock_control_async_data *data, uint32_t ctx)
{
	enum clock_control_nrf_type type = (enum clock_control_nrf_type)subsys;
	struct nrf_clock_control_sub_data *subdata = get_sub_data(dev, type);
	int err;

	err = set_starting_state(&subdata->flags, ctx);
	if (err < 0) {
		return err;
	}

	subdata->cb = data->cb;
	subdata->user_data = data->user_data;

	 get_sub_config(dev, type)->start();

	return 0;
}

static int api_start(struct device *dev, clock_control_subsys_t subsys,
			     struct clock_control_async_data *data)
{
	return async_start(dev, subsys, data, CTX_API);
}

static void blocking_start_callback(struct device *dev,
				    clock_control_subsys_t subsys,
				    void *user_data)
{
	struct k_sem *sem = user_data;

	k_sem_give(sem);
}

static int api_blocking_start(struct device *dev, clock_control_subsys_t subsys)
{
	struct k_sem sem = Z_SEM_INITIALIZER(sem, 0, 1);
	struct clock_control_async_data data = {
		.cb = blocking_start_callback,
		.user_data = &sem
	};
	int err;

	if (!IS_ENABLED(CONFIG_MULTITHREADING)) {
		return -ENOTSUP;
	}

	err = api_start(dev, subsys, &data);
	if (err < 0) {
		return err;
	}

	return k_sem_take(&sem, K_MSEC(500));
}

static clock_control_subsys_t get_subsys(struct onoff_manager *mgr)
{
	struct nrf_clock_control_data *data = DEVICE_GET(clock_nrf)->data;
	size_t offset = (size_t)(mgr - data->mgr);

	return (clock_control_subsys_t)offset;
}

static void onoff_stop(struct onoff_manager *mgr,
			onoff_notify_fn notify)
{
	int res;

	res = stop(DEVICE_GET(clock_nrf), get_subsys(mgr), CTX_ONOFF);
	notify(mgr, res);
}

static void onoff_started_callback(struct device *dev,
				   clock_control_subsys_t sys,
				   void *user_data)
{
	enum clock_control_nrf_type type = (enum clock_control_nrf_type)sys;
	struct onoff_manager *mgr = get_onoff_manager(dev, type);
	onoff_notify_fn notify = user_data;

	notify(mgr, 0);
}

static void onoff_start(struct onoff_manager *mgr,
			onoff_notify_fn notify)
{
	struct clock_control_async_data data = {
		.cb = onoff_started_callback,
		.user_data = notify
	};
	int err;

	err = async_start(DEVICE_GET(clock_nrf), get_subsys(mgr),
			  &data, CTX_ONOFF);
	if (err < 0) {
		notify(mgr, err);
	}
}

static void lfclk_spinwait(nrf_clock_lfclk_t t)
{
	nrf_clock_domain_t d = NRF_CLOCK_DOMAIN_LFCLK;
	nrf_clock_lfclk_t type;

	while (!(nrf_clock_is_running(NRF_CLOCK, d, (void *)&type)
		 && (type == t))) {
		/* empty */
	}
}

void z_nrf_clock_control_lf_on(enum nrf_lfclk_start_mode start_mode)
{
	static atomic_t on;
	static struct onoff_client cli;

	if (atomic_set(&on, 1) == 0) {
		int err;
		struct onoff_manager *mgr =
				get_onoff_manager(DEVICE_GET(clock_nrf),
						  CLOCK_CONTROL_NRF_TYPE_LFCLK);

		sys_notify_init_spinwait(&cli.notify);
		err = onoff_request(mgr, &cli);
		__ASSERT_NO_MSG(err >= 0);
	}

	switch (start_mode) {
	case NRF_LFCLK_START_MODE_SPINWAIT_STABLE:
		lfclk_spinwait(CLOCK_CONTROL_NRF_K32SRC);
		break;

	case NRF_LFCLK_START_MODE_SPINWAIT_RUNNING:
		lfclk_spinwait(NRF_CLOCK_LFCLK_RC);
		break;

	case NRF_LFCLK_START_MODE_NOWAIT:
		break;

	default:
		__ASSERT_NO_MSG(false);
	}
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

static int clk_init(struct device *dev)
{
	int err;
	static const struct onoff_transitions transitions = {
		.start = onoff_start,
		.stop = onoff_stop
	};

	IRQ_CONNECT(DT_INST_IRQN(0), DT_INST_IRQ(0, priority),
		    nrf_power_clock_isr, 0, 0);

	irq_enable(DT_INST_IRQN(0));

	nrf_clock_lf_src_set(NRF_CLOCK, CLOCK_CONTROL_NRF_K32SRC);

	if (IS_ENABLED(CONFIG_CLOCK_CONTROL_NRF_K32SRC_RC_CALIBRATION)) {
		struct nrf_clock_control_data *data = dev->data;

		z_nrf_clock_calibration_init(data->mgr);
	}

	clock_irqs_enable();

	for (enum clock_control_nrf_type i = 0;
		i < CLOCK_CONTROL_NRF_TYPE_COUNT; i++) {
		struct nrf_clock_control_sub_data *subdata =
						get_sub_data(dev, i);

		err = onoff_manager_init(get_onoff_manager(dev, i),
					 &transitions);
		if (err < 0) {
			return err;
		}

		subdata->flags = CLOCK_CONTROL_STATUS_OFF;
	}

	return 0;
}
static const struct clock_control_driver_api clock_control_api = {
	.on = api_blocking_start,
	.off = api_stop,
	.async_on = api_start,
	.get_status = get_status,
};

static struct nrf_clock_control_data data;

static const struct nrf_clock_control_config config = {
	.subsys = {
		[CLOCK_CONTROL_NRF_TYPE_HFCLK] = {
			.start = generic_hfclk_start,
			.stop = generic_hfclk_stop,
			IF_ENABLED(CONFIG_LOG, (.name = "hfclk",))
		},
		[CLOCK_CONTROL_NRF_TYPE_LFCLK] = {
			.start = lfclk_start,
			.stop = lfclk_stop,
			IF_ENABLED(CONFIG_LOG, (.name = "lfclk",))
		}
	}
};

DEVICE_AND_API_INIT(clock_nrf, DT_INST_LABEL(0),
		    clk_init, &data, &config, PRE_KERNEL_1,
		    CONFIG_KERNEL_INIT_PRIORITY_DEVICE,
		    &clock_control_api);

#if defined(CONFIG_USB_NRFX)
static bool power_event_check_and_clean(nrf_power_event_t evt, uint32_t intmask)
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
	struct device *dev = DEVICE_GET(clock_nrf);

	if (clock_event_check_and_clean(NRF_CLOCK_EVENT_HFCLKSTARTED,
					NRF_CLOCK_INT_HF_STARTED_MASK)) {
		struct nrf_clock_control_sub_data *data =
				get_sub_data(dev, CLOCK_CONTROL_NRF_TYPE_HFCLK);

		/* Check needed due to anomaly 201:
		 * HFCLKSTARTED may be generated twice.
		 *
		 * Also software should be notified about clock being on only
		 * if generic request occured.
		 */
		if ((GET_STATUS(data->flags) == CLOCK_CONTROL_STATUS_STARTING)
			&& (hfclk_users & HF_USER_GENERIC)) {
			clkstarted_handle(dev, CLOCK_CONTROL_NRF_TYPE_HFCLK);
		}
	}

	if (clock_event_check_and_clean(NRF_CLOCK_EVENT_LFCLKSTARTED,
					NRF_CLOCK_INT_LF_STARTED_MASK)) {
		if (IS_ENABLED(
			CONFIG_CLOCK_CONTROL_NRF_K32SRC_RC_CALIBRATION)) {
			z_nrf_clock_calibration_lfclk_started();
		}
		clkstarted_handle(dev, CLOCK_CONTROL_NRF_TYPE_LFCLK);
	}

	usb_power_isr();

	if (IS_ENABLED(CONFIG_CLOCK_CONTROL_NRF_K32SRC_RC_CALIBRATION)) {
		z_nrf_clock_calibration_isr();
	}
}

#ifdef CONFIG_USB_NRFX
void nrf5_power_usb_power_int_enable(bool enable)
{
	uint32_t mask;

	mask = NRF_POWER_INT_USBDETECTED_MASK |
	       NRF_POWER_INT_USBREMOVED_MASK |
	       NRF_POWER_INT_USBPWRRDY_MASK;

	if (enable) {
		nrf_power_int_enable(NRF_POWER, mask);
		irq_enable(DT_INST_IRQN(0));
	} else {
		nrf_power_int_disable(NRF_POWER, mask);
	}
}
#endif

static int cmd_status(const struct shell *shell, size_t argc, char **argv)
{
	nrf_clock_hfclk_t hfclk_src;
	bool hf_status;
	bool lf_status =
		nrf_clock_is_running(NRF_CLOCK, NRF_CLOCK_DOMAIN_LFCLK, NULL);
	struct onoff_manager *hf_mgr =
				get_onoff_manager(DEVICE_GET(clock_nrf),
						  CLOCK_CONTROL_NRF_TYPE_HFCLK);
	struct onoff_manager *lf_mgr =
				get_onoff_manager(DEVICE_GET(clock_nrf),
						  CLOCK_CONTROL_NRF_TYPE_LFCLK);
	uint32_t abs_start, abs_stop;
	int key = irq_lock();
	uint64_t now = k_uptime_get();

	(void)nrf_clock_is_running(NRF_CLOCK, NRF_CLOCK_DOMAIN_HFCLK,
					(void *)&hfclk_src);
	hf_status = (hfclk_src == NRF_CLOCK_HFCLK_HIGH_ACCURACY);

	abs_start = hf_start_tstamp;
	abs_stop = hf_stop_tstamp;
	irq_unlock(key);

	shell_print(shell, "HF clock:");
	shell_print(shell, "\t- %srunning (users: %u)",
			hf_status ? "" : "not ", hf_mgr->refs);
	shell_print(shell, "\t- last start: %u ms (%u ms ago)",
			(uint32_t)abs_start, (uint32_t)(now - abs_start));
	shell_print(shell, "\t- last stop: %u ms (%u ms ago)",
			(uint32_t)abs_stop, (uint32_t)(now - abs_stop));
	shell_print(shell, "LF clock:");
	shell_print(shell, "\t- %srunning (users: %u)",
			lf_status ? "" : "not ", lf_mgr->refs);

	return 0;
}

SHELL_STATIC_SUBCMD_SET_CREATE(subcmds,
	SHELL_CMD_ARG(status, NULL, "Status", cmd_status, 1, 0),
	SHELL_SUBCMD_SET_END
);

SHELL_COND_CMD_REGISTER(CONFIG_CLOCK_CONTROL_NRF_SHELL,
			nrf_clock_control, &subcmds,
			"Clock control commmands",
			cmd_status);
