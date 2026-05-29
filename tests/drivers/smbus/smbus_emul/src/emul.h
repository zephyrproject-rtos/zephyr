/*
 * Copyright (c) 2022 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/* Read from PCI configuration space */
uint32_t emul_pci_read(unsigned int reg);

/* Write to PCI configuration space */
void emul_pci_write(pcie_bdf_t bdf, unsigned int reg, uint32_t value);

void emul_out8(uint8_t data, io_port_t addr);
uint8_t emul_in8(io_port_t addr);

void emul_set_io(uint8_t value, io_port_t addr);
uint8_t emul_get_io(io_port_t addr);

enum emul_isr_type {
	EMUL_SMBUS_INTR,
	EMUL_SMBUS_SMBALERT,
	EMUL_SMBUS_HOST_NOTIFY,
};

void run_isr(enum emul_isr_type);

struct smbus_peripheral {
	sys_snode_t node;
	uint8_t raw_data[256];
	uint8_t offset;
	uint8_t addr;
	bool smbalert;
	bool smbalert_handled;
	bool host_notify;
};

bool peripheral_handle_host_notify(void);

static inline void peripheral_clear_smbalert(struct smbus_peripheral *periph)
{
	periph->smbalert_handled = false;
}

void emul_register_smbus_peripheral(struct smbus_peripheral *peripheral);
