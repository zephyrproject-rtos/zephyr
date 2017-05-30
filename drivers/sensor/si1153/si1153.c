/* sensor_si1153.c - Driver for Silicon Labs Proximity/Ambient Light Sensor */

/*
 * Copyright (c) 2017 IpTronix S.r.l.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <kernel.h>
#include <string.h>
#include <sensor.h>
#include <init.h>
#include <gpio.h>
#include <misc/byteorder.h>
#include <misc/__assert.h>
#include <i2c.h>

#include "si1153.h"

#define SI1153_DEBUG

struct si115x_samples {
	u32_t timestamp;         /* Timestamp to record */
	u8_t  irq_status;
	s32_t ch0;
	s32_t ch1;
	s32_t ch2;
	s32_t ch3;
};

static struct si1153_data si1153_data;

static int si1153_reg_read(struct si1153_data *data, u8_t reg)
{
	u8_t buf[1];
	int ret;

	ret = i2c_burst_read(data->i2c_master, data->i2c_slave_addr,
						reg, buf, 1);
	if (ret) {
		return ret;
	}

	return buf[0];
}

static int si1153_reg_write(struct si1153_data *data, u8_t reg, u8_t val)
{
	return i2c_reg_write_byte(data->i2c_master, data->i2c_slave_addr,
			reg, val);
}

static int si1153_block_read(struct si1153_data *data, u8_t start,
		int size, uint8_t *buf)
{
	return i2c_burst_read(data->i2c_master, data->i2c_slave_addr,
			start, buf, size);
}

static int si1153_block_write(struct si1153_data *data, u8_t start,
		int size, uint8_t *buf)
{
	return i2c_burst_write(data->i2c_master, data->i2c_slave_addr,
			start, buf, size);
}

static int _wait_until_sleep(struct si1153_data *data)
{
	int retval;
	u8_t count = 0;

	/* This loops until the Si115x is known to be in its sleep state
	 * or if an i2c error occurs
	 */
	while (count < 5) {
		retval = si1153_reg_read(data, SI115x_REG_RESPONSE0);
		if (retval <  0) {
			return retval;
		}
		if ((retval & RSP0_CHIPSTAT_MASK) == RSP0_SLEEP) {
			break;
		}
		count++;
	}

	return 0;
}

/**
 * @brief
 *   Resets the Si115x/6x, clears any interrupts and initializes the HW_KEY
 *   register.
 * @param[in] si115x_handle
 *   The programmer's toolkit handle
 * @retval  0
 *   Success
 * @retval  <0
 *   Error
 */
static int si115x_reset(struct si1153_data *data)
{
	int retval = 0;

	/*
	 * Do not access the Si115x earlier than 25 ms from power-up.
	 */
	k_sleep(K_MSEC(25));

	/* Perform the Reset Command */
	retval = si1153_reg_write(data, SI115x_REG_COMMAND, 1);

	/* Delay for 10 ms. This delay is needed to allow the Si115x
	 * to perform internal reset sequence.
	 */
	k_sleep(K_MSEC(10));

	return retval;
}

static int _send_cmd(struct si1153_data *data, u8_t command)
{
	int response;
	int retval;
	u8_t count = 0;

	/* Get the response register contents */
	response = si1153_reg_read(data, SI115x_REG_RESPONSE0);
	if (response < 0) {
		return response;
	}

	response = response & RSP0_COUNTER_MASK;

	/* Double-check the response register is consistent */
	while (count < 5) {
		retval = _wait_until_sleep(data);
		if (retval != 0) {
			return retval;
		}
		if (command == 0) {
			break; /* Skip if the command is NOP */
		}

		retval = si1153_reg_read(data, SI115x_REG_RESPONSE0);
		if (retval < 0) {
			return retval;
		}
		else if ((retval&RSP0_COUNTER_MASK) == response) {
			break;
		}

		response = retval & RSP0_COUNTER_MASK;

		count++;
	}

	/* Send the Command */
	retval = si1153_reg_write(data, SI115x_REG_COMMAND, command);
	if (retval != 0) {
		return retval;
	}

	count = 0;
	/* Expect a change in the response register */
	while (count < 5) {

		if (command == 0) {
			break; /* Skip if the command is NOP */
		}

		retval = si1153_reg_read(data, SI115x_REG_RESPONSE0);
		if (retval < 0) {
			return retval;
		}
		else if ((retval & RSP0_COUNTER_MASK) != response) {
			break;
		}

		count++;
	}

	return 0;
}

static int si115x_force(struct si1153_data *data)
{
	/* Sends a FORCE command to the Si115x/6x */
	return _send_cmd(data, CMD_FORCE_CH);
}

static int si115x_start(struct si1153_data *data)
{
	/* Sends a PSALSAUTO command to the Si113x/4x */
	return _send_cmd(data, CMD_AUTO_CH);
}


static int si115x_param_set(struct si1153_data *data, u8_t address, u8_t value)
{
	int retval;
	u8_t buffer[2];
	s16_t response_stored;

	/* Writes a byte to an Si115x/6x Parameter
	 * @note This function ensures that the Si115x/6x is idle and ready to
	 * receive a command before writing the parameter. Furthermore,
	 * command completion is checked. If setting parameter is not done
	 * properly, no measurements will occur. This is the most common
	 * error. It is highly recommended that host code make use of this
	 * function.
	 */

	retval = _wait_until_sleep(data);
	if (retval != 0) {
		return retval;
	}

	retval = si1153_reg_read(data, SI115x_REG_RESPONSE0);
	if (retval < 0) {
		return retval;
	}

	response_stored = RSP0_COUNTER_MASK & retval;

	buffer[0] = value;
	buffer[1] = 0x80 + (address & 0x3F);

	retval = si1153_block_write(data, SI115x_REG_HOSTIN0, 2, buffer);
	if (retval != 0) {
		return retval;
	}

	/* Wait for command to finish */
	retval = si1153_reg_read(data, SI115x_REG_RESPONSE0);
	if (retval < 0) {
		return retval;
	}

	while ((retval & RSP0_COUNTER_MASK) == response_stored) {
		retval = si1153_reg_read(data, SI115x_REG_RESPONSE0);
		if (retval < 0) {
			return retval;
		}
	}

	return 0;
}

/**
 * @brief
 *  Implements the algorithm for detecting gestures on the sensor STK.
 *  Should be called with new sample data every time an interrupt is
 *  received.
 * @param[in] samples
 *   New sample data received from the sensor.
 * @return
 *   Returns the type of gesture detected (as defined by gesture_t).
 */

typedef enum
{
  NONE,
  UP,
  DOWN,
  LEFT,
  RIGHT,
  PROX
} gesture_t;

#define PS_THRESHOLD 4000
#define GESTURE_TIME_DIFF 10

#define CALCULATE_MID_POINT(a,b) ((a)+(((b)-(a))/2))
#define CALCULATE_ABS_DIFF(a,b) ((a)>(b)?((a)-(b)):((b)-(a)))

static int si115x_gesture_algorithm(struct si115x_samples *samples)
{
	static u32_t ps_entry_time[3] = { 0, 0, 0 };
	static u32_t ps_exit_time[3]  = { 0, 0, 0 };
	static u8_t  ps_state[3] = { 0, 0, 0 };
	u32_t        ps[3];
	u8_t         i;
	u32_t        diff_x ;
	u32_t        diff_y1 ;
	u32_t        diff_y2 ;
	u32_t        ps_time[3] ;
	gesture_t    ret = NONE;  /*gesture result return value */

	/*save new samples into ps array */
	ps[0] = samples->ch2;   /* BOTTOM LED */
	ps[1] = samples->ch1;	/* LEFT LED   */
	ps[2] = samples->ch3;   /* RIGHT LED  */

	/* Check state of all three measurements */
	for (i = 0; i < 3; i++) {
		/**
		 *  If measurement higher than the ps_threshold value,
		 *  record the time of entry and change the state to
		 *  look for the exit time
		 */
		if (ps[i] >= PS_THRESHOLD) {
			ret = PROX;
			if (ps_state[i] == 0) {
				ps_state[i]      = 1;
				ps_entry_time[i] = samples->timestamp;
			}
		} else {
			if (ps_state[i] == 1) {
				ps_state[i]     = 0;
				ps_exit_time[i] = samples->timestamp;
			}
		}
	}

	/**
	 * If there is no object in front of the board,
	 * look at history to see if a gesture occured
	 */
	if ((ps[0] < PS_THRESHOLD) &&
		(ps[1] < PS_THRESHOLD) &&
		(ps[2] < PS_THRESHOLD)) {
		/**
		 * If the ps_max values are high enough and there exit entry
		 * and exit times, then begin processing gestures
		 */
		if ((ps_entry_time[0] != 0) && (ps_entry_time[1] != 0) &&
			(ps_entry_time[2] != 0) && (ps_exit_time[0] != 0) &&
			(ps_exit_time[1] != 0) && (ps_exit_time[2] != 0)) {
			/**
			 * Make sure no timestamps overflowed, indicated possibility
			 * if any of them are close to overflowing
			 */
			if ((ps_exit_time[0] > 0xFC000000L) ||
				(ps_exit_time[1] > 0xFC000000L) ||
				(ps_exit_time[2] > 0xFC000000L) ||
				(ps_entry_time[0] > 0xFC000000L) ||
				(ps_entry_time[1] > 0xFC000000L) ||
				(ps_entry_time[2] > 0xFC000000L)) {
				/**
				 *  If any of them are close to overflowing,
				 *  overflow them all so they all have the same reference
				 */
				ps_exit_time[0] += 0x1FFFFFFFL;
				ps_exit_time[1] += 0x1FFFFFFFL;
				ps_exit_time[2] += 0x1FFFFFFFL;

				ps_entry_time[0] += 0x1FFFFFFFL;
				ps_entry_time[1] += 0x1FFFFFFFL;
				ps_entry_time[2] += 0x1FFFFFFFL;
			}

			/**
			 * Calculate the midpoint (between entry and exit times)
			 * of each waveform the order of these midpoints helps
			 * determine the gesture
			 */
			ps_time[0] = CALCULATE_MID_POINT(ps_entry_time[0], ps_exit_time[0]);
			ps_time[1] = CALCULATE_MID_POINT(ps_entry_time[1], ps_exit_time[1]);
			ps_time[2] = CALCULATE_MID_POINT(ps_entry_time[2], ps_exit_time[2]);

			/**
			 * The diff_x and diff_y values help determine a gesture by
			 * comparing the LED measurements that are on a single axis
			 */
			diff_x  = CALCULATE_ABS_DIFF(ps_time[1], ps_time[2]);
			diff_y1 = CALCULATE_ABS_DIFF(ps_time[0], ps_time[1]);
			diff_y2 = CALCULATE_ABS_DIFF(ps_time[0], ps_time[2]);

			/**
			 * Take the average of all three midpoints to make
			 * a comparison point for each midpoint
			 */
			if ((ps_exit_time[0] - ps_entry_time[0]) > GESTURE_TIME_DIFF ||
				(ps_exit_time[1] - ps_entry_time[1]) > GESTURE_TIME_DIFF ||
				(ps_exit_time[2] - ps_entry_time[2]) > GESTURE_TIME_DIFF) {
				if( ( (ps_time[0] < ps_time[1]) &&  (diff_y1 > diff_x) ) ||
					( (ps_time[0] <= ps_time[2]) && (diff_y2 > diff_x) ) ){
					/**
					 * An up gesture occured if the bottom LED had its midpoint first
					 */
					ret = UP;
				}
				else if ( ( (ps_time[0] < ps_time[1]) && (diff_y1 > diff_x) ) ||
						  ( (ps_time[0] > ps_time[2]) && (diff_y2 > diff_x) ) ) {
					/**
					 * A down gesture occured if the bottom LED had its midpoint last
					 */
					ret = DOWN;
				}
				else if((ps_time[0] < ps_time[1]) &&
						(ps_time[2] < ps_time[1]) &&
						(diff_x > ((diff_y1+diff_y2)/2))) {
					/**
					 * A left gesture occured if the left LED had its midpoint last
					 */
					ret = LEFT;
				}
				else if( (ps_time[0] < ps_time[2]) &&
						(ps_time[1] < ps_time[2]) &&
						(diff_x > ((diff_y1+diff_y2)/2))) {
					/**
					 * A right gesture occured if the right LED had midpoint later than the right LED
					 */
					ret = RIGHT;
				}
			}
		}

		for (i = 0; i < 3; i++) {
			ps_exit_time[i]  = 0;
			ps_entry_time[i] = 0;
		}

	}

	return ret;
}

/**
 * Configuration for left, right and bottom channel.
 */
#define ADCCONFIG   (ADCCFG_DR_4096 | ADCCFG_AM_LARGE_IR)
#define ADCSENS     (ADCSENS_HSIG_NORM | ADCSENS_SW_GAIN_4MEAS | ADCSENS_HW_GAIN_48_8US)
#define ADCPOST     (ADCPOST_24BIT | ADCPOST_POSTSHIFT_0 | ADCPOST_THRESH_EN_0)
#define MEASURECFG  (MEASCFG_NO_MEAS | MEASCFG_LED_NOM | MEASCFG_BANK_SEL_A)

static int si115x_init(struct si1153_data *data)
{
	int retval;

	retval = si115x_reset(data);

	retval |= si115x_param_set(data, PARAM_CH_LIST, 0x0f);

	retval |= si115x_param_set(data, PARAM_ADCCONFIG0 , 0x78);
	retval |= si115x_param_set(data, PARAM_ADCSENS0   , 0x71);
	retval |= si115x_param_set(data, PARAM_ADCPOST0   , 0x40);
	retval |= si115x_param_set(data, PARAM_MEASCONFIG0, 0);

	/* Left channel LED1 */
	retval |= si115x_param_set(data, PARAM_ADCCONFIG1 , ADCCONFIG);
	retval |= si115x_param_set(data, PARAM_ADCSENS1   , ADCSENS);
	retval |= si115x_param_set(data, PARAM_ADCPOST1   , ADCPOST);
	retval |= si115x_param_set(data, PARAM_MEASCONFIG1, MEASURECFG | MEASCFG_LED1_ENA);

	/* Bottom channel LED2 */
	retval |= si115x_param_set(data, PARAM_ADCCONFIG2 , ADCCONFIG);
	retval |= si115x_param_set(data, PARAM_ADCSENS2   , ADCSENS);
	retval |= si115x_param_set(data, PARAM_ADCPOST2   , ADCPOST);
	retval |= si115x_param_set(data, PARAM_MEASCONFIG2, MEASURECFG | MEASCFG_LED2_ENA);

	/* Right channel LED3 */
	retval |= si115x_param_set(data, PARAM_ADCCONFIG3 , ADCCONFIG);
	retval |= si115x_param_set(data, PARAM_ADCSENS3   , ADCSENS);
	retval |= si115x_param_set(data, PARAM_ADCPOST3   , ADCPOST);
	retval |= si115x_param_set(data, PARAM_MEASCONFIG3, MEASURECFG | MEASCFG_LED3_ENA);

	retval |= si115x_param_set(data, PARAM_LED1_A, LED_CURRENT_199);
	retval |= si115x_param_set(data, PARAM_LED2_A, LED_CURRENT_199);
	retval |= si115x_param_set(data, PARAM_LED3_A, LED_CURRENT_310);

	retval |= si1153_reg_write(data, SI115x_REG_IRQ_ENABLE, 0x0f);

	return retval;
}

static void si115x_handler(struct si1153_data *data, struct si115x_samples *samples)
{
	u8_t buffer[13];
	int irq_status;
	int count;

	/* wait for interrupt signaling the registers are update */
	count = 0;
	do {
		irq_status = si1153_reg_read(data, SI115x_REG_IRQ_STATUS);
		k_busy_wait(50);
		count++;
	} while ((count < 20) && ((irq_status & 0xf) != 0xf));
	if (count >= 40) {
		printk("SI1153:ERROR interrupt timeout\n");
		return;
	}
	si1153_block_read(data, SI115x_REG_IRQ_STATUS, 13, buffer);

	samples->irq_status = irq_status;

	samples->timestamp = (u32_t)k_uptime_get();

	samples->ch0  = buffer[1] << 16;
	samples->ch0 |= buffer[2] <<  8;
	samples->ch0 |= buffer[3];
	if (samples->ch0 & 0x800000) {
		samples->ch0 |= 0xFF000000;
	}

	samples->ch1  = buffer[4] << 16;
	samples->ch1 |= buffer[5] <<  8;
	samples->ch1 |= buffer[6];
	if (samples->ch1 & 0x800000) {
		samples->ch1 |= 0xFF000000;
	}

	samples->ch2  = buffer[7] << 16;
	samples->ch2 |= buffer[8] <<  8;
	samples->ch2 |= buffer[9];
	if (samples->ch2 & 0x800000) {
		samples->ch2 |= 0xFF000000;
	}

	samples->ch3  = buffer[10] << 16;
	samples->ch3 |= buffer[11] <<  8;
	samples->ch3 |= buffer[12];
	if (samples->ch3 & 0x800000) {
		samples->ch3 |= 0xFF000000;
	}

}

static int si1153_sample_fetch(struct device *dev, enum sensor_channel chan)
{
#ifndef CONFIG_SI1153_OWN_THREAD
	struct si1153_data *data = dev->driver_data;
	struct si115x_samples samples;

	si115x_force(data);

	si115x_handler(data, &samples);

	data->gesture = si115x_gesture_algorithm(&samples);
	data->ch0 = samples.ch0;
	data->ch1 = samples.ch1;
	data->ch2 = samples.ch2;
	data->ch3 = samples.ch3;

#ifdef SI1153_DEBUG
	char*strgesture[] = {"NONE", "UP", "DOWN", "LEFT", "RIGHT", "PROX"};
	printk("Gesture %d %s %d %d %d %d\n",
			data->gesture, strgesture[data->gesture],
			data->ch0,
			data->ch1,
			data->ch2,
			data->ch3);
#endif

#endif
	return 0;
}

static int si1153_channel_get(struct device *dev,
					  enum sensor_channel chan,
					  struct sensor_value *val)
{
	struct si1153_data *data = dev->driver_data;

	switch (chan) {

	case SENSOR_CHAN_LIGHT:
	case SENSOR_CHAN_IR:
		val->val1 = data->ch0;
		val->val2 = 0;
		break;
	case SENSOR_CHAN_PROX:
		val->val1 = data->ch1;
		val->val2 = 0;
		val++;
		val->val1 = data->ch2;
		val->val2 = 0;
		val++;
		val->val1 = data->ch3;
		val->val2 = 0;
		break;
	case SENSOR_CHAN_GESTURE:
		val->val1 = data->gesture;
		val->val2 = 0;
		break;
	default:
		return -EINVAL;
	}
	return 0;
}

static const struct sensor_driver_api si1153_api_funcs = {
	.sample_fetch  = si1153_sample_fetch,
	.channel_get   = si1153_channel_get,
};

#ifdef CONFIG_SI1153_OWN_THREAD
static void si1153_thread(void *p1, void *p2, void *p3)
{
	struct si1153_data *data = (struct si1153_data *)p1;
	struct si115x_samples samples;
	int old = -1;

	while (1) {

		si115x_force(data);

		si115x_handler(data, &samples);

		data->gesture = si115x_gesture_algorithm(&samples);

		data->ch0 = samples.ch0;
		data->ch1 = samples.ch1;
		data->ch2 = samples.ch2;
		data->ch3 = samples.ch3;
#ifdef SI1153_DEBUG
		printk("Gesture %d %d %d %d\n",
				data->ch0,
				data->ch1,
				data->ch2,
				data->ch3);
#endif
		if (data->gesture != old) {
#ifdef SI1153_DEBUG
			char *strgesture[] = {"NONE", "UP", "DOWN", "LEFT", "RIGHT", "PROX"};
			printk("Gesture %d %s\n", data->gesture, strgesture[data->gesture]);
#endif
			old = data->gesture;
		}

		k_busy_wait(500);
	}
}
#endif


static int si1153_init(struct device *dev)
{
	struct si1153_data *data = dev->driver_data;

	data->i2c_master = device_get_binding(CONFIG_SI1153_I2C_DEV_NAME);
	if (!data->i2c_master) {
		SYS_LOG_DBG("i2c master not found: %s",
				CONFIG_SI1153_I2C_DEV_NAME);
		return -EINVAL;
	}

	data->i2c_slave_addr = CONFIG_SI1153_I2C_DEV_ADDRESS;
#ifdef CONFIG_SI1153_INTERRUPT
	/* the SI1153 INT pin is connected on ARD_D7 SPI1_SS_CS_B[1] */
	data->gpio_port = CONFIG_SI1153_GPIO_DEV_NAME;
	data->int_pin   = CONFIG_SI1153_GPIO_PIN_NUM;
#endif
	if (si115x_init(data)) {
		return -EINVAL;
	}

#if defined(CONFIG_SI1153_OWN_THREAD)
	k_thread_spawn(data->thread_stack, CONFIG_SI1153_THREAD_STACK_SIZE,
		    si1153_thread, (void *)(data), NULL, NULL,
			K_PRIO_COOP(CONFIG_SI1153_THREAD_PRIORITY), 0, 0);
#endif
	return 0;
}

DEVICE_AND_API_INIT(si1153, CONFIG_SI1153_DEV_NAME,
		si1153_init, &si1153_data,
		NULL, POST_KERNEL, CONFIG_SENSOR_INIT_PRIORITY,
		&si1153_api_funcs);

