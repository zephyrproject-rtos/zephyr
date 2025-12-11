/*
 * Copyright (c) 2020 Nuvoton Technology Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_I2C_I2C_NPCX_CONTROLLER_H_
#define ZEPHYR_DRIVERS_I2C_I2C_NPCX_CONTROLLER_H_

#include <zephyr/device.h>

#include "soc_miwu.h"

#ifdef __cplusplus
extern "C" {
#endif

/* I2C peripheral register mode */
#define NPCX_I2C_BANK_NORMAL 0
#define NPCX_I2C_BANK_FIFO   1

/* 32 bytes Tx FIFO and 32 bytes Rx FIFO. */
#define NPCX_I2C_FIFO_MAX_SIZE 32

/* Support 65535 bytes during DMA transaction */
#define NPCX_I2C_DMA_MAX_SIZE 65535

/* The delay for the I2C bus recovery bitbang in ~100K Hz */
#define I2C_RECOVER_BUS_DELAY_US 5

/*
 * Internal SMBus Interface driver states values, which reflect events
 * which occurred on the bus
 */
enum npcx_i2c_oper_state {
	NPCX_I2C_IDLE,
	NPCX_I2C_WAIT_START,
	NPCX_I2C_WAIT_RESTART,
	NPCX_I2C_WRITE_DATA,
	NPCX_I2C_WRITE_SUSPEND,
	NPCX_I2C_READ_DATA,
	NPCX_I2C_READ_SUSPEND,
	NPCX_I2C_WAIT_STOP,
	NPCX_I2C_ERROR_RECOVERY,
};

enum npcx_i2c_flag {
	NPCX_I2C_FLAG_TARGET1,
	NPCX_I2C_FLAG_TARGET2,
	NPCX_I2C_FLAG_TARGET3,
	NPCX_I2C_FLAG_TARGET4,
	NPCX_I2C_FLAG_TARGET5,
	NPCX_I2C_FLAG_TARGET6,
	NPCX_I2C_FLAG_TARGET7,
	NPCX_I2C_FLAG_TARGET8,
	NPCX_I2C_FLAG_COUNT,
};

enum i2c_pm_policy_state_flag {
	I2C_PM_POLICY_STATE_FLAG_TGT,
	I2C_PM_POLICY_STATE_FLAG_COUNT,
};

/* Device config */
struct i2c_ctrl_config {
	uintptr_t base;              /* i2c controller base address */
	struct npcx_clk_cfg clk_cfg; /* clock configuration */
	uint8_t irq;                 /* i2c controller irq */
#ifdef CONFIG_I2C_TARGET
	/* i2c wake-up input source configuration */
	const struct npcx_wui smb_wui;
	bool wakeup_source;
#endif /* CONFIG_I2C_TARGET */
};

/* Driver data */
struct i2c_ctrl_data {
	struct k_sem lock_sem;               /* mutex of i2c controller */
	struct k_sem sync_sem;               /* semaphore used for synchronization */
	uint32_t bus_freq;                   /* operation freq of i2c */
#if defined(CONFIG_I2C_NPCX_INVALID_STOP_WORKAROUND)
	uint32_t stop_dealy_cycle_time;      /* delay time in cycles before sending STOP */
#endif
	enum npcx_i2c_oper_state oper_state; /* controller operation state */
	int trans_err;                       /* error code during transaction */
	struct i2c_msg *msg;                 /* cache msg for transaction state machine */
	struct i2c_msg *msg_head;
	int is_write;     /* direction of current msg */
	uint8_t *ptr_msg; /* current msg pointer for FIFO read/write */
	uint16_t addr;    /* slave address of transaction */
	uint8_t msg_max_num;
	uint8_t msg_curr_idx;
	uint8_t port;       /* current port used the controller */
	bool is_configured; /* is port configured? */
	const struct npcx_i2c_timing_cfg *ptr_speed_confs;
#ifdef CONFIG_I2C_TARGET
	struct i2c_target_config *target_cfg[NPCX_I2C_FLAG_COUNT];
	uint8_t target_idx; /* current target_cfg index */
	atomic_t registered_target_mask;
	/* i2c wake-up callback configuration */
	struct miwu_callback smb_wk_cb;
#endif /* CONFIG_I2C_TARGET */

#if defined(CONFIG_PM) && defined(CONFIG_I2C_TARGET)
	ATOMIC_DEFINE(pm_policy_state_flag, I2C_PM_POLICY_STATE_FLAG_COUNT);
#endif /* CONFIG_PM && CONFIG_I2C_TARGET */
};

/* Driver convenience defines */
#define HAL_I2C_INSTANCE(dev)                                                                      \
	((struct smb_reg *)((const struct i2c_ctrl_config *)(dev)->config)->base)

static inline void i2c_ctrl_start(const struct device *dev)
{
	struct smb_reg *const inst = HAL_I2C_INSTANCE(dev);

	inst->SMBCTL1 |= BIT(NPCX_SMBCTL1_START);
}

static inline void i2c_ctrl_data_write(const struct device *dev, uint8_t data)
{
	struct smb_reg *const inst = HAL_I2C_INSTANCE(dev);

	inst->SMBSDA = data;
}

static inline uint8_t i2c_ctrl_data_read(const struct device *dev)
{
	struct smb_reg *const inst = HAL_I2C_INSTANCE(dev);

	return inst->SMBSDA;
}

static inline void i2c_ctrl_irq_enable(const struct device *dev, int enable)
{
	const struct i2c_ctrl_config *const config = dev->config;

	if (enable) {
		irq_enable(config->irq);
	} else {
		irq_disable(config->irq);
	}
}

static inline void i2c_ctrl_notify(const struct device *dev, int error)
{
	struct i2c_ctrl_data *const data = dev->data;

	data->trans_err = error;
	k_sem_give(&data->sync_sem);
}

static inline bool i2c_ctrl_is_scl_sda_both_high(const struct device *dev)
{
	struct smb_reg *const inst = HAL_I2C_INSTANCE(dev);

	if (IS_BIT_SET(inst->SMBCTL3, NPCX_SMBCTL3_SCL_LVL) &&
	    IS_BIT_SET(inst->SMBCTL3, NPCX_SMBCTL3_SDA_LVL)) {
		return true;
	}

	return false;
}

static inline void i2c_ctrl_bank_sel(const struct device *dev, int bank)
{
	struct smb_reg *const inst = HAL_I2C_INSTANCE(dev);

	/* All DMA registers locate at bank 0 */
	if (IS_ENABLED(CONFIG_I2C_NPCX_DMA_DRIVEN)) {
		return;
	}

	if (bank) {
		inst->SMBCTL3 |= BIT(NPCX_SMBCTL3_BNK_SEL);
	} else {
		inst->SMBCTL3 &= ~BIT(NPCX_SMBCTL3_BNK_SEL);
	}
}

#if defined(CONFIG_I2C_NPCX_FIFO_DRIVEN)
static inline void i2c_ctrl_fifo_clear_status(const struct device *dev)
{
	struct smb_reg *const inst = HAL_I2C_INSTANCE(dev);

	inst->SMBFIF_CTS |= BIT(NPCX_SMBFIF_CTS_CLR_FIFO);
}

static inline void i2c_ctrl_fifo_rx_setup_threshold_nack(const struct device *dev, int threshold,
							 int last)
{
	struct smb_reg *const inst = HAL_I2C_INSTANCE(dev);
	uint8_t value = MIN(threshold, NPCX_I2C_FIFO_MAX_SIZE);

	SET_FIELD(inst->SMBRXF_CTL, NPCX_SMBRXF_CTL_RX_THR, value);

	/*
	 * Is it last received transaction? If so, set LAST bit. Then the
	 * hardware will generate NACK automatically when receiving last byte.
	 */
	if (last && (value == threshold)) {
		inst->SMBRXF_CTL |= BIT(NPCX_SMBRXF_CTL_LAST);
	}
}
#endif

#if defined(CONFIG_I2C_NPCX_DMA_DRIVEN)
static inline void i2c_ctrl_dma_clear_status(const struct device *dev)
{
	struct smb_reg *const inst = HAL_I2C_INSTANCE(dev);

	/* Clear DMA interrupt bit */
	inst->DMA_CTRL |= BIT(NPCX_DMA_CTL_INTCLR);
}
#endif

/**
 * @brief Lock the mutex of npcx i2c controller.
 *
 * @param i2c_dev Pointer to the device structure for i2c controller instance.
 */
void npcx_i2c_ctrl_mutex_lock(const struct device *i2c_dev);

/**
 * @brief Unlock the mutex of npcx i2c controller.
 *
 * @param i2c_dev Pointer to the device structure for i2c controller instance.
 */
void npcx_i2c_ctrl_mutex_unlock(const struct device *i2c_dev);

/**
 * @brief Configure operation of a npcx i2c controller.
 *
 * @param i2c_dev Pointer to the device structure for i2c controller instance.
 * @param dev_config Bit-packed 32-bit value to the device runtime configuration
 * for the I2C controller.
 *
 * @retval 0 If successful.
 * @retval -EIO General input / output error, failed to configure device.
 * @retval -ERANGE Out of supported i2c frequency.
 */
int npcx_i2c_ctrl_configure(const struct device *i2c_dev, uint32_t dev_config);

/**
 * @brief Get I2C controller speed.
 *
 * @param i2c_dev Pointer to the device structure for i2c controller instance.
 * @param speed Pointer to store the I2C speed.
 *
 * @retval 0 If successful.
 * @retval -ERANGE Stored I2C frequency out of supported range.
 * @retval -EIO    Controller is not configured.
 */
int npcx_i2c_ctrl_get_speed(const struct device *i2c_dev, uint32_t *speed);

/**
 * @brief Perform data transfer to via npcx i2c controller.
 *
 * @param i2c_dev Pointer to the device structure for i2c controller instance.
 * @param msgs Array of messages to transfer.
 * @param num_msgs Number of messages to transfer.
 * @param addr Address of the I2C target device.
 * @param port Port index of selected i2c port.
 *
 * @retval 0 If successful.
 * @retval -EIO General input / output error.
 * @retval -ENXIO No slave address match.
 * @retval -ETIMEDOUT Timeout occurred for a i2c transaction.
 */
int npcx_i2c_ctrl_transfer(const struct device *i2c_dev, struct i2c_msg *msgs,
			      uint8_t num_msgs, uint16_t addr, uint8_t port);

/**
 * @brief Toggle the SCL to generate maximum 9 clocks until the target release
 * the SDA line and send a STOP condition.
 *
 * @param i2c_dev Pointer to the device structure for i2c controller instance.
 *
 * @retval 0 If successful.
 * @retval -EBUSY fail to recover the bus.
 */
int npcx_i2c_ctrl_recover_bus(const struct device *dev);

/**
 * @brief Registers the provided config as Target device of a npcx i2c controller.
 *
 * @param i2c_dev Pointer to the device structure for i2c controller instance.
 * @param target_cfg Config struct used by the i2c target driver
 * @param port Port index of selected i2c port.
 *
 * @retval 0 Is successful
 * @retval -EBUSY If i2c transaction is proceeding.
 */
int npcx_i2c_ctrl_target_register(const struct device *i2c_dev,
				 struct i2c_target_config *target_cfg, uint8_t port);

/**
 * @brief Unregisters the provided config as Target device of a npcx i2c controller.
 *
 * @param i2c_dev Pointer to the device structure for i2c controller instance.
 * @param target_cfg Config struct used by the i2c target driver
 * @param port Port index of selected i2c port.
 *
 * @retval 0 Is successful
 * @retval -EBUSY If i2c transaction is proceeding.
 * @retval -EINVAL If parameters are invalid
 */
int npcx_i2c_ctrl_target_unregister(const struct device *i2c_dev,
				    struct i2c_target_config *target_cfg, uint8_t port);

/* Common public fucntions */
bool i2c_ctrl_toggle_scls(const struct device *dev);
size_t i2c_ctrl_calculate_msg_remains(const struct device *dev);
void i2c_ctrl_handle_write_int_event(const struct device *dev);
void i2c_ctrl_handle_read_int_event(const struct device *dev);
void i2c_ctrl_stop(const struct device *dev);

/* FIFO-Driven public fucntions */
#if defined(CONFIG_I2C_NPCX_FIFO_DRIVEN)
void i2c_ctrl_fifo_hold_bus(const struct device *dev, int stall);
#endif

/* DMA-Driven public fucntions */
#if defined(CONFIG_I2C_NPCX_DMA_DRIVEN)
size_t i2c_ctrl_dma_proceed_write(const struct device *dev);
size_t i2c_ctrl_dma_proceed_read(const struct device *dev);
void i2c_ctrl_handle_write_dma_int_event(const struct device *dev);
void i2c_ctrl_handle_read_dma_int_event(const struct device *dev);
#endif

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_DRIVERS_I2C_I2C_NPCX_CONTROLLER_H_ */
