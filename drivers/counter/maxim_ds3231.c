/*
 * Copyright (c) 2019-2020 Peter Bigot Consulting, LLC
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT maxim_ds3231

#include <device.h>
#include <drivers/rtc/maxim_ds3231.h>
#include <drivers/gpio.h>
#include <drivers/i2c.h>
#include <kernel.h>
#include <logging/log.h>
#include <sys/timeutil.h>
#include <sys/util.h>

LOG_MODULE_REGISTER(DS3231, CONFIG_COUNTER_LOG_LEVEL);

#define REG_MONCEN_CENTURY 0x80
#define REG_HOURS_12H 0x40
#define REG_HOURS_PM 0x20
#define REG_HOURS_20 0x20
#define REG_HOURS_10 0x20
#define REG_DAYDATE_DOW 0x40
#define REG_ALARM_IGN 0x80

enum {
	SYNCSM_IDLE,
	SYNCSM_PREP_READ,
	SYNCSM_FINISH_READ,
	SYNCSM_PREP_WRITE,
	SYNCSM_FINISH_WRITE,
};

struct register_map {
	uint8_t sec;
	uint8_t min;
	uint8_t hour;
	uint8_t dow;
	uint8_t dom;
	uint8_t moncen;
	uint8_t year;

	struct {
		uint8_t sec;
		uint8_t min;
		uint8_t hour;
		uint8_t date;
	} __packed alarm1;

	struct {
		uint8_t min;
		uint8_t hour;
		uint8_t date;
	} __packed alarm2;

	uint8_t ctrl;
	uint8_t ctrl_stat;
	uint8_t aging;
	int8_t temp_units;
	uint8_t temp_frac256;
};

struct gpios {
	const char *ctrl;
	gpio_pin_t pin;
	gpio_dt_flags_t flags;
};

struct ds3231_config {
	/* Common structure first because generic API expects this here. */
	struct counter_config_info generic;
	const char *bus_name;
	struct gpios isw_gpios;
	uint16_t addr;
};

struct ds3231_data {
	const struct device *ds3231;
	const struct device *i2c;
	const struct device *isw;
	struct register_map registers;

	struct k_sem lock;

	/* Timer structure used for synchronization */
	struct k_timer sync_timer;

	/* Work structures for the various cases of ISW interrupt. */
	struct k_work alarm_work;
	struct k_work sqw_work;
	struct k_work sync_work;

	/* Forward ISW interrupt to proper worker. */
	struct gpio_callback isw_callback;

	/* syncclock captured in the last ISW interrupt handler */
	uint32_t isw_syncclock;

	struct maxim_ds3231_syncpoint syncpoint;
	struct maxim_ds3231_syncpoint new_sp;

	time_t rtc_registers;
	time_t rtc_base;
	uint32_t syncclock_base;

	/* Pointer to the structure used to notify when a synchronize
	 * or set operation completes.  Null when nobody's waiting for
	 * such an operation, or when doing a no-notify synchronize
	 * through the signal API.
	 */
	union {
		void *ptr;
		struct sys_notify *notify;
		struct k_poll_signal *signal;
	} sync;

	/* Handlers and state when using the counter alarm API. */
	counter_alarm_callback_t counter_handler[2];
	uint32_t counter_ticks[2];

	/* Handlers and state for DS3231 alarm API. */
	maxim_ds3231_alarm_callback_handler_t alarm_handler[2];
	void *alarm_user_data[2];
	uint8_t alarm_flags[2];

	/* Flags recording requests for ISW monitoring. */
	uint8_t isw_mon_req;
#define ISW_MON_REQ_Alarm 0x01
#define ISW_MON_REQ_Sync 0x02

	/* Status of synchronization operations. */
	uint8_t sync_state;
	bool sync_signal;
};

/*
 * Set and clear specific bits in the control register.
 *
 * This function assumes the device register cache is valid and will
 * update the device only if the value changes as a result of applying
 * the set and clear changes.
 *
 * Caches and returns the value with the changes applied.
 */
static int sc_ctrl(const struct device *dev,
		   uint8_t set,
		   uint8_t clear)
{
	struct ds3231_data *data = dev->data;
	const struct ds3231_config *cfg = dev->config;
	struct register_map *rp = &data->registers;
	uint8_t ctrl = (rp->ctrl & ~clear) | set;
	int rc = ctrl;

	if (rp->ctrl != ctrl) {
		uint8_t buf[2] = {
			offsetof(struct register_map, ctrl),
			ctrl,
		};
		rc = i2c_write(data->i2c, buf, sizeof(buf), cfg->addr);
		if (rc >= 0) {
			rp->ctrl = ctrl;
			rc = ctrl;
		}
	}
	return rc;
}

int maxim_ds3231_ctrl_update(const struct device *dev,
			     uint8_t set_bits,
			     uint8_t clear_bits)
{
	struct ds3231_data *data = dev->data;

	k_sem_take(&data->lock, K_FOREVER);

	int rc = sc_ctrl(dev, set_bits, clear_bits);

	k_sem_give(&data->lock);

	return rc;
}

/*
 * Read the ctrl_stat register then set and clear bits in it.
 *
 * OSF, A1F, and A2F will be written with 1s if the corresponding bits
 * do not appear in either set or clear.  This ensures that if any
 * flag becomes set between the read and the write that indicator will
 * not be cleared.
 *
 * Returns the value as originally read (disregarding the effect of
 * clears and sets).
 */
static inline int rsc_stat(const struct device *dev,
			   uint8_t set,
			   uint8_t clear)
{
	uint8_t const ign = MAXIM_DS3231_REG_STAT_OSF | MAXIM_DS3231_ALARM1
			 | MAXIM_DS3231_ALARM2;
	struct ds3231_data *data = dev->data;
	const struct ds3231_config *cfg = dev->config;
	struct register_map *rp = &data->registers;
	uint8_t addr = offsetof(struct register_map, ctrl_stat);
	int rc;

	rc = i2c_write_read(data->i2c, cfg->addr,
			    &addr, sizeof(addr),
			    &rp->ctrl_stat, sizeof(rp->ctrl_stat));
	if (rc >= 0) {
		uint8_t stat = rp->ctrl_stat & ~clear;

		if (rp->ctrl_stat != stat) {
			uint8_t buf[2] = {
				addr,
				stat | (ign & ~(set | clear)),
			};
			rc = i2c_write(data->i2c, buf, sizeof(buf), cfg->addr);
		}
		if (rc >= 0) {
			rc = rp->ctrl_stat;
		}
	}
	return rc;
}

int maxim_ds3231_stat_update(const struct device *dev,
			     uint8_t set_bits,
			     uint8_t clear_bits)
{
	struct ds3231_data *data = dev->data;

	k_sem_take(&data->lock, K_FOREVER);

	int rv = rsc_stat(dev, set_bits, clear_bits);

	k_sem_give(&data->lock);

	return rv;
}

/*
 * Look for current users of the interrupt/square-wave signal and
 * enable monitoring iff at least one consumer is active.
 */
static void validate_isw_monitoring(const struct device *dev)
{
	struct ds3231_data *data = dev->data;
	const struct ds3231_config *cfg = dev->config;
	const struct register_map *rp = &data->registers;
	uint8_t isw_mon_req = 0;

	if (rp->ctrl & (MAXIM_DS3231_ALARM1 | MAXIM_DS3231_ALARM2)) {
		isw_mon_req |= ISW_MON_REQ_Alarm;
	}
	if (data->sync_state != SYNCSM_IDLE) {
		isw_mon_req |= ISW_MON_REQ_Sync;
	}
	LOG_DBG("ISW %p : %d ?= %d", data->isw, isw_mon_req, data->isw_mon_req);
	if ((data->isw != NULL)
	    && (isw_mon_req != data->isw_mon_req)) {
		int rc = 0;

		/* Disable before reconfigure */
		rc = gpio_pin_interrupt_configure(data->isw, cfg->isw_gpios.pin,
						  GPIO_INT_DISABLE);

		if ((rc >= 0)
		    && ((isw_mon_req & ISW_MON_REQ_Sync)
			!= (data->isw_mon_req & ISW_MON_REQ_Sync))) {
			if (isw_mon_req & ISW_MON_REQ_Sync) {
				rc = sc_ctrl(dev, 0,
					     MAXIM_DS3231_REG_CTRL_INTCN
					     | MAXIM_DS3231_REG_CTRL_RS_Msk);
			} else {
				rc = sc_ctrl(dev, MAXIM_DS3231_REG_CTRL_INTCN, 0);
			}
		}

		data->isw_mon_req = isw_mon_req;

		/* Enable if any requests active */
		if ((rc >= 0) && (isw_mon_req != 0)) {
			rc = gpio_pin_interrupt_configure(data->isw,
							  cfg->isw_gpios.pin,
							  GPIO_INT_EDGE_TO_ACTIVE);
		}

		LOG_INF("ISW reconfigure to %x: %d", isw_mon_req, rc);
	}
}

static const uint8_t *decode_time(struct tm *tp,
			       const uint8_t *rp,
			       bool with_sec)
{
	uint8_t reg;

	if (with_sec) {
		uint8_t reg = *rp++;

		tp->tm_sec = 10 * ((reg >> 4) & 0x07) + (reg & 0x0F);
	}

	reg = *rp++;
	tp->tm_min = 10 * ((reg >> 4) & 0x07) + (reg & 0x0F);

	reg = *rp++;
	tp->tm_hour = (reg & 0x0F);
	if (REG_HOURS_12H & reg) {
		/* 12-hour */
		if (REG_HOURS_10 & reg) {
			tp->tm_hour += 10;
		}
		if (REG_HOURS_PM & reg) {
			tp->tm_hour += 12;
		}
	} else {
		/* 24 hour */
		tp->tm_hour += 10 * ((reg >> 4) & 0x03);
	}

	return rp;
}

static uint8_t decode_alarm(const uint8_t *ap,
			 bool with_sec,
			 time_t *tp)
{
	struct tm tm = {
		/* tm_year zero is 1900 with underflows a 32-bit counter
		 * representation.  Use 1978-01, the first January after the
		 * POSIX epoch where the first day of the month is the first
		 * day of the week.
		 */
		.tm_year = 78,
	};
	const uint8_t *dp = decode_time(&tm, ap, with_sec);
	uint8_t flags = 0;
	uint8_t amf = MAXIM_DS3231_ALARM_FLAGS_IGNDA;

	/* Done decoding time, now decode day/date. */
	if (REG_DAYDATE_DOW & *dp) {
		flags |= MAXIM_DS3231_ALARM_FLAGS_DOW;

		/* Because tm.tm_wday does not contribute to the UNIX
		 * time that the civil time translates into, we need
		 * to also record the tm_mday for our selected base
		 * 1978-01 that will produce the correct tm_wday.
		 */
		tm.tm_mday = (*dp & 0x07);
		tm.tm_wday = tm.tm_mday - 1;
	} else {
		tm.tm_mday = 10 * ((*dp >> 4) & 0x3) + (*dp & 0x0F);
	}

	/* Walk backwards to extract the alarm mask flags. */
	while (true) {
		if (REG_ALARM_IGN & *dp) {
			flags |= amf;
		}
		amf >>= 1;
		if (dp-- == ap) {
			break;
		}
	}

	/* Convert to the reduced representation. */
	*tp = timeutil_timegm(&tm);
	return flags;
}

static int encode_alarm(uint8_t *ap,
			bool with_sec,
			time_t time,
			uint8_t flags)
{
	struct tm tm;
	uint8_t val;

	(void)gmtime_r(&time, &tm);

	/* For predictable behavior the low 4 bits of flags
	 * (corresponding to AxMy) must be 0b1111, 0b1110, 0b1100,
	 * 0b1000, or 0b0000.  This corresponds to the bitwise inverse
	 * being one less than a power of two.
	 */
	if (!is_power_of_two(1U + (0x0F & ~flags))) {
		LOG_DBG("invalid alarm mask in flags: %02x", flags);
		return -EINVAL;
	}

	if (with_sec) {
		if (flags & MAXIM_DS3231_ALARM_FLAGS_IGNSE) {
			val = REG_ALARM_IGN;
		} else {
			val = ((tm.tm_sec / 10) << 4) | (tm.tm_sec % 10);
		}
		*ap++ = val;
	}

	if (flags & MAXIM_DS3231_ALARM_FLAGS_IGNMN) {
		val = REG_ALARM_IGN;
	} else {
		val = ((tm.tm_min / 10) << 4) | (tm.tm_min % 10);
	}
	*ap++ = val;

	if (flags & MAXIM_DS3231_ALARM_FLAGS_IGNHR) {
		val = REG_ALARM_IGN;
	} else {
		val = ((tm.tm_hour / 10) << 4) | (tm.tm_hour % 10);
	}
	*ap++ = val;

	if (flags & MAXIM_DS3231_ALARM_FLAGS_IGNDA) {
		val = REG_ALARM_IGN;
	} else if (flags & MAXIM_DS3231_ALARM_FLAGS_DOW) {
		val = REG_DAYDATE_DOW | (tm.tm_wday + 1);
	} else {
		val = ((tm.tm_mday / 10) << 4) | (tm.tm_mday % 10);
	}
	*ap++ = val;

	return 0;
}

static uint32_t decode_rtc(struct ds3231_data *data)
{
	struct tm tm = { 0 };
	const struct register_map *rp = &data->registers;

	decode_time(&tm, &rp->sec, true);
	tm.tm_wday = (rp->dow & 0x07) - 1;
	tm.tm_mday = 10 * ((rp->dom >> 4) & 0x03) + (rp->dom & 0x0F);
	tm.tm_mon = 10 * (((0xF0 & ~REG_MONCEN_CENTURY) & rp->moncen) >> 4)
		    + (rp->moncen & 0x0F) - 1;
	tm.tm_year = (10 * (rp->year >> 4)) + (rp->year & 0x0F);
	if (REG_MONCEN_CENTURY & rp->moncen) {
		tm.tm_year += 100;
	}

	data->rtc_registers = timeutil_timegm(&tm);
	return data->rtc_registers;
}

static int update_registers(const struct device *dev)
{
	struct ds3231_data *data = dev->data;
	const struct ds3231_config *cfg = dev->config;
	uint32_t syncclock;
	int rc;
	uint8_t addr = 0;

	data->syncclock_base = maxim_ds3231_read_syncclock(dev);
	rc = i2c_write_read(data->i2c, cfg->addr,
			    &addr, sizeof(addr),
			    &data->registers, sizeof(data->registers));
	syncclock = maxim_ds3231_read_syncclock(dev);
	if (rc < 0) {
		return rc;
	}
	data->rtc_base = decode_rtc(data);

	return 0;
}

int maxim_ds3231_get_alarm(const struct device *dev,
			   uint8_t id,
			   struct maxim_ds3231_alarm *cp)
{
	struct ds3231_data *data = dev->data;
	const struct ds3231_config *cfg = dev->config;
	int rv = 0;
	uint8_t addr;
	uint8_t len;

	if (id == 0) {
		addr = offsetof(struct register_map, alarm1);
		len = sizeof(data->registers.alarm1);
	} else if (id < cfg->generic.channels) {
		addr = offsetof(struct register_map, alarm2);
		len = sizeof(data->registers.alarm2);
	} else {
		rv = -EINVAL;
		goto out;
	}

	k_sem_take(&data->lock, K_FOREVER);

	/* Update alarm structure */
	uint8_t *rbp = &data->registers.sec + addr;

	rv = i2c_write_read(data->i2c, cfg->addr,
			    &addr, sizeof(addr),
			    rbp, len);

	if (rv < 0) {
		LOG_DBG("get_config at %02x failed: %d\n", addr, rv);
		goto out_locked;
	}

	*cp = (struct maxim_ds3231_alarm){ 0 };
	cp->flags = decode_alarm(rbp, (id == 0), &cp->time);
	cp->handler = data->alarm_handler[id];
	cp->user_data = data->alarm_user_data[id];

out_locked:
	k_sem_give(&data->lock);

out:
	return rv;
}

static int cancel_alarm(const struct device *dev,
			uint8_t id)
{
	struct ds3231_data *data = dev->data;

	data->alarm_handler[id] = NULL;
	data->alarm_user_data[id] = NULL;

	return sc_ctrl(dev, 0, MAXIM_DS3231_ALARM1 << id);
}

static int ds3231_counter_cancel_alarm(const struct device *dev,
				       uint8_t id)
{
	struct ds3231_data *data = dev->data;
	const struct ds3231_config *cfg = dev->config;
	int rv = 0;

	if (id >= cfg->generic.channels) {
		rv = -EINVAL;
		goto out;
	}

	k_sem_take(&data->lock, K_FOREVER);

	rv = cancel_alarm(dev, id);

	k_sem_give(&data->lock);

out:
	/* Throw away information counter API disallows */
	if (rv >= 0) {
		rv = 0;
	}

	return rv;
}

static int set_alarm(const struct device *dev,
		     uint8_t id,
		     const struct maxim_ds3231_alarm *cp)
{
	struct ds3231_data *data = dev->data;
	const struct ds3231_config *cfg = dev->config;
	uint8_t addr;
	uint8_t len;

	if (id == 0) {
		addr = offsetof(struct register_map, alarm1);
		len = sizeof(data->registers.alarm1);
	} else if (id < cfg->generic.channels) {
		addr = offsetof(struct register_map, alarm2);
		len = sizeof(data->registers.alarm2);
	} else {
		return -EINVAL;
	}

	uint8_t buf[5] = { addr };
	int rc = encode_alarm(buf + 1, (id == 0), cp->time, cp->flags);

	if (rc < 0) {
		return rc;
	}

	/* @todo resolve race condition: a previously stored alarm may
	 * trigger between clear of AxF and the write of the new alarm
	 * control.
	 */
	rc = rsc_stat(dev, 0U, (MAXIM_DS3231_ALARM1 << id));
	if (rc >= 0) {
		rc = i2c_write(data->i2c, buf, len + 1, cfg->addr);
	}
	if ((rc >= 0)
	    && (cp->handler != NULL)) {
		rc = sc_ctrl(dev, MAXIM_DS3231_ALARM1 << id, 0);
	}
	if (rc >= 0) {
		memmove(&data->registers.sec + addr, buf + 1, len);
		data->alarm_handler[id] = cp->handler;
		data->alarm_user_data[id] = cp->user_data;
		data->alarm_flags[id] = cp->flags;
		validate_isw_monitoring(dev);
	}

	return rc;
}

int maxim_ds3231_set_alarm(const struct device *dev,
			   uint8_t id,
			   const struct maxim_ds3231_alarm *cp)
{
	struct ds3231_data *data = dev->data;

	k_sem_take(&data->lock, K_FOREVER);

	int rc = set_alarm(dev, id, cp);

	k_sem_give(&data->lock);

	return rc;
}

int maxim_ds3231_check_alarms(const struct device *dev)
{
	struct ds3231_data *data = dev->data;
	const struct register_map *rp = &data->registers;
	uint8_t mask = (MAXIM_DS3231_ALARM1 | MAXIM_DS3231_ALARM2);

	k_sem_take(&data->lock, K_FOREVER);

	/* Fetch and clear only the alarm flags that are not
	 * interrupt-enabled.
	 */
	int rv = rsc_stat(dev, 0U, (rp->ctrl & mask) ^ mask);

	if (rv >= 0) {
		rv &= mask;
	}

	k_sem_give(&data->lock);

	return rv;
}

static int check_handled_alarms(const struct device *dev)
{
	struct ds3231_data *data = dev->data;
	const struct register_map *rp = &data->registers;
	uint8_t mask = (MAXIM_DS3231_ALARM1 | MAXIM_DS3231_ALARM2);

	/* Fetch and clear only the alarm flags that are
	 * interrupt-enabled.  Leave any flags that are not enabled;
	 * it may be an alarm that triggered a wakeup.
	 */
	mask &= rp->ctrl;

	int rv = rsc_stat(dev, 0U, mask);

	if (rv > 0) {
		rv &= mask;
	}

	return rv;
}

static void counter_alarm_forwarder(const struct device *dev,
				    uint8_t id,
				    uint32_t syncclock,
				    void *ud)
{
	/* Dummy handler marking a counter callback. */
}

static void alarm_worker(struct k_work *work)
{
	struct ds3231_data *data = CONTAINER_OF(work, struct ds3231_data,
						alarm_work);
	const struct device *ds3231 = data->ds3231;
	const struct ds3231_config *cfg = ds3231->config;

	k_sem_take(&data->lock, K_FOREVER);

	int af = check_handled_alarms(ds3231);

	while (af > 0) {
		uint8_t id;

		for (id = 0; id < cfg->generic.channels; ++id) {
			if ((af & (MAXIM_DS3231_ALARM1 << id)) == 0) {
				continue;
			}


			maxim_ds3231_alarm_callback_handler_t handler
				= data->alarm_handler[id];
			void *ud = data->alarm_user_data[id];

			if (data->alarm_flags[id] & MAXIM_DS3231_ALARM_FLAGS_AUTODISABLE) {
				int rc = cancel_alarm(ds3231, id);

				LOG_DBG("autodisable %d: %d", id, rc);
				validate_isw_monitoring(ds3231);
			}

			if (handler == counter_alarm_forwarder) {
				counter_alarm_callback_t cb = data->counter_handler[id];
				uint32_t ticks = data->counter_ticks[id];

				data->counter_handler[id] = NULL;
				data->counter_ticks[id] = 0;

				if (cb) {
					k_sem_give(&data->lock);

					cb(ds3231, id, ticks, ud);

					k_sem_take(&data->lock, K_FOREVER);
				}

			} else if (handler != NULL) {
				k_sem_give(&data->lock);

				handler(ds3231, id, data->isw_syncclock, ud);

				k_sem_take(&data->lock, K_FOREVER);
			}
		}
		af = check_handled_alarms(ds3231);
	}

	k_sem_give(&data->lock);

	if (af < 0) {
		LOG_ERR("failed to read alarm flags");
		return;
	}

	LOG_DBG("ALARM %02x at %u latency %u", af, data->isw_syncclock,
		maxim_ds3231_read_syncclock(ds3231) - data->isw_syncclock);
}

static void sqw_worker(struct k_work *work)
{
	struct ds3231_data *data = CONTAINER_OF(work, struct ds3231_data, sqw_work);
	uint32_t syncclock = maxim_ds3231_read_syncclock(data->ds3231);

	/* This is a placeholder for potential application-controlled
	 * use of the square wave functionality.
	 */
	LOG_DBG("SQW %u latency %u", data->isw_syncclock,
		syncclock - data->isw_syncclock);
}

static int read_time(const struct device *dev,
		     time_t *time)
{
	struct ds3231_data *data = dev->data;
	const struct ds3231_config *cfg = dev->config;
	uint8_t addr = 0;

	int rc = i2c_write_read(data->i2c, cfg->addr,
				&addr, sizeof(addr),
				&data->registers, 7);

	if (rc >= 0) {
		*time = decode_rtc(data);
	}

	return rc;
}

static int ds3231_counter_get_value(const struct device *dev,
				    uint32_t *ticks)
{
	struct ds3231_data *data = dev->data;
	time_t time = 0;

	k_sem_take(&data->lock, K_FOREVER);

	int rc = read_time(dev, &time);

	k_sem_give(&data->lock);

	if (rc >= 0) {
		*ticks = time;
	}

	return rc;
}

static void sync_finish(const struct device *dev,
			int rc)
{
	struct ds3231_data *data = dev->data;
	struct sys_notify *notify = NULL;
	struct k_poll_signal *signal = NULL;

	if (data->sync_signal) {
		signal = data->sync.signal;
	} else {
		notify = data->sync.notify;
	}
	data->sync.ptr = NULL;
	data->sync_signal = false;
	data->sync_state = SYNCSM_IDLE;
	(void)validate_isw_monitoring(dev);

	LOG_DBG("sync complete, notify %d to %p or %p\n", rc, notify, signal);
	k_sem_give(&data->lock);

	if (notify != NULL) {
		maxim_ds3231_notify_callback cb =
			(maxim_ds3231_notify_callback)sys_notify_finalize(notify, rc);

		if (cb) {
			cb(dev, notify, rc);
		}
	} else if (signal != NULL) {
		k_poll_signal_raise(signal, rc);
	}
}

static void sync_prep_read(const struct device *dev)
{
	struct ds3231_data *data = dev->data;
	int rc = sc_ctrl(dev, 0U, MAXIM_DS3231_REG_CTRL_INTCN
			 | MAXIM_DS3231_REG_CTRL_RS_Msk);

	if (rc < 0) {
		sync_finish(dev, rc);
		return;
	}
	data->sync_state = SYNCSM_FINISH_READ;
	validate_isw_monitoring(dev);
}

static void sync_finish_read(const struct device *dev)
{
	struct ds3231_data *data = dev->data;
	time_t time = 0;

	(void)read_time(dev, &time);
	data->syncpoint.rtc.tv_sec = time;
	data->syncpoint.rtc.tv_nsec = 0;
	data->syncpoint.syncclock = data->isw_syncclock;
	sync_finish(dev, 0);
}

static void sync_timer_handler(struct k_timer *tmr)
{
	struct ds3231_data *data = CONTAINER_OF(tmr, struct ds3231_data,
						sync_timer);

	LOG_INF("sync_timer fired");
	k_work_submit(&data->sync_work);
}

static void sync_prep_write(const struct device *dev)
{
	struct ds3231_data *data = dev->data;
	uint32_t syncclock = maxim_ds3231_read_syncclock(dev);
	uint32_t offset = syncclock - data->new_sp.syncclock;
	uint32_t syncclock_Hz = maxim_ds3231_syncclock_frequency(dev);
	uint32_t offset_s = offset / syncclock_Hz;
	uint32_t offset_ms = (offset % syncclock_Hz) * 1000U / syncclock_Hz;
	time_t when = data->new_sp.rtc.tv_sec;

	when += offset_s;
	offset_ms += data->new_sp.rtc.tv_nsec / NSEC_PER_USEC / USEC_PER_MSEC;
	if (offset_ms >= MSEC_PER_SEC) {
		offset_ms -= MSEC_PER_SEC;
	} else {
		when += 1;
	}

	uint32_t rem_ms = MSEC_PER_SEC - offset_ms;

	if (rem_ms < 5) {
		when += 1;
		rem_ms += MSEC_PER_SEC;
	}
	data->new_sp.rtc.tv_sec = when;
	data->new_sp.rtc.tv_nsec = 0;

	data->sync_state = SYNCSM_FINISH_WRITE;
	k_timer_start(&data->sync_timer, K_MSEC(rem_ms), K_NO_WAIT);
	LOG_INF("sync %u in %u ms after %u", (uint32_t)when, rem_ms, syncclock);
}

static void sync_finish_write(const struct device *dev)
{
	struct ds3231_data *data = dev->data;
	const struct ds3231_config *cfg = dev->config;
	time_t when = data->new_sp.rtc.tv_sec;
	struct tm tm;
	uint8_t buf[8];
	uint8_t *bp = buf;
	uint8_t val;

	*bp++ = offsetof(struct register_map, sec);

	(void)gmtime_r(&when, &tm);
	val = ((tm.tm_sec / 10) << 4) | (tm.tm_sec % 10);
	*bp++ = val;

	val = ((tm.tm_min / 10) << 4) | (tm.tm_min % 10);
	*bp++ = val;

	val = ((tm.tm_hour / 10) << 4) | (tm.tm_hour % 10);
	*bp++ = val;

	*bp++ = 1 + tm.tm_wday;

	val = ((tm.tm_mday / 10) << 4) | (tm.tm_mday % 10);
	*bp++ = val;

	tm.tm_mon += 1;
	val = ((tm.tm_mon / 10) << 4) | (tm.tm_mon % 10);
	if (tm.tm_year >= 100) {
		tm.tm_year -= 100;
		val |= REG_MONCEN_CENTURY;
	}
	*bp++ = val;

	val = ((tm.tm_year / 10) << 4) | (tm.tm_year % 10);
	*bp++ = val;

	uint32_t syncclock = maxim_ds3231_read_syncclock(dev);
	int rc = i2c_write(data->i2c, buf, bp - buf, cfg->addr);

	if (rc >= 0) {
		data->syncpoint.rtc.tv_sec = when;
		data->syncpoint.rtc.tv_nsec = 0;
		data->syncpoint.syncclock = syncclock;
		LOG_INF("sync %u at %u", (uint32_t)when, syncclock);
	}
	sync_finish(dev, rc);
}

static void sync_worker(struct k_work *work)
{
	struct ds3231_data *data = CONTAINER_OF(work, struct ds3231_data, sync_work);
	uint32_t syncclock = maxim_ds3231_read_syncclock(data->ds3231);
	bool unlock = true;

	k_sem_take(&data->lock, K_FOREVER);

	LOG_DBG("SYNC.%u %u latency %u", data->sync_state, data->isw_syncclock,
		syncclock - data->isw_syncclock);
	switch (data->sync_state) {
	default:
	case SYNCSM_IDLE:
		break;
	case SYNCSM_PREP_READ:
		sync_prep_read(data->ds3231);
		break;
	case SYNCSM_FINISH_READ:
		sync_finish_read(data->ds3231);
		break;
	case SYNCSM_PREP_WRITE:
		sync_prep_write(data->ds3231);
		break;
	case SYNCSM_FINISH_WRITE:
		sync_finish_write(data->ds3231);
		unlock = false;
		break;
	}

	if (unlock) {
		k_sem_give(&data->lock);
	}
}

static void isw_gpio_callback(const struct device *port,
			      struct gpio_callback *cb,
			      uint32_t pins)
{
	struct ds3231_data *data = CONTAINER_OF(cb, struct ds3231_data,
						isw_callback);

	data->isw_syncclock = maxim_ds3231_read_syncclock(data->ds3231);
	if (data->registers.ctrl & MAXIM_DS3231_REG_CTRL_INTCN) {
		k_work_submit(&data->alarm_work);
	} else if (data->sync_state != SYNCSM_IDLE) {
		k_work_submit(&data->sync_work);
	} else {
		k_work_submit(&data->sqw_work);
	}
}

int z_impl_maxim_ds3231_get_syncpoint(const struct device *dev,
				      struct maxim_ds3231_syncpoint *syncpoint)
{
	struct ds3231_data *data = dev->data;
	int rv = 0;

	k_sem_take(&data->lock, K_FOREVER);

	if (data->syncpoint.rtc.tv_sec == 0) {
		rv = -ENOENT;
	} else {
		__ASSERT_NO_MSG(syncpoint != NULL);
		*syncpoint = data->syncpoint;
	}

	k_sem_give(&data->lock);

	return rv;
}

int maxim_ds3231_synchronize(const struct device *dev,
			     struct sys_notify *notify)
{
	struct ds3231_data *data = dev->data;
	int rv = 0;

	if (notify == NULL) {
		rv = -EINVAL;
		goto out;
	}

	if (data->isw == NULL) {
		rv = -ENOTSUP;
		goto out;
	}

	k_sem_take(&data->lock, K_FOREVER);

	if (data->sync_state != SYNCSM_IDLE) {
		rv = -EBUSY;
		goto out_locked;
	}

	data->sync_signal = false;
	data->sync.notify = notify;
	data->sync_state = SYNCSM_PREP_READ;

out_locked:
	k_sem_give(&data->lock);

	if (rv >= 0) {
		k_work_submit(&data->sync_work);
	}

out:
	return rv;
}

int z_impl_maxim_ds3231_req_syncpoint(const struct device *dev,
				      struct k_poll_signal *sig)
{
	struct ds3231_data *data = dev->data;
	int rv = 0;

	if (data->isw == NULL) {
		rv = -ENOTSUP;
		goto out;
	}

	k_sem_take(&data->lock, K_FOREVER);

	if (data->sync_state != SYNCSM_IDLE) {
		rv = -EBUSY;
		goto out_locked;
	}

	data->sync_signal = true;
	data->sync.signal = sig;
	data->sync_state = SYNCSM_PREP_READ;

out_locked:
	k_sem_give(&data->lock);

	if (rv >= 0) {
		k_work_submit(&data->sync_work);
	}

out:
	return rv;
}

int maxim_ds3231_set(const struct device *dev,
		     const struct maxim_ds3231_syncpoint *syncpoint,
		     struct sys_notify *notify)
{
	struct ds3231_data *data = dev->data;
	int rv = 0;

	if ((syncpoint == NULL)
	    || (notify == NULL)) {
		rv = -EINVAL;
		goto out;
	}
	if (data->isw == NULL) {
		rv = -ENOTSUP;
		goto out;
	}

	k_sem_take(&data->lock, K_FOREVER);

	if (data->sync_state != SYNCSM_IDLE) {
		rv = -EBUSY;
		goto out_locked;
	}

	data->new_sp = *syncpoint;
	data->sync_signal = false;
	data->sync.notify = notify;
	data->sync_state = SYNCSM_PREP_WRITE;

out_locked:
	k_sem_give(&data->lock);

	if (rv >= 0) {
		k_work_submit(&data->sync_work);
	}

out:
	return rv;
}

static int ds3231_init(const struct device *dev)
{
	struct ds3231_data *data = dev->data;
	const struct ds3231_config *cfg = dev->config;
	const struct device *i2c = device_get_binding(cfg->bus_name);
	int rc;

	/* Initialize and take the lock */
	k_sem_init(&data->lock, 0, 1);

	data->ds3231 = dev;
	if (i2c == NULL) {
		LOG_WRN("Failed to get I2C %s", cfg->bus_name);
		rc = -EINVAL;
		goto out;
	}

	data->i2c = i2c;
	rc = update_registers(dev);
	if (rc < 0) {
		LOG_WRN("Failed to fetch registers: %d", rc);
		goto out;
	}

	/* INTCN and AxIE to power-up default, RS to 1 Hz */
	rc = sc_ctrl(dev,
		     MAXIM_DS3231_REG_CTRL_INTCN,
		     MAXIM_DS3231_REG_CTRL_RS_Msk
		     | MAXIM_DS3231_ALARM1 | MAXIM_DS3231_ALARM2);
	if (rc < 0) {
		LOG_WRN("Failed to reset config: %d", rc);
		goto out;
	}

	/* Do not clear pending flags in the status register.  This
	 * device may have been used for external wakeup, which can be
	 * detected using the extended API.
	 */

	if (cfg->isw_gpios.ctrl != NULL) {
		const struct device *gpio = device_get_binding(cfg->isw_gpios.ctrl);

		if (gpio == NULL) {
			LOG_WRN("Failed to get INTn/SQW GPIO %s",
				cfg->isw_gpios.ctrl);
			rc = -EINVAL;
			goto out;
		}

		k_timer_init(&data->sync_timer, sync_timer_handler, NULL);
		k_work_init(&data->alarm_work, alarm_worker);
		k_work_init(&data->sqw_work, sqw_worker);
		k_work_init(&data->sync_work, sync_worker);
		gpio_init_callback(&data->isw_callback,
				   isw_gpio_callback,
				   BIT(cfg->isw_gpios.pin));

		rc = gpio_pin_configure(gpio, cfg->isw_gpios.pin,
					GPIO_INPUT | cfg->isw_gpios.flags);
		if (rc >= 0) {
			rc = gpio_pin_interrupt_configure(gpio, cfg->isw_gpios.pin,
							  GPIO_INT_DISABLE);
		}
		if (rc >= 0) {
			rc = gpio_add_callback(gpio, &data->isw_callback);
		}
		if (rc >= 0) {
			data->isw = gpio;
		} else {
			LOG_WRN("Failed to configure ISW callback: %d", rc);
		}
	}

out:
	k_sem_give(&data->lock);

	LOG_DBG("Initialized %d", rc);
	if (rc > 0) {
		rc = 0;
	}

	return rc;
}

static int ds3231_counter_start(const struct device *dev)
{
	return -EALREADY;
}

static int ds3231_counter_stop(const struct device *dev)
{
	return -ENOTSUP;
}

int ds3231_counter_set_alarm(const struct device *dev,
			     uint8_t id,
			     const struct counter_alarm_cfg *alarm_cfg)
{
	struct ds3231_data *data = dev->data;
	const struct register_map *rp = &data->registers;
	const struct ds3231_config *cfg = dev->config;
	time_t when;
	int rc = 0;

	if (id >= cfg->generic.channels) {
		rc = -ENOTSUP;
		goto out;
	}

	k_sem_take(&data->lock, K_FOREVER);

	if (rp->ctrl & (MAXIM_DS3231_ALARM1 << id)) {
		rc = -EBUSY;
		goto out_locked;
	}

	if ((alarm_cfg->flags & COUNTER_ALARM_CFG_ABSOLUTE) == 0) {
		rc = read_time(dev, &when);
		if (rc >= 0) {
			when += alarm_cfg->ticks;
		}
	} else {
		when = alarm_cfg->ticks;
	}

	struct maxim_ds3231_alarm alarm = {
		.time = (uint32_t)when,
		.handler = counter_alarm_forwarder,
		.user_data = alarm_cfg->user_data,
		.flags = MAXIM_DS3231_ALARM_FLAGS_AUTODISABLE,
	};

	if (rc >= 0) {
		data->counter_handler[id] = alarm_cfg->callback;
		data->counter_ticks[id] = alarm.time;
		rc = set_alarm(dev, id, &alarm);
	}

out_locked:
	k_sem_give(&data->lock);

out:
	/* Throw away information counter API disallows */
	if (rc >= 0) {
		rc = 0;
	}

	return rc;
}

static uint32_t ds3231_counter_get_top_value(const struct device *dev)
{
	return UINT32_MAX;
}

static uint32_t ds3231_counter_get_pending_int(const struct device *dev)
{
	return 0;
}

static int ds3231_counter_set_top_value(const struct device *dev,
					const struct counter_top_cfg *cfg)
{
	return -ENOTSUP;
}

static const struct counter_driver_api ds3231_api = {
	.start = ds3231_counter_start,
	.stop = ds3231_counter_stop,
	.get_value = ds3231_counter_get_value,
	.set_alarm = ds3231_counter_set_alarm,
	.cancel_alarm = ds3231_counter_cancel_alarm,
	.set_top_value = ds3231_counter_set_top_value,
	.get_pending_int = ds3231_counter_get_pending_int,
	.get_top_value = ds3231_counter_get_top_value,
};

static const struct ds3231_config ds3231_0_config = {
	.generic = {
		.max_top_value = UINT32_MAX,
		.freq = 1,
		.flags = COUNTER_CONFIG_INFO_COUNT_UP,
		.channels = 2,
	},
	.bus_name = DT_INST_BUS_LABEL(0),
	/* Driver does not currently use 32k GPIO. */
#if DT_INST_NODE_HAS_PROP(0, isw_gpios)
	.isw_gpios = {
		DT_INST_GPIO_LABEL(0, isw_gpios),
		DT_INST_GPIO_PIN(0, isw_gpios),
		DT_INST_GPIO_FLAGS(0, isw_gpios),
	},
#endif
	.addr = DT_INST_REG_ADDR(0),
};

static struct ds3231_data ds3231_0_data;

#if CONFIG_COUNTER_MAXIM_DS3231_INIT_PRIORITY <= CONFIG_I2C_INIT_PRIORITY
#error COUNTER_MAXIM_DS3231_INIT_PRIORITY must be greater than I2C_INIT_PRIORITY
#endif

DEVICE_DT_INST_DEFINE(0, ds3231_init, NULL, &ds3231_0_data,
		    &ds3231_0_config,
		    POST_KERNEL, CONFIG_COUNTER_MAXIM_DS3231_INIT_PRIORITY,
		    &ds3231_api);

#ifdef CONFIG_USERSPACE

#include <syscall_handler.h>

int z_vrfy_maxim_ds3231_get_syncpoint(const struct device *dev,
				      struct maxim_ds3231_syncpoint *syncpoint)
{
	struct maxim_ds3231_syncpoint value;
	int rv;

	Z_OOPS(Z_SYSCALL_SPECIFIC_DRIVER(dev, K_OBJ_DRIVER_COUNTER, &ds3231_api));
	Z_OOPS(Z_SYSCALL_MEMORY_WRITE(syncpoint, sizeof(*syncpoint)));

	rv = z_impl_maxim_ds3231_get_syncpoint(dev, &value);

	if (rv >= 0) {
		Z_OOPS(z_user_to_copy(syncpoint, &value, sizeof(*syncpoint)));
	}

	return rv;
}

#include <syscalls/maxim_ds3231_get_syncpoint_mrsh.c>

int z_vrfy_maxim_ds3231_req_syncpoint(const struct device *dev,
				      struct k_poll_signal *sig)
{
	Z_OOPS(Z_SYSCALL_SPECIFIC_DRIVER(dev, K_OBJ_DRIVER_COUNTER, &ds3231_api));
	if (sig != NULL) {
		Z_OOPS(Z_SYSCALL_OBJ(sig, K_OBJ_POLL_SIGNAL));
	}

	return z_impl_maxim_ds3231_req_syncpoint(dev, sig);
}

#include <syscalls/maxim_ds3231_req_syncpoint_mrsh.c>

#endif /* CONFIG_USERSPACE */
