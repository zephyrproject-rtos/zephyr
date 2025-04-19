/*
 * Copyright (c) 2025 Siemens Mobility GmbH
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT ti_k3_mspi_controller

#include "mspi_ti_k3.h"

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

LOG_MODULE_REGISTER(mspi_ti_k3, CONFIG_MSPI_LOG_LEVEL);

struct mspi_ti_k3_config {
	DEVICE_MMIO_ROM;
	struct mspi_cfg mspi_config;
	const struct pinctrl_dev_config *pinctrl;
	const uint32_t fifo_addr;
	const uint32_t sram_allocated_for_read;

	const struct mspi_ti_k3_timing_cfg initial_timing_cfg;
};

struct mspi_ti_k3_data {
	DEVICE_MMIO_RAM;
	struct k_mutex lock;
};

/* helper function to easily modify parts of registers */
static void mspi_ti_k3_set_bits_shifted(const uint32_t value, const uint32_t num_bits,
					const uint32_t shift, const mem_addr_t address)
{
	__ASSERT(num_bits <= 32, "Invalid number of bits provided");
	__ASSERT(shift <= 31, "Invalid shift value provided");
	__ASSERT((value & ~BIT_MASK(num_bits)) == 0,
		 "Tried writing a value that overflows the number of bits that should be changed");

	uint32_t tmp = sys_read32(address);

	tmp &= ~(BIT_MASK(num_bits) << shift);
	tmp |= (value << shift);

	sys_write32(tmp, address);
}

/**
 * Set value in part of a register. The reg is the name of the register where
 * content should be set and field is the field that should be changed.  It is
 * implemented via macro concatenation to prevent overly long code. An
 * explanation of how the registers / fields are named are in the corresponding
 * header file.
 */
#define MSPI_TI_K3_REG_WRITE(value, reg, field, base_addr)                                         \
	mspi_ti_k3_set_bits_shifted(value, TI_K3_OSPI_##reg##_##field##_FLD_SIZE,                  \
				    TI_K3_OSPI_##reg##_##field##_FLD_OFFSET,                       \
				    base_addr + TI_K3_OSPI_##reg##_REG)

/**
 * Read an entire register by name. Short version for sys_read32 with base_addr and offset
 */
#define MSPI_TI_K3_REG_READ(reg, base_addr) sys_read32(base_addr + TI_K3_OSPI_##reg##_REG)

/**
 * Read part of a register. This is done via macro concatenation to allow
 * shorter code. The reg is the name of the register from which should be read
 * and the field is which field of the register should be extracted.
 */
#define MSPI_TI_K3_REG_READ_MASKED(reg, field, base_addr)                                          \
	((sys_read32(base_addr + TI_K3_OSPI_##reg##_REG) >>                                        \
	  TI_K3_OSPI_##reg##_##field##_FLD_OFFSET) &                                               \
	 BIT_MASK(TI_K3_OSPI_##reg##_##field##_FLD_SIZE))

/**
 * Wait for the OSPI controller to enter idle with the default timeout
 */
int mspi_ti_k3_wait_for_idle(const struct device *controller)
{
	const mem_addr_t base_addr = DEVICE_MMIO_GET(controller);
	uint32_t idle = MSPI_TI_K3_REG_READ_MASKED(CONFIG, IDLE, base_addr);
	uint32_t retries = TI_K3_OSPI_GET_NUM_RETRIES(CONFIG_MSPI_COMPLETION_TIMEOUT_TOLERANCE);

	while (idle == 0 && retries > 0) {
		k_sleep(TI_K3_OSPI_TIME_BETWEEN_RETRIES);
		idle = MSPI_TI_K3_REG_READ_MASKED(CONFIG, IDLE, base_addr);
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
static int mspi_ti_k3_check_transfer_package(const struct mspi_xfer *request, uint32_t index)
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
static int mspi_ti_k3_check_transfer_request(const struct mspi_xfer *request)
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

	if (request->rx_dummy &
		    ~BIT_MASK(TI_K3_OSPI_DEV_INSTR_RD_CONFIG_DUMMY_RD_CLK_CYCLES_FLD_SIZE) ||
	    request->tx_dummy &
		    ~BIT_MASK(TI_K3_OSPI_DEV_INSTR_WR_CONFIG_DUMMY_WR_CLK_CYCLES_FLD_SIZE)) {
		LOG_ERR("Request contains too many dummy cycles");
		return -ENOTSUP;
	}

	int ret = 0;

	for (uint32_t i = 0; i < request->num_packet; ++i) {
		ret = mspi_ti_k3_check_transfer_package(request, i);
		if (ret < 0) {
			return ret;
		}
	}

	return 0;
}

static int mspi_ti_k3_init(const struct device *dev)
{
	DEVICE_MMIO_MAP(dev, K_MEM_CACHE_NONE);
	const struct mspi_ti_k3_config *config = dev->config;
	const struct mspi_ti_k3_timing_cfg *timing_config = &config->initial_timing_cfg;
	struct mspi_ti_k3_data *data = dev->data;
	const mem_addr_t base_addr = DEVICE_MMIO_GET(dev);
	int ret;

	k_mutex_init(&data->lock);

	ret = pinctrl_apply_state(config->pinctrl, PINCTRL_STATE_DEFAULT);
	if (ret < 0) {
		LOG_ERR("Failed to apply pinctrl");
		return ret;
	}

	/* disable OSPI */
	MSPI_TI_K3_REG_WRITE(0, CONFIG, ENABLE_SPI, base_addr);

	ret = mspi_ti_k3_wait_for_idle(dev);
	if (ret < 0) {
		return ret;
	}

	/* disable direct access the driver always uses indirect accesses */
	MSPI_TI_K3_REG_WRITE(0, CONFIG, ENB_DIR_ACC_CTRL, base_addr);

	/* disable DTR protocol */
	MSPI_TI_K3_REG_WRITE(0, CONFIG, ENABLE_DTR_PROTOCOL, base_addr);

	/* leave XIP mode */
	MSPI_TI_K3_REG_WRITE(0, CONFIG, ENTER_XIP_MODE, base_addr);

	/* set how many FSS0 SRAM locations are allocated for read; the other ones
	 * are allocated for writes
	 */
	MSPI_TI_K3_REG_WRITE(config->sram_allocated_for_read, SRAM_PARTITION_CFG, ADDR, base_addr);

	/* only allow one CS to be active */
	MSPI_TI_K3_REG_WRITE(0, CONFIG, PERIPH_SEL_DEC, base_addr);

	/* CS selection is based on "manual" pin selection */
	MSPI_TI_K3_REG_WRITE(0, CONFIG, ENABLE_AHB_DECODER, base_addr);

	/* DQ3 should not be used as reset pin */
	MSPI_TI_K3_REG_WRITE(1, CONFIG, RESET_CFG, base_addr);

	/* Set baud rate division to 32; formula: (n + 1) * 2 */
	MSPI_TI_K3_REG_WRITE(15, CONFIG, MSTR_BAUD_DIV, base_addr);

	/* disable dual byte opcodes */
	MSPI_TI_K3_REG_WRITE(0, CONFIG, DUAL_BYTE_OPCODE_EN, base_addr);

	/* disable PHY pipeline mode */
	MSPI_TI_K3_REG_WRITE(0, CONFIG, PIPELINE_PHY, base_addr);

	/* disable PHY module generally */
	MSPI_TI_K3_REG_WRITE(0, CONFIG, PHY_MODE_ENABLE, base_addr);

	/* disable CRC */
	MSPI_TI_K3_REG_WRITE(0, CONFIG, CRC_ENABLE, base_addr);

	/* disable DMA generally since it's not supported */
	MSPI_TI_K3_REG_WRITE(0, CONFIG, ENB_DMA_IF, base_addr);

	/* disable automatic write protection disablement of MSPI peripherals */
	MSPI_TI_K3_REG_WRITE(0, CONFIG, WR_PROT_FLASH, base_addr);

	/* disable possible reset pin */
	MSPI_TI_K3_REG_WRITE(0, CONFIG, RESET_PIN, base_addr);

	/* general clock cycle delays */
	MSPI_TI_K3_REG_WRITE(timing_config->nss, DEV_DELAY, D_NSS, base_addr);
	MSPI_TI_K3_REG_WRITE(timing_config->btwn, DEV_DELAY, D_BTWN, base_addr);
	MSPI_TI_K3_REG_WRITE(timing_config->after, DEV_DELAY, D_AFTER, base_addr);
	MSPI_TI_K3_REG_WRITE(timing_config->init, DEV_DELAY, D_INIT, base_addr);

	/* set trigger reg address and range to 0 */
	MSPI_TI_K3_REG_WRITE(0, IND_AHB_ADDR_TRIGGER, ADDR, base_addr);
	MSPI_TI_K3_REG_WRITE(0, INDIRECT_TRIGGER_ADDR_RANGE, IND_RANGE_WIDTH, base_addr);

	/* disable loop-back via DQS */
	MSPI_TI_K3_REG_WRITE(1, RD_DATA_CAPTURE, BYPASS, base_addr);

	/* disable auto polling for write completion */
	MSPI_TI_K3_REG_WRITE(1, WRITE_COMPLETION_CTRL, DISABLE_POLLING, base_addr);

	/* disable automatic write enable command before indirect write transactions */
	MSPI_TI_K3_REG_WRITE(0, DEV_INSTR_WR_CONFIG, WEL_DIS, base_addr);

	/* reset mode bit (hardware CRC checking on read, if supported) */
	MSPI_TI_K3_REG_WRITE(0, DEV_INSTR_RD_CONFIG, MODE_BIT_ENABLE, base_addr);

	/* disable DDR mode */
	MSPI_TI_K3_REG_WRITE(0, DEV_INSTR_RD_CONFIG, DDR_EN, base_addr);

	uint32_t val;

	/* disable all interrupts via masking */
	val = sys_read32(base_addr + TI_K3_OSPI_IRQ_MASK_REG);
	val &= ~TI_K3_OSPI_IRQ_MASK_ALL;
	sys_write32(val, base_addr + TI_K3_OSPI_IRQ_MASK_REG);

	/* clear currently pending interrupts */
	val = sys_read32(base_addr + TI_K3_OSPI_IRQ_STATUS_REG);
	val |= TI_K3_OSPI_IRQ_STATUS_ALL;
	sys_write32(val, base_addr + TI_K3_OSPI_IRQ_STATUS_REG);

	/* re-enable OSPI controller */
	MSPI_TI_K3_REG_WRITE(1, CONFIG, ENABLE_SPI, base_addr);

	return 0;
}

static int mspi_ti_k3_small_transfer(const struct device *controller, const struct mspi_xfer *req,
				     uint32_t index, const uint64_t start_time)
{
	const mem_addr_t base_address = DEVICE_MMIO_GET(controller);
	const struct mspi_xfer_packet *packet = &req->packets[index];
	uint32_t dummy_cycles = 0;

	/* reset previous command configuration completely */
	sys_write32(0, base_address + TI_K3_OSPI_FLASH_CMD_CTRL_REG);

	if (packet->dir == MSPI_RX) {
		if (packet->num_bytes != 0) {
			MSPI_TI_K3_REG_WRITE(1, FLASH_CMD_CTRL, ENB_READ_DATA, base_address);
			MSPI_TI_K3_REG_WRITE(packet->num_bytes - 1, FLASH_CMD_CTRL,
					     NUM_RD_DATA_BYTES, base_address);
		}
		dummy_cycles = req->rx_dummy;
	} else {
		if (packet->num_bytes != 0) {
			MSPI_TI_K3_REG_WRITE(1, FLASH_CMD_CTRL, ENB_WRITE_DATA, base_address);
			MSPI_TI_K3_REG_WRITE(packet->num_bytes - 1, FLASH_CMD_CTRL,
					     NUM_WR_DATA_BYTES, base_address);
			if (packet->num_bytes > 4) {
				uint32_t upper = 0;

				memcpy(&upper, &packet->data_buf[4], packet->num_bytes - 4);
				sys_write32(upper,
					    base_address + TI_K3_OSPI_FLASH_WR_DATA_UPPER_REG);
			}
			uint32_t lower = 0;

			memcpy(&lower, &packet->data_buf[0], MIN(packet->num_bytes, 4));
			sys_write32(lower, base_address + TI_K3_OSPI_FLASH_WR_DATA_LOWER_REG);
		}
		dummy_cycles = req->tx_dummy;
	}

	MSPI_TI_K3_REG_WRITE(packet->cmd, FLASH_CMD_CTRL, CMD_OPCODE, base_address);
	MSPI_TI_K3_REG_WRITE(dummy_cycles, FLASH_CMD_CTRL, NUM_DUMMY_CYCLES, base_address);

	if (req->addr_length) {
		MSPI_TI_K3_REG_WRITE(1, FLASH_CMD_CTRL, ENB_COMD_ADDR, base_address);
		MSPI_TI_K3_REG_WRITE(req->addr_length - 1, FLASH_CMD_CTRL, NUM_ADDR_BYTES,
				     base_address);
		MSPI_TI_K3_REG_WRITE(packet->address, FLASH_CMD_ADDR, ADDR, base_address);
	}

	/* start transaction */
	MSPI_TI_K3_REG_WRITE(1, FLASH_CMD_CTRL, CMD_EXEC, base_address);

	uint32_t exec_status =
		MSPI_TI_K3_REG_READ_MASKED(FLASH_CMD_CTRL, CMD_EXEC_STATUS, base_address);
	while (exec_status != 0 && k_uptime_get() - start_time < req->timeout) {
		k_sleep(TI_K3_OSPI_TIME_BETWEEN_RETRIES);
		exec_status =
			MSPI_TI_K3_REG_READ_MASKED(FLASH_CMD_CTRL, CMD_EXEC_STATUS, base_address);
	}
	if (exec_status != 0) {
		LOG_ERR("Timeout while waiting for dedicated command to finish");
		return -EIO;
	}

	if (packet->dir == MSPI_RX) {
		if (packet->num_bytes > 4) {
			uint32_t higher = MSPI_TI_K3_REG_READ(FLASH_RD_DATA_UPPER, base_address);

			memcpy(&packet->data_buf[4], &higher, packet->num_bytes - 4);
		}
		uint32_t lower = MSPI_TI_K3_REG_READ(FLASH_RD_DATA_LOWER, base_address);

		lower = sys_read32(base_address + TI_K3_OSPI_FLASH_RD_DATA_LOWER_REG);
		memcpy(&packet->data_buf[0], &lower, MIN(packet->num_bytes, 4));
	}

	return 0;
}

static int mspi_ti_k3_indirect_read(const struct device *controller, const struct mspi_xfer *req,
				    uint32_t index, const uint64_t start_time)
{
	const mem_addr_t base_address = DEVICE_MMIO_GET(controller);
	const struct mspi_ti_k3_config *config = controller->config;
	const struct mspi_xfer_packet *packet = &req->packets[index];

	MSPI_TI_K3_REG_WRITE(packet->cmd, DEV_INSTR_RD_CONFIG, RD_OPCODE_NON_XIP, base_address);
	MSPI_TI_K3_REG_WRITE(packet->address, INDIRECT_READ_XFER_START, ADDR, base_address);
	MSPI_TI_K3_REG_WRITE(packet->num_bytes, INDIRECT_READ_XFER_NUM_BYTES, VALUE, base_address);
	MSPI_TI_K3_REG_WRITE(req->addr_length - 1, DEV_SIZE_CONFIG, NUM_ADDR_BYTES, base_address);
	MSPI_TI_K3_REG_WRITE(req->rx_dummy, DEV_INSTR_RD_CONFIG, DUMMY_RD_CLK_CYCLES, base_address);

	/* Start transfer */
	MSPI_TI_K3_REG_WRITE(1, INDIRECT_READ_XFER_CTRL, START, base_address);

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
		num_new_words = MSPI_TI_K3_REG_READ_MASKED(SRAM_FILL, INDAC_READ, base_address);
		while (remaining_bytes > 0 && num_new_words > 0) {
			current_new_word = sys_read32(config->fifo_addr);
			bytes_to_copy_from_current_word = MIN(remaining_bytes, 4);
			memcpy(write_ptr, &current_new_word, bytes_to_copy_from_current_word);
			write_ptr += bytes_to_copy_from_current_word;
			remaining_bytes -= bytes_to_copy_from_current_word;
			--num_new_words;
		}
	}

	/* wait until official indirect read completion */
	uint32_t done_status = MSPI_TI_K3_REG_READ_MASKED(INDIRECT_READ_XFER_CTRL,
							  IND_OPS_DONE_STATUS, base_address);
	while (done_status == 0 && k_uptime_get() - start_time < req->timeout) {
		k_sleep(TI_K3_OSPI_TIME_BETWEEN_RETRIES);
		done_status = MSPI_TI_K3_REG_READ_MASKED(INDIRECT_READ_XFER_CTRL,
							 IND_OPS_DONE_STATUS, base_address);
	}
	if (done_status == 0) {
		LOG_ERR("Timeout waiting for official indirect read done confirmation");
		goto timeout;
	}
	MSPI_TI_K3_REG_WRITE(1, INDIRECT_READ_XFER_CTRL, IND_OPS_DONE_STATUS, base_address);

	return 0;

timeout:
	MSPI_TI_K3_REG_WRITE(1, INDIRECT_READ_XFER_CTRL, CANCEL, base_address);
	return -EIO;
}

static int mspi_ti_k3_indirect_write(const struct device *controller, const struct mspi_xfer *req,
				     uint32_t index, const uint64_t start_time)
{
	const mem_addr_t base_address = DEVICE_MMIO_GET(controller);
	const struct mspi_ti_k3_config *config = controller->config;
	const struct mspi_xfer_packet *packet = &req->packets[index];

	MSPI_TI_K3_REG_WRITE(packet->cmd, DEV_INSTR_WR_CONFIG, WR_OPCODE_NON_XIP, base_address);
	MSPI_TI_K3_REG_WRITE(req->tx_dummy, DEV_INSTR_WR_CONFIG, DUMMY_WR_CLK_CYCLES, base_address);
	MSPI_TI_K3_REG_WRITE(req->addr_length - 1, DEV_SIZE_CONFIG, NUM_ADDR_BYTES, base_address);
	MSPI_TI_K3_REG_WRITE(packet->address, INDIRECT_WRITE_XFER_START, ADDR, base_address);
	MSPI_TI_K3_REG_WRITE(packet->num_bytes, INDIRECT_WRITE_XFER_NUM_BYTES, VALUE, base_address);

	MSPI_TI_K3_REG_WRITE(1, INDIRECT_WRITE_XFER_CTRL, START, base_address);

	uint32_t read_offset = 0;
	uint32_t remaining_bytes = packet->num_bytes;
	uint32_t free_words = 0;
	uint32_t current_word_to_write;

	while (remaining_bytes > 0) {
		if (k_uptime_get() - start_time > req->timeout) {
			LOG_ERR("Timeout while sending data to MSPI device");
			goto timeout;
		}
		free_words = config->sram_allocated_for_read -
			     MSPI_TI_K3_REG_READ_MASKED(SRAM_FILL, INDAC_WRITE, base_address);
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
	uint32_t done_status = MSPI_TI_K3_REG_READ_MASKED(INDIRECT_WRITE_XFER_CTRL,
							  IND_OPS_DONE_STATUS, base_address);
	while (done_status == 0 && k_uptime_get() - start_time < req->timeout) {
		k_sleep(TI_K3_OSPI_TIME_BETWEEN_RETRIES);
		done_status = MSPI_TI_K3_REG_READ_MASKED(INDIRECT_WRITE_XFER_CTRL,
							 IND_OPS_DONE_STATUS, base_address);
	}
	if (done_status == 0) {
		LOG_ERR("Timeout while waiting for official write done confirmation");
		goto timeout;
	}
	MSPI_TI_K3_REG_WRITE(1, INDIRECT_WRITE_XFER_CTRL, IND_OPS_DONE_STATUS, base_address);

	return 0;
timeout:
	MSPI_TI_K3_REG_WRITE(1, INDIRECT_WRITE_XFER_CTRL, CANCEL, base_address);
	return -EIO;
}

static int mspi_ti_k3_transceive(const struct device *controller, const struct mspi_dev_id *dev_id,
				 const struct mspi_xfer *req)
{
	uint64_t start_time = k_uptime_get();
	struct mspi_ti_k3_data *data = controller->data;
	int ret = 0;

	ret = mspi_ti_k3_check_transfer_request(req);
	if (ret) {
		return ret;
	}

	if (req->timeout > CONFIG_MSPI_COMPLETION_TIMEOUT_TOLERANCE) {
		LOG_ERR("Request timeout exceeds configured maximum in Kconfig");
		return -EINVAL;
	}

	ret = k_mutex_lock(&data->lock, K_MSEC(req->timeout));
	if (ret < 0) {
		return ret;
	}

	for (uint32_t i = 0; i < req->num_packet; ++i) {
		const struct mspi_xfer_packet *packet = &req->packets[i];
		/* the FLASH_CMD_REGISTER is good for small transfers with only very little/no data
		 */
		if (packet->num_bytes <= 8) {
			ret = mspi_ti_k3_small_transfer(controller, req, i, start_time);
			if (ret < 0) {
				goto exit;
			}
		} else {
			/* big transfer via indirect transfer mode */
			if (packet->dir == MSPI_RX) {
				ret = mspi_ti_k3_indirect_read(controller, req, i, start_time);
			} else {
				ret = mspi_ti_k3_indirect_write(controller, req, i, start_time);
			}
			if (ret < 0) {
				goto exit;
			}
		}
	}

exit:
	k_mutex_unlock(&data->lock);
	return ret;
}

static int mspi_ti_k3_set_opcode_lines(const mem_addr_t base_addr, enum mspi_io_mode io_mode)
{
	switch (io_mode) {
	case MSPI_IO_MODE_SINGLE:
	case MSPI_IO_MODE_DUAL_1_1_2:
	case MSPI_IO_MODE_DUAL_1_2_2:
	case MSPI_IO_MODE_QUAD_1_1_4:
	case MSPI_IO_MODE_QUAD_1_4_4:
	case MSPI_IO_MODE_OCTAL_1_1_8:
	case MSPI_IO_MODE_OCTAL_1_8_8:
		MSPI_TI_K3_REG_WRITE(0, DEV_INSTR_RD_CONFIG, INSTR_TYPE, base_addr);
		return 0;
	case MSPI_IO_MODE_DUAL:
		MSPI_TI_K3_REG_WRITE(1, DEV_INSTR_RD_CONFIG, INSTR_TYPE, base_addr);
		return 0;
	case MSPI_IO_MODE_QUAD:
		MSPI_TI_K3_REG_WRITE(2, DEV_INSTR_RD_CONFIG, INSTR_TYPE, base_addr);
		return 0;
	case MSPI_IO_MODE_OCTAL:
		MSPI_TI_K3_REG_WRITE(3, DEV_INSTR_RD_CONFIG, INSTR_TYPE, base_addr);
		return 0;
	default:
		return -ENOTSUP;
	}
}

static int mspi_ti_k3_set_addr_lines(const mem_addr_t base_addr, enum mspi_io_mode io_mode)
{
	switch (io_mode) {
	case MSPI_IO_MODE_SINGLE:
	case MSPI_IO_MODE_DUAL_1_1_2:
	case MSPI_IO_MODE_QUAD_1_1_4:
	case MSPI_IO_MODE_OCTAL_1_1_8:
		MSPI_TI_K3_REG_WRITE(0, DEV_INSTR_RD_CONFIG, ADDR_XFER_TYPE_STD_MODE, base_addr);
		MSPI_TI_K3_REG_WRITE(0, DEV_INSTR_WR_CONFIG, ADDR_XFER_TYPE_STD_MODE, base_addr);
		return 0;
	case MSPI_IO_MODE_DUAL:
	case MSPI_IO_MODE_DUAL_1_2_2:
		MSPI_TI_K3_REG_WRITE(1, DEV_INSTR_RD_CONFIG, ADDR_XFER_TYPE_STD_MODE, base_addr);
		MSPI_TI_K3_REG_WRITE(1, DEV_INSTR_WR_CONFIG, ADDR_XFER_TYPE_STD_MODE, base_addr);
		return 0;
	case MSPI_IO_MODE_QUAD:
	case MSPI_IO_MODE_QUAD_1_4_4:
		MSPI_TI_K3_REG_WRITE(2, DEV_INSTR_RD_CONFIG, ADDR_XFER_TYPE_STD_MODE, base_addr);
		MSPI_TI_K3_REG_WRITE(2, DEV_INSTR_WR_CONFIG, ADDR_XFER_TYPE_STD_MODE, base_addr);
		return 0;
	case MSPI_IO_MODE_OCTAL:
	case MSPI_IO_MODE_OCTAL_1_8_8:
		MSPI_TI_K3_REG_WRITE(3, DEV_INSTR_RD_CONFIG, ADDR_XFER_TYPE_STD_MODE, base_addr);
		MSPI_TI_K3_REG_WRITE(3, DEV_INSTR_WR_CONFIG, ADDR_XFER_TYPE_STD_MODE, base_addr);
		return 0;
	default:
		return -ENOTSUP;
	}
}

static int mspi_ti_k3_set_data_lines(const mem_addr_t base_addr, enum mspi_io_mode io_mode)
{
	switch (io_mode) {
	case MSPI_IO_MODE_SINGLE:
		MSPI_TI_K3_REG_WRITE(0, DEV_INSTR_RD_CONFIG, DATA_XFER_TYPE_EXT_MODE, base_addr);
		MSPI_TI_K3_REG_WRITE(0, DEV_INSTR_WR_CONFIG, DATA_XFER_TYPE_EXT_MODE, base_addr);
		return 0;
	case MSPI_IO_MODE_DUAL:
	case MSPI_IO_MODE_DUAL_1_1_2:
	case MSPI_IO_MODE_DUAL_1_2_2:
		MSPI_TI_K3_REG_WRITE(1, DEV_INSTR_RD_CONFIG, DATA_XFER_TYPE_EXT_MODE, base_addr);
		MSPI_TI_K3_REG_WRITE(1, DEV_INSTR_WR_CONFIG, DATA_XFER_TYPE_EXT_MODE, base_addr);
		return 0;
	case MSPI_IO_MODE_QUAD:
	case MSPI_IO_MODE_QUAD_1_1_4:
	case MSPI_IO_MODE_QUAD_1_4_4:
		MSPI_TI_K3_REG_WRITE(2, DEV_INSTR_RD_CONFIG, DATA_XFER_TYPE_EXT_MODE, base_addr);
		MSPI_TI_K3_REG_WRITE(2, DEV_INSTR_WR_CONFIG, DATA_XFER_TYPE_EXT_MODE, base_addr);
		return 0;
	case MSPI_IO_MODE_OCTAL:
	case MSPI_IO_MODE_OCTAL_1_1_8:
	case MSPI_IO_MODE_OCTAL_1_8_8:
		MSPI_TI_K3_REG_WRITE(3, DEV_INSTR_RD_CONFIG, DATA_XFER_TYPE_EXT_MODE, base_addr);
		MSPI_TI_K3_REG_WRITE(3, DEV_INSTR_WR_CONFIG, DATA_XFER_TYPE_EXT_MODE, base_addr);
		return 0;
	default:
		return -ENOTSUP;
	}
}

int mspi_ti_k3_dev_config(const struct device *controller, const struct mspi_dev_id *dev_id,
			  const enum mspi_dev_cfg_mask param_mask, const struct mspi_dev_cfg *cfg)
{
	const mem_addr_t base_addr = DEVICE_MMIO_GET(controller);
	struct mspi_ti_k3_data *data = controller->data;
	int ret = 0;

	ret = k_mutex_lock(&data->lock, K_MSEC(CONFIG_MSPI_COMPLETION_TIMEOUT_TOLERANCE));

	if (ret < 0) {
		LOG_ERR("Error waiting for MSPI controller lock for changing device config");
		return ret;
	}

	if (param_mask & TI_K3_OSPI_NOT_IMPLEMENT_DEV_CONFIG_PARAMS) {
		LOG_ERR("Device config includes non implemented features");
		return -ENOSYS;
	}
	if (param_mask & TI_K3_OSPI_IGNORED_DEV_CONFIG_PARAMS) {
		LOG_WRN("Device configuration includes ignored parameters. These are taken from "
			"the transceive request instead");
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

	/* Disable OSPI during configuration */
	MSPI_TI_K3_REG_WRITE(0, CONFIG, ENABLE_SPI, base_addr);

	ret = mspi_ti_k3_wait_for_idle(controller);
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

		MSPI_TI_K3_REG_WRITE(num, CONFIG, PERIPH_CS_LINES, base_addr);
	}
	if (param_mask & MSPI_DEVICE_CONFIG_IO_MODE) {
		ret = mspi_ti_k3_set_opcode_lines(base_addr, cfg->io_mode);
		if (ret) {
			goto exit;
		}
		ret = mspi_ti_k3_set_data_lines(base_addr, cfg->io_mode);
		if (ret) {
			goto exit;
		}
		ret = mspi_ti_k3_set_addr_lines(base_addr, cfg->io_mode);
		if (ret) {
			goto exit;
		}
	}
	if (param_mask & MSPI_DEVICE_CONFIG_CPP) {
		switch (cfg->cpp) {
		case MSPI_CPP_MODE_0:
			MSPI_TI_K3_REG_WRITE(0, CONFIG, SEL_CLK_POL, base_addr);
			MSPI_TI_K3_REG_WRITE(0, CONFIG, SEL_CLK_PHASE, base_addr);
			break;
		case MSPI_CPP_MODE_1:
			MSPI_TI_K3_REG_WRITE(0, CONFIG, SEL_CLK_POL, base_addr);
			MSPI_TI_K3_REG_WRITE(1, CONFIG, SEL_CLK_PHASE, base_addr);
			break;
		case MSPI_CPP_MODE_2:
			MSPI_TI_K3_REG_WRITE(1, CONFIG, SEL_CLK_POL, base_addr);
			MSPI_TI_K3_REG_WRITE(0, CONFIG, SEL_CLK_PHASE, base_addr);
			break;
		case MSPI_CPP_MODE_3:
			MSPI_TI_K3_REG_WRITE(1, CONFIG, SEL_CLK_POL, base_addr);
			MSPI_TI_K3_REG_WRITE(1, CONFIG, SEL_CLK_PHASE, base_addr);
			break;
		default:
			LOG_ERR("Invalid clock polarity/phase configuration");
			ret = -ENOTSUP;
			goto exit;
		}
	}
exit:
	/* Re-enable OSPI */
	MSPI_TI_K3_REG_WRITE(1, CONFIG, ENABLE_SPI, base_addr);

	k_mutex_unlock(&data->lock);

	return ret;
}

#ifdef CONFIG_MSPI_TIMING
int mspi_ti_k3_timing(const struct device *controller, const struct mspi_dev_id *dev_id,
		      const uint32_t param_mask, void *timing_cfg)
{
	ARG_UNUSED(dev_id);

	const mem_addr_t base_addr = DEVICE_MMIO_GET(controller);
	struct mspi_ti_k3_data *data = controller->data;
	struct mspi_ti_k3_timing_cfg *timing = timing_cfg;
	int ret;

	ret = k_mutex_lock(&data->lock, K_MSEC(CONFIG_MSPI_COMPLETION_TIMEOUT_TOLERANCE));

	if (ret < 0) {
		LOG_ERR("Error waiting for MSPI controller lock for changing timing");
		return ret;
	}

	if (param_mask & MSPI_TI_K3_TIMING_PARAM_NSS) {
		MSPI_TI_K3_REG_WRITE(timing->nss, DEV_DELAY, D_NSS, base_addr);
	}

	if (param_mask & MSPI_TI_K3_TIMING_PARAM_BTWN) {
		MSPI_TI_K3_REG_WRITE(timing->btwn, DEV_DELAY, D_BTWN, base_addr);
	}

	if (param_mask & MSPI_TI_K3_TIMING_PARAM_AFTER) {
		MSPI_TI_K3_REG_WRITE(timing->after, DEV_DELAY, D_AFTER, base_addr);
	}

	if (param_mask & MSPI_TI_K3_TIMING_PARAM_INIT) {
		MSPI_TI_K3_REG_WRITE(timing->init, DEV_DELAY, D_INIT, base_addr);
	}

	k_mutex_unlock(&data->lock);

	return 0;
}
#endif /* CONFIG_MSPI_TIMING */

static DEVICE_API(mspi, mspi_ti_k3_driver_api) = {
	.config = NULL,
	.dev_config = mspi_ti_k3_dev_config,
	.xip_config = NULL,
	.scramble_config = NULL,
#ifdef CONFIG_MSPI_TIMING
	.timing_config = mspi_ti_k3_timing,
#else
	.timing_config = NULL,
#endif
	.get_channel_status = NULL,
	.register_callback = NULL,
	.transceive = mspi_ti_k3_transceive,
};

#define MSPI_CONFIG(n)                                                                             \
	{.op_mode = DT_INST_ENUM_IDX_OR(n, op_mode, MSPI_OP_MODE_CONTROLLER),                      \
	 .sw_multi_periph = DT_INST_PROP(n, software_multiperipheral)}

#define TI_K3_MSPI_DEFINE(n)                                                                       \
	PINCTRL_DT_DEFINE(DT_DRV_INST(n));                                                         \
	static const struct mspi_ti_k3_config mspi_ti_k3_config##n = {                             \
		DEVICE_MMIO_ROM_INIT(DT_DRV_INST(n)),                                              \
		.pinctrl = PINCTRL_DT_INST_DEV_CONFIG_GET(n),                                      \
		.mspi_config = MSPI_CONFIG(n),                                                     \
		.fifo_addr = DT_REG_ADDR_BY_IDX(DT_DRV_INST(n), 1),                                \
		.sram_allocated_for_read = DT_PROP(DT_DRV_INST(n), sram_allocated_for_read),       \
		.initial_timing_cfg = {                                                            \
			.nss = DT_PROP_OR(DT_DRV_INST(n), init_nss_delay,                          \
					  TI_K3_OSPI_DEFAULT_DELAY),                               \
			.btwn = DT_PROP_OR(DT_DRV_INST(n), init_btwn_delay,                        \
					   TI_K3_OSPI_DEFAULT_DELAY),                              \
			.after = DT_PROP_OR(DT_DRV_INST(n), init_after_delay,                      \
					    TI_K3_OSPI_DEFAULT_DELAY),                             \
			.init = DT_PROP_OR(DT_DRV_INST(n), init_init_delay,                        \
					   TI_K3_OSPI_DEFAULT_DELAY),                              \
		}};                                                                                \
	static struct mspi_ti_k3_data mspi_ti_k3_data##n = {};                                     \
	DEVICE_DT_INST_DEFINE(n, mspi_ti_k3_init, NULL, &mspi_ti_k3_data##n,                       \
			      &mspi_ti_k3_config##n, PRE_KERNEL_2, CONFIG_MSPI_INIT_PRIORITY,      \
			      &mspi_ti_k3_driver_api);

DT_INST_FOREACH_STATUS_OKAY(TI_K3_MSPI_DEFINE)
