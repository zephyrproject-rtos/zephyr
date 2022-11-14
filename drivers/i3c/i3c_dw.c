/* dw_i3c.c - I3C file for DesignWare */

/*
 * Copyright (C) 2022 Samsung Electronics Co., Ltd.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/device.h>
#include <zephyr/drivers/i3c.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/irq.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/__assert.h>

#include "i3c_dw.h"

LOG_MODULE_REGISTER(i3c_dw);

#define DEV_CTRL(config)	((config)->regs + DEVICE_CTRL)
#define DEV_CONFIG(dev)		((const struct dw_i3c_config *)(dev)->config)
#define DEV_DATA(dev)		((struct dw_i3c_data *)(dev)->data)
#define DIV_ROUND_UP(a, b)	(((a) + (b) - 1) / b)

#define DT_DRV_COMPAT		snps_dw_i3c

/**
 * @brief DesignWare I3C target information
 *
 * @param dev: Common I3C device data
 * @param controller: Pointer to the controller device
 * @param work: Work structure for queue
 * @param ibi_handle: Function to handle In-Bound Interrupt
 * @param dev_type: Type of the device
 * @param index: Index of the device
 * @param assigned_dyn_addr: Assigned dynamic address during DAA
 * @param lvr: Legacy Virtual Register value
 */
struct dw_target_info {
	struct i3c_device_desc dev;
	const struct device *controller;
	struct k_work work;
	void (*ibi_handle)();
	uint32_t dev_type;
	uint32_t index;
	uint8_t assigned_dyn_addr;
	uint8_t lvr;
};

/**
 * @brief DesignWare I3C command data
 *
 * @param cmd_lo: lower 32-bit of the command
 * @param cmd_hi: higher 32-bit of the command
 * @param tx_len: Length of the message to be transmitted
 * @param rx_len: Length of the response data
 * @param error: Error returned during transfer
 * @param buf: Command buffer to send
 */
struct dw_i3c_cmd {
	uint32_t cmd_lo;
	uint32_t cmd_hi;
	uint16_t tx_len;
	uint16_t rx_len;
	uint8_t error;
	void *buf;
};

/**
 * @brief DesignWare I3C SDR(Single Data Rate) transfer
 *
 * @param ret: Return value
 * @param ncmds: Number of commands
 * @param cmds: Array of commands
 */
struct dw_i3c_xfer {
	int32_t ret;
	uint32_t ncmds;
	struct dw_i3c_cmd cmds[16];
};

/**
 * @brief DesignWare I3C configuration
 *
 * @param core_clk: Device core clock information
 * @param regs: Configuration base address
 * @param max_slv_devs: Maximum connected target devices
 * @param cmdfifodepth: Depth of the command FIFO
 * @param datafifodepth: Depth of the data FIFO
 * @param configure_irq: IRQ config function
 * @param device_list : I3C/I2C device list structure
 */
struct dw_i3c_config {
	uint32_t core_clk;
	uint32_t regs;
	uint16_t max_slv_devs;
	uint8_t cmdfifodepth;
	uint8_t datafifodepth;
	void (*configure_irq)();
	struct i3c_dev_list device_list;

};

/**
 * @brief I3C controller object
 *
 * @param dev: associated controller device
 * @param ops: controller operations
 * @param addrslots: structure to keep track of addresses
 * status and ease the DAA (Dynamic Address Assignment) procedure
 * @param addrs: 2 lists containing all I3C/I2C devices connected to the bus
 * @param mode: bus mode
 */
struct i3c_controller {
	const struct device *dev;
	const struct i3c_driver_api *ops;
	struct i3c_addr_slots addrslots;
	uint8_t *addrs;
	enum i3c_bus_mode mode;
};

/**
 * @brief DesignWare I3C controller information
 *
 * @param base: common data for I3C controller
 * @param free_pos: Free position
 * @param datstartaddr: DAT(Device Address Table) start address
 * @param dctstartaddr: DCT(Device Characteristics Table) start address
 * @param target: Slave device information
 * @param sem_xfer: Semaphore for data transfer
 * @param mt: Mutex for synchronization
 * @param xfer: Structure for Data transfer
 */
struct dw_i3c_data {
	struct i3c_controller base;
	uint32_t free_pos;
	uint16_t datstartaddr;
	uint16_t dctstartaddr;
	struct dw_target_info *target;
	struct k_sem sem_xfer;
	struct k_mutex mt;
	struct dw_i3c_xfer xfer;
};

/**
 * @brief I3C error codes
 *
 * These are standard error codes as defined by the DW I3C specifications
 */
enum i3c_error_code {
	/** unknown error, usually means the error is not I3C related */
	I3C_ERROR_UNKNOWN = 0,

	/** M0 error indicates illegally formatted CCC(Common Command Codes) */
	I3C_ERROR_M0 = 1,

	/** M1 error indicates error in RnW bit during transfer */
	I3C_ERROR_M1,

	/** M2 error indicates no ACK for Broadcast address */
	I3C_ERROR_M2,
};

/**
 * @brief CCC command structure
 *
 * @param rnw: true if the CCC should retrieve data from the device,
 *	       only valid for unicast commands
 * @param id: CCC command id
 * @param addr: can be an I3C device address or the broadcast address
 * @param len: payload length
 * @param data: payload data (this must be DMA-able)
 * @param err: I3C error code
 */
struct i3c_ccc_cmd {
	uint8_t rnw;
	uint8_t id;
	uint8_t addr;
	uint16_t len;
	void *data;
	enum i3c_error_code err;
};

/**
 * @brief Get free position for address
 *
 * @param free_pos: variable provides info about free and used
 *		    addresses among available I3C addresses
 *
 * @retval returns the position of available free address
 */
static int32_t get_free_pos(uint32_t free_pos)
{
	return find_lsb_set(free_pos) - 1;
}

/**
 * @brief Read data to RX buffer during I3C private transfer
 *
 * @param dev: Pointer to the device structure
 * @param buf: Pointer to buffer in which data to be read
 * @param nbytes: Number of bytes to be read
 */
static void read_rx_data_buffer(const struct device *dev,
				uint8_t *buf, int32_t nbytes)
{
	const struct dw_i3c_config *config = DEV_CONFIG(dev);
	int32_t i, packet_size;
	uint32_t reg_val;

	if (!buf) {
		return;
	}

	packet_size = RX_TX_BUFFER_DATA_PACKET_SIZE;

	/* if bytes received are greater than/equal to data packet size */
	if (nbytes >= packet_size) {
		for (i = 0; i <= nbytes - packet_size; i += packet_size) {
			reg_val = sys_read32(config->regs + RX_TX_DATA_PORT);
			memcpy(buf + i, &reg_val, packet_size);
		}
	}

	/* if bytes less than data packet size or not multiple of packet size */
	packet_size = RX_TX_BUFFER_DATA_PACKET_SIZE - 1;

	if (nbytes & packet_size) {
		reg_val = sys_read32(config->regs + RX_TX_DATA_PORT);
		memcpy(buf + (nbytes & ~packet_size),
			      &reg_val, nbytes & packet_size);
	}
}

/**
 * @brief Write data from RX buffer during I3C private transfer
 *
 * @param dev: Pointer to the device structure
 * @param buf: Pointer to buffer in from which data to send
 * @param nbytes: Number bytes to be send
 */
static void write_tx_data_buffer(const struct device *dev,
				 const uint8_t *buf, int32_t nbytes)
{
	const struct dw_i3c_config *config = DEV_CONFIG(dev);
	int32_t i, packet_size;
	uint32_t reg_val;

	if (!buf) {
		return;
	}

	packet_size = RX_TX_BUFFER_DATA_PACKET_SIZE;

	/* if bytes received are greater than/equal to data packet size */
	if (nbytes >= packet_size) {
		for (i = 0; i <= nbytes - packet_size; i += packet_size) {
			memcpy(&reg_val, buf + i, packet_size);
			sys_write32(reg_val, config->regs + RX_TX_DATA_PORT);
		}
	}

	/* if bytes less than data packet size or not multiple of packet size */
	packet_size = RX_TX_BUFFER_DATA_PACKET_SIZE - 1;

	if (nbytes & packet_size) {
		reg_val = 0;
		memcpy(&reg_val, buf + (nbytes & ~packet_size),
		       nbytes & packet_size);
		sys_write32(reg_val, config->regs + RX_TX_DATA_PORT);
	}
}

/**
 * @brief Terminate the transfer process
 *
 * @param dev: Pointer to the device structure
 */
static void xfer_done(const struct device *dev)
{
	const struct dw_i3c_config *config = DEV_CONFIG(dev);
	struct dw_i3c_data *i3c = DEV_DATA(dev);
	struct dw_i3c_xfer *xfer = &i3c->xfer;
	struct dw_i3c_cmd *cmd;
	uint32_t nresp, resp;
	int32_t i, ret = 0;

	nresp = sys_read32(config->regs + QUEUE_STATUS_LEVEL);
	nresp = QUEUE_STATUS_LEVEL_RESP(nresp);
	for (i = 0; i < nresp; i++) {
		resp = sys_read32(config->regs + RESPONSE_QUEUE_PORT);
		cmd = &xfer->cmds[RESPONSE_PORT_TID(resp)];
		cmd->rx_len = RESPONSE_PORT_DATA_LEN(resp);
		cmd->error = RESPONSE_PORT_ERR_STATUS(resp);
	}

	for (i = 0; i < nresp; i++) {
		switch (xfer->cmds[i].error) {
		case RESPONSE_NO_ERROR:
			break;
		case RESPONSE_ERROR_PARITY:		/* FALLTHROUGH */
		case RESPONSE_ERROR_IBA_NACK:		/* FALLTHROUGH */
		case RESPONSE_ERROR_TRANSF_ABORT:	/* FALLTHROUGH */
		case RESPONSE_ERROR_CRC:		/* FALLTHROUGH */
		case RESPONSE_ERROR_FRAME:
			ret = -EIO;
			break;
		case RESPONSE_ERROR_OVER_UNDER_FLOW:
			ret = -ENOSPC;
			break;
		case RESPONSE_ERROR_I2C_W_NACK_ERR:	/* FALLTHROUGH */
		case RESPONSE_ERROR_ADDRESS_NACK:	/* FALLTHROUGH */
		default:
			ret = -EINVAL;
			break;
		}
	}
	xfer->ret = ret;

	if (ret < 0) {
		sys_write32(RESET_CTRL_RX_FIFO | RESET_CTRL_TX_FIFO |
			    RESET_CTRL_RESP_QUEUE | RESET_CTRL_CMD_QUEUE,
			    config->regs + RESET_CTRL);
		sys_write32(sys_read32(DEV_CTRL(config)) | DEV_CTRL_RESUME,
				       DEV_CTRL(config));
	}
	k_sem_give(&i3c->sem_xfer);
}

/**
 * @brief Initiate the transfer procedure
 *
 * @param dev: Pointer to the device structure
 */
static void init_xfer(const struct device *dev)
{
	const struct dw_i3c_config *config = DEV_CONFIG(dev);
	struct dw_i3c_data *i3c = DEV_DATA(dev);
	struct dw_i3c_xfer *xfer = &i3c->xfer;
	struct dw_i3c_cmd *cmd;
	uint32_t thld_ctrl;
	int32_t i;

	for (i = 0; i < xfer->ncmds; i++) {
		cmd = &xfer->cmds[i];
		/* Push data to TXFIFO */
		write_tx_data_buffer(dev, cmd->buf, cmd->tx_len);
	}

	/* Setting Response Buffer Threshold */
	thld_ctrl = sys_read32(config->regs + QUEUE_THLD_CTRL);
	thld_ctrl &= ~QUEUE_THLD_CTRL_RESP_BUF_MASK;
	thld_ctrl |= QUEUE_THLD_CTRL_RESP_BUF(xfer->ncmds);
	sys_write32(thld_ctrl, config->regs + QUEUE_THLD_CTRL);

	/* Enqueue CMD */
	for (i = 0; i < xfer->ncmds; i++) {
		cmd = &xfer->cmds[i];
		sys_write32(cmd->cmd_hi, config->regs + COMMAND_QUEUE_PORT);
		sys_write32(cmd->cmd_lo, config->regs + COMMAND_QUEUE_PORT);
	}
}

/**
 * @brief Get the position of the address
 *
 * @param dev: Pointer to the device structure
 * @param addr: Address of the device
 *
 * @retval position of the device
 * @retval -EINVAL if not found
 */
static int32_t get_addr_pos(const struct device *dev, uint8_t addr)
{
	const struct dw_i3c_config *config = DEV_CONFIG(dev);
	struct dw_i3c_data *i3c = DEV_DATA(dev);
	int32_t pos;

	for (pos = 0; pos < config->max_slv_devs; pos++) {
		if (addr == i3c->base.addrs[pos]) {
			return pos;
		}
	}

	return -EINVAL;
}

/**
 * @brief Function to handle the I3C private transfers
 *
 * @param dev: Pointer to the device structure
 * @target: Pointer to structure containing info of target
 * @param msgs: Pointer to the I3C message lists
 * @param num_msgs: Number of messages to be send
 *
 * @retval  0 if successful
 * @retval -EINVAL if msgs structure is NULL
 * @retval -ENOTSUP if no. of messages over runs fifo
 */
static int32_t dw_i3c_xfers(const struct device *dev,
			    struct i3c_device_desc *target,
			    struct i3c_msg *msgs, uint8_t num_msgs)
{
	const struct dw_i3c_config *config = DEV_CONFIG(dev);
	struct dw_i3c_data *i3c = DEV_DATA(dev);
	struct dw_i3c_xfer *xfer = &i3c->xfer;
	int32_t ret, pos, nrxwords = 0, ntxwords = 0;
	int8_t i;

	if (!msgs || !num_msgs) {
		return -EINVAL;
	}

	if (num_msgs > config->cmdfifodepth) {
		return -ENOTSUP;
	}
	pos = get_addr_pos(dev, target->dynamic_addr);
	if (pos < 0) {
		LOG_ERR("%s Invalid Slave address\n", __func__);
		return pos;
	}

	for (i = 0; i < num_msgs; i++) {
		if (msgs[i].flags & I3C_MSG_READ) {
			nrxwords += DIV_ROUND_UP(msgs[i].len, 4);
		} else {
			ntxwords += DIV_ROUND_UP(msgs[i].len, 4);
		}
	}

	if (ntxwords > config->datafifodepth ||
	    nrxwords > config->datafifodepth) {
		return -ENOTSUP;
	}

	ret = k_mutex_lock(&i3c->mt, K_MSEC(1000));
	if (ret) {
		LOG_ERR("%s Mutex timeout\n", __func__);
		return ret;
	}
	memset(xfer, 0, sizeof(struct dw_i3c_xfer));

	xfer->ncmds = num_msgs;
	xfer->ret = -1;

	for (i = 0; i < num_msgs; i++) {
		struct dw_i3c_cmd *cmd = &xfer->cmds[i];

		cmd->cmd_hi = COMMAND_PORT_ARG_DATA_LEN(msgs[i].len) |
			      COMMAND_PORT_TRANSFER_ARG;
		cmd->cmd_lo = COMMAND_PORT_TID(i) | COMMAND_PORT_ROC |
			      COMMAND_PORT_DEV_INDEX(pos);
		if (msgs[i].hdr_mode) {
			cmd->cmd_lo |= COMMAND_PORT_CP;
		}

		cmd->buf = msgs[i].buf;
		if (msgs[i].flags & I3C_MSG_READ) {
			uint8_t rd_speed = (msgs[i].hdr_mode) ? (I3C_DDR_SPEED)
					   : (i3c->target[pos].dev.data_speed.maxrd);
			cmd->cmd_lo |= (COMMAND_PORT_READ_TRANSFER |
					COMMAND_PORT_SPEED(rd_speed));
			cmd->rx_len = msgs[i].len;
		} else {
			uint8_t wr_speed = (msgs[i].hdr_mode) ? (I3C_DDR_SPEED)
					   : (i3c->target[pos].dev.data_speed.maxwr);
			cmd->cmd_lo |= COMMAND_PORT_SPEED(wr_speed);
			cmd->tx_len = msgs[i].len;
		}

		if (i == (num_msgs - 1)) {
			cmd->cmd_lo |= COMMAND_PORT_TOC;
		}
	}

	init_xfer(dev);

	ret = k_sem_take(&i3c->sem_xfer, K_MSEC(1000));
	if (ret) {
		LOG_ERR("%s Semaphore timeout\n", __func__);
		k_mutex_unlock(&i3c->mt);
		return ret;
	}

	/* Reading back from RX FIFO to user buffer */
	for (i = 0; i < xfer->ncmds; i++) {
		struct dw_i3c_cmd *cmd = &xfer->cmds[i];

		if ((cmd->rx_len > 0) && (cmd->error == 0)) {
			read_rx_data_buffer(dev, cmd->buf, cmd->rx_len);
		}
	}

	ret = xfer->ret;
	k_mutex_unlock(&i3c->mt);

	return ret;
}

/**
 * @brief Send a CCC command
 *
 * @param dev: Pointer to the device structure
 * @param ccc: Pointer to command structure with command which needs to be send
 *
 * @retval  0 if successful
 * @retval -EINVAL if a structure is found NULL
 * @retval  Otherwise a value linked to transfer errors as per I3C specs
 */
static int32_t dw_i3c_send_ccc_cmd(const struct device *dev,
				   struct i3c_ccc_payload *ccc)
{
	struct dw_i3c_data *i3c = DEV_DATA(dev);
	struct dw_i3c_xfer *xfer = &i3c->xfer;
	struct dw_i3c_cmd *cmd;
	int32_t ret, pos = 0;
	int8_t i;

	if (!ccc) {
		return -EINVAL;
	}

	if (ccc->ccc.id & I3C_CCC_DIRECT) {
		pos = get_addr_pos(dev, ccc->targets.payloads->addr);
		if (pos < 0) {
			LOG_ERR("%s Invalid Slave address\n", __func__);
			return pos;
		}
	}

	ret = k_mutex_lock(&i3c->mt, K_MSEC(1000));
	if (ret) {
		LOG_ERR("%s Mutex timeout\n", __func__);
		return ret;
	}

	memset(xfer, 0, sizeof(struct dw_i3c_xfer));
	xfer->ncmds = 1;
	xfer->ret = -1;

	cmd = &xfer->cmds[0];
	cmd->buf = ccc->ccc.data;

	cmd->cmd_hi = COMMAND_PORT_ARG_DATA_LEN(ccc->ccc.data_len) |
		COMMAND_PORT_TRANSFER_ARG;
	cmd->cmd_lo = COMMAND_PORT_CP | COMMAND_PORT_TOC | COMMAND_PORT_ROC |
		      COMMAND_PORT_DEV_INDEX(pos) |
		      COMMAND_PORT_CMD(ccc->ccc.id);

	if (ccc->targets.payloads->rnw) {
		cmd->cmd_lo |= COMMAND_PORT_READ_TRANSFER;
		cmd->rx_len = ccc->ccc.data_len;
	} else {
		cmd->tx_len = ccc->ccc.data_len;
	}

	init_xfer(dev);

	ret = k_sem_take(&i3c->sem_xfer, K_MSEC(1000));
	if (ret) {
		LOG_ERR("%s Semaphore timeout\n", __func__);
		k_mutex_unlock(&i3c->mt);
		return ret;
	}

	for (i = 0; i < xfer->ncmds; i++) {
		if (xfer->cmds[i].rx_len && !xfer->cmds[i].error) {
			read_rx_data_buffer(dev, xfer->cmds[i].buf,
					    xfer->cmds[i].rx_len);
		}
	}

	if (xfer->cmds[0].error == RESPONSE_ERROR_IBA_NACK) {
		cmd->error = I3C_ERROR_M2;
	}

	ret = xfer->ret;
	k_mutex_unlock(&i3c->mt);

	return ret;
}

/**
 * @brief Initialize the CCC command structure
 *
 * @param cmd: Pointer to the command data structure
 * @param addr: Target Address
 * @param data: Data to be sent
 * @param len: Length of the data
 * @param rnw: read or write value flag
 * @id: Command ID value
 *
 */
static void i3c_ccc_cmd_init(struct i3c_ccc_payload *cmd, uint8_t addr,
			     void *data, uint16_t len, uint8_t rnw, uint8_t id)
{
	/* allocate memory for internal payload pointer of cmd */
	struct i3c_ccc_target_payload target_payload;

	cmd->targets.payloads = &target_payload;
	/* Populate structure */
	cmd->targets.payloads->addr = addr;
	cmd->ccc.data = data;
	cmd->ccc.data_len = len;
	cmd->targets.payloads->rnw = (rnw ? 1 : 0);
	cmd->ccc.id = id;
}

/**
 * @brief Send CCC command to enable/disable events
 *
 * @param controller: Pointer to the controller device
 * @param addr: Address of the target device
 * @param evts: Events
 * @param enable: Flag to enable/disable
 *
 * @retval 0 if successful or error code
 */
static int32_t i3c_controller_en_dis_evt(const struct device *dev,
				     uint8_t addr, uint8_t evts, bool enable)
{
	struct i3c_ccc_payload cmd;
	struct i3c_ccc_events events;
	uint8_t id = (enable ? I3C_CCC_ENEC(addr == I3C_BROADCAST_ADDR) :
			I3C_CCC_DISEC(addr == I3C_BROADCAST_ADDR));

	events.events = evts;
	i3c_ccc_cmd_init(&cmd, addr, &events, sizeof(events), 0, id);

	return dw_i3c_send_ccc_cmd(dev, &cmd);
}

/**
 * @brief Enable the In-Band Interrupt (IBI)
 *
 * @param dev: Pointer to the device structure
 * @param dyn_addr: Dynamic address for which IBI to be enabled
 *
 * @retval 0 if successful or error code
 * @retval -EINVAL if fails to get position of assigned address
 */
static int32_t dw_i3c_enable_ibi(const struct device *dev,
				 struct i3c_device_desc *target)
{
	const struct dw_i3c_config *config = DEV_CONFIG(dev);
	uint32_t bitpos, sir_con;
	int32_t pos;

	pos = get_addr_pos(dev, target->dynamic_addr);
	if (pos < 0) {
		LOG_ERR("%s Invalid Slave address\n", __func__);
		return pos;
	}

	sir_con = sys_read32(config->regs + IBI_SIR_REQ_REJECT);
	bitpos = IBI_SIR_REQ_ID(target->dynamic_addr);
	sir_con &= ~BIT(bitpos);
	sys_write32(sir_con, config->regs + IBI_SIR_REQ_REJECT);

	i3c_controller_en_dis_evt(dev, target->dynamic_addr,
			      I3C_CCC_EVENT_SIR, true);

	return 0;
}

/**
 * @brief Disable the In-Band Interrupt (IBI)
 *
 * @param dev: Pointer to the device structure
 * @param dyn_addr: Dynamic address for which IBI need to disable
 *
 * @retval 0 if successful or error code
 * @retval -EINVAL if fails to get position of assigned address
 */
static int32_t dw_i3c_disable_ibi(const struct device *dev,
				  struct i3c_device_desc *target)
{
	const struct dw_i3c_config *config = DEV_CONFIG(dev);
	uint32_t bitpos, sir_con;
	int32_t pos;

	pos = get_addr_pos(dev, target->dynamic_addr);
	if (pos < 0) {
		LOG_ERR("%s Invalid Slave address\n", __func__);
		return pos;
	}

	sir_con = sys_read32(config->regs + IBI_SIR_REQ_REJECT);
	bitpos = IBI_SIR_REQ_ID(target->dynamic_addr);
	sir_con |= BIT(bitpos);
	sys_write32(sir_con, config->regs + IBI_SIR_REQ_REJECT);

	i3c_controller_en_dis_evt(dev, target->dynamic_addr,
			      I3C_CCC_EVENT_SIR, false);

	return 0;
}

/**
 * @brief SIR handler
 *
 * @param dev: Pointer to the device structure
 * @param addr: Address of the target device
 */
static void sir_handle(const struct device *dev, uint8_t addr)
{
	struct dw_i3c_data *i3c = DEV_DATA(dev);
	int32_t pos;

	pos = get_addr_pos(dev, addr);
	if (pos < 0) {
		LOG_ERR("%s Invalid Slave address\n", __func__);
		return;
	}

	k_work_submit(&i3c->target[pos].work);
}

/**
 * @brief IBIS handler
 *
 * @param dev: Pointer to the device structure
 */
static void ibis_handle(const struct device *dev)
{
	const struct dw_i3c_config *config = DEV_CONFIG(dev);
	uint32_t nibis, ibi_stat;
	uint8_t id, i;

	nibis = sys_read32(config->regs + QUEUE_STATUS_LEVEL);
	nibis = QUEUE_STATUS_IBI_BUF_BLR(nibis);
	for (i = 0; i < nibis; i++) {
		ibi_stat = sys_read32(config->regs + IBI_QUEUE_STATUS);
		id = IBI_QUEUE_IBI_ID(ibi_stat);
		if ((IBI_QUEUE_IBI_ID_ADDR(id) != I3C_HOT_JOIN_ADDR) &&
						  (id & BIT(0))) {
			sir_handle(dev, IBI_QUEUE_IBI_ID_ADDR(id));
		} else {
			LOG_ERR("%s Hot Join & Secondary Controller Not supported",
					__func__);
		}
	}
}

/**
 * @brief DesignWare IRQ handler
 *
 * @param dev: Pointer to the device structure
 *
 */
static void i3c_dw_irq(const struct device *dev)
{
	const struct dw_i3c_config *config = DEV_CONFIG(dev);
	uint32_t status;

	status = sys_read32(config->regs + INTR_STATUS);
	if (status & (INTR_TRANSFER_ERR_STAT | INTR_RESP_READY_STAT)) {
		xfer_done(dev);

		if (status & INTR_TRANSFER_ERR_STAT) {
			sys_write32(INTR_TRANSFER_ERR_STAT,
				    config->regs + INTR_STATUS);
		}
	}

	if (status & INTR_IBI_THLD_STAT) {
		ibis_handle(dev);
	}

}

/**
 * @brief Attach the I3C target device to bus
 *
 * @param dev: Pointer to the device structure
 * @param target_dev: Pointer to the target device to be attached
 *
 * @retval  0 if successful
 * @retval -EINVAL if a structure is found NULL
 * @retval -EBUSY if no I3C address slot is free
 */
static int32_t attach_i3c_target(const struct device *dev,
				struct dw_target_info *target_dev)
{
	const struct dw_i3c_config *config = DEV_CONFIG(dev);
	struct dw_i3c_data *i3c = DEV_DATA(dev);
	int32_t status, pos;

	if (!dev || !target_dev) {
		return -EINVAL;
	}

	if (!target_dev->dev.static_addr && !target_dev->dev.dynamic_addr) {
		return 0;
	}

	if (target_dev->dev.static_addr) {
		status = i3c_addr_slots_status(&i3c->base.addrslots,
						      target_dev->dev.static_addr);
		if (status != I3C_ADDR_SLOT_STATUS_FREE) {
			return -EBUSY;
		}
		i3c_addr_slots_set(&i3c->base.addrslots,
				   target_dev->dev.static_addr,
				   I3C_ADDR_SLOT_STATUS_I3C_DEV);
	}

	if (target_dev->dev.dynamic_addr) {
		status = i3c_addr_slots_status(&i3c->base.addrslots,
						      target_dev->dev.dynamic_addr);
		if (status != I3C_ADDR_SLOT_STATUS_FREE) {
			return -EBUSY;
		}
		i3c_addr_slots_set(&i3c->base.addrslots,
				   target_dev->dev.dynamic_addr,
				   I3C_ADDR_SLOT_STATUS_I3C_DEV);
	}

	pos = get_free_pos(i3c->free_pos);
	if (pos < 0) {
		return pos;
	}

	target_dev->index = pos;
	i3c->free_pos &= ~BIT(pos);
	i3c->base.addrs[pos] = target_dev->dev.dynamic_addr ?
			       : target_dev->dev.static_addr;
	sys_write32(DEV_ADDR_TABLE_DYNAMIC_ADDR(i3c->base.addrs[pos]),
		    config->regs + DEV_ADDR_TABLE_LOC(i3c->datstartaddr, pos));

	return 0;
}

/**
 * @brief Check parity
 *
 * @param p: data
 *
 * @retval 0 if input is odd
 * @retval 1 if input is even
 */
static uint8_t odd_parity(uint8_t p)
{
	p ^= p >> 4;
	p &= 0xf;

	return (0x9669 >> p) & 1;
}

/**
 * @brief Send CCC command to get maximum data speed
 *
 * @param dev: Pointer to the device structure
 * @param info: Pointer to structure containing target device info
 *
 * @retval  0 if successful
 * @retval -EINVAL if a structure is found NULL
 * @retval  Otherwise a value linked to transfer errors as per I3C specs
 */
static int32_t i3c_controller_getmxds(const struct device *dev,
				  struct i3c_device_desc *info)
{
	union i3c_ccc_getmxds *getmaxds;
	struct i3c_ccc_payload cmd;
	int32_t ret;

	i3c_ccc_cmd_init(&cmd, info->dynamic_addr, &getmaxds,
			 sizeof(getmaxds), true, I3C_CCC_GETMXDS);
	ret = dw_i3c_send_ccc_cmd(dev, &cmd);
	if (ret) {
		return ret;
	}

	if (cmd.ccc.data_len != I3C_GETMXDS_FORMAT_1_LEN
	    && cmd.ccc.data_len != I3C_GETMXDS_FORMAT_2_LEN) {
		return -EINVAL;
	}

	info->data_speed.maxrd = getmaxds->fmt2.maxrd;
	info->data_speed.maxwr = getmaxds->fmt2.maxwr;
	if (cmd.ccc.data_len == I3C_GETMXDS_FORMAT_2_LEN) {
		info->data_speed.max_read_turnaround =
				(getmaxds->fmt2.maxrdturn[0] |
				((uint32_t)getmaxds->fmt2.maxrdturn[1] << 8) |
				((uint32_t)getmaxds->fmt2.maxrdturn[2] << 16));
	}

	return 0;
}

/**
 * @brief Retrieve the device information
 *
 * @param dev: Pointer to the device structure
 * @param info: Pointer to structure containing target device info
 * @param flag: Flag to check if BCR and DCR needs to be checked
 *
 * @retval  0 if successful
 * @retval -EINVAL if a structure/pointer is found NULL
 * @retval  Otherwise a value linked to cmd transfer errors as per I3C specs
 */
int32_t i3c_controller_retrieve_dev_info(const struct device *dev,
					 struct i3c_device_desc *info,
					 int8_t flag)
{
	int8_t ret;
	enum i3c_addr_slot_status slot_status;
	struct dw_i3c_data *i3c = DEV_DATA(dev);
	struct i3c_ccc_getpid getpid;
	struct i3c_ccc_getbcr getbcr;
	struct i3c_ccc_getdcr getdcr;

	if (!info->dynamic_addr) {
		return -EINVAL;
	}

	slot_status = i3c_addr_slots_status(&i3c->base.addrslots,
			info->dynamic_addr);
	if (slot_status == I3C_ADDR_SLOT_STATUS_RSVD ||
			slot_status == I3C_ADDR_SLOT_STATUS_I2C_DEV) {
		return -EINVAL;
	}

	if (flag) {

		ret = i3c_ccc_do_getpid(info, &getpid);
		if (ret) {
			return ret;
		}
		info->pid_h = (getpid.pid[5] << 8) | getpid.pid[4];
		info->pid_l = ((getpid.pid[3] << 24) | (getpid.pid[2] << 16) |
				(getpid.pid[1] << 8) | getpid.pid[0]);

		ret = i3c_ccc_do_getbcr(info, &getbcr);
		if (ret) {
			return ret;
		}

		ret = i3c_ccc_do_getdcr(info, &getdcr);
		if (ret) {
			return ret;
		}
	}

	if (info->bcr & I3C_BCR_MAX_DATA_SPEED_LIMIT) {
		ret = i3c_controller_getmxds(dev, info);
		if (ret) {
			return ret;
		}
	}

	return 0;
}

/**
 * @brief Add a target device to list of devices
 *
 * @param dev: Pointer to the device structure
 * @param pos: Position of the target to be added
 *
 * @retval  0 if successful
 * @retval -EINVAL if a structure/pointer is found NULL
 * @retval  Otherwise a value linked to cmd transfer errors as per I3C specs
 */
static int32_t add_to_dev_tbl(const struct device *dev, int32_t pos)
{
	const struct dw_i3c_config *config = DEV_CONFIG(dev);
	struct dw_i3c_data *i3c = DEV_DATA(dev);
	struct dw_target_info *target_dev = NULL;
	uint32_t reg_val;
	int8_t i, ret;

	for (i = 0; i < config->max_slv_devs; i++) {
		if (i3c->target[i].dev_type == 0) {
			i3c->target[i].dev_type = I3C_SLAVE;
			target_dev = &i3c->target[i];
			break;
		}
	}

	if (!target_dev) {
		LOG_ERR("%s Too many targets\n", __func__);
		return -EINVAL;
	}

	target_dev->dev.dynamic_addr = i3c->base.addrs[pos];

	ret = attach_i3c_target(dev, target_dev);
	if (ret) {
		LOG_ERR("%s Attach I3C target failed\n", __func__);
		return -EINVAL;
	}

	reg_val = sys_read32(config->regs +
			     DEV_CHAR_TABLE_LOC1(i3c->dctstartaddr, pos));
	target_dev->dev.pid_h = DEV_CHAR_TABLE_MSB_PID(reg_val) >> 16;
	target_dev->dev.pid_l = DEV_CHAR_TABLE_LSB_PID(reg_val) << 16;
	reg_val = sys_read32(config->regs +
			     DEV_CHAR_TABLE_LOC2(i3c->dctstartaddr, pos));
	target_dev->dev.pid_l |= DEV_CHAR_TABLE_LSB_PID(reg_val);

	reg_val = sys_read32(config->regs +
			     DEV_CHAR_TABLE_LOC3(i3c->dctstartaddr, pos));
	target_dev->dev.bcr = DEV_CHAR_TABLE_BCR(reg_val);
	target_dev->dev.dcr = DEV_CHAR_TABLE_DCR(reg_val);

	ret = i3c_controller_retrieve_dev_info(dev, &target_dev->dev, 0);
	if (ret) {
		LOG_ERR("%s Get target info error\n", __func__);
		return ret;
	}

	return 0;
}

/**
 * @brief Reset the DAA
 *
 * @param controller: Pointer to the controller device
 * @param addr: Target broadcast address
 *
 * @retval  0 if successful
 * @retval -EINVAL if a structure/pointer is found NULL
 * @retval  Otherwise a value linked to cmd transfer errors as per I3C specs
 */
int32_t i3c_controller_rstdaa(const struct device *dev, uint8_t addr)
{
	enum i3c_addr_slot_status addrstat;
	struct dw_i3c_data *i3c = DEV_DATA(dev);
	struct i3c_ccc_payload cmd;

	if (!dev) {
		return -EINVAL;
	}

	addrstat = i3c_addr_slots_status(&i3c->base.addrslots, addr);
	if (addr != I3C_BROADCAST_ADDR
	    && addrstat != I3C_ADDR_SLOT_STATUS_I3C_DEV) {
		return -EINVAL;
	}

	i3c_ccc_cmd_init(&cmd, addr, NULL, 0, 0, I3C_CCC_RSTDAA);
	return dw_i3c_send_ccc_cmd(dev, &cmd);
}

/**
 * @brief Do the DAA procedure
 *
 * @param dev: Pointer to the device structure
 *
 * @retval  0 if successful
 * @retval -EINVAL if a structure/pointer is found NULL
 * @retval -ENOMEM if free address is not found
 * @retval  Otherwise semaphore/mutex acquiring failed
 */
static int32_t do_daa(const struct device *dev)
{
	const struct dw_i3c_config *config = DEV_CONFIG(dev);
	struct dw_i3c_data *i3c = DEV_DATA(dev);
	struct dw_i3c_xfer *xfer = &i3c->xfer;
	struct dw_i3c_cmd *cmd;
	uint32_t prev_devs, new_devs;
	uint8_t p, last_addr = 0;
	int32_t pos, addr, ret;

	prev_devs = ~(i3c->free_pos);

	/* Prepare DAT before launching DAA */
	for (pos = 0; pos < config->max_slv_devs; pos++) {
		if (prev_devs & BIT(pos)) {
			continue;
		}

		addr = i3c_addr_slots_next_free_find(&i3c->base.addrslots);
		if (addr < 0) {
			return addr;
		}
		i3c->base.addrs[pos] = addr;
		p = odd_parity(addr);
		last_addr = addr;
		addr |= (p << 7);
		sys_write32(DEV_ADDR_TABLE_DYNAMIC_ADDR(addr), config->regs +
			    DEV_ADDR_TABLE_LOC(i3c->datstartaddr, pos));
	}

	pos = get_free_pos(i3c->free_pos);

	ret = k_mutex_lock(&i3c->mt, K_MSEC(1000));
	if (ret) {
		LOG_ERR("%s Mutex timeout\n", __func__);
		return ret;
	}
	memset(xfer, 0, sizeof(struct dw_i3c_xfer));

	xfer->ncmds = 1;
	xfer->ret = -1;

	cmd = &xfer->cmds[0];
	cmd->cmd_hi = COMMAND_PORT_TRANSFER_ARG;
	cmd->cmd_lo = COMMAND_PORT_TOC | COMMAND_PORT_ROC |
		      COMMAND_PORT_DEV_COUNT(config->max_slv_devs - pos) |
		      COMMAND_PORT_DEV_INDEX(pos) |
		      COMMAND_PORT_CMD(I3C_CCC_ENTDAA) |
		      COMMAND_PORT_ADDR_ASSGN_CMD;

	init_xfer(dev);

	ret = k_sem_take(&i3c->sem_xfer, K_MSEC(1000));
	if (ret) {
		LOG_ERR("%s Semaphore timeout\n", __func__);
		k_mutex_unlock(&i3c->mt);
		return ret;
	}

	k_mutex_unlock(&i3c->mt);

	if (config->max_slv_devs == cmd->rx_len) {
		new_devs = 0;
	} else {
		new_devs = GENMASK(config->max_slv_devs - cmd->rx_len - 1, 0);
	}
	new_devs &= ~prev_devs;

	for (pos = 0; pos < config->max_slv_devs; pos++) {
		if (new_devs & BIT(pos)) {
			add_to_dev_tbl(dev, pos);
		}
	}

	/* send Disable event CCC : disable all target event */
	uint8_t events = I3C_CCC_EVENT_SIR | I3C_CCC_EVENT_MR | I3C_CCC_EVENT_HJ;

	i3c_controller_en_dis_evt(dev, I3C_BROADCAST_ADDR, events, false);

	return 0;
}

/**
 * @brief Detaches the target devices from device addr table
 *
 * @param dev: Pointer to the device structure
 * @param target_dev: Pointer to the target device structure
 *
 * @retval  0 if successful
 * @retval -EINVAL if a structure/pointer is found NULL
 *	    or if address status is not as expected
 */
static int32_t detach_i3c_target(const struct device *dev,
				struct dw_target_info *target_dev)
{
	if (!dev || !target_dev) {
		return -EINVAL;
	}

	if (!target_dev->dev.static_addr && !target_dev->dev.dynamic_addr) {
		return 0;
	}

	const struct dw_i3c_config *config = DEV_CONFIG(dev);
	struct dw_i3c_data         *i3c    = DEV_DATA(dev);
	int32_t                   pos;
	int32_t status;

	if (target_dev->dev.static_addr) {
		status = i3c_addr_slots_status(&i3c->base.addrslots,
				target_dev->dev.static_addr);
		if (status != I3C_ADDR_SLOT_STATUS_I3C_DEV) {
			return -EINVAL;
		}
		i3c_addr_slots_set(&i3c->base.addrslots,
				target_dev->dev.static_addr,
				I3C_ADDR_SLOT_STATUS_FREE);
	}

	if (target_dev->dev.dynamic_addr) {
		status = i3c_addr_slots_status(&i3c->base.addrslots,
				target_dev->dev.dynamic_addr);
		if (status != I3C_ADDR_SLOT_STATUS_I3C_DEV) {
			return -EINVAL;
		}
		i3c_addr_slots_set(&i3c->base.addrslots,
				target_dev->dev.dynamic_addr,
				I3C_ADDR_SLOT_STATUS_FREE);
		target_dev->dev.dynamic_addr = 0;
	}

	pos = target_dev->index;
	i3c->free_pos |= BIT(pos);
	sys_write32(0, config->regs + DEV_ADDR_TABLE_LOC(i3c->datstartaddr, pos));

	return 0;
}

/**
 * @brief Does Dynamic addressing of targets
 *
 * @param dev: Pointer to the device structure
 *
 * @retval  0 if successful
 * @retval -EINVAL if a structure/pointer is found NULL
 *	    or if address status is not as expected
 * @retval -ENOMEM if free address is not found
 * @retval  Otherwise semaphore/mutex acquiring failed
 */
static int32_t dw_i3c_do_daa(const struct device *dev)
{
	const struct dw_i3c_config *config = DEV_CONFIG(dev);
	struct dw_i3c_data         *i3c    = DEV_DATA(dev);
	int32_t ret = -1;

	/* send RSTDAA : reset all dynamic address */
	i3c_controller_rstdaa(dev, I3C_BROADCAST_ADDR);

	/* send Disbale Event Cmd : disable all target event before starting DAA */
	i3c_controller_en_dis_evt(dev, I3C_BROADCAST_ADDR, I3C_CCC_EVENT_SIR |
			I3C_CCC_EVENT_MR | I3C_CCC_EVENT_HJ, false);

	/* Detach if any, before re-addressing */
	for (int8_t i = 0; i < config->max_slv_devs; i++) {
		ret = detach_i3c_target(dev, &i3c->target[i]);
		if (ret) {
			return ret;
		}
		i3c->target[i].dev_type = 0;
	}
	/* start DAA */
	ret = do_daa(dev);
	if (ret) {
		return ret;
	}

	return 0;
}

/**
 * @brief Define the mode of I3C bus
 *
 * @param dev: Pointer to the device structure
 *
 * @retval  0 if successful
 * @retval -EINVAL if there exist LVR related error
 */
static int32_t define_i3c_bus_mode(const struct device *dev)
{
	const struct dw_i3c_config *config = DEV_CONFIG(dev);
	struct dw_i3c_data *i3c = DEV_DATA(dev);
	int8_t i;

	i3c->base.mode = I3C_BUS_MODE_PURE;

	for (i = 0; i < config->max_slv_devs; i++) {
		if (i3c->target[i].dev_type != I2C_SLAVE) {
			continue;
		}

		switch (i3c->target[i].lvr & I3C_LVR_I2C_INDEX_MASK) {
		case I3C_LVR_I2C_INDEX(0):
			if (i3c->base.mode < I3C_BUS_MODE_MIXED_FAST) {
				i3c->base.mode = I3C_BUS_MODE_MIXED_FAST;
			}
			break;
		case I3C_LVR_I2C_INDEX(1):
			if (i3c->base.mode < I3C_BUS_MODE_MIXED_LIMITED) {
				i3c->base.mode = I3C_BUS_MODE_MIXED_LIMITED;
			}
			break;
		case I3C_LVR_I2C_INDEX(2):
			if (i3c->base.mode < I3C_BUS_MODE_MIXED_SLOW) {
				i3c->base.mode = I3C_BUS_MODE_MIXED_SLOW;
			}
			break;
		default:
			LOG_ERR("%s I2C Slave device(%d) LVR error\n",
				__func__, i);
			return -EINVAL;
		}
	}
	return 0;
}

/**
 * @brief Initialize the SCL timing
 *
 * @param dev: Pointer to the device structure
 */
static void dw_init_scl_timing(const struct device *dev)
{
	const struct dw_i3c_config *config = DEV_CONFIG(dev);
	struct dw_i3c_data *i3c = DEV_DATA(dev);
	uint32_t scl_timing, core_rate, core_period;
	uint16_t hcnt, lcnt;

	core_rate = config->core_clk;
	core_period = DIV_ROUND_UP(I3C_PERIOD_NS, core_rate);

	/* I3C_PP */
	hcnt = DIV_ROUND_UP(I3C_BUS_THIGH_MAX_NS, core_period) - 1;
	if (hcnt < SCL_I3C_TIMING_CNT_MIN) {
		hcnt = SCL_I3C_TIMING_CNT_MIN;
	}
	lcnt = DIV_ROUND_UP(core_rate, I3C_BUS_TYP_I3C_SCL_RATE) - hcnt;
	if (lcnt < SCL_I3C_TIMING_CNT_MIN) {
		lcnt = SCL_I3C_TIMING_CNT_MIN;
	}
	scl_timing = SCL_I3C_TIMING_HCNT(hcnt) | SCL_I3C_TIMING_LCNT(lcnt);
	sys_write32(scl_timing, config->regs + SCL_I3C_PP_TIMING);

	sys_write32(BUS_I3C_MST_FREE(lcnt), config->regs + BUS_FREE_TIMING);

	/* I3C_OD */
	lcnt = DIV_ROUND_UP(I3C_BUS_TLOW_OD_MIN_NS, core_period);
	scl_timing = SCL_I3C_TIMING_HCNT(hcnt) | SCL_I3C_TIMING_LCNT(lcnt);
	sys_write32(scl_timing, config->regs + SCL_I3C_OD_TIMING);

	/* I3C */
	lcnt = DIV_ROUND_UP(core_rate, I3C_BUS_SDR1_SCL_RATE) - hcnt;
	scl_timing = SCL_EXT_LCNT_1(lcnt);
	lcnt = DIV_ROUND_UP(core_rate, I3C_BUS_SDR2_SCL_RATE) - hcnt;
	scl_timing |= SCL_EXT_LCNT_2(lcnt);
	lcnt = DIV_ROUND_UP(core_rate, I3C_BUS_SDR3_SCL_RATE) - hcnt;
	scl_timing |= SCL_EXT_LCNT_3(lcnt);
	lcnt = DIV_ROUND_UP(core_rate, I3C_BUS_SDR4_SCL_RATE) - hcnt;
	scl_timing |= SCL_EXT_LCNT_4(lcnt);
	sys_write32(scl_timing, config->regs + SCL_EXT_LCNT_TIMING);

	/* I2C FM+ */
	lcnt = DIV_ROUND_UP(I3C_BUS_I2C_FMP_TLOW_MIN_NS, core_period);
	hcnt = DIV_ROUND_UP(core_rate, I3C_BUS_I2C_FM_PLUS_SCL_RATE) - lcnt;
	scl_timing = SCL_I2C_FMP_TIMING_HCNT(hcnt) | SCL_I2C_FMP_TIMING_LCNT(lcnt);
	sys_write32(scl_timing, config->regs + SCL_I2C_FMP_TIMING);

	/* I2C FM */
	lcnt = DIV_ROUND_UP(I3C_BUS_I2C_FM_TLOW_MIN_NS, core_period);
	hcnt = DIV_ROUND_UP(core_rate, I3C_BUS_I2C_FM_SCL_RATE) - lcnt;
	scl_timing = SCL_I2C_FM_TIMING_HCNT(hcnt) | SCL_I2C_FM_TIMING_LCNT(lcnt);
	sys_write32(scl_timing, config->regs + SCL_I2C_FM_TIMING);

	if (i3c->base.mode != I3C_BUS_MODE_PURE) {
		sys_write32(BUS_I3C_MST_FREE(lcnt),
			    config->regs + BUS_FREE_TIMING);
		sys_write32(sys_read32(DEV_CTRL(config)) |
			    DEV_CTRL_I2C_SLAVE_PRESENT, DEV_CTRL(config));
	}
}

/**
 * @brief Configure the controller address
 *
 * @param dev: Pointer to the device structure
 *
 * @retval  0 if successful
 * @retval -EINVAL if a structure/pointer is found NULL
 * @retval -ENOMEM if no such free address is found
 * @retval -EBUSY if no I3C address slot is free
 */
static int32_t controller_addr_config(const struct device *dev)
{
	const struct dw_i3c_config *config = DEV_CONFIG(dev);
	struct dw_i3c_data *i3c = DEV_DATA(dev);
	int32_t addr;

	addr = i3c_addr_slots_next_free_find(&i3c->base.addrslots);
	if (addr < 0) {
		return addr;
	}

	if (i3c_addr_slots_status(&i3c->base.addrslots, addr)
	    != I3C_ADDR_SLOT_STATUS_FREE) {
		return -EBUSY;
	}
	i3c_addr_slots_set(&i3c->base.addrslots, addr,
				     I3C_ADDR_SLOT_STATUS_I3C_DEV);

	sys_write32(DEV_ADDR_DYNAMIC_ADDR_VALID | DEV_ADDR_DYNAMIC(addr),
		    config->regs + DEVICE_ADDR);

	return 0;
}

/**
 * @brief Enable interrupt signal
 *
 * @param dev: Pointer to the device structure
 */
static void en_inter_sig_stat(const struct device *dev)
{
	const struct dw_i3c_config *config = DEV_CONFIG(dev);
	uint32_t thld_ctrl;

	config->configure_irq();

	thld_ctrl = sys_read32(config->regs + QUEUE_THLD_CTRL);
	thld_ctrl &= (~QUEUE_THLD_CTRL_RESP_BUF_MASK
		      & ~QUEUE_THLD_CTRL_IBI_STS_MASK);
	sys_write32(thld_ctrl, config->regs + QUEUE_THLD_CTRL);

	/* Set Data Threshold level */
	thld_ctrl = sys_read32(config->regs + DATA_BUFFER_THLD_CTRL);
	thld_ctrl &= ~DATA_BUFFER_THLD_CTRL_RX_BUF;
	sys_write32(thld_ctrl, config->regs + DATA_BUFFER_THLD_CTRL);

	/* Clearing any previous interrupts, if any */
	sys_write32(INTR_ALL, config->regs + INTR_STATUS);

	sys_write32(INTR_CONTROLLER_MASK, config->regs + INTR_STATUS_EN);
	sys_write32(INTR_CONTROLLER_MASK, config->regs + INTR_SIGNAL_EN);
}

/**
 * @brief DesignWare I3C init function
 *
 * @param dev: Pointer to the device structure
 *
 * @retval  0 if successful
 * @retval -EINVAL if a structure/pointer is found NULL
 *	    or if there exist LVR related error
 * @retval -ENOTSUP if targets no. exceeds more than supported
 * @retval -ENOMEM if no such free address is found
 * @retval -EBUSY if no I3C address slot is free
 */
static int32_t i3c_dw_init(const struct device *dev)
{
	const struct dw_i3c_config *config = DEV_CONFIG(dev);
	struct dw_i3c_data *i3c = DEV_DATA(dev);
	int32_t ret;

	if (dev == NULL) {
		LOG_ERR("%s Device ptr null !!\n", __func__);
		return -EINVAL;
	}

	if (config->max_slv_devs > 4) {
		LOG_ERR("%s Max target dev count exceeded !\n", __func__);
		return -ENOTSUP;
	}

	/* Allocate Memory */
	i3c->base.addrs = (uint8_t *)k_malloc(config->max_slv_devs *
			   sizeof(uint8_t));
	i3c->target = (struct dw_target_info *)k_malloc(config->max_slv_devs
		      * sizeof(struct dw_target_info));
	i3c->base.dev = dev;
	i3c->base.ops = dev->api;

	/* Initializing feasible address slots for DAA */
	ret = i3c_addr_slots_init(&i3c->base.addrslots, &config->device_list);
	if (ret != 0) {
		return -ENOMEM;
	}

	/* I3C pure, I2C pure or Mix */
	ret = define_i3c_bus_mode(dev);
	if (ret) {
		return ret;
	}

	/* reset all */
	sys_write32(RESET_CTRL_ALL, config->regs + RESET_CTRL);

	/* get DAT, DCT pointer */
	i3c->datstartaddr = DEVICE_ADDR_TABLE_ADDR(sys_read32(config->regs +
			    DEVICE_ADDR_TABLE_POINTER));
	i3c->dctstartaddr = DEVICE_CHAR_TABLE_ADDR(sys_read32(config->regs +
			    DEV_CHAR_TABLE_POINTER));

	/* free positions available among available slots */
	i3c->free_pos = GENMASK(config->max_slv_devs - 1, 0);

	/* Config timing registers */
	dw_init_scl_timing(dev);

	/* Self-assigning a dynamic address */
	ret = controller_addr_config(dev);
	if (ret) {
		return ret;
	}

	/* Enable Interrupt signals */
	en_inter_sig_stat(dev);

	/* disable IBI */
	sys_write32(IBI_REQ_REJECT_ALL, config->regs + IBI_SIR_REQ_REJECT);
	sys_write32(IBI_REQ_REJECT_ALL, config->regs + IBI_MR_REQ_REJECT);

	/* disable hot-join */
	sys_write32(sys_read32(DEV_CTRL(config)) | DEV_CTRL_HOT_JOIN_NACK,
			DEV_CTRL(config));

	/* enable I3C controller */
	sys_write32(sys_read32(DEV_CTRL(config)) | DEV_CTRL_ENABLE,
			DEV_CTRL(config));

	k_sem_init(&i3c->sem_xfer, 0, 1);
	k_mutex_init(&i3c->mt);
	return 0;
}

static const struct i3c_driver_api i3c_dw_api = {
	.do_daa = dw_i3c_do_daa,
	.i3c_xfers = dw_i3c_xfers,
	.do_ccc = dw_i3c_send_ccc_cmd,
	.ibi_enable = dw_i3c_enable_ibi,
	.ibi_disable = dw_i3c_disable_ibi,
};

#define I3C_DW_IRQ_HANDLER(n)				\
	static void i3c_dw_irq_config_##n(void)		\
{							\
	IRQ_CONNECT(DT_INST_IRQN(n),			\
			DT_INST_IRQ(n, priority),	\
			i3c_dw_irq,			\
			DEVICE_DT_INST_GET(n),		\
			0);				\
	irq_enable(DT_INST_IRQN(n));			\
}

#define GET_I3C_SLAVE(id) {						\
	.dev.static_addr	= DT_REG_ADDR(id),			\
	.dev.dyn_addr		= 0,					\
	.dev.pid_h		= DT_PROP_BY_IDX(id, pid, 0),		\
	.dev.pid_l		= DT_PROP_BY_IDX(id, pid, 1),		\
	.assigned_dyn_addr	= DT_PROP(id, assigned_address),	\
	.lvr			= DT_PROP(id, lvr),			\
	.dev_type		= DT_PROP(id, type),			\
},

#define I3C_DW_DEVICE(n)						\
	I3C_DW_IRQ_HANDLER(n)						\
	static struct dw_i3c_data i3c_dw_data_##n = {			\
		DT_FOREACH_CHILD(DT_DRV_INST(n), GET_I3C_SLAVE)		\
	};								\
	static struct dw_i3c_config i3c_dw_cfg_##n = {			\
		.regs		= DT_INST_REG_ADDR(n),			\
		.core_clk	= DT_INST_PROP(n, core_clk),		\
		.cmdfifodepth	= DT_INST_PROP(n, cmd_queue_size),	\
		.datafifodepth	= DT_INST_PROP(n, data_fifo_size),	\
		.max_slv_devs	= DT_INST_PROP(n, max_devices),		\
		.configure_irq = &i3c_dw_irq_config_##n,		\
	};								\
	I3C_DEVICE_DT_INST_DEFINE(n,					\
			&i3c_dw_init,					\
			NULL,						\
			&i3c_dw_data_##n,				\
			&i3c_dw_cfg_##n,				\
			POST_KERNEL,					\
			CONFIG_I3C_CONTROLLER_INIT_PRIORITY,		\
			&i3c_dw_api);

DT_INST_FOREACH_STATUS_OKAY(I3C_DW_DEVICE)
