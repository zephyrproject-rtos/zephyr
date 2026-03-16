/*
 * Copyright (c) 2025 Renesas Electronics Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT renesas_rza2m_riic

#include <math.h>
#include <zephyr/sys/util.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/drivers/interrupt_controller/gic.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(renesas_rza2m_riic);

#include "i2c-priv.h"

#define RZA2M_RIIC_DIV_TIME_NS (1000000000.0)

struct i2c_rza2m_riic_clk_settings {
	uint32_t cks_value;
	uint32_t brl_value;
	uint32_t brh_value;
};

struct i2c_rza2m_riic_bitrate {
	uint32_t bitrate;
	uint32_t duty;
	uint32_t divider;
	uint32_t brl;
	uint32_t brh;
	double duty_error_percent;
};

struct i2c_rza2m_riic_config {
	DEVICE_MMIO_ROM; /* Must be first */
	const struct device *clock_dev;
	clock_control_subsys_t clock_subsys;
	const struct pinctrl_dev_config *pcfg;
	uint32_t bitrate;
	double rise_time_s;
	double fall_time_s;
	double duty_cycle_percent;
	uint32_t noise_filter_stage;
};

struct i2c_rza2m_riic_data {
	DEVICE_MMIO_RAM; /* Must be first */
	struct i2c_rza2m_riic_clk_settings clk_settings;
	uint32_t clk_rate;
	uint32_t interrupt_mask;
	uint32_t status_bits;
	struct k_sem sem;
	struct k_mutex i2c_lock_mtx; /* For I2C transfer locking mechanism */
	uint32_t dev_config;
};

/* Registers */
#define RIIC_CR1  0x00 /* I²C Bus Control Register 1 */
#define RIIC_CR2  0x04 /* I²C Bus Control Register 2 */
#define RIIC_MR1  0x08 /* I²C Bus Mode Register 1 */
#define RIIC_MR2  0x0c /* I²C Bus Mode Register 2 */
#define RIIC_MR3  0x10 /* I²C Bus Mode Register 3 */
#define RIIC_FER  0x14 /* I²C Bus Function Enable Register */
#define RIIC_SER  0x18 /* I²C Bus Status Enable Register */
#define RIIC_IER  0x1c /* I²C Bus Interrupt Enable Register */
#define RIIC_SR1  0x20 /* I²C Bus Status Register 1 */
#define RIIC_SR2  0x24 /* I²C Bus Status Register 2 */
#define RIIC_SAR0 0x28 /* I²C Target Address Register 0 */
#define RIIC_SAR1 0x2c /* I²C Target Address Register 1 */
#define RIIC_SAR2 0x30 /* I²C Target Address Register 2 */
#define RIIC_BRL  0x34 /* I²C Bus Bit Rate Low-Level Register */
#define RIIC_BRH  0x38 /* I²C Bus Bit Rate High-Level Register */
#define RIIC_DRT  0x3c /* I²C Bus Transmit Data Register */
#define RIIC_DRR  0x40 /* I²C Bus Receive Data Register */

#define RIIC_CR1_ICE    BIT(7) /* Bus Interface Enable */
#define RIIC_CR1_IICRST BIT(6) /* Bus Interface Internal Reset */
#define RIIC_CR1_CLO    BIT(5) /* Extra SCL Clock Cycle Output */
#define RIIC_CR1_SOWP   BIT(4) /* SCLO/SDAO Write Protect */
#define RIIC_CR1_SCLO   BIT(3) /* SCL Output Control */
#define RIIC_CR1_SDAO   BIT(2) /* SDA Output Control */
#define RIIC_CR1_SCLI   BIT(1) /* SCL Bus Input Monitor */
#define RIIC_CR1_SDAI   BIT(0) /* SDA Bus Input Monitor */

#define RIIC_CR2_BBSY BIT(7) /* Bus Busy Detection Flag */
#define RIIC_CR2_MST  BIT(6) /* Controller/Target Mode */
#define RIIC_CR2_TRS  BIT(5) /* Transmit/Receive Mode */
#define RIIC_CR2_SP   BIT(3) /* Stop Condition Issuance Request */
#define RIIC_CR2_RS   BIT(2) /* Restart Condition Issuance Request */
#define RIIC_CR2_ST   BIT(1) /* Start Condition Issuance Request */

#define RIIC_MR1_BCWP     BIT(3) /* BC Write Protect */
#define RIIC_MR1_CKS_MASK 0x70
#define RIIC_MR1_CKS(x)   ((((x) << 4) & RIIC_MR1_CKS_MASK) | RIIC_MR1_BCWP)

#define RIIC_MR2_DLCS BIT(7) /* SDA Output Delay Clock Source Selection */
#define RIIC_MR2_TMOH BIT(2) /* Timeout H Count Control */
#define RIIC_MR2_TMOL BIT(1) /* Timeout L Count Control */
#define RIIC_MR2_TMOS BIT(0) /* Timeout Detection Time Selection */

#define RIIC_MR3_DMBE  BIT(7) /* SMBus/I2C Bus Selection */
#define RIIC_MR3_WAIT  BIT(6) /* WAIT */
#define RIIC_MR3_RDRFS BIT(5) /* RDRF Flag Set Timing Selection */
#define RIIC_MR3_ACKWP BIT(4) /* ACKBT Write Protect */
#define RIIC_MR3_ACKBT BIT(3) /* Transmit Acknowledge */
#define RIIC_MR3_ACKBR BIT(2) /* Receive Acknowledge */

#define RIIC_FER_FMPE  BIT(7) /* Fast-mode Plus Enable */
#define RIIC_FER_SCLE  BIT(6) /* SCL Synchronous Circuit Enable */
#define RIIC_FER_NFE   BIT(5) /* Digital Noise Filter Circuit Enable */
#define RIIC_FER_NACKE BIT(4) /* NACK Reception Transfer Suspension Enable */
#define RIIC_FER_SALE  BIT(3) /* Target Arbitration-Lost Detection Enable */
#define RIIC_FER_NALE  BIT(2) /* NACK Transmission Arbitration-Lost Detection Enable */
#define RIIC_FER_MALE  BIT(1) /* Controller Arbitration-Lost Detection Enable */
#define RIIC_FER_TMOE  BIT(0) /* Timeout Function Enable */

#define RIIC_SER_HOAE       BIT(7) /* Host Address Enable */
#define RIIC_SER_DIE        BIT(5) /* Device-ID Address Detection Enable */
#define RIIC_SER_GCE        BIT(3) /* General Call Address Enable */
#define RIIC_SER_SAR2       BIT(2) /* Target Address Register 2 Enable */
#define RIIC_SER_SAR1       BIT(1) /* Target Address Register 1 Enable */
#define RIIC_SER_SAR0       BIT(0) /* Target Address Register 0 Enable */
#define RIIC_SER_SLAVE_MASK (RIIC_SER_SAR0 | RIIC_SER_SAR1 | RIIC_SER_SAR2)

#define RIIC_IER_TIE   BIT(7) /* Transmit Data Empty Interrupt Enable */
#define RIIC_IER_TEIE  BIT(6) /* Transmit End Interrupt Enable */
#define RIIC_IER_RIE   BIT(5) /* Receive Data Full Interrupt Enable */
#define RIIC_IER_NAKIE BIT(4) /* NACK Reception Interrupt Enable */
#define RIIC_IER_SPIE  BIT(3) /* Stop Condition Detection Interrupt Enable */
#define RIIC_IER_STIE  BIT(2) /* Start Condition Detection Interrupt Enable */
#define RIIC_IER_ALIE  BIT(1) /* Arbitration-Lost Interrupt Enable */
#define RIIC_IER_TMOIE BIT(0) /* Timeout Interrupt Enable */

#define RIIC_SR1_HOA      BIT(7) /* Host Address Detection Flag */
#define RIIC_SR1_DID      BIT(5) /* Device-ID Address Detection Flag */
#define RIIC_SR1_GCA      BIT(3) /* General Call Address Detection Flag */
#define RIIC_SR1_AAS2     BIT(2) /* Target Address 2 Detection Flag */
#define RIIC_SR1_AAS1     BIT(1) /* Target Address 1 Detection Flag */
#define RIIC_SR1_AAS0     BIT(0) /* Target Address 0 Detection Flag */
#define RIIC_SR1_AAS_MASK (RIIC_SR1_AAS0 | RIIC_SR1_AAS1 | RIIC_SR1_AAS2)

#define RIIC_SR2_TDRE  BIT(7) /* Transmit Data Empty Flag */
#define RIIC_SR2_TEND  BIT(6) /* Transmit End Flag */
#define RIIC_SR2_RDRF  BIT(5) /* Receive Data Full Flag */
#define RIIC_SR2_NACKF BIT(4) /* NACK Reception Flag */
#define RIIC_SR2_STOP  BIT(3) /* Stop Condition Detection Flag */
#define RIIC_SR2_START BIT(2) /* Start Condition Detection Flag */
#define RIIC_SR2_AL    BIT(1) /* Arbitration-Lost Flag */
#define RIIC_SR2_TMOF  BIT(0) /* Timeout Flag */

#define RIIC_BR_RESERVED    0xE0 /* Should be 1 on writes */
#define MAX_WAIT_US         500
#define TRANSFER_TIMEOUT_MS 10 /* Timeout for @i2c_rza2m_riic_data::transfer_mtx */
#define RIIC_MAX_TIMEOUT    (10 * USEC_PER_MSEC) /* Timeout for clearing status bits in us */

static void i2c_rza2m_riic_write(const struct device *dev, uint32_t offs, uint32_t value)
{
	sys_write32(value, DEVICE_MMIO_GET(dev) + offs);
}

static uint32_t i2c_rza2m_riic_read(const struct device *dev, uint32_t offs)
{
	return sys_read32(DEVICE_MMIO_GET(dev) + offs);
}

static int i2c_rza2m_riic_wait_for_clear(const struct device *dev, uint32_t offs, uint16_t mask)
{
	if (!WAIT_FOR(((i2c_rza2m_riic_read(dev, offs) & mask) == 0), RIIC_MAX_TIMEOUT,
		      k_busy_wait(USEC_PER_MSEC))) {
		return -EIO;
	}
	return 0;
}

static inline void i2c_rza2m_riic_clear_set_bit(const struct device *dev, uint32_t offs,
						uint16_t clear, uint16_t set)
{
	i2c_rza2m_riic_write(dev, offs, (i2c_rza2m_riic_read(dev, offs) & ~clear) | set);
}

static int i2c_rza2m_riic_wait_for_state(const struct device *dev, uint8_t mask, bool forever)
{
	struct i2c_rza2m_riic_data *data = dev->data;
	int ret;
	uint32_t int_backup;

	data->interrupt_mask = mask;
	data->status_bits = i2c_rza2m_riic_read(dev, RIIC_SR2);
	if (data->status_bits & mask) {
		data->interrupt_mask = 0;
		data->status_bits &= mask;
		return 0;
	}

	/* Reset interrupts semaphore */
	k_sem_reset(&data->sem);

	/* Save previous interrupts before modifying */
	int_backup = i2c_rza2m_riic_read(dev, RIIC_IER);

	/* Enable additionally interrupts */
	i2c_rza2m_riic_write(dev, RIIC_IER, mask | int_backup);

	/* Wait for the interrupts */
	ret = k_sem_take(&data->sem, (forever) ? K_FOREVER : K_USEC(MAX_WAIT_US));

	/* Restore previous interrupts and wait for the changes to take effect */
	i2c_rza2m_riic_write(dev, RIIC_IER, int_backup);
	WAIT_FOR((i2c_rza2m_riic_read(dev, RIIC_IER) != int_backup), RIIC_MAX_TIMEOUT, k_msleep(1));

	if (!ret) {
		return 0;
	}
	data->status_bits = i2c_rza2m_riic_read(dev, RIIC_SR2) & mask;
	if (data->status_bits) {
		data->interrupt_mask = 0;
		return 0;
	}
	return ret;
}

static inline void riic_transmit_ack(const struct device *dev)
{
	i2c_rza2m_riic_clear_set_bit(dev, RIIC_MR3, 0, RIIC_MR3_ACKWP);
	i2c_rza2m_riic_clear_set_bit(dev, RIIC_MR3, RIIC_MR3_ACKBT, 0);
	i2c_rza2m_riic_clear_set_bit(dev, RIIC_MR3, RIIC_MR3_ACKWP, 0);
}

static inline void riic_transmit_nack(const struct device *dev)
{
	i2c_rza2m_riic_clear_set_bit(dev, RIIC_MR3, 0, RIIC_MR3_ACKWP);
	i2c_rza2m_riic_clear_set_bit(dev, RIIC_MR3, 0, RIIC_MR3_ACKBT);
	i2c_rza2m_riic_clear_set_bit(dev, RIIC_MR3, RIIC_MR3_ACKWP, 0);
}

static int i2c_rza2m_riic_finish(const struct device *dev)
{
	i2c_rza2m_riic_clear_set_bit(dev, RIIC_CR2, 0, RIIC_CR2_SP);
	i2c_rza2m_riic_wait_for_state(dev, RIIC_IER_SPIE, false);
	if (i2c_rza2m_riic_read(dev, RIIC_SR2) & RIIC_SR2_START) {
		i2c_rza2m_riic_clear_set_bit(dev, RIIC_SR2, RIIC_SR2_START, 0);
	}
	i2c_rza2m_riic_clear_set_bit(dev, RIIC_SR2, RIIC_SR2_NACKF | RIIC_SR2_STOP, 0);
	return 0;
}

static int i2c_rza2m_riic_set_addr(const struct device *dev, uint16_t chip, uint16_t flags)
{
	uint8_t read = !!(flags & I2C_MSG_READ);
	struct i2c_rza2m_riic_data *data = dev->data;

	k_busy_wait(MAX_WAIT_US);
	if (i2c_rza2m_riic_wait_for_state(dev, RIIC_IER_TIE, false)) {
		i2c_rza2m_riic_finish(dev);
		return 1;
	}
	/* Set target address & transfer mode */
	if (flags & I2C_MSG_ADDR_10_BITS) {
		i2c_rza2m_riic_write(dev, RIIC_DRT, 0xf0 | ((chip >> 7) & 0x6) | read);
		i2c_rza2m_riic_wait_for_state(dev, RIIC_IER_TIE, false);
		i2c_rza2m_riic_write(dev, RIIC_DRT, chip & 0xff);
	} else {
		i2c_rza2m_riic_write(dev, RIIC_DRT, ((chip & 0x7f) << 1) | read);
	}
	i2c_rza2m_riic_wait_for_state(dev, RIIC_IER_NAKIE, false);
	if (data->status_bits & RIIC_SR2_NACKF) {
		return 1;
	}
	if ((i2c_rza2m_riic_read(dev, RIIC_MR3) & RIIC_MR3_ACKBR) == 0) {
		return 0;
	}
	return 1;
}

static int i2c_rza2m_riic_transfer_msg(const struct device *dev, struct i2c_msg *msg)
{
	uint32_t i;

	if ((msg->flags & I2C_MSG_RW_MASK) == I2C_MSG_READ) {
		/* Controller read operation in sync mode */
		/* Before reading, wait for target address transmission to complete */
		if (i2c_rza2m_riic_wait_for_state(dev, RIIC_IER_RIE, false)) {
			return -EIO;
		}

		if (msg->len == 1) {
			i2c_rza2m_riic_clear_set_bit(dev, RIIC_MR3, 0, RIIC_MR3_WAIT);
		}

		/* Dummy read for clearing RDRF flag */
		i2c_rza2m_riic_read(dev, RIIC_DRR);

		for (i = 0; i < msg->len; i++) {
			if (i2c_rza2m_riic_wait_for_state(dev, RIIC_IER_RIE, false)) {
				return -EIO;
			}
			if (msg->len == i + 2) {
				i2c_rza2m_riic_clear_set_bit(dev, RIIC_MR3, 0, RIIC_MR3_WAIT);
			}
			if (msg->len == i + 1) {
				i2c_rza2m_riic_clear_set_bit(dev, RIIC_CR2, 0, RIIC_CR2_SP);
				riic_transmit_nack(dev);
			} else {
				riic_transmit_ack(dev);
			}

			/* Receive next byte */
			msg->buf[i] = i2c_rza2m_riic_read(dev, RIIC_DRR) & 0xff;
		}
	} else {
		/* Controller write operation in sync mode */
		for (i = 0; i < msg->len; i++) {
			if (i2c_rza2m_riic_wait_for_state(dev, RIIC_IER_TIE, false)) {
				return -EIO;
			}
			i2c_rza2m_riic_write(dev, RIIC_DRT, msg->buf[i]);
		}
		i2c_rza2m_riic_wait_for_state(dev, RIIC_IER_TEIE, false);
	}

	return 0;
}

#define OPERATION(msg) (((struct i2c_msg *)msg)->flags & I2C_MSG_RW_MASK)
static int i2c_rza2m_riic_validate_msgs(const struct device *dev, struct i2c_msg *msgs,
					uint8_t num_msgs)
{
	struct i2c_msg *current, *next;

	if (!num_msgs) {
		return 0;
	}

	current = msgs;
	current->flags |= I2C_MSG_RESTART;
	for (int i = 1; i <= num_msgs; i++) {
		if (i < num_msgs) {
			next = current + 1;

			/* Restart condition between messages of different directions is required */
			if ((OPERATION(current) != OPERATION(next)) &&
			    !(next->flags & I2C_MSG_RESTART)) {
				LOG_ERR("%s: Restart condition between messages of "
					"different directions is required."
					"Current/Total: [%d/%d]",
					__func__, i, num_msgs);
				return -EIO;
			}

			/* Stop condition is only allowed on last message */
			if (current->flags & I2C_MSG_STOP) {
				LOG_ERR("%s: Invalid stop flag. Stop condition is only allowed on "
					"last message. "
					"Current/Total: [%d/%d]",
					__func__, i, num_msgs);
				return -EIO;
			}
		} else {
			current->flags |= I2C_MSG_STOP;
		}
		current++;
	}

	return 0;
}

static int i2c_rza2m_riic_start_and_set_addr(const struct device *dev, struct i2c_msg *msg,
					     uint16_t addr)
{
	if (msg->flags & I2C_MSG_RESTART) {
		if (i2c_rza2m_riic_read(dev, RIIC_SR2) & RIIC_SR2_START) {
			/* Generate RESTART condition */
			i2c_rza2m_riic_clear_set_bit(dev, RIIC_CR2, 0, RIIC_CR2_RS);
		} else {
			/* Generate START condition */
			i2c_rza2m_riic_clear_set_bit(dev, RIIC_CR2, 0, RIIC_CR2_ST);
		}

		/* Send target address */
		if (i2c_rza2m_riic_set_addr(dev, addr, msg->flags)) {
			i2c_rza2m_riic_finish(dev);
			return -EIO; /* No ACK received */
		}
	}

	return 0;
}

static int i2c_rza2m_riic_handle_msg(const struct device *dev, struct i2c_msg *msg)
{
	int ret = 0;

	if (msg->len) {
		ret = i2c_rza2m_riic_transfer_msg(dev, msg);
		if (ret != 0) {
			return ret;
		}
	}

	if ((msg->flags & I2C_MSG_STOP) == I2C_MSG_STOP) {
		ret = i2c_rza2m_riic_finish(dev);
		if (ret != 0) {
			return ret;
		}
	}

	return 0;
}

static int i2c_rza2m_riic_transfer(const struct device *dev, struct i2c_msg *msgs, uint8_t num_msgs,
				   uint16_t addr)
{
	struct i2c_rza2m_riic_data *data = dev->data;
	int ret = 0;

	if (!num_msgs) {
		return 0;
	}

	ret = i2c_rza2m_riic_validate_msgs(dev, msgs, num_msgs);
	if (ret) {
		return ret;
	}
	k_mutex_lock(&data->i2c_lock_mtx, K_FOREVER);

	/* Wait for the bus to be available */
	ret = i2c_rza2m_riic_wait_for_clear(dev, RIIC_CR2, RIIC_CR2_BBSY);
	if (ret < 0) {
		LOG_ERR("Bus is busy. Another transfer was in progress.");
		ret = -EIO;
		goto exit;
	}

	while (num_msgs) {
		ret = i2c_rza2m_riic_start_and_set_addr(dev, msgs, addr);
		if (ret) {
			break;
		}

		ret = i2c_rza2m_riic_handle_msg(dev, msgs);
		if (ret) {
			break;
		}

		msgs++;
		num_msgs--;
	}

	/* Complete without error */
exit:
	k_mutex_unlock(&data->i2c_lock_mtx);
	return ret;
}

static void i2c_rza2m_riic_calc_bitrate(const struct device *dev, uint32_t total_brl_brh,
					uint32_t brh, uint32_t divider,
					struct i2c_rza2m_riic_bitrate *result)
{
	const struct i2c_rza2m_riic_config *config = dev->config;
	struct i2c_rza2m_riic_data *data = dev->data;
	const uint32_t noise_filter_stages = config->noise_filter_stage;
	const double rise_time_s = config->rise_time_s;
	const double fall_time_s = config->fall_time_s;
	const double requested_duty = config->duty_cycle_percent;
	const uint32_t peripheral_clock = data->clk_rate;
	uint32_t constant_add = 0;

	/* A constant is added to BRL and BRH in all formulas. This constand is 3 + nf when CKS ==
	 * 0, or 2 + nf when CKS != 0.
	 */
	if (divider == 0) {
		constant_add = 3 + noise_filter_stages;
	} else {
		/* All dividers other than 0 use an addition of 2 + noise_filter_stages. */
		constant_add = 2 + noise_filter_stages;
	}

	/* Converts all divided numbers to double to avoid data loss. */
	double divided_p0 = (peripheral_clock >> divider);

	result->bitrate =
		1 / ((total_brl_brh + 2 * constant_add) / divided_p0 + rise_time_s + fall_time_s);
	result->duty =
		100 *
		((rise_time_s + ((brh + constant_add) / divided_p0)) /
		 (rise_time_s + fall_time_s + ((total_brl_brh + 2 * constant_add)) / divided_p0));
	result->divider = divider;
	result->brh = brh;
	result->brl = total_brl_brh - brh;
	result->duty_error_percent =
		(result->duty > requested_duty ? (result->duty - requested_duty)
					       : (requested_duty - result->duty)) /
		requested_duty;
}

static bool i2c_rza2m_riic_find_bitrate_for_divider(const struct device *dev, int temp_divider,
						    uint32_t requested_duty, uint32_t min_brh,
						    uint32_t min_brl_brh,
						    struct i2c_rza2m_riic_bitrate *bitrate)
{
	const struct i2c_rza2m_riic_config *config = dev->config;
	struct i2c_rza2m_riic_data *data = dev->data;
	const double rise_time_s = config->rise_time_s;
	const double fall_time_s = config->fall_time_s;
	const uint32_t peripheral_clock = data->clk_rate;
	uint32_t constant_add = (temp_divider == 0) ? (3 + config->noise_filter_stage)
						    : (2 + config->noise_filter_stage);

	double divided_p0 = (peripheral_clock >> temp_divider);
	uint32_t total_brl_brh =
		ceil(((1 / (double)config->bitrate) - (rise_time_s + fall_time_s)) * divided_p0 -
		     (2 * constant_add));

	if ((total_brl_brh > 62) || (total_brl_brh < min_brl_brh)) {
		return false;
	}

	uint32_t temp_brh = total_brl_brh * requested_duty / 100;

	if (temp_brh < min_brh) {
		temp_brh = min_brh;
	}

	struct i2c_rza2m_riic_bitrate temp_bitrate = {};

	i2c_rza2m_riic_calc_bitrate(dev, total_brl_brh, temp_brh, temp_divider, &temp_bitrate);

	/* Adjust duty cycle down if it helps. */
	struct i2c_rza2m_riic_bitrate test_bitrate = temp_bitrate;

	while (test_bitrate.duty > requested_duty) {
		temp_brh -= 1;

		if ((temp_brh < min_brh) || ((total_brl_brh - temp_brh) > 31)) {
			break;
		}

		struct i2c_rza2m_riic_bitrate new_bitrate = {};

		i2c_rza2m_riic_calc_bitrate(dev, total_brl_brh, temp_brh, temp_divider,
					    &new_bitrate);

		if (new_bitrate.duty_error_percent < temp_bitrate.duty_error_percent) {
			temp_bitrate = new_bitrate;
		} else {
			break;
		}
	}

	/* Adjust duty cycle up if it helps. */
	while (test_bitrate.duty < requested_duty) {
		++temp_brh;

		if ((temp_brh > total_brl_brh) || (temp_brh > 31) ||
		    ((total_brl_brh - temp_brh) < min_brh)) {
			break;
		}

		struct i2c_rza2m_riic_bitrate new_bitrate = {};

		i2c_rza2m_riic_calc_bitrate(dev, total_brl_brh, temp_brh, temp_divider,
					    &new_bitrate);

		if (new_bitrate.duty_error_percent < temp_bitrate.duty_error_percent) {
			temp_bitrate = new_bitrate;
		} else {
			break;
		}
	}

	if ((temp_bitrate.brh < 32) && (temp_bitrate.brl < 32)) {
		/* Valid setting found. */
		*bitrate = temp_bitrate;
		return true;
	}

	return false;
}

static void i2c_rza2m_riic_calc_clock_setting(const struct device *dev,
					      struct i2c_rza2m_riic_clk_settings *clk_cfg)
{
	const struct i2c_rza2m_riic_config *config = dev->config;
	const uint32_t noise_filter_stages = config->noise_filter_stage;
	const uint32_t requested_duty = config->duty_cycle_percent;

	/* Start with maximum possible bitrate. */
	uint32_t min_brh = noise_filter_stages + 1;
	uint32_t min_brl_brh = 2 * min_brh;
	struct i2c_rza2m_riic_bitrate bitrate = {};

	i2c_rza2m_riic_calc_bitrate(dev, min_brl_brh, min_brh, 0, &bitrate);

	/* Start with the smallest divider because it gives the most resolution. */
	for (int temp_divider = 0; temp_divider <= 7; ++temp_divider) {
		struct i2c_rza2m_riic_bitrate temp_bitrate = {};

		if (i2c_rza2m_riic_find_bitrate_for_divider(dev, temp_divider, requested_duty,
							    min_brh, min_brl_brh, &temp_bitrate)) {
			bitrate = temp_bitrate;
			break;
		}
	}

	clk_cfg->brl_value = bitrate.brl;
	clk_cfg->brh_value = bitrate.brh;
	clk_cfg->cks_value = bitrate.divider;
}

static int i2c_rza2m_riic_configure(const struct device *dev, uint32_t dev_config)
{
	const struct i2c_rza2m_riic_config *config = dev->config;
	struct i2c_rza2m_riic_data *data = dev->data;

	if (data->dev_config == dev_config) {
		return 0;
	}

	if (I2C_SPEED_GET(dev_config) != I2C_SPEED_STANDARD &&
	    I2C_SPEED_GET(dev_config) != I2C_SPEED_FAST &&
	    I2C_SPEED_GET(dev_config) != I2C_SPEED_FAST_PLUS) {
		LOG_ERR("%s: supported only I2C_SPEED_STANDARD, I2C_SPEED_FAST and "
			"I2C_SPEED_FAST_PLUS",
			dev->name);
		return -EIO;
	}
	i2c_rza2m_riic_calc_clock_setting(dev, &data->clk_settings);

	/* Prohibiting the bus configuration during transfer. */
	if (k_mutex_lock(&data->i2c_lock_mtx, K_MSEC(TRANSFER_TIMEOUT_MS))) {
		LOG_ERR("Bus is busy");
		return -EIO;
	}

	/* Perform RIIC reset */
	i2c_rza2m_riic_clear_set_bit(dev, RIIC_CR1, RIIC_CR1_ICE, 0);
	i2c_rza2m_riic_clear_set_bit(dev, RIIC_CR1, 0, RIIC_CR1_IICRST);
	i2c_rza2m_riic_clear_set_bit(dev, RIIC_CR1, 0, RIIC_CR1_ICE);

	/* Configure the clock settings */
	/* Set the internal reference clock source for generating RIIC clock */
	i2c_rza2m_riic_write(dev, RIIC_MR1, RIIC_MR1_CKS(data->clk_settings.cks_value));

	/* Set the number of counts that the clock remains high, bit 7 to 5 should be written as 1
	 */
	i2c_rza2m_riic_write(dev, RIIC_BRH, data->clk_settings.brh_value | RIIC_BR_RESERVED);

	/* Set the number of counts that the clock remains low, bit 7 to 5 should be written as 1 */
	i2c_rza2m_riic_write(dev, RIIC_BRL, data->clk_settings.brl_value | RIIC_BR_RESERVED);

	/* Ensure the HW is in controller mode and does not behave as a target to another controller
	 * on the same channel.
	 */
	i2c_rza2m_riic_write(dev, RIIC_SER, 0);

	/* Set Noise Filter Stage Selection. */
	i2c_rza2m_riic_write(dev, RIIC_MR3, config->noise_filter_stage - 1);
	i2c_rza2m_riic_clear_set_bit(dev, RIIC_MR3, 0, RIIC_MR3_RDRFS);

	/* Enable FM+ slope circuit if fast mode plus is enabled. */
	if (I2C_SPEED_GET(dev_config) == I2C_SPEED_FAST_PLUS) {
		i2c_rza2m_riic_clear_set_bit(dev, RIIC_FER, 0, RIIC_FER_FMPE);
	}

	/* RIIC reset */
	i2c_rza2m_riic_clear_set_bit(dev, RIIC_CR1, RIIC_CR1_IICRST, 0);

	data->dev_config = dev_config;
	k_mutex_unlock(&data->i2c_lock_mtx);

	return 0;
}

static int i2c_rza2m_riic_get_config(const struct device *dev, uint32_t *dev_config)
{
	struct i2c_rza2m_riic_data *data = dev->data;

	*dev_config = data->dev_config;
	return 0;
}

static int i2c_rza2m_riic_init(const struct device *dev)
{
	const struct i2c_rza2m_riic_config *config = dev->config;
	struct i2c_rza2m_riic_data *data = dev->data;
	uint32_t bitrate_cfg;
	int ret;

	k_sem_init(&data->sem, 0, 1);

	k_mutex_init(&data->i2c_lock_mtx);

	if (!device_is_ready(config->clock_dev)) {
		LOG_ERR("Device %s is not ready", dev->name);
		return -ENODEV;
	}

	/* Configure dt provided device signals when available */
	ret = pinctrl_apply_state(config->pcfg, PINCTRL_STATE_DEFAULT);
	if (ret < 0) {
		LOG_ERR("Can't apply pinctrl state for %s", dev->name);
		return ret;
	}

	ret = clock_control_on(config->clock_dev, (clock_control_subsys_t)config->clock_subsys);
	if (ret < 0) {
		return ret;
	}

	ret = clock_control_get_rate(config->clock_dev,
				     (clock_control_subsys_t)config->clock_subsys, &data->clk_rate);
	if (ret < 0) {
		return ret;
	}

	DEVICE_MMIO_MAP(dev, K_MEM_CACHE_NONE);

	bitrate_cfg = i2c_map_dt_bitrate(config->bitrate);
	ret = i2c_rza2m_riic_configure(dev, I2C_MODE_CONTROLLER | bitrate_cfg);
	if (ret != 0) {
		LOG_ERR("Can't configure device %s", dev->name);
	}

	return ret;
}

static void i2c_rza2m_riic_isr(const struct device *dev)
{
	struct i2c_rza2m_riic_data *data = dev->data;
	uint32_t value;

	/* Only for controller mode */
	value = i2c_rza2m_riic_read(dev, RIIC_SR2);
	if (value & data->interrupt_mask) {
		data->status_bits = value & data->interrupt_mask;
		k_sem_give(&data->sem);
		data->interrupt_mask = 0;
	}
}

static void i2c_rza2m_riic_isr_tei(const struct device *dev)
{
	i2c_rza2m_riic_isr(dev);
	i2c_rza2m_riic_clear_set_bit(dev, RIIC_SR2, RIIC_SR2_TEND, 0);
	i2c_rza2m_riic_wait_for_clear(dev, RIIC_SR2, RIIC_SR2_TEND);
}

static void i2c_rza2m_riic_isr_spi(const struct device *dev)
{
	i2c_rza2m_riic_isr(dev);
	i2c_rza2m_riic_clear_set_bit(dev, RIIC_SR2, RIIC_SR2_STOP, 0);
	i2c_rza2m_riic_wait_for_clear(dev, RIIC_SR2, RIIC_SR2_STOP);
}

static void i2c_rza2m_riic_isr_sti(const struct device *dev)
{
	i2c_rza2m_riic_isr(dev);
	i2c_rza2m_riic_clear_set_bit(dev, RIIC_SR2, RIIC_SR2_START, 0);
	i2c_rza2m_riic_wait_for_clear(dev, RIIC_SR2, RIIC_SR2_START);
}

static void i2c_rza2m_riic_isr_naki(const struct device *dev)
{
	i2c_rza2m_riic_isr(dev);
	i2c_rza2m_riic_clear_set_bit(dev, RIIC_SR2, RIIC_SR2_NACKF, 0);
	i2c_rza2m_riic_wait_for_clear(dev, RIIC_SR2, RIIC_SR2_NACKF);
}

static void i2c_rza2m_riic_isr_ali(const struct device *dev)
{
	i2c_rza2m_riic_isr(dev);
	i2c_rza2m_riic_clear_set_bit(dev, RIIC_SR2, RIIC_SR2_AL, 0);
	i2c_rza2m_riic_wait_for_clear(dev, RIIC_SR2, RIIC_SR2_AL);
}

static void i2c_rza2m_riic_isr_tmoi(const struct device *dev)
{
	i2c_rza2m_riic_isr(dev);
	i2c_rza2m_riic_clear_set_bit(dev, RIIC_SR2, RIIC_SR2_TMOF, 0);
	i2c_rza2m_riic_wait_for_clear(dev, RIIC_SR2, RIIC_SR2_TMOF);
}

static DEVICE_API(i2c, i2c_rza2m_riic_driver_api) = {
	.configure = i2c_rza2m_riic_configure,
	.get_config = i2c_rza2m_riic_get_config,
	.transfer = i2c_rza2m_riic_transfer,
};

#define I2C_RZA2M_IRQ_CONNECT(n, irq_name, isr)                                                    \
	IRQ_CONNECT(DT_INST_IRQ_BY_NAME(n, irq_name, irq) - GIC_SPI_INT_BASE,                      \
		    DT_INST_IRQ_BY_NAME(n, irq_name, priority), isr, DEVICE_DT_INST_GET(n),        \
		    DT_INST_IRQ_BY_NAME(n, irq_name, flags));                                      \
	irq_enable(DT_INST_IRQ_BY_NAME(n, irq_name, irq) - GIC_SPI_INT_BASE);

#define I2C_RZA2M_CONFIG_FUNC(n)                                                                   \
	I2C_RZA2M_IRQ_CONNECT(n, tei, i2c_rza2m_riic_isr_tei)                                      \
	I2C_RZA2M_IRQ_CONNECT(n, rxi, i2c_rza2m_riic_isr)                                          \
	I2C_RZA2M_IRQ_CONNECT(n, txi, i2c_rza2m_riic_isr)                                          \
	I2C_RZA2M_IRQ_CONNECT(n, spi, i2c_rza2m_riic_isr_spi)                                      \
	I2C_RZA2M_IRQ_CONNECT(n, sti, i2c_rza2m_riic_isr_sti)                                      \
	I2C_RZA2M_IRQ_CONNECT(n, naki, i2c_rza2m_riic_isr_naki)                                    \
	I2C_RZA2M_IRQ_CONNECT(n, ali, i2c_rza2m_riic_isr_ali)                                      \
	I2C_RZA2M_IRQ_CONNECT(n, tmoi, i2c_rza2m_riic_isr_tmoi)

#define I2C_RZA2M_RIIC_INIT(n)                                                                     \
	PINCTRL_DT_INST_DEFINE(n);                                                                 \
	uint32_t clock_subsys_riic##n = DT_INST_CLOCKS_CELL(n, clk_id);                            \
	static const struct i2c_rza2m_riic_config i2c_rza2m_riic_config_##n = {                    \
		DEVICE_MMIO_ROM_INIT(DT_DRV_INST(n)),                                              \
		.clock_dev = DEVICE_DT_GET(DT_INST_CLOCKS_CTLR(n)),                                \
		.clock_subsys = (clock_control_subsys_t)(&clock_subsys_riic##n),                   \
		.bitrate = DT_INST_PROP(n, clock_frequency),                                       \
		.rise_time_s = DT_INST_PROP(n, rise_time_ns) / RZA2M_RIIC_DIV_TIME_NS,             \
		.fall_time_s = DT_INST_PROP(n, fall_time_ns) / RZA2M_RIIC_DIV_TIME_NS,             \
		.duty_cycle_percent = DT_INST_PROP(n, duty_cycle_percent),                         \
		.noise_filter_stage = DT_INST_PROP(n, noise_filter_stages),                        \
		.pcfg = PINCTRL_DT_INST_DEV_CONFIG_GET(n),                                         \
	};                                                                                         \
                                                                                                   \
	static struct i2c_rza2m_riic_data i2c_rza2m_riic_data_##n;                                 \
                                                                                                   \
	static int i2c_rza2m_riic_init_##n(const struct device *dev)                               \
	{                                                                                          \
		I2C_RZA2M_CONFIG_FUNC(n)                                                           \
		return i2c_rza2m_riic_init(dev);                                                   \
	}                                                                                          \
                                                                                                   \
	I2C_DEVICE_DT_INST_DEFINE(n, i2c_rza2m_riic_init_##n, NULL, &i2c_rza2m_riic_data_##n,      \
				  &i2c_rza2m_riic_config_##n, POST_KERNEL,                         \
				  CONFIG_I2C_INIT_PRIORITY, &i2c_rza2m_riic_driver_api);

DT_INST_FOREACH_STATUS_OKAY(I2C_RZA2M_RIIC_INIT)
