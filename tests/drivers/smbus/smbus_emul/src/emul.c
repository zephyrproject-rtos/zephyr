/*
 * Copyright (c) 2022 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/types.h>
#include <zephyr/ztest.h>

#include <zephyr/drivers/pcie/pcie.h>
#include <zephyr/drivers/smbus.h>

#include <intel_pch_smbus.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(emul, LOG_LEVEL_DBG);

#include "emul.h"

/**
 * Emulate Intel PCH Host Controller hardware as PCI device with I/O access
 */

/* PCI Configuration space */
uint32_t pci_config_area[32] = {
	[PCIE_CONF_CMDSTAT] = PCIE_CONF_CMDSTAT_INTERRUPT, /* Mark INT */
	[8]	= 1, /* I/O BAR */
	[16]	= 1, /* Enable SMBus */
};

/* I/O and MMIO registers */
uint8_t io_area[24] = {
	0,
};

struct e32_block {
	uint8_t buf[SMBUS_BLOCK_BYTES_MAX];
	uint8_t offset;
} e32;

/**
 * Emulate SMBus peripheral device, connected to the bus as
 * simple EEPROM device of size 256 bytes
 */

/* List of peripheral devices registered */
sys_slist_t peripherals;

void emul_register_smbus_peripheral(struct smbus_peripheral *peripheral)
{
	sys_slist_prepend(&peripherals, &peripheral->node);
}

static struct smbus_peripheral *emul_get_smbus_peripheral(uint8_t addr)
{
	struct smbus_peripheral *peripheral, *tmp;

	SYS_SLIST_FOR_EACH_CONTAINER_SAFE(&peripherals, peripheral, tmp, node) {
		if (peripheral->addr == addr) {
			return peripheral;
		}
	}

	return NULL;
}

static bool peripheral_handle_smbalert(void)
{
	struct smbus_peripheral *peripheral, *tmp, *found = NULL;

	SYS_SLIST_FOR_EACH_CONTAINER_SAFE(&peripherals, peripheral, tmp, node) {
		if (peripheral->smbalert && !peripheral->smbalert_handled) {
			found = peripheral;
		}
	}

	if (found == NULL) {
		LOG_WRN("No (more) smbalert handlers found");
		return false;
	}

	LOG_DBG("Return own address: 0x%02x", found->addr);

	io_area[PCH_SMBUS_HD0] = found->addr;
	found->smbalert_handled = true;

	return true;
}

bool peripheral_handle_host_notify(void)
{
	struct smbus_peripheral *peripheral, *tmp;

	SYS_SLIST_FOR_EACH_CONTAINER_SAFE(&peripherals, peripheral, tmp, node) {
		if (peripheral->host_notify) {
			LOG_DBG("Save own peripheral address to NDA");
			io_area[PCH_SMBUS_NDA] = peripheral->addr << 1;

			return true;
		}
	}

	return false;
}

static void peripheral_write(uint8_t reg, uint8_t value)
{
	uint8_t addr = PCH_SMBUS_TSA_ADDR_GET(io_area[PCH_SMBUS_TSA]);
	struct smbus_peripheral *peripheral;

	peripheral = emul_get_smbus_peripheral(addr);
	if (peripheral) {
		peripheral->raw_data[reg] = value;
		LOG_DBG("peripheral: [0x%02x] <= 0x%02x", reg, value);
	} else {
		LOG_ERR("Peripheral not found, addr 0x%02x", addr);
	}
}

static void peripheral_read(uint8_t reg, uint8_t *value)
{
	uint8_t addr = PCH_SMBUS_TSA_ADDR_GET(io_area[PCH_SMBUS_TSA]);
	struct smbus_peripheral *peripheral;

	peripheral = emul_get_smbus_peripheral(addr);
	if (peripheral) {
		*value = peripheral->raw_data[reg];
		LOG_DBG("peripheral: [0x%02x] => 0x%02x", reg, *value);
	} else {
		LOG_ERR("Peripheral not found, addr 0x%02x", addr);
	}
}

static void emul_start_smbus_protocol(void)
{
	uint8_t smbus_cmd = PCH_SMBUS_HCTL_CMD_GET(io_area[PCH_SMBUS_HCTL]);
	bool write =
		(io_area[PCH_SMBUS_TSA] & PCH_SMBUS_TSA_RW) == SMBUS_MSG_WRITE;
	uint8_t addr = PCH_SMBUS_TSA_ADDR_GET(io_area[PCH_SMBUS_TSA]);
	struct smbus_peripheral *peripheral;

	LOG_DBG("Start SMBUS protocol");

	if (unlikely(addr == SMBUS_ADDRESS_ARA)) {
		if (peripheral_handle_smbalert()) {
			goto isr;
		}
	}

	peripheral = emul_get_smbus_peripheral(addr);
	if (peripheral == NULL) {
		LOG_WRN("Set Device Error");
		emul_set_io(emul_get_io(PCH_SMBUS_HSTS) |
			    PCH_SMBUS_HSTS_DEV_ERROR, PCH_SMBUS_HSTS);
		goto isr;
	}

	switch (smbus_cmd) {
	case PCH_SMBUS_HCTL_CMD_QUICK:
		LOG_DBG("Quick command");
		break;
	case PCH_SMBUS_HCTL_CMD_BYTE:
		if (write) {
			LOG_DBG("Byte Write command");
			peripheral_write(0, io_area[PCH_SMBUS_HCMD]);
		} else {
			LOG_DBG("Byte Read command");
			peripheral_read(0, &io_area[PCH_SMBUS_HD0]);
		}
		break;
	case PCH_SMBUS_HCTL_CMD_BYTE_DATA:
		if (write) {
			LOG_DBG("Byte Data Write command");
			peripheral_write(io_area[PCH_SMBUS_HCMD],
					 io_area[PCH_SMBUS_HD0]);
		} else {
			LOG_DBG("Byte Data Read command");
			peripheral_read(io_area[PCH_SMBUS_HCMD],
					&io_area[PCH_SMBUS_HD0]);
		}
		break;
	case PCH_SMBUS_HCTL_CMD_WORD_DATA:
		if (write) {
			LOG_DBG("Word Data Write command");
			peripheral_write(io_area[PCH_SMBUS_HCMD],
					 io_area[PCH_SMBUS_HD0]);
			peripheral_write(io_area[PCH_SMBUS_HCMD] + 1,
					 io_area[PCH_SMBUS_HD1]);

		} else {
			LOG_DBG("Word Data Read command");
			peripheral_read(io_area[PCH_SMBUS_HCMD],
					&io_area[PCH_SMBUS_HD0]);
			peripheral_read(io_area[PCH_SMBUS_HCMD] + 1,
					&io_area[PCH_SMBUS_HD1]);
		}
		break;
	case PCH_SMBUS_HCTL_CMD_PROC_CALL:
		if (!write) {
			LOG_ERR("Incorrect operation flag");
			return;
		}

		LOG_DBG("Process Call command");

		peripheral_write(io_area[PCH_SMBUS_HCMD],
				 io_area[PCH_SMBUS_HD0]);
		peripheral_write(io_area[PCH_SMBUS_HCMD] + 1,
				 io_area[PCH_SMBUS_HD1]);

		/**
		 * For the testing purposes implement data
		 * swap for the Proc Call, that would be
		 * easy for testing.
		 *
		 * Note: real device should have some other
		 * logic for Proc Call.
		 */
		peripheral_read(io_area[PCH_SMBUS_HCMD],
				&io_area[PCH_SMBUS_HD1]);
		peripheral_read(io_area[PCH_SMBUS_HCMD] + 1,
				&io_area[PCH_SMBUS_HD0]);
		break;
	case PCH_SMBUS_HCTL_CMD_BLOCK:
		if (write) {
			uint8_t count = io_area[PCH_SMBUS_HD0];
			uint8_t reg = io_area[PCH_SMBUS_HCMD];

			LOG_DBG("Block Write command");

			if (count > SMBUS_BLOCK_BYTES_MAX) {
				return;
			}

			for (int i = 0; i < count; i++) {
				peripheral_write(reg++, e32.buf[i]);
			}
		} else {
			/**
			 * count is set by peripheral device, just
			 * assume it to be maximum block count
			 */
			uint8_t count = SMBUS_BLOCK_BYTES_MAX;
			uint8_t reg = io_area[PCH_SMBUS_HCMD];

			LOG_DBG("Block Read command");

			for (int i = 0; i < count; i++) {
				peripheral_read(reg++, &e32.buf[i]);
			}

			/* Set count */
			io_area[PCH_SMBUS_HD0] = count;
		}
		break;
	case PCH_SMBUS_HCTL_CMD_BLOCK_PROC:
		if (!write) {
			LOG_ERR("Incorrect operation flag");
		} else {
			uint8_t snd_count = io_area[PCH_SMBUS_HD0];
			uint8_t reg = io_area[PCH_SMBUS_HCMD];
			uint8_t rcv_count;

			LOG_DBG("Block Process Call command");

			if (snd_count > SMBUS_BLOCK_BYTES_MAX) {
				return;
			}

			/**
			 * Make Block Process Call swap block buffer bytes
			 * for testing purposes only, return the same "count"
			 * bytes
			 */
			for (int i = 0; i < snd_count; i++) {
				peripheral_write(reg++, e32.buf[i]);
			}

			rcv_count = snd_count;
			if (snd_count + rcv_count > SMBUS_BLOCK_BYTES_MAX) {
				return;
			}

			for (int i = 0; i < rcv_count; i++) {
				peripheral_read(--reg, &e32.buf[i]);
			}

			/* Clear offset count */
			e32.offset = 0;

			/* Set count */
			io_area[PCH_SMBUS_HD0] = rcv_count;
		}
		break;
	default:
		LOG_ERR("Protocol is not implemented yet in emul");
		break;
	}

isr:
	if (io_area[PCH_SMBUS_HCTL] & PCH_SMBUS_HCTL_INTR_EN) {
		/* Fire emulated interrupt if enabled */
		run_isr(EMUL_SMBUS_INTR);
	}
}

static void emul_evaluate_write(uint8_t value, io_port_t addr)
{
	switch (addr) {
	case PCH_SMBUS_HCTL:
		if (value & PCH_SMBUS_HCTL_START) {
			/* This is write only */
			io_area[PCH_SMBUS_HCTL] = value & ~PCH_SMBUS_HCTL_START;

			emul_start_smbus_protocol();
		}
		break;
	default:
		break;
	}
}

static const char *pch_get_reg_name(uint8_t reg)
{
	switch (reg) {
	case PCH_SMBUS_HSTS:
		return "HSTS";
	case PCH_SMBUS_HCTL:
		return "HCTL";
	case PCH_SMBUS_HCMD:
		return "HCMD";
	case PCH_SMBUS_TSA:
		return "TSA";
	case PCH_SMBUS_HD0:
		return "HD0";
	case PCH_SMBUS_HD1:
		return "HD1";
	case PCH_SMBUS_HBD:
		return "HBD";
	case PCH_SMBUS_PEC:
		return "PEC";
	case PCH_SMBUS_RSA:
		return "RSA";
	case PCH_SMBUS_SD:
		return "SD";
	case PCH_SMBUS_AUXS:
		return "AUXS";
	case PCH_SMBUS_AUXC:
		return "AUXC";
	case PCH_SMBUS_SMLC:
		return "SMLC";
	case PCH_SMBUS_SMBC:
		return "SMBC";
	case PCH_SMBUS_SSTS:
		return "SSTS";
	case PCH_SMBUS_SCMD:
		return "SCMD";
	case PCH_SMBUS_NDA:
		return "NDA";
	case PCH_SMBUS_NDLB:
		return "NDLB";
	case PCH_SMBUS_NDHB:
		return "NDHB";
	default:
		return "Unknown";
	}
}

uint32_t emul_pci_read(unsigned int reg)
{
	LOG_DBG("PCI [%x] => 0x%x", reg, pci_config_area[reg]);
	return pci_config_area[reg];
}

void emul_pci_write(pcie_bdf_t bdf, unsigned int reg, uint32_t value)
{
	LOG_DBG("PCI [%x] <= 0x%x", reg, value);
	pci_config_area[reg] = value;
}

/* This function is used to set registers for emulation purpose */
void emul_set_io(uint8_t value, io_port_t addr)
{
	io_area[addr] = value;
}

uint8_t emul_get_io(io_port_t addr)
{
	return io_area[addr];
}

void emul_out8(uint8_t value, io_port_t addr)
{
	switch (addr) {
	case PCH_SMBUS_HSTS:
		/* Writing clears status bits */
		io_area[addr] &= ~value;
		break;
	case PCH_SMBUS_SSTS:
		/* Writing clears status bits */
		io_area[addr] &= ~value;
		break;
	case PCH_SMBUS_HBD:
		/* Using internal E32 buffer offset */
		e32.buf[e32.offset++] = value;
		break;
	case PCH_SMBUS_AUXC:
		if (value & PCH_SMBUS_AUXC_EN_32BUF) {
			LOG_DBG("Enabled 32 bit buffer block mode");
		}
		io_area[addr] = value;
		break;
	default:
		io_area[addr] = value;
		break;
	}

	LOG_DBG("I/O [%s] <= 0x%x => 0x%x", pch_get_reg_name(addr), value,
		io_area[addr]);

	/**
	 * Evaluate should decide about starting actual SMBus
	 * protocol transaction emulation
	 */
	emul_evaluate_write(value, addr);
}

uint8_t emul_in8(io_port_t addr)
{
	uint8_t value;

	switch (addr) {
	case PCH_SMBUS_HBD:
		/* Using internal E32 buffer offset */
		value = e32.buf[e32.offset++];
		break;
	case PCH_SMBUS_HCTL:
		/* Clear e32 block buffer offset */
		e32.offset = 0;
		LOG_WRN("E32 buffer offset is cleared");
		value = io_area[addr];
		break;
	default:
		value = io_area[addr];
		break;
	}

	LOG_DBG("I/O [%s] => 0x%x", pch_get_reg_name(addr), value);

	return value;
}
