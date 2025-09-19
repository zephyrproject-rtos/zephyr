#include <soc.h>
#include <zephyr/sys/onoff.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/drivers/clock_control/nrf_clock_control.h>
#include "nrf_clock_calibration.h"
#include <nrfx_clock_xo.h>
#include <zephyr/logging/log.h>
#include <zephyr/shell/shell.h>
#include <zephyr/irq.h>
#include <nrf_erratas.h>
#include "clock_control_nrf_common.h"

LOG_MODULE_REGISTER(clock_control_xo, CONFIG_CLOCK_CONTROL_LOG_LEVEL);

#define DT_DRV_COMPAT nordic_nrf_clock_xo

#define CLOCK_DEVICE_XO DEVICE_DT_GET(DT_NODELABEL(xo))

#define CTX_ONOFF		BIT(6)
#define CTX_API			BIT(7)
#define CTX_MASK (CTX_ONOFF | CTX_API)

#define STATUS_MASK		0x7
#define GET_STATUS(flags)	(flags & STATUS_MASK)
#define GET_CTX(flags)		(flags & CTX_MASK)

/* Used only by HF clock */
#define XO_USER_BT		BIT(0)
#define XO_USER_GENERIC		BIT(1)

/* Helper logging macros which prepends subsys name to the log. */
#ifdef CONFIG_LOG
#define CLOCK_LOG(lvl, dev, ...) \
	LOG_##lvl("%s: " GET_ARG_N(1, __VA_ARGS__), \
		"xo" \
		COND_CODE_0(NUM_VA_ARGS_LESS_1(__VA_ARGS__),\
				(), (, GET_ARGS_LESS_N(1, __VA_ARGS__))))
#else
#define CLOCK_LOG(...)
#endif

#define ERR(dev, ...) CLOCK_LOG(ERR, dev, __VA_ARGS__)
#define WRN(dev, ...) CLOCK_LOG(WRN, dev, __VA_ARGS__)
#define INF(dev, ...) CLOCK_LOG(INF, dev, __VA_ARGS__)
#define DBG(dev, ...) CLOCK_LOG(DBG, dev, __VA_ARGS__)

typedef void (*clk_ctrl_xo_func_t)(void);

typedef struct {
	struct onoff_manager mgr;
	clock_control_cb_t cb;
	void *user_data;
	uint32_t flags;
} xo_data_t;

typedef struct {
    clk_ctrl_xo_func_t start;		/* Clock start function */
	clk_ctrl_xo_func_t stop;		/* Clock stop function */
#ifdef CONFIG_LOG
	const char *name;
#endif
} xo_config_t;

static atomic_t xo_users;
static uint64_t xo_start_tstamp;
static uint64_t xo_stop_tstamp;

static int set_starting_state(uint32_t *flags, uint32_t ctx)
{
	int err = 0;
	unsigned int key = irq_lock();
	uint32_t current_ctx = GET_CTX(*flags);

	if ((*flags & (STATUS_MASK)) == CLOCK_CONTROL_STATUS_OFF) {
		*flags = CLOCK_CONTROL_STATUS_STARTING | ctx;
	} else if (current_ctx != ctx) {
		err = -EPERM;
	} else {
		err = -EALREADY;
	}

	irq_unlock(key);

	return err;
}

static int async_start(const struct device *dev, clock_control_cb_t cb,
			void *user_data, uint32_t ctx)
{
	int err;

	err = set_starting_state(&((xo_data_t *)dev->data)->flags, ctx);
	if (err < 0) {
		return err;
	}

	((xo_data_t *)dev->data)->cb = cb;
	((xo_data_t *)dev->data)->user_data = user_data;

	((xo_config_t *)dev->config)->start();

	return 0;
}

static int set_off_state(uint32_t *flags, uint32_t ctx)
{
	int err = 0;
	unsigned int key = irq_lock();
	uint32_t current_ctx = GET_CTX(*flags);

	if ((current_ctx != 0) && (current_ctx != ctx)) {
		err = -EPERM;
	} else {
		*flags = CLOCK_CONTROL_STATUS_OFF;
	}

	irq_unlock(key);

	return err;
}

static void set_on_state(uint32_t *flags)
{
	unsigned int key = irq_lock();

	*flags = CLOCK_CONTROL_STATUS_ON | GET_CTX(*flags);
	irq_unlock(key);
}

static int stop(const struct device *dev, uint32_t ctx)
{
	int err;

	err = set_off_state(&((xo_data_t *)dev->data)->flags, ctx);
	if (err < 0) {
		return err;
	}

	((xo_config_t *)dev->config)->stop();

	return 0;
}

static void xo_start(void)
{
	if (IS_ENABLED(CONFIG_CLOCK_CONTROL_NRF_SHELL)) {
		xo_start_tstamp = k_uptime_get();
	}

	nrfx_clock_xo_start();
}

static void xo_stop(void)
{
	if (IS_ENABLED(CONFIG_CLOCK_CONTROL_NRF_SHELL)) {
		xo_stop_tstamp = k_uptime_get();
	}

	nrfx_clock_xo_stop();
}

static void clkstarted_handle(const struct device *dev)
{
	clock_control_cb_t callback = ((xo_data_t *)dev->data)->cb;
	void *user_data = ((xo_data_t *)dev->data)->user_data;

	((xo_data_t *)dev->data)->cb = NULL;
	set_on_state(&((xo_data_t *)dev->data)->flags);
	DBG(dev, "Clock started");

	if (callback) {
		callback(dev, (clock_control_subsys_t)CLOCK_CONTROL_NRF_TYPE_HFCLK, user_data);
	}
}

static void generic_xo_start(void)
{
	nrf_clock_hfclk_t type;
	bool already_started = false;
	unsigned int key = irq_lock();

	xo_users |= XO_USER_GENERIC;
	if (xo_users & XO_USER_BT) {
		(void)nrfx_clock_xo_running_check(&type);
		if (type == NRF_CLOCK_HFCLK_HIGH_ACCURACY) {
			already_started = true;
			/* Set on state in case clock interrupt comes and we
			 * want to avoid handling that.
			 */
			set_on_state(
				&((xo_data_t *)CLOCK_DEVICE_XO->data)->flags);
		}
	}

	irq_unlock(key);

	if (already_started) {
		/* Clock already started by z_nrf_clock_bt_ctlr_hf_request */
		clkstarted_handle(CLOCK_DEVICE_XO);
		return;
	}

	xo_start();
}

static void generic_xo_stop(void)
{
	/* It's not enough to use only atomic_and() here for synchronization,
	 * as the thread could be preempted right after that function but
	 * before xo_stop() is called and the preempting code could request
	 * the XO again. Then, the XO would be stopped inappropriately
	 * and xo_user would be left with an incorrect value.
	 */
	unsigned int key = irq_lock();

	xo_users &= ~XO_USER_GENERIC;
	/* Skip stopping if BT is still requesting the clock. */
	if (!(xo_users & XO_USER_BT)) {
		xo_stop();
	}

	irq_unlock(key);
}

static void onoff_started_callback(const struct device *dev,
				   clock_control_subsys_t sys,
				   void *user_data)
{
	ARG_UNUSED(sys);

	onoff_notify_fn notify = user_data;

	notify(&(((xo_data_t *)dev->data)->mgr), 0);
}


static void onoff_start(struct onoff_manager *mgr,
			onoff_notify_fn notify)
{
	int err;

	err = async_start(CLOCK_DEVICE_XO, onoff_started_callback, notify, CTX_ONOFF);
	if (err < 0) {
		notify(mgr, err);
	}
}

static void onoff_stop(struct onoff_manager *mgr,
			onoff_notify_fn notify)
{
	int res;

	res = stop(CLOCK_DEVICE_XO, CTX_ONOFF);
	notify(mgr, res);
}

static void clock_event_handler(nrfx_clock_xo_evt_type_t event)
{
	const struct device *dev = CLOCK_DEVICE_XO;

	switch (event) {
#if NRF_CLOCK_HAS_XO_TUNE
	case NRFX_CLOCK_XO_EVT_XO_TUNED:
		clkstarted_handle(dev);
		break;
	case NRFX_CLOCK_XO_EVT_XO_TUNE_ERROR:
	case NRFX_CLOCK_XO_EVT_XO_TUNE_FAILED:
		/* No processing needed. */
		break;
	case NRFX_CLOCK_XO_EVT_HFCLK_STARTED:
		/* HFCLK is stable after XOTUNED event.
		 * HFCLK_STARTED means only that clock has been started.
		 */
		break;
#else
	/* HFCLK started should be used only if tune operation is done implicitly. */
	case NRFX_CLOCK_XO_EVT_HFCLK_STARTED:
	{
		/* Check needed due to anomaly 201:
		 * HFCLKSTARTED may be generated twice.
		 */
		if (GET_STATUS(((xo_data_t*)dev->data)->flags)
					== CLOCK_CONTROL_STATUS_STARTING) {
			clkstarted_handle(dev);
		}

		break;
	}
#endif

#if NRF_CLOCK_HAS_PLL
	case NRFX_CLOCK_XO_EVT_PLL_STARTED:
		/* No processing needed. */
		break;
#endif
	default:
		__ASSERT_NO_MSG(0);
		break;
	}
}

static void blocking_start_callback(const struct device *dev,
				    clock_control_subsys_t subsys,
				    void *user_data)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(subsys);

	struct k_sem *sem = user_data;

	k_sem_give(sem);
}

void z_nrf_clock_bt_ctlr_hf_request(void)
{
	if (atomic_or(&xo_users, XO_USER_BT) & XO_USER_GENERIC) {
		/* generic request already activated clock. */
		return;
	}

	xo_start();
}

void z_nrf_clock_bt_ctlr_hf_release(void)
{
	/* It's not enough to use only atomic_and() here for synchronization,
	 * see the explanation in generic_hfclk_stop().
	 */
	unsigned int key = irq_lock();

	xo_users &= ~XO_USER_BT;
	/* Skip stopping if generic is still requesting the clock. */
	if (!(xo_users & XO_USER_GENERIC)) {

		/* State needs to be set to OFF as BT API does not call stop API which
		 * normally setting this state.
		 */
		((xo_data_t*)CLOCK_DEVICE_XO->data)->flags = CLOCK_CONTROL_STATUS_OFF;
		xo_stop();
	}

	irq_unlock(key);
}

#if DT_NODE_EXISTS(DT_NODELABEL(hfxo))
uint32_t z_nrf_clock_bt_ctlr_hf_get_startup_time_us(void)
{
	return DT_PROP(DT_NODELABEL(hfxo), startup_time_us);
}
#endif

static int api_start(const struct device *dev, clock_control_subsys_t subsys,
		     clock_control_cb_t cb, void *user_data)
{
	ARG_UNUSED(subsys);

	return async_start(dev, cb, user_data, CTX_API);
}

static int api_blocking_start(const struct device *dev,
			      clock_control_subsys_t subsys)
{
	struct k_sem sem = Z_SEM_INITIALIZER(sem, 0, 1);
	int err;

	if (!IS_ENABLED(CONFIG_MULTITHREADING)) {
		return -ENOTSUP;
	}

	err = api_start(dev, subsys, blocking_start_callback, &sem);
	if (err < 0) {
		return err;
	}

	return k_sem_take(&sem, K_MSEC(500));
}

static int api_stop(const struct device *dev, clock_control_subsys_t subsys)
{
	ARG_UNUSED(subsys);

	return stop(dev, CTX_API);
}

static enum clock_control_status api_get_status(const struct device *dev,
					    clock_control_subsys_t subsys)
{
	return GET_STATUS(((xo_data_t *)dev->data)->flags);
}

static int api_request(const struct device *dev,
		       const struct nrf_clock_spec *spec,
		       struct onoff_client *cli)
{
	xo_data_t *dev_data = dev->data;

	ARG_UNUSED(spec);

	return onoff_request(&dev_data->mgr, cli);
}

static int api_release(const struct device *dev,
		       const struct nrf_clock_spec *spec)
{
	xo_data_t *dev_data = dev->data;

	ARG_UNUSED(spec);

	return onoff_release(&dev_data->mgr);
}

static int api_cancel_or_release(const struct device *dev,
				 const struct nrf_clock_spec *spec,
				 struct onoff_client *cli)
{
	xo_data_t *dev_data = dev->data;

	ARG_UNUSED(spec);

	return onoff_cancel_or_release(&dev_data->mgr, cli);
}

static int clk_init(const struct device *dev)
{
	nrfx_err_t nrfx_err;
	int err;
	static const struct onoff_transitions transitions = {
		.start = onoff_start,
		.stop = onoff_stop
	};
    
	clock_control_nrf_common_connect_irq();

	nrfx_err = nrfx_clock_xo_init(clock_event_handler);
	if (nrfx_err != NRFX_SUCCESS) {
		return -EIO;
	}

	if (IS_ENABLED(CONFIG_CLOCK_CONTROL_NRF_DRIVER_CALIBRATION)) {
		xo_data_t *data = ((xo_data_t*)dev->data);

		z_nrf_clock_calibration_init(&data->mgr);
	}

	err = onoff_manager_init(&((xo_data_t*)dev->data)->mgr,
					&transitions);
	if (err < 0) {
		return err;
	}

	((xo_data_t*)dev->data)->flags = CLOCK_CONTROL_STATUS_OFF;

	return 0;
}

CLOCK_CONTROL_NRF_IRQ_HANDLERS_ITERABLE(clock_control_nrf_xo,
                                	&nrfx_clock_xo_irq_handler);

static DEVICE_API(nrf_clock_control, clock_control_api) = {
	.std_api = {
		.on = api_blocking_start,
		.off = api_stop,
		.async_on = api_start,
		.get_status = api_get_status,
	},
	.request = api_request,
	.release = api_release,
	.cancel_or_release = api_cancel_or_release,
};

static xo_data_t data;

static const xo_config_t config = {
    .start = generic_xo_start,
    .stop = generic_xo_stop,
    IF_ENABLED(CONFIG_LOG, (.name = "xo",))
};

DEVICE_DT_DEFINE(DT_NODELABEL(xo), clk_init, NULL,
		 &data, &config,
		 PRE_KERNEL_1, CONFIG_CLOCK_CONTROL_INIT_PRIORITY,
		 &clock_control_api);
