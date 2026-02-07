/*
 * Copyright (c) 2025 - 2026 Siemens Mobility GmbH
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT cdns_mspi_controller

#include "mspi_cadence.h"

#include <zephyr/arch/arm/cortex_a_r/sys_io.h>
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/drivers/mspi.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/__assert.h>
#include <zephyr/sys/device_mmio.h>
#include <zephyr/sys/sys_io.h>
#include <zephyr/sys/time_units.h>
#include <zephyr/sys/util.h>
#include <zephyr/sys/util_macro.h>

LOG_MODULE_REGISTER(mspi_cadence, CONFIG_MSPI_LOG_LEVEL);

struct mspi_cadence_config {
	DEVICE_MMIO_ROM;
	struct mspi_cfg mspi_config;
	const struct pinctrl_dev_config *pinctrl;
	const uint32_t fifo_addr;
	const uint32_t sram_allocated_for_read;

	const struct mspi_cadence_timing_cfg initial_timing_cfg;
};

struct mspi_cadence_data {
	DEVICE_MMIO_RAM;
	struct k_mutex access_lock;
	struct k_sem transfer_lock;
	const struct mspi_dev_id *current_peripheral;
};

/**
 * Wait for the MSPI controller to enter idle with the default timeout
 */
int mspi_cadence_wait_for_idle(const struct device *controller)
{
	const mem_addr_t base_addr = DEVICE_MMIO_GET(controller);
	uint32_t config_reg = sys_read32(base_addr + CADENCE_MSPI_CONFIG_OFFSET);
	uint32_t idle = config_reg & CADENCE_MSPI_CONFIG_REG_IDLE_BIT;
	uint32_t retries = CADENCE_MSPI_GET_NUM_RETRIES(CONFIG_MSPI_COMPLETION_TIMEOUT_TOLERANCE);

	while (idle == 0 && retries > 0) {
		k_sleep(CADENCE_MSPI_TIME_BETWEEN_RETRIES);
		config_reg = sys_read32(base_addr + CADENCE_MSPI_CONFIG_OFFSET);
		idle = config_reg & CADENCE_MSPI_CONFIG_REG_IDLE_BIT;
		--retries;
	}
	if (retries == 0) {
		LOG_ERR("Timeout while waiting for MSPI to enter idle");
		return -EIO;
	}
	return 0;
}

/**
 * Check whether a single request package is requesting something that the driver
 * doesn't implement / the hardware doesn't support
 */
static int mspi_cadence_check_transfer_package(const struct mspi_xfer *request, uint32_t index)
{
	const struct mspi_xfer_packet *packet = &request->packets[index];
	/* check that we won't truncate the address */
	if (packet->address >> (8 * request->addr_length)) {
		LOG_ERR("Address too long for amount of address bytes");
		return -EINVAL;
	}
	if (packet->cb_mask != MSPI_BUS_NO_CB) {
		LOG_ERR("Callbacks aren't implemented");
		return -ENOSYS;
	}
	if (packet->cmd >> 16) {
		LOG_ERR("Commands over 2 byte long aren't supported");
		return -ENOTSUP;
	}
	if (packet->cmd >> 8) {
		LOG_ERR("Support for dual byte opcodes hasn't been implemented");
		return -ENOSYS;
	}
	if (packet->num_bytes) {
		__ASSERT(packet->data_buf != NULL,
			 "Request gave a NULL buffer when bytes should be transfererd");
	}
	return 0;
}

/**
 * Check whether a full request has invalid / not supported parts
 */
static int mspi_cadence_check_transfer_request(const struct mspi_xfer *request)
{
	if (request->async) {
		LOG_ERR("Asynchronous requests are not implemented");
		return -ENOSYS;
	}

	if (request->cmd_length == 2) {
		LOG_ERR("Dual byte opcode is not implemented");
		return -ENOSYS;
	} else if (request->cmd_length > 2) {
		LOG_ERR("Cmds over 2 bytes long aren't supported");
		return -ENOTSUP;
	} else if (request->cmd_length != 1) {
		LOG_ERR("Can't handle transfer without cmd");
		return -ENOSYS;
	}

	if (request->addr_length > 4) {
		LOG_ERR("Address too long. Only up to 32 bit are supported");
		return -ENOTSUP;
	}

	if (request->priority != 0) {
		LOG_WRN("Ignoring request to give transfer higher priority");
	}

	if (request->num_packet == 0) {
		LOG_ERR("Got transfer requests without packages");
		return -EINVAL;
	}
	__ASSERT(request->packets != NULL, "Packets in transfer request are NULL");

	if (request->xfer_mode != MSPI_PIO) {
		LOG_ERR("Other modes than PIO are not supported");
		return -ENOTSUP;
	}

	if (request->rx_dummy > CADENCE_MSPI_INSTR_RD_CONFIG_REG_DUMMY_RD_CLK_CYCLES_MAX_VALUE ||
	    request->tx_dummy > CADENCE_MSPI_INSTR_WR_CONFIG_REG_DUMMY_WR_CLK_CYCLES_MAX_VALUE) {
		return -ENOTSUP;
	}

	int ret = 0;

	for (uint32_t i = 0; i < request->num_packet; ++i) {
		ret = mspi_cadence_check_transfer_package(request, i);
		if (ret < 0) {
			return ret;
		}
	}

	return 0;
}

static int mspi_cadence_init(const struct device *dev)
{
	DEVICE_MMIO_MAP(dev, K_MEM_CACHE_NONE);
	const struct mspi_cadence_config *config = dev->config;
	const struct mspi_cadence_timing_cfg *timing_config = &config->initial_timing_cfg;
	struct mspi_cadence_data *data = dev->data;
	const mem_addr_t base_addr = DEVICE_MMIO_GET(dev);
	int ret;

	k_mutex_init(&data->access_lock);
	k_sem_init(&data->transfer_lock, 1, 1);

	ret = pinctrl_apply_state(config->pinctrl, PINCTRL_STATE_DEFAULT);
	if (ret < 0) {
		LOG_ERR("Failed to apply pinctrl");
		return ret;
	}

	/* Disable MSPI */
	uint32_t config_reg = sys_read32(base_addr + CADENCE_MSPI_CONFIG_OFFSET);

	config_reg &= ~CADENCE_MSPI_CONFIG_REG_ENABLE_SPI_BIT;

	sys_write32(config_reg, base_addr + CADENCE_MSPI_CONFIG_OFFSET);

	ret = mspi_cadence_wait_for_idle(dev);
	if (ret < 0) {
		return ret;
	}

	config_reg = sys_read32(base_addr + CADENCE_MSPI_CONFIG_OFFSET);

	/* Disable direct access the driver always uses indirect accesses */
	config_reg &= ~CADENCE_MSPI_CONFIG_REG_ENB_DIR_ACC_CTRL_BIT;

	/* Disable DTR protocol */
	config_reg &= ~CADENCE_MSPI_CONFIG_REG_ENABLE_DTR_PROTOCOL_BIT;

	/* Leave XIP mode */
	config_reg &= ~CADENCE_MSPI_CONFIG_REG_ENTER_XIP_MODE_BIT;

	/* Only allow one CS to be active */
	config_reg &= ~CADENCE_MSPI_CONFIG_REG_PERIPH_SEL_DEC_BIT;

	/* CS selection is based on "manual" pin selection */
	config_reg &= ~CADENCE_MSPI_CONFIG_REG_ENABLE_AHB_DECODER_BIT;

	/* DQ3 should not be used as reset pin */
	config_reg |= CADENCE_MSPI_CONFIG_REG_RESET_CFG_BIT;

	/* Set baud rate division to 32; formula: (n + 1) * 2 */
	config_reg &= ~CADENCE_MSPI_CONFIG_REG_MSTR_BAUD_DIV_MASK;
	config_reg |= FIELD_PREP(CADENCE_MSPI_CONFIG_REG_MSTR_BAUD_DIV_MASK, 15);

	/* Disable dual byte opcodes */
	config_reg &= ~CADENCE_MSPI_CONFIG_REG_DUAL_BYTE_OPCODE_EN_BIT;

	/* Disable PHY pipeline mode */
	config_reg &= ~CADENCE_MSPI_CONFIG_REG_PIPELINE_PHY_BIT;

	/* Disable PHY module generally */
	config_reg &= ~CADENCE_MSPI_CONFIG_REG_PHY_MODE_ENABLE_BIT;

	/* Disable CRC */
	config_reg &= ~CADENCE_MSPI_CONFIG_REG_CRC_ENABLE_BIT;

	/* Disable DMA generally since it's not supported */
	config_reg &= ~CADENCE_MSPI_CONFIG_REG_ENB_DMA_IF_BIT;

	/* Disable automatic write protection disablement of MSPI peripherals */
	config_reg &= ~CADENCE_MSPI_CONFIG_REG_WR_PROT_FLASH_BIT;

	/* Disable possible reset pin */
	config_reg &= ~CADENCE_MSPI_CONFIG_REG_RESET_PIN_BIT;

	sys_write32(config_reg, base_addr + CADENCE_MSPI_CONFIG_OFFSET);

	/* Set how many FSS0 SRAM locations are allocated for read; the other ones
	 * are allocated for writes
	 */
	uint32_t sram_partition = sys_read32(base_addr + CADENCE_MSPI_SRAM_PARTITION_CFG_OFFSET);

	sram_partition &= ~CADENCE_MSPI_SRAM_PARTITION_CFG_REG_ADDR_MASK;
	sram_partition |= FIELD_PREP(CADENCE_MSPI_SRAM_PARTITION_CFG_REG_ADDR_MASK,
				     config->sram_allocated_for_read);

	sys_write32(sram_partition, base_addr + CADENCE_MSPI_SRAM_PARTITION_CFG_OFFSET);

	/* General clock cycle delays */
	uint32_t timing_config_reg = sys_read32(base_addr + CADENCE_MSPI_DEV_DELAY_OFFSET);

	timing_config_reg &= ~CADENCE_MSPI_DEV_DELAY_REG_D_NSS_MASK;
	timing_config_reg |= FIELD_PREP(CADENCE_MSPI_DEV_DELAY_REG_D_NSS_MASK, timing_config->nss);

	timing_config_reg &= ~CADENCE_MSPI_DEV_DELAY_REG_D_BTWN_MASK;
	timing_config_reg |=
		FIELD_PREP(CADENCE_MSPI_DEV_DELAY_REG_D_BTWN_MASK, timing_config->btwn);

	timing_config_reg &= ~CADENCE_MSPI_DEV_DELAY_REG_D_AFTER_MASK;
	timing_config_reg |=
		FIELD_PREP(CADENCE_MSPI_DEV_DELAY_REG_D_AFTER_MASK, timing_config->after);

	timing_config_reg &= ~CADENCE_MSPI_DEV_DELAY_REG_D_INIT_MASK;
	timing_config_reg |=
		FIELD_PREP(CADENCE_MSPI_DEV_DELAY_REG_D_INIT_MASK, timing_config->init);

	sys_write32(timing_config_reg, base_addr + CADENCE_MSPI_DEV_DELAY_OFFSET);

	/* Set trigger reg address and range to 0 */
	uint32_t ind_ahb_addr_trigger =
		sys_read32(base_addr + CADENCE_MSPI_IND_AHB_ADDR_TRIGGER_OFFSET);

	ind_ahb_addr_trigger &= ~CADENCE_MSPI_IND_AHB_ADDR_TRIGGER_OFFSET;

	sys_write32(ind_ahb_addr_trigger, base_addr + CADENCE_MSPI_IND_AHB_ADDR_TRIGGER_OFFSET);

	uint32_t indirect_trigger_addr_range =
		sys_read32(base_addr + CADENCE_MSPI_INDIRECT_TRIGGER_ADDR_RANGE_OFFSET);

	indirect_trigger_addr_range &=
		~CADENCE_MSPI_INDIRECT_TRIGGER_ADDR_RANGE_REG_IND_RANGE_WIDTH_MASK;

	sys_write32(indirect_trigger_addr_range,
		    base_addr + CADENCE_MSPI_INDIRECT_TRIGGER_ADDR_RANGE_OFFSET);

	/* Disable loop-back via DQS */
	uint32_t rd_data_capture = sys_read32(base_addr + CADENCE_MSPI_RD_DATA_CAPTURE_OFFSET);

	rd_data_capture |= CADENCE_MSPI_RD_DATA_CAPTURE_REG_BYPASS_BIT;

	sys_write32(rd_data_capture, base_addr + CADENCE_MSPI_RD_DATA_CAPTURE_OFFSET);

	/* Disable auto polling for write completion */
	uint32_t write_completion_ctrl =
		sys_read32(base_addr + CADENCE_MSPI_WRITE_COMPLETION_CTRL_OFFSET);

	write_completion_ctrl |= CADENCE_MSPI_WRITE_COMPLETION_CTRL_REG_DISABLE_POLLING_BIT;

	sys_write32(write_completion_ctrl, base_addr + CADENCE_MSPI_WRITE_COMPLETION_CTRL_OFFSET);

	/* Disable automatic write enable command before indirect write transactions */
	uint32_t device_write = sys_read32(base_addr + CADENCE_MSPI_DEV_INSTR_WR_CONFIG_OFFSET);

	device_write &= ~CADENCE_MSPI_DEV_INSTR_WR_CONFIG_REG_WEL_DIS_BIT;

	sys_write32(device_write, base_addr + CADENCE_MSPI_DEV_INSTR_WR_CONFIG_OFFSET);

	/* Reset mode bit (hardware CRC checking on read, if supported) and disable DDR mode */
	uint32_t device_read = sys_read32(base_addr + CADENCE_MSPI_DEV_INSTR_RD_CONFIG_OFFSET);

	device_read &= ~CADENCE_MSPI_DEV_INSTR_RD_CONFIG_REG_MODE_BIT_ENABLE_BIT;
	device_read &= ~CADENCE_MSPI_DEV_INSTR_RD_CONFIG_REG_DDR_EN_BIT;

	sys_write32(device_read, base_addr + CADENCE_MSPI_DEV_INSTR_RD_CONFIG_OFFSET);

	uint32_t val;

	/* Disable all interrupts via masking */
	val = sys_read32(base_addr + CADENCE_MSPI_IRQ_MASK_OFFSET);
	val &= ~CADENCE_MSPI_IRQ_MASK_REG_ALL;
	sys_write32(val, base_addr + CADENCE_MSPI_IRQ_MASK_OFFSET);

	/* Clear currently pending interrupts */
	val = sys_read32(base_addr + CADENCE_MSPI_IRQ_STATUS_OFFSET);
	val |= CADENCE_MSPI_IRQ_STATUS_REG_ALL;
	sys_write32(val, base_addr + CADENCE_MSPI_IRQ_STATUS_OFFSET);

	/* Re-enable MSPI controller */
	config_reg = sys_read32(base_addr + CADENCE_MSPI_CONFIG_OFFSET);
	config_reg |= CADENCE_MSPI_CONFIG_REG_ENABLE_SPI_BIT;
	sys_write32(config_reg, base_addr + CADENCE_MSPI_CONFIG_OFFSET);

	return 0;
}

static int mspi_cadence_stig(const struct device *controller, const struct mspi_xfer *req,
			     uint32_t index, const uint64_t start_time)
{
	const mem_addr_t base_address = DEVICE_MMIO_GET(controller);
	const struct mspi_xfer_packet *packet = &req->packets[index];
	uint32_t dummy_cycles = 0;

	/* Reset previous command configuration completely */
	sys_write32(0, base_address + CADENCE_MSPI_FLASH_CMD_CTRL_OFFSET);

	uint32_t flash_cmd_ctrl = 0;

	if (packet->dir == MSPI_RX) {
		if (packet->num_bytes != 0) {
			flash_cmd_ctrl |= CADENCE_MSPI_FLASH_CMD_CTRL_REG_ENB_READ_DATA_BIT;
			flash_cmd_ctrl |=
				FIELD_PREP(CADENCE_MSPI_FLASH_CMD_CTRL_REG_NUM_RD_DATA_BYTES_MASK,
					   packet->num_bytes - 1);
		}
		dummy_cycles = req->rx_dummy;
	} else {
		if (packet->num_bytes != 0) {
			flash_cmd_ctrl |= CADENCE_MSPI_FLASH_CMD_CTRL_REG_ENB_WRITE_DATA_BIT;
			flash_cmd_ctrl |=
				FIELD_PREP(CADENCE_MSPI_FLASH_CMD_CTRL_REG_NUM_WR_DATA_BYTES_MASK,
					   packet->num_bytes - 1);

			if (packet->num_bytes > 4) {
				uint32_t upper = 0;

				memcpy(&upper, &packet->data_buf[4], packet->num_bytes - 4);
				sys_write32(upper,
					    base_address + CADENCE_MSPI_FLASH_WR_DATA_UPPER_OFFSET);
			}
			uint32_t lower = 0;

			memcpy(&lower, &packet->data_buf[0], MIN(packet->num_bytes, 4));
			sys_write32(lower, base_address + CADENCE_MSPI_FLASH_WR_DATA_LOWER_OFFSET);
		}
		dummy_cycles = req->tx_dummy;
	}

	flash_cmd_ctrl |= FIELD_PREP(CADENCE_MSPI_FLASH_CMD_CTRL_REG_CMD_OPCODE_MASK, packet->cmd);
	flash_cmd_ctrl |=
		FIELD_PREP(CADENCE_MSPI_FLASH_CMD_CTRL_REG_NUM_DUMMY_CYCLES_MASK, dummy_cycles);

	if (req->addr_length) {
		flash_cmd_ctrl |= CADENCE_MSPI_FLASH_CMD_CTRL_REG_ENB_COMD_ADDR_BIT;
		flash_cmd_ctrl |= FIELD_PREP(CADENCE_MSPI_FLASH_CMD_CTRL_REG_NUM_ADDR_BYTES_MASK,
					     req->addr_length - 1);
		sys_write32(packet->address, base_address + CADENCE_MSPI_FLASH_CMD_ADDR_OFFSET);
	}

	/* Start transaction */
	flash_cmd_ctrl |= CADENCE_MSPI_FLASH_CMD_CTRL_REG_CMD_EXEC_BIT;
	sys_write32(flash_cmd_ctrl, base_address + CADENCE_MSPI_FLASH_CMD_CTRL_OFFSET);

	uint32_t exec_status = sys_read32(base_address + CADENCE_MSPI_FLASH_CMD_CTRL_OFFSET) &
			       CADENCE_MSPI_FLASH_CMD_CTRL_REG_CMD_EXEC_STATUS_BIT;
	while (exec_status != 0 && k_uptime_get() - start_time < req->timeout) {
		k_sleep(CADENCE_MSPI_TIME_BETWEEN_RETRIES);
		exec_status = sys_read32(base_address + CADENCE_MSPI_FLASH_CMD_CTRL_OFFSET) &
			      CADENCE_MSPI_FLASH_CMD_CTRL_REG_CMD_EXEC_STATUS_BIT;
	}
	if (exec_status != 0) {
		LOG_ERR("Timeout while waiting for dedicated command to finish");
		return -EIO;
	}

	if (packet->dir == MSPI_RX) {
		if (packet->num_bytes > 4) {
			uint32_t higher =
				sys_read32(base_address + CADENCE_MSPI_FLASH_RD_DATA_UPPER_OFFSET);

			memcpy(&packet->data_buf[4], &higher, packet->num_bytes - 4);
		}
		uint32_t lower = sys_read32(base_address + CADENCE_MSPI_FLASH_RD_DATA_LOWER_OFFSET);

		memcpy(&packet->data_buf[0], &lower, MIN(packet->num_bytes, 4));
	}

	/*
	 * The STIG register must be reset after the transfer or weird things like
	 * skipping every 2nd byte can occur
	 */
	sys_write32(0, base_address + CADENCE_MSPI_FLASH_CMD_CTRL_OFFSET);

	return 0;
}

static int mspi_cadence_indirect_read(const struct device *controller, const struct mspi_xfer *req,
				      uint32_t index, const uint64_t start_time)
{
	const mem_addr_t base_address = DEVICE_MMIO_GET(controller);
	const struct mspi_cadence_config *config = controller->config;
	const struct mspi_xfer_packet *packet = &req->packets[index];

	uint32_t dev_instr_rd_cfg =
		sys_read32(base_address + CADENCE_MSPI_DEV_INSTR_RD_CONFIG_OFFSET);

	dev_instr_rd_cfg &= ~CADENCE_MSPI_DEV_INSTR_RD_CONFIG_REG_RD_OPCODE_NON_XIP_MASK;
	dev_instr_rd_cfg |= FIELD_PREP(CADENCE_MSPI_DEV_INSTR_RD_CONFIG_REG_RD_OPCODE_NON_XIP_MASK,
				       packet->cmd);
	dev_instr_rd_cfg &= ~CADENCE_MSPI_DEV_INSTR_RD_CONFIG_REG_DUMMY_RD_CLK_CYCLES_MASK;
	dev_instr_rd_cfg |= FIELD_PREP(
		CADENCE_MSPI_DEV_INSTR_RD_CONFIG_REG_DUMMY_RD_CLK_CYCLES_MASK, req->rx_dummy);

	sys_write32(dev_instr_rd_cfg, base_address + CADENCE_MSPI_DEV_INSTR_RD_CONFIG_OFFSET);

	sys_write32(packet->address, base_address + CADENCE_MSPI_INDIRECT_READ_XFER_START_OFFSET);
	sys_write32(packet->num_bytes,
		    base_address + CADENCE_MSPI_INDIRECT_READ_XFER_NUM_BYTES_OFFSET);

	uint32_t dev_size_config = sys_read32(base_address + CADENCE_MSPI_DEV_SIZE_CONFIG_OFFSET);

	dev_size_config &= ~CADENCE_MSPI_DEV_SIZE_CONFIG_REG_NUM_ADDR_BYTES_MASK;
	dev_size_config |= FIELD_PREP(CADENCE_MSPI_DEV_SIZE_CONFIG_REG_NUM_ADDR_BYTES_MASK,
				      req->addr_length - 1);

	sys_write32(dev_size_config, base_address + CADENCE_MSPI_DEV_SIZE_CONFIG_OFFSET);

	/* Start transfer */
	uint32_t indirect_read_ctrl =
		sys_read32(base_address + CADENCE_MSPI_INDIRECT_READ_XFER_CTRL_OFFSET);

	indirect_read_ctrl |= CADENCE_MSPI_INDIRECT_READ_XFER_CTRL_REG_START_BIT;

	sys_write32(indirect_read_ctrl, base_address + CADENCE_MSPI_INDIRECT_READ_XFER_CTRL_OFFSET);

	uint32_t remaining_bytes = packet->num_bytes;
	uint8_t *write_ptr = packet->data_buf;
	uint32_t num_new_words = 0;
	uint32_t current_new_word;
	int bytes_to_copy_from_current_word;

	while (remaining_bytes > 0) {
		if (k_uptime_get() - start_time > req->timeout) {
			LOG_ERR("Timeout while receiving data from MSPI device");
			goto timeout;
		}
		uint32_t indac_read = sys_read32(base_address + CADENCE_MSPI_SRAM_FILL_OFFSET);

		num_new_words = FIELD_GET(CADENCE_MSPI_SRAM_FILL_REG_INDAC_READ_MASK, indac_read);
		while (remaining_bytes > 0 && num_new_words > 0) {
			current_new_word = sys_read32(config->fifo_addr);
			bytes_to_copy_from_current_word = MIN(remaining_bytes, 4);
			memcpy(write_ptr, &current_new_word, bytes_to_copy_from_current_word);
			write_ptr += bytes_to_copy_from_current_word;
			remaining_bytes -= bytes_to_copy_from_current_word;
			--num_new_words;
		}
	}

	/* Wait until official indirect read completion */
	uint32_t done_status =
		sys_read32(base_address + CADENCE_MSPI_INDIRECT_READ_XFER_CTRL_OFFSET) &
		CADENCE_MSPI_INDIRECT_READ_XFER_CTRL_REG_IND_OPS_DONE_STATUS_BIT;
	while (done_status == 0 && k_uptime_get() - start_time < req->timeout) {
		k_sleep(CADENCE_MSPI_TIME_BETWEEN_RETRIES);
		done_status =
			sys_read32(base_address + CADENCE_MSPI_INDIRECT_READ_XFER_CTRL_OFFSET) &
			CADENCE_MSPI_INDIRECT_READ_XFER_CTRL_REG_IND_OPS_DONE_STATUS_BIT;
	}
	if (done_status == 0) {
		LOG_ERR("Timeout waiting for official indirect read done confirmation");
		goto timeout;
	}
	indirect_read_ctrl = sys_read32(base_address + CADENCE_MSPI_INDIRECT_READ_XFER_CTRL_OFFSET);
	indirect_read_ctrl |= CADENCE_MSPI_INDIRECT_READ_XFER_CTRL_REG_IND_OPS_DONE_STATUS_BIT;
	sys_write32(indirect_read_ctrl, base_address + CADENCE_MSPI_INDIRECT_READ_XFER_CTRL_OFFSET);

	return 0;

timeout:
	indirect_read_ctrl = sys_read32(base_address + CADENCE_MSPI_INDIRECT_READ_XFER_CTRL_OFFSET);
	indirect_read_ctrl |= CADENCE_MSPI_INDIRECT_READ_XFER_CTRL_REG_CANCEL_BIT;
	sys_write32(indirect_read_ctrl, base_address + CADENCE_MSPI_INDIRECT_READ_XFER_CTRL_OFFSET);
	return -EIO;
}

static int mspi_cadence_indirect_write(const struct device *controller, const struct mspi_xfer *req,
				       uint32_t index, const uint64_t start_time)
{
	const mem_addr_t base_address = DEVICE_MMIO_GET(controller);
	const struct mspi_cadence_config *config = controller->config;
	const struct mspi_xfer_packet *packet = &req->packets[index];

	uint32_t dev_instr_wr_cfg =
		sys_read32(base_address + CADENCE_MSPI_DEV_INSTR_WR_CONFIG_OFFSET);

	dev_instr_wr_cfg &= ~CADENCE_MSPI_DEV_INSTR_WR_CONFIG_REG_WR_OPCODE_NON_XIP_MASK;
	dev_instr_wr_cfg |= FIELD_PREP(CADENCE_MSPI_DEV_INSTR_WR_CONFIG_REG_WR_OPCODE_NON_XIP_MASK,
				       packet->cmd);
	dev_instr_wr_cfg &= ~CADENCE_MSPI_DEV_INSTR_WR_CONFIG_REG_DUMMY_WR_CLK_CYCLES_MASK;
	dev_instr_wr_cfg |= FIELD_PREP(
		CADENCE_MSPI_DEV_INSTR_WR_CONFIG_REG_DUMMY_WR_CLK_CYCLES_MASK, req->tx_dummy);

	sys_write32(dev_instr_wr_cfg, base_address + CADENCE_MSPI_DEV_INSTR_WR_CONFIG_OFFSET);

	sys_write32(packet->address, base_address + CADENCE_MSPI_INDIRECT_WRITE_XFER_START_OFFSET);
	sys_write32(packet->num_bytes,
		    base_address + CADENCE_MSPI_INDIRECT_WRITE_XFER_NUM_BYTES_OFFSET);

	uint32_t dev_size_config = sys_read32(base_address + CADENCE_MSPI_DEV_SIZE_CONFIG_OFFSET);

	dev_size_config &= ~CADENCE_MSPI_DEV_SIZE_CONFIG_REG_NUM_ADDR_BYTES_MASK;
	dev_size_config |= FIELD_PREP(CADENCE_MSPI_DEV_SIZE_CONFIG_REG_NUM_ADDR_BYTES_MASK,
				      req->addr_length - 1);

	sys_write32(dev_size_config, base_address + CADENCE_MSPI_DEV_SIZE_CONFIG_OFFSET);

	uint32_t indirect_write_ctrl =
		sys_read32(base_address + CADENCE_MSPI_INDIRECT_WRITE_XFER_CTRL_OFFSET);

	indirect_write_ctrl |= CADENCE_MSPI_INDIRECT_WRITE_XFER_CTRL_REG_START_BIT;

	sys_write32(indirect_write_ctrl,
		    base_address + CADENCE_MSPI_INDIRECT_WRITE_XFER_CTRL_OFFSET);

	uint32_t read_offset = 0;
	uint32_t remaining_bytes = packet->num_bytes;
	uint32_t free_words = 0;
	uint32_t current_word_to_write;

	while (remaining_bytes > 0) {
		if (k_uptime_get() - start_time > req->timeout) {
			LOG_ERR("Timeout while sending data to MSPI device");
			goto timeout;
		}
		uint32_t sram_fill = sys_read32(base_address + CADENCE_MSPI_SRAM_FILL_OFFSET);

		free_words = config->sram_allocated_for_read -
			     FIELD_GET(CADENCE_MSPI_SRAM_FILL_REG_INDAC_WRITE_MASK, sram_fill);
		while (free_words > 0 && remaining_bytes > 0) {
			current_word_to_write = 0;
			memcpy(&current_word_to_write, &packet->data_buf[read_offset],
			       MIN(remaining_bytes, 4));
			sys_write32(current_word_to_write, config->fifo_addr);
			remaining_bytes = (remaining_bytes > 4 ? remaining_bytes - 4 : 0);
			read_offset += 4;
			--free_words;
		}
	}

	/* Wait for official finish */
	uint32_t done_status =
		sys_read32(base_address + CADENCE_MSPI_INDIRECT_WRITE_XFER_CTRL_OFFSET) &
		CADENCE_MSPI_INDIRECT_WRITE_XFER_CTRL_REG_IND_OPS_DONE_STATUS_BIT;
	while (done_status == 0 && k_uptime_get() - start_time < req->timeout) {
		k_sleep(CADENCE_MSPI_TIME_BETWEEN_RETRIES);
		done_status =
			sys_read32(base_address + CADENCE_MSPI_INDIRECT_WRITE_XFER_CTRL_OFFSET) &
			CADENCE_MSPI_INDIRECT_WRITE_XFER_CTRL_REG_IND_OPS_DONE_STATUS_BIT;
	}
	if (done_status == 0) {
		LOG_ERR("Timeout while waiting for official write done confirmation");
		goto timeout;
	}
	indirect_write_ctrl =
		sys_read32(base_address + CADENCE_MSPI_INDIRECT_WRITE_XFER_CTRL_OFFSET);
	indirect_write_ctrl |= CADENCE_MSPI_INDIRECT_WRITE_XFER_CTRL_REG_IND_OPS_DONE_STATUS_BIT;
	sys_write32(indirect_write_ctrl,
		    base_address + CADENCE_MSPI_INDIRECT_WRITE_XFER_CTRL_OFFSET);

	return 0;
timeout:
	indirect_write_ctrl =
		sys_read32(base_address + CADENCE_MSPI_INDIRECT_WRITE_XFER_CTRL_OFFSET);
	indirect_write_ctrl |= CADENCE_MSPI_INDIRECT_WRITE_XFER_CTRL_REG_CANCEL_BIT;
	sys_write32(indirect_write_ctrl,
		    base_address + CADENCE_MSPI_INDIRECT_WRITE_XFER_CTRL_OFFSET);
	return -EIO;
}

static int mspi_cadence_transceive(const struct device *controller,
				   const struct mspi_dev_id *dev_id, const struct mspi_xfer *req)
{
	uint64_t start_time = k_uptime_get();
	struct mspi_cadence_data *data = controller->data;
	int ret = 0;

	if (data->current_peripheral != dev_id) {
		LOG_ERR("Tried to send data over MSPI despite not having acquired the controller "
			"beforehand by calling mspi_dev_config");
		return -EINVAL;
	}

	ret = mspi_cadence_check_transfer_request(req);
	if (ret) {
		return ret;
	}

	if (req->timeout > CONFIG_MSPI_COMPLETION_TIMEOUT_TOLERANCE) {
		LOG_ERR("Request timeout exceeds configured maximum in Kconfig");
		return -EINVAL;
	}

	ret = k_sem_take(&data->transfer_lock, K_MSEC(req->timeout));
	if (ret < 0) {
		return ret;
	}

	for (uint32_t i = 0; i < req->num_packet; ++i) {
		const struct mspi_xfer_packet *packet = &req->packets[i];
		/* the FLASH_CMD_REGISTER is good for small transfers with only very little/no data
		 */
		if (packet->num_bytes <= 8) {
			ret = mspi_cadence_stig(controller, req, i, start_time);
			if (ret < 0) {
				goto exit;
			}
		} else {
			/* big transfer via indirect transfer mode */
			if (packet->dir == MSPI_RX) {
				ret = mspi_cadence_indirect_read(controller, req, i, start_time);
			} else {
				ret = mspi_cadence_indirect_write(controller, req, i, start_time);
			}
			if (ret < 0) {
				goto exit;
			}
		}
	}

exit:
	k_sem_give(&data->transfer_lock);
	return ret;
}

static int mspi_cadence_set_opcode_lines(const mem_addr_t base_addr, enum mspi_io_mode io_mode)
{
	uint32_t rd_config = sys_read32(base_addr + CADENCE_MSPI_DEV_INSTR_RD_CONFIG_OFFSET);

	rd_config &= ~CADENCE_MSPI_DEV_INSTR_RD_CONFIG_REG_INSTR_TYPE_MASK;

	switch (io_mode) {
	case MSPI_IO_MODE_SINGLE:
	case MSPI_IO_MODE_DUAL_1_1_2:
	case MSPI_IO_MODE_DUAL_1_2_2:
	case MSPI_IO_MODE_QUAD_1_1_4:
	case MSPI_IO_MODE_QUAD_1_4_4:
	case MSPI_IO_MODE_OCTAL_1_1_8:
	case MSPI_IO_MODE_OCTAL_1_8_8:
		rd_config |= FIELD_PREP(CADENCE_MSPI_DEV_INSTR_RD_CONFIG_REG_INSTR_TYPE_MASK, 0);
		break;
	case MSPI_IO_MODE_DUAL:
		rd_config |= FIELD_PREP(CADENCE_MSPI_DEV_INSTR_RD_CONFIG_REG_INSTR_TYPE_MASK, 1);
		break;
	case MSPI_IO_MODE_QUAD:
		rd_config |= FIELD_PREP(CADENCE_MSPI_DEV_INSTR_RD_CONFIG_REG_INSTR_TYPE_MASK, 2);
		break;
	case MSPI_IO_MODE_OCTAL:
		rd_config |= FIELD_PREP(CADENCE_MSPI_DEV_INSTR_RD_CONFIG_REG_INSTR_TYPE_MASK, 3);
		break;
	default:
		return -ENOTSUP;
	}

	sys_write32(rd_config, base_addr + CADENCE_MSPI_DEV_INSTR_RD_CONFIG_OFFSET);

	return 0;
}

static int mspi_cadence_set_addr_lines(const mem_addr_t base_addr, enum mspi_io_mode io_mode)
{
	uint32_t rd_config = sys_read32(base_addr + CADENCE_MSPI_DEV_INSTR_RD_CONFIG_OFFSET);
	uint32_t wr_config = sys_read32(base_addr + CADENCE_MSPI_DEV_INSTR_WR_CONFIG_OFFSET);

	rd_config &= ~CADENCE_MSPI_DEV_INSTR_RD_CONFIG_REG_ADDR_XFER_TYPE_STD_MODE_MASK;
	wr_config &= ~CADENCE_MSPI_DEV_INSTR_WR_CONFIG_REG_ADDR_XFER_TYPE_STD_MODE_MASK;

	switch (io_mode) {
	case MSPI_IO_MODE_SINGLE:
	case MSPI_IO_MODE_DUAL_1_1_2:
	case MSPI_IO_MODE_QUAD_1_1_4:
	case MSPI_IO_MODE_OCTAL_1_1_8:
		rd_config |= FIELD_PREP(
			CADENCE_MSPI_DEV_INSTR_RD_CONFIG_REG_ADDR_XFER_TYPE_STD_MODE_MASK, 0);
		wr_config |= FIELD_PREP(
			CADENCE_MSPI_DEV_INSTR_WR_CONFIG_REG_ADDR_XFER_TYPE_STD_MODE_MASK, 0);
		break;
	case MSPI_IO_MODE_DUAL:
	case MSPI_IO_MODE_DUAL_1_2_2:
		rd_config |= FIELD_PREP(
			CADENCE_MSPI_DEV_INSTR_RD_CONFIG_REG_ADDR_XFER_TYPE_STD_MODE_MASK, 1);
		wr_config |= FIELD_PREP(
			CADENCE_MSPI_DEV_INSTR_WR_CONFIG_REG_ADDR_XFER_TYPE_STD_MODE_MASK, 1);
		break;
	case MSPI_IO_MODE_QUAD:
	case MSPI_IO_MODE_QUAD_1_4_4:
		rd_config |= FIELD_PREP(
			CADENCE_MSPI_DEV_INSTR_RD_CONFIG_REG_ADDR_XFER_TYPE_STD_MODE_MASK, 2);
		wr_config |= FIELD_PREP(
			CADENCE_MSPI_DEV_INSTR_WR_CONFIG_REG_ADDR_XFER_TYPE_STD_MODE_MASK, 2);
		break;
	case MSPI_IO_MODE_OCTAL:
	case MSPI_IO_MODE_OCTAL_1_8_8:
		rd_config |= FIELD_PREP(
			CADENCE_MSPI_DEV_INSTR_RD_CONFIG_REG_ADDR_XFER_TYPE_STD_MODE_MASK, 3);
		wr_config |= FIELD_PREP(
			CADENCE_MSPI_DEV_INSTR_WR_CONFIG_REG_ADDR_XFER_TYPE_STD_MODE_MASK, 3);
		break;
	default:
		return -ENOTSUP;
	}

	sys_write32(rd_config, base_addr + CADENCE_MSPI_DEV_INSTR_RD_CONFIG_OFFSET);
	sys_write32(wr_config, base_addr + CADENCE_MSPI_DEV_INSTR_WR_CONFIG_OFFSET);

	return 0;
}

static int mspi_cadence_set_data_lines(const mem_addr_t base_addr, enum mspi_io_mode io_mode)
{
	uint32_t rd_config = sys_read32(base_addr + CADENCE_MSPI_DEV_INSTR_RD_CONFIG_OFFSET);
	uint32_t wr_config = sys_read32(base_addr + CADENCE_MSPI_DEV_INSTR_WR_CONFIG_OFFSET);

	rd_config &= ~CADENCE_MSPI_DEV_INSTR_RD_CONFIG_REG_DATA_XFER_TYPE_EXT_MODE_MASK;
	wr_config &= ~CADENCE_MSPI_DEV_INSTR_WR_CONFIG_REG_DATA_XFER_TYPE_EXT_MODE_MASK;

	switch (io_mode) {
	case MSPI_IO_MODE_SINGLE:
		rd_config |= FIELD_PREP(
			CADENCE_MSPI_DEV_INSTR_RD_CONFIG_REG_DATA_XFER_TYPE_EXT_MODE_MASK, 0);
		wr_config |= FIELD_PREP(
			CADENCE_MSPI_DEV_INSTR_WR_CONFIG_REG_DATA_XFER_TYPE_EXT_MODE_MASK, 0);
		break;
	case MSPI_IO_MODE_DUAL:
	case MSPI_IO_MODE_DUAL_1_1_2:
	case MSPI_IO_MODE_DUAL_1_2_2:
		rd_config |= FIELD_PREP(
			CADENCE_MSPI_DEV_INSTR_RD_CONFIG_REG_DATA_XFER_TYPE_EXT_MODE_MASK, 1);
		wr_config |= FIELD_PREP(
			CADENCE_MSPI_DEV_INSTR_WR_CONFIG_REG_DATA_XFER_TYPE_EXT_MODE_MASK, 1);
		break;
	case MSPI_IO_MODE_QUAD:
	case MSPI_IO_MODE_QUAD_1_1_4:
	case MSPI_IO_MODE_QUAD_1_4_4:
		rd_config |= FIELD_PREP(
			CADENCE_MSPI_DEV_INSTR_RD_CONFIG_REG_DATA_XFER_TYPE_EXT_MODE_MASK, 2);
		wr_config |= FIELD_PREP(
			CADENCE_MSPI_DEV_INSTR_WR_CONFIG_REG_DATA_XFER_TYPE_EXT_MODE_MASK, 2);
		break;
	case MSPI_IO_MODE_OCTAL:
	case MSPI_IO_MODE_OCTAL_1_1_8:
	case MSPI_IO_MODE_OCTAL_1_8_8:
		rd_config |= FIELD_PREP(
			CADENCE_MSPI_DEV_INSTR_RD_CONFIG_REG_DATA_XFER_TYPE_EXT_MODE_MASK, 3);
		wr_config |= FIELD_PREP(
			CADENCE_MSPI_DEV_INSTR_WR_CONFIG_REG_DATA_XFER_TYPE_EXT_MODE_MASK, 3);
		break;
	default:
		return -ENOTSUP;
	}

	sys_write32(rd_config, base_addr + CADENCE_MSPI_DEV_INSTR_RD_CONFIG_OFFSET);
	sys_write32(wr_config, base_addr + CADENCE_MSPI_DEV_INSTR_WR_CONFIG_OFFSET);

	return 0;
}

int mspi_cadence_dev_config(const struct device *controller, const struct mspi_dev_id *dev_id,
			    const enum mspi_dev_cfg_mask param_mask, const struct mspi_dev_cfg *cfg)
{
	const mem_addr_t base_addr = DEVICE_MMIO_GET(controller);
	struct mspi_cadence_data *data = controller->data;
	int ret = 0;

	if (data->current_peripheral != dev_id) {
		ret = k_mutex_lock(&data->access_lock,
				   K_MSEC(CONFIG_MSPI_COMPLETION_TIMEOUT_TOLERANCE));
		if (ret < 0) {
			LOG_ERR("Error waiting for MSPI controller lock for changing device "
				"config");
			return ret;
		}
		data->current_peripheral = dev_id;
	}

	if (param_mask & CADENCE_MSPI_IGNORED_DEV_CONFIG_PARAMS) {
		if (param_mask == MSPI_DEVICE_CONFIG_ALL) {
			LOG_ERR("Device configuration includes ignored / not implemented "
				"parameters. For "
				"better compatibility these are ignored without returning an error "
				"due to the usage "
				"of MSPI_DEVICE_CONFIG_ALL. To see which "
				"parameters are explicitly ignored check mspi_cadence.h");
		} else {
			LOG_ERR("Device configuration includes ignored / not implemented "
				"parameters. Check mspi_cadence.h to figure out what isn't "
				"supported");
			return -ENOSYS;
		}
	}

	if (param_mask & MSPI_DEVICE_CONFIG_BREAK_TIME) {
		if (cfg->time_to_break != 0) {
			LOG_ERR("Automatically breaking up transfers is not supported");
			return -ENOSYS;
		}
	}

	if (param_mask & MSPI_DEVICE_CONFIG_MEM_BOUND) {
		if (cfg->mem_boundary != 0) {
			LOG_ERR("Automatically breaking up transfers is not supported");
			return -ENOSYS;
		}
	}

	if (param_mask & MSPI_DEVICE_CONFIG_ENDIAN) {
		if (cfg->endian != MSPI_XFER_LITTLE_ENDIAN) {
			LOG_ERR("Only little Endian is supported for now");
			/* There is no hardware native support for big endian but it can be
			 * done in software
			 */
			return -ENOSYS;
		}
	}

	if (param_mask & MSPI_DEVICE_CONFIG_CE_POL) {
		if (cfg->ce_polarity != MSPI_CE_ACTIVE_LOW) {
			LOG_ERR("Non active low chip enable polarities haven't been implemented "
				"yet");
			return -ENOSYS;
		}
	}

	if (param_mask & MSPI_DEVICE_CONFIG_DQS) {
		if (cfg->dqs_enable) {
			LOG_ERR("DQS is not implemented yet");
			return -ENOSYS;
		}
	}

	if (param_mask & MSPI_DEVICE_CONFIG_DATA_RATE) {
		if (cfg->data_rate != MSPI_DATA_RATE_SINGLE) {
			LOG_ERR("Only single data rate is supported for now");
			return -ENOSYS;
		}
	}

	/* Disable MSPI during configuration */
	uint32_t config = sys_read32(base_addr + CADENCE_MSPI_CONFIG_OFFSET);

	config &= ~CADENCE_MSPI_CONFIG_REG_ENABLE_SPI_BIT;

	sys_write32(config, base_addr + CADENCE_MSPI_CONFIG_OFFSET);

	ret = mspi_cadence_wait_for_idle(controller);
	if (ret < 0) {
		goto exit;
	}

	if (param_mask & MSPI_DEVICE_CONFIG_CE_NUM) {
		uint32_t num;

		if (cfg->ce_num > 3) {
			LOG_ERR("Non implemented chip select. Only hardware CS 0 to 3 are "
				"implemented");
			ret = -ENOSYS;
			goto exit;
		}
		num = ~BIT(cfg->ce_num) & BIT_MASK(4);

		config &= ~CADENCE_MSPI_CONFIG_REG_PERIPH_CS_LINES_MASK;
		config |= FIELD_PREP(CADENCE_MSPI_CONFIG_REG_PERIPH_CS_LINES_MASK, num);
		sys_write32(config, base_addr + CADENCE_MSPI_CONFIG_OFFSET);
	}
	if (param_mask & MSPI_DEVICE_CONFIG_IO_MODE) {
		ret = mspi_cadence_set_opcode_lines(base_addr, cfg->io_mode);
		if (ret) {
			goto exit;
		}
		ret = mspi_cadence_set_data_lines(base_addr, cfg->io_mode);
		if (ret) {
			goto exit;
		}
		ret = mspi_cadence_set_addr_lines(base_addr, cfg->io_mode);
		if (ret) {
			goto exit;
		}
	}
	if (param_mask & MSPI_DEVICE_CONFIG_CPP) {
		config &= ~CADENCE_MSPI_CONFIG_REG_SEL_CLK_POL_BIT;
		config &= ~CADENCE_MSPI_CONFIG_REG_SEL_CLK_PHASE_BIT;
		switch (cfg->cpp) {
		case MSPI_CPP_MODE_0:
			config |= FIELD_PREP(CADENCE_MSPI_CONFIG_REG_SEL_CLK_POL_BIT, 0);
			config |= FIELD_PREP(CADENCE_MSPI_CONFIG_REG_SEL_CLK_PHASE_BIT, 0);
			break;
		case MSPI_CPP_MODE_1:
			config |= FIELD_PREP(CADENCE_MSPI_CONFIG_REG_SEL_CLK_POL_BIT, 0);
			config |= FIELD_PREP(CADENCE_MSPI_CONFIG_REG_SEL_CLK_PHASE_BIT, 1);
			break;
		case MSPI_CPP_MODE_2:
			config |= FIELD_PREP(CADENCE_MSPI_CONFIG_REG_SEL_CLK_POL_BIT, 1);
			config |= FIELD_PREP(CADENCE_MSPI_CONFIG_REG_SEL_CLK_PHASE_BIT, 0);
			break;
		case MSPI_CPP_MODE_3:
			config |= FIELD_PREP(CADENCE_MSPI_CONFIG_REG_SEL_CLK_POL_BIT, 1);
			config |= FIELD_PREP(CADENCE_MSPI_CONFIG_REG_SEL_CLK_PHASE_BIT, 1);
			break;
		default:
			LOG_ERR("Invalid clock polarity/phase configuration");
			ret = -ENOTSUP;
			goto exit;
		}
	}
	sys_write32(config, base_addr + CADENCE_MSPI_CONFIG_OFFSET);

exit:
	/* Re-enable MSPI */
	config = sys_read32(base_addr + CADENCE_MSPI_CONFIG_OFFSET);
	config |= CADENCE_MSPI_CONFIG_REG_ENABLE_SPI_BIT;
	sys_write32(config, base_addr + CADENCE_MSPI_CONFIG_OFFSET);

	/* Unlock in case of an error */
	if (ret != 0) {
		k_mutex_unlock(&data->access_lock);
	}

	return ret;
}

int mspi_cadence_get_channel_status(const struct device *controller, uint8_t channel)
{
	ARG_UNUSED(channel);

	const mem_addr_t base_addr = DEVICE_MMIO_GET(controller);
	struct mspi_cadence_data *data = controller->data;

	if ((sys_read32(base_addr + CADENCE_MSPI_CONFIG_OFFSET) &
	     CADENCE_MSPI_CONFIG_REG_IDLE_BIT) != 0) {
		return -EBUSY;
	}

	if (k_sem_count_get(&data->transfer_lock) == 0) {
		return -EBUSY;
	}

	data->current_peripheral = NULL;

	k_mutex_unlock(&data->access_lock);
	return 0;
}

#ifdef CONFIG_MSPI_TIMING
int mspi_cadence_timing(const struct device *controller, const struct mspi_dev_id *dev_id,
			const uint32_t param_mask, void *timing_cfg)
{
	ARG_UNUSED(dev_id);

	const mem_addr_t base_addr = DEVICE_MMIO_GET(controller);
	struct mspi_cadence_data *data = controller->data;
	struct mspi_cadence_timing_cfg *timing = timing_cfg;
	int ret = 0;

	/* Ensure no transfers in the meantime */
	ret = k_sem_take(&data->transfer_lock, K_MSEC(CONFIG_MSPI_COMPLETION_TIMEOUT_TOLERANCE));
	if (ret < 0) {
		LOG_ERR("Error waiting for MSPI controller lock for changing timing");
		return ret;
	}

	if (dev_id != data->current_peripheral) {
		LOG_ERR("Tried chaning timing for another peripheral than the one the access lock "
			"is held for");
		ret = -EINVAL;
		goto exit;
	}

	uint32_t dev_delay = sys_read32(base_addr + CADENCE_MSPI_DEV_DELAY_OFFSET);

	if (param_mask & MSPI_CADENCE_TIMING_PARAM_NSS) {
		dev_delay &= ~CADENCE_MSPI_DEV_DELAY_REG_D_NSS_MASK;
		dev_delay |= FIELD_PREP(CADENCE_MSPI_DEV_DELAY_REG_D_NSS_MASK, timing->nss);
	}

	if (param_mask & MSPI_CADENCE_TIMING_PARAM_BTWN) {
		dev_delay &= ~CADENCE_MSPI_DEV_DELAY_REG_D_BTWN_MASK;
		dev_delay |= FIELD_PREP(CADENCE_MSPI_DEV_DELAY_REG_D_BTWN_MASK, timing->btwn);
	}

	if (param_mask & MSPI_CADENCE_TIMING_PARAM_AFTER) {
		dev_delay &= ~CADENCE_MSPI_DEV_DELAY_REG_D_AFTER_MASK;
		dev_delay |= FIELD_PREP(CADENCE_MSPI_DEV_DELAY_REG_D_AFTER_MASK, timing->after);
	}

	if (param_mask & MSPI_CADENCE_TIMING_PARAM_INIT) {
		dev_delay &= ~CADENCE_MSPI_DEV_DELAY_REG_D_INIT_MASK;
		dev_delay |= FIELD_PREP(CADENCE_MSPI_DEV_DELAY_REG_D_INIT_MASK, timing->init);
	}

	sys_write32(dev_delay, base_addr + CADENCE_MSPI_DEV_DELAY_OFFSET);

exit:
	k_sem_give(&data->transfer_lock);

	return ret;
}
#endif /* CONFIG_MSPI_TIMING */

static DEVICE_API(mspi, mspi_cadence_driver_api) = {
	.config = NULL,
	.dev_config = mspi_cadence_dev_config,
	.xip_config = NULL,
	.scramble_config = NULL,
#ifdef CONFIG_MSPI_TIMING
	.timing_config = mspi_cadence_timing,
#else
	.timing_config = NULL,
#endif
	.get_channel_status = mspi_cadence_get_channel_status,
	.register_callback = NULL,
	.transceive = mspi_cadence_transceive,
};

#define CADENCE_CHECK_MULTIPERIPHERAL(n)                                                           \
	BUILD_ASSERT(DT_PROP_OR(DT_DRV_INST(n), software_multiperipheral, 0) == 0,                 \
		     "Multiperipherals arent's supported by the driver as of now")

#define MSPI_CONFIG(n)                                                                             \
	{.op_mode = DT_INST_ENUM_IDX_OR(n, op_mode, MSPI_OP_MODE_CONTROLLER),                      \
	 .sw_multi_periph = DT_INST_PROP(n, software_multiperipheral)}

#define CADENCE_MSPI_DEFINE(n)                                                                     \
	CADENCE_CHECK_MULTIPERIPHERAL(n);                                                          \
	PINCTRL_DT_DEFINE(DT_DRV_INST(n));                                                         \
	static const struct mspi_cadence_config mspi_cadence_config_##n = {                        \
		DEVICE_MMIO_ROM_INIT(DT_DRV_INST(n)),                                              \
		.pinctrl = PINCTRL_DT_INST_DEV_CONFIG_GET(n),                                      \
		.mspi_config = MSPI_CONFIG(n),                                                     \
		.fifo_addr = DT_REG_ADDR_BY_IDX(DT_DRV_INST(n), 1),                                \
		.sram_allocated_for_read = DT_PROP(DT_DRV_INST(n), read_buffer_size),              \
		.initial_timing_cfg = {                                                            \
			.nss = DT_PROP_OR(DT_DRV_INST(n), init_nss_delay,                          \
					  CADENCE_MSPI_DEFAULT_DELAY),                             \
			.btwn = DT_PROP_OR(DT_DRV_INST(n), init_btwn_delay,                        \
					   CADENCE_MSPI_DEFAULT_DELAY),                            \
			.after = DT_PROP_OR(DT_DRV_INST(n), init_after_delay,                      \
					    CADENCE_MSPI_DEFAULT_DELAY),                           \
			.init = DT_PROP_OR(DT_DRV_INST(n), init_init_delay,                        \
					   CADENCE_MSPI_DEFAULT_DELAY),                            \
		}};                                                                                \
	static struct mspi_cadence_data mspi_cadence_data_##n = {};                                \
	DEVICE_DT_INST_DEFINE(n, mspi_cadence_init, NULL, &mspi_cadence_data_##n,                  \
			      &mspi_cadence_config_##n, PRE_KERNEL_2, CONFIG_MSPI_INIT_PRIORITY,   \
			      &mspi_cadence_driver_api);

DT_INST_FOREACH_STATUS_OKAY(CADENCE_MSPI_DEFINE)
