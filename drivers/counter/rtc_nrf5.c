#include <errno.h>
#include <counter.h>
#include <soc.h>
#include <nrf_rtc.h>
#include <nrf5_common.h>
#include <misc/__assert.h>

#include <misc/printk.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef void (*irq_config_func_t)(struct device *dev);

/** Configuration data */
struct rtc_nrf5_config {
	NRF_RTC_Type *base;
	IRQn_Type IRQn;
	/* Number of available capture/compare registers */
	u8_t cc_num;
	irq_config_func_t config_func;
};

/** Driver data */
struct rtc_nrf5_data {
	bool enabled;
	struct k_mutex access_mutex;
	volatile u32_t counter_32; /* 32 bit long counter used to conform to api
				    * counter length. Nordic RTC peripherals
				    * counter is only 24 bit long.
				    */
	u8_t free_cc; /* Number of free Capture/Compare registers.
		       * Semaphore is not needed here cause driver does not
		       * block on rtc_nrf5_set_alarm() function.
		       */
	bool *cc_busy_table; /* Table containing information which
			      * Capture/Compare channels are allocated.
			      */
	u32_t *alarms_table; /* Pointer to array with alarm values */
	counter_callback_t *cb_fns; /* Pointer to array storing alarms
				     * callback functions
				     */
	void **user_data_table; /* Pointer to array of user data pointers */
};

#define DEV_RTC_CFG(dev) \
	((struct rtc_nrf5_config * const)(dev)->config->config_info)
#define DEV_RTC_DATA(dev) ((struct rtc_nrf5_data * const)(dev)->driver_data)
#define RTC_STRUCT(dev) ((NRF_RTC_Type *)(DEV_RTC_CFG(dev))->base)

static int rtc_nrf5_start(struct device *dev)
{
	NRF_RTC_Type *rtc = RTC_STRUCT(dev);
	struct rtc_nrf5_data *data = DEV_RTC_DATA(dev);

	k_mutex_lock(&data->access_mutex, K_FOREVER);

	rtc->TASKS_START = 1;
	data->enabled = true;

	k_mutex_unlock(&data->access_mutex);

	return 0;
}

static int rtc_nrf5_stop(struct device *dev)
{
	NRF_RTC_Type *rtc = RTC_STRUCT(dev);
	struct rtc_nrf5_data *data = DEV_RTC_DATA(dev);

	k_mutex_lock(&data->access_mutex, K_FOREVER);

	rtc->TASKS_STOP = 1;
	data->enabled = false;

	k_mutex_unlock(&data->access_mutex);

	return 0;
}

#define RTC_CC_EVENT(i) rtc->EVENTS_COMPARE[i]

static inline void _cancel_alarm(struct device *dev,
				 u8_t cc_index)
{
	NRF_RTC_Type *rtc = RTC_STRUCT(dev);
	struct rtc_nrf5_data *data = DEV_RTC_DATA(dev);

	u32_t int_mask = (u32_t)NRF_RTC_INT_COMPARE0_MASK;

	int_mask <<= cc_index;

	nrf_rtc_int_disable(rtc, int_mask);
	data->free_cc++;
	data->cc_busy_table[cc_index] = false;
	RTC_CC_EVENT(cc_index) = 0;
}

static inline int _set_alarm(struct device *dev,
			    counter_callback_t callback,
			    u32_t count, void *user_data)
{
	NRF_RTC_Type *rtc = RTC_STRUCT(dev);
	struct rtc_nrf5_config *config = DEV_RTC_CFG(dev);
	struct rtc_nrf5_data *data = DEV_RTC_DATA(dev);
	int ret = -ENOMEM;

	u32_t int_mask = (u32_t)NRF_RTC_INT_COMPARE0_MASK;

	for (int i = 0; i < config->cc_num; i++) {
		if (data->cc_busy_table[i] == false) {
			data->free_cc--;
			data->cc_busy_table[i] = true;

			data->alarms_table[i] = count;
			data->cb_fns[i] = callback;
			data->user_data_table[i] = user_data;

			RTC_CC_EVENT(i) = 0;

			if ((int32_t)(count - counter_read(dev)) >
			    RTC_MIN_DELTA) {
				RTC_CC_EVENT(i) = 0;
				nrf_rtc_int_enable(rtc, int_mask);
				rtc->CC[i] = count & RTC_MASK;
				ret = i;
			}

			/* Check again if condition is met. If there was any
			 * preemption and condition is not met cancel the alarm.
			 */
			if ((int32_t)(count - counter_read(dev)) <
			    RTC_MIN_DELTA) {
				_cancel_alarm(dev, (uint8_t)i);
				ret = -ECANCELED;
			}

			break;
		}
		int_mask <<= 1;
	}

	return ret;
}

static int rtc_nrf5_set_alarm(struct device *dev,
			      counter_callback_t callback,
			      u32_t count, void *user_data)
{
	struct rtc_nrf5_data *data = DEV_RTC_DATA(dev);
	int ret;

	k_mutex_lock(&data->access_mutex, K_FOREVER);

	if (!data->enabled) {
		ret = -ENOTSUP;
		goto out;
	}

	if (data->free_cc == 0) {
		ret = -ENOMEM;
		goto out;
	}

	ret = _set_alarm(dev, callback, count, user_data);

out:
	k_mutex_unlock(&data->access_mutex);

	return ret;
}

static int rtc_nrf5_cancel_alarm(struct device *dev,
				 int alarm_descriptor)
{
	struct rtc_nrf5_data *data = DEV_RTC_DATA(dev);
	struct rtc_nrf5_config *config = DEV_RTC_CFG(dev);

	int ret;

	k_mutex_lock(&data->access_mutex, K_FOREVER);

	if (alarm_descriptor < 0 || alarm_descriptor > config->cc_num) {
		ret = -EINVAL;
		goto out;
	} else if (data->cc_busy_table[alarm_descriptor] == false) {
		ret = -ENOTSUP;
		goto out;
	} else {
		_cancel_alarm(dev, (uint8_t)alarm_descriptor);
		ret = 0;
	}

out:
	k_mutex_unlock(&data->access_mutex);

	return ret;
}

static int rtc_nrf5_config(struct device *dev, struct counter_config *config)
{
	NRF_RTC_Type *rtc = RTC_STRUCT(dev);
	struct rtc_nrf5_data *data = DEV_RTC_DATA(dev);
	struct rtc_nrf5_config *init_config = DEV_RTC_CFG(dev);

	k_mutex_lock(&data->access_mutex, K_FOREVER);

	if (data->free_cc < init_config->cc_num) {
		k_mutex_unlock(&data->access_mutex);
		return -EBUSY;
	}

	nrf_rtc_task_trigger(rtc, NRF_RTC_TASK_STOP);

	u32_t prescaler = config->divider - 1;

	__ASSERT(prescaler <=
		 (RTC_PRESCALER_PRESCALER_Msk >> RTC_PRESCALER_PRESCALER_Pos),
		 "Incorrect RTC frequency prescaler value\n");
	nrf_rtc_prescaler_set(rtc, prescaler);

	nrf_rtc_task_trigger(rtc, NRF_RTC_TASK_CLEAR);
	data->counter_32 = config->init_val;

	if (data->enabled) {
		nrf_rtc_task_trigger(rtc, NRF_RTC_TASK_START);
	}

	k_mutex_unlock(&data->access_mutex);

	return 0;
}

static u32_t rtc_nrf5_read(struct device *dev)
{
	NRF_RTC_Type *rtc = RTC_STRUCT(dev);
	struct rtc_nrf5_data *data = DEV_RTC_DATA(dev);

	u32_t rtc_val;

	do {
		rtc_val = data->counter_32 + rtc->COUNTER;
		if (rtc->EVENTS_OVRFLW == 1) {
			rtc_val = data->counter_32 + rtc->COUNTER;
			rtc_val += (RTC_MASK + 1);
		}
	} while ((rtc_val != data->counter_32 + rtc->COUNTER) &&
		 rtc->EVENTS_OVRFLW == 0);

	return rtc_val;
}

static u32_t rtc_nrf5_get_pending_int(struct device *dev)
{
	struct rtc_nrf5_config *config = DEV_RTC_CFG(dev);

	return NVIC_GetPendingIRQ(config->IRQn);
}

static int rtc_nrf5_init(struct device *dev)
{
	NRF_RTC_Type *rtc = RTC_STRUCT(dev);
	struct rtc_nrf5_config *config = DEV_RTC_CFG(dev);

	config->config_func(dev);

	irq_enable(config->IRQn);

	/* Only overflow interrupt needs to be enabled for conforming
	 * 32 bit long counter.
	 */
	rtc->EVENTS_OVRFLW = 0;
	nrf_rtc_int_enable(rtc, NRF_RTC_INT_OVERFLOW_MASK);

	return 0;
}

static inline void _overflow_handler(struct device *dev)
{
	NRF_RTC_Type *rtc = RTC_STRUCT(dev);
	struct rtc_nrf5_data *data = DEV_RTC_DATA(dev);

	u32_t key = irq_lock();

	rtc->EVENTS_OVRFLW = 0;
	data->counter_32 += (RTC_MASK + 1);
	irq_unlock(key);
}

static inline void _cc_event_handler(struct device *dev,
				    u32_t cc_index,
				    u32_t int_mask)
{
	NRF_RTC_Type *rtc = RTC_STRUCT(dev);
	struct rtc_nrf5_data *data = DEV_RTC_DATA(dev);

	RTC_CC_EVENT(cc_index) = 0;

	/* The following checks if rtc counter value is grater than alarm.
	 * It also handles 32 bit counter wraparound
	 */
	if ((int32_t)(data->alarms_table[cc_index] - rtc_nrf5_read(dev)) <= 0) {
		nrf_rtc_int_disable(rtc, int_mask);
		if (data->cb_fns[cc_index] != NULL) {
			data->cb_fns[cc_index](dev,
					       data->user_data_table[cc_index]);
		}
		data->free_cc++;
		data->cc_busy_table[cc_index] = false;
	} else {
		/* Do nothing, CC event was triggered but value of extended 32
		 * bit long counter is not equal to set alarm value.
		 */
		;
	}
}

#define OVERFLOW_EVENT (nrf_rtc_int_is_enabled(rtc, NRF_RTC_INT_OVERFLOW_MASK) \
		      && nrf_rtc_event_pending(rtc, NRF_RTC_EVENT_OVERFLOW))
#define TICK_EVENT (nrf_rtc_int_is_enabled(rtc, NRF_RTC_INT_TICK_MASK) && \
		    nrf_rtc_event_pending(rtc, NRF_RTC_EVENT_TICK))

static void rtc_nrf5_isr(struct device *dev)
{
	NRF_RTC_Type *rtc = RTC_STRUCT(dev);
	struct rtc_nrf5_config *config = DEV_RTC_CFG(dev);

	if (OVERFLOW_EVENT) {
		_overflow_handler(dev);
	}
	/* RTC TICK event is not used in Zephyr,
	 * so do not waste time checking this event.
	if (TICK_EVENT) {
		rtc->TICK = 0;
		;
	}
	*/

	u32_t i;
	u32_t int_mask = (u32_t)NRF_RTC_INT_COMPARE0_MASK;

	for (i = 0; i < config->cc_num; i++) {
		if (nrf_rtc_int_is_enabled(rtc, int_mask) &&
		    RTC_CC_EVENT(i) == 1) {
			_cc_event_handler(dev, i, int_mask);
		}
		int_mask <<= 1;
	}
}

static const struct counter_driver_api rtc_nrf5_drv_api = {
	.config = rtc_nrf5_config,
	.start = rtc_nrf5_start,
	.stop = rtc_nrf5_stop,
	.read = rtc_nrf5_read,
	.set_alarm = rtc_nrf5_set_alarm,
	.cancel_alarm = rtc_nrf5_cancel_alarm,
	.get_pending_int = rtc_nrf5_get_pending_int
};

#ifdef CONFIG_RTC_0
DEVICE_DECLARE(rtc_nrf5_0);

void irq_config_rtc_0(struct device *dev)
{
	IRQ_CONNECT(RTC0_IRQn, CONFIG_RTC_0_IRQ_PRI,
		    rtc_nrf5_isr, DEVICE_GET(rtc_nrf5_0), 0);
}

static struct rtc_nrf5_config rtc_nrf5_config_0 = {
	.base = NRF_RTC0,
	.IRQn = RTC0_IRQn,
	.cc_num = RTC0_CC_NUM,
	.config_func = irq_config_rtc_0
};

static bool cc_busy_table_0[RTC0_CC_NUM];
static u32_t alarms_table_0[RTC0_CC_NUM];
static counter_callback_t alarm_cb_fns_0[RTC0_CC_NUM];
static void *user_data_table_0[RTC0_CC_NUM];

struct rtc_nrf5_data rtc_nrf5_data_0 = {
	.enabled = false,
	.access_mutex = K_MUTEX_INITIALIZER(rtc_nrf5_data_0.access_mutex),
	.counter_32 = 0,
	.free_cc = RTC0_CC_NUM,
	.cc_busy_table = cc_busy_table_0,
	.alarms_table = alarms_table_0,
	.cb_fns = alarm_cb_fns_0,
	.user_data_table = user_data_table_0
};

DEVICE_AND_API_INIT(rtc_nrf5_0, CONFIG_RTC_0_NAME, rtc_nrf5_init,
		    &rtc_nrf5_data_0, &rtc_nrf5_config_0,
		    POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_DEVICE,
		    &rtc_nrf5_drv_api);

#endif /* CONFIG_RTC_0 */

#ifdef CONFIG_RTC_1
DEVICE_DECLARE(rtc_nrf5_1);

void irq_config_rtc_1(struct device *dev)
{
	IRQ_CONNECT(RTC1_IRQn, CONFIG_RTC_0_IRQ_PRI,
		    rtc_nrf5_isr, DEVICE_GET(rtc_nrf5_0), 0);
}

static struct rtc_nrf5_config rtc_nrf5_config_1 = {
	.base = NRF_RTC1,
	.IRQn = RTC1_IRQn,
	.cc_num = RTC1_CC_NUM,
	.config_func = irq_config_rtc_1
};

static bool cc_busy_table_1[RTC1_CC_NUM];
static u32_t alarms_table_1[RTC1_CC_NUM];
static counter_callback_t alarm_cb_fns_1[RTC1_CC_NUM];
static void *user_data_table_1[RTC1_CC_NUM];

struct rtc_nrf5_data rtc_nrf5_data_1 = {
	.enabled = false,
	.access_mutex = K_MUTEX_INITIALIZER(rtc_nrf5_data_1.access_mutex),
	.counter_32 = 0,
	.free_cc = RTC1_CC_NUM,
	.cc_busy_table = cc_busy_table_1,
	.alarms_table = alarms_table_1,
	.cb_fns = alarm_cb_fns_1,
	.user_data_table = user_data_table_1
};

DEVICE_AND_API_INIT(rtc_nrf5_1, CONFIG_RTC_1_NAME, rtc_nrf5_init,
		    &rtc_nrf5_data_1, &rtc_nrf5_config_1,
		    POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_DEVICE,
		    &rtc_nrf5_drv_api);
#endif /* CONFIG_RTC_1 */

#ifdef CONFIG_RTC_2
#	if defined(NRF51)
		#error "NRF51 family does not have RTC_2 peripheral."
#	elif defined(NRF52832_XXAA) || defined(NRF52840_XXAA)
DEVICE_DECLARE(rtc_nrf5_2);

void irq_config_rtc_2(struct device *dev)
{
	IRQ_CONNECT(RTC2_IRQn, CONFIG_RTC_0_IRQ_PRI,
		    rtc_nrf5_isr, DEVICE_GET(rtc_nrf5_0), 0);
}

static struct rtc_nrf5_config rtc_nrf5_config_2 = {
	.base = NRF_RTC2,
	.IRQn = RTC2_IRQn,
	.cc_num = RTC2_CC_NUM,
	.config_func = irq_config_rtc_2
};

static bool cc_busy_table_2[RTC2_CC_NUM];
static u32_t alarms_table_2[RTC2_CC_NUM];
static counter_callback_t alarm_cb_fns_2[RTC2_CC_NUM];
static void *user_data_table_2[RTC2_CC_NUM];

struct rtc_nrf5_data rtc_nrf5_data_2 = {
	.enabled = false,
	.access_mutex = K_MUTEX_INITIALIZER(rtc_nrf5_data_2.access_mutex),
	.counter_32 = 0,
	.free_cc = RTC2_CC_NUM,
	.cc_busy_table = cc_busy_table_2,
	.alarms_table = alarms_table_2,
	.cb_fns = alarm_cb_fns_2,
	.user_data_table = user_data_table_2
};

DEVICE_AND_API_INIT(rtc_nrf5_2, CONFIG_RTC_2_NAME, rtc_nrf5_init,
		    &rtc_nrf5_data_2, &rtc_nrf5_config_2,
		    POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_DEVICE,
		    &rtc_nrf5_drv_api);
#	endif
#endif /* CONFIG_RTC_2 */

#ifdef __cplusplus
}
#endif
