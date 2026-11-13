/*
 * Copyright The Zephyr Project Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Emulator for an SFF-8472 managed SFP/SFP+ module. It serves a canned module
 * image over the two standard I2C addresses: the serial ID page (A0h) at the
 * node's reg address (0x50) and the diagnostics page (A2h) at 0x51, which is
 * registered during initialization.
 */

#define DT_DRV_COMPAT zephyr_sfp_module_emul

#include <string.h>

#include <zephyr/device.h>
#include <zephyr/drivers/emul.h>
#include <zephyr/drivers/emul_stub_device.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/i2c_emul.h>
#include <zephyr/sff/sff8472.h>
#include <zephyr/sys/byteorder.h>

#include "emul_sff8472.h"

#define SFF8472_EMUL_DIAG_ADDR 0x51U /* diagnostics page (A2h), fixed by SFF-8472 */

struct sff8472_emul_data {
	uint8_t page_a0[SFF8472_PAGE_SIZE];
	uint8_t page_a2[SFF8472_PAGE_SIZE];
	struct i2c_emul diag_emul; /* secondary emulator for the A2h address */
};

static void emul_set_ascii(uint8_t *buf, size_t off, const char *str, size_t field_len)
{
	size_t len = strlen(str);

	for (size_t i = 0U; i < field_len; i++) {
		buf[off + i] = (i < len) ? (uint8_t)str[i] : (uint8_t)' ';
	}
}

static void emul_build_module(struct sff8472_emul_data *data)
{
	uint8_t *a0 = data->page_a0;
	uint8_t *a2 = data->page_a2;
	uint8_t cc;

	(void)memset(a0, 0, SFF8472_PAGE_SIZE);
	(void)memset(a2, 0, SFF8472_PAGE_SIZE);

	/* Serial ID page: a fictional 10G 850 nm SFP+ with internal DDM. */
	a0[SFF8472_A0_IDENTIFIER] = SFF8472_ID_SFP;
	a0[SFF8472_A0_CONNECTOR] = 0x07U; /* LC */
	a0[SFF8472_A0_BR_NOMINAL] = 103U; /* 10300 MBd */
	emul_set_ascii(a0, SFF8472_A0_VENDOR_NAME, "ACME PHOTONICS", SFF8472_VENDOR_NAME_LEN);
	a0[SFF8472_A0_VENDOR_OUI + 0U] = 0x00U;
	a0[SFF8472_A0_VENDOR_OUI + 1U] = 0x90U;
	a0[SFF8472_A0_VENDOR_OUI + 2U] = 0x65U;
	emul_set_ascii(a0, SFF8472_A0_VENDOR_PN, "ACME-10G-SR", SFF8472_VENDOR_PN_LEN);
	emul_set_ascii(a0, SFF8472_A0_VENDOR_REV, "A1", SFF8472_VENDOR_REV_LEN);
	sys_put_be16(850U, &a0[SFF8472_A0_WAVELENGTH]);
	emul_set_ascii(a0, SFF8472_A0_VENDOR_SN, "SN12345678", SFF8472_VENDOR_SN_LEN);
	emul_set_ascii(a0, SFF8472_A0_DATE_CODE, "230115", SFF8472_DATE_CODE_LEN);
	a0[SFF8472_A0_DIAG_MON_TYPE] = SFF8472_DIAG_MON_DDM_IMPLEMENTED | SFF8472_DIAG_MON_INT_CAL;

	cc = 0U;
	for (size_t i = 0U; i < SFF8472_A0_CC_BASE; i++) {
		cc += a0[i];
	}
	a0[SFF8472_A0_CC_BASE] = cc;

	cc = 0U;
	for (size_t i = 64U; i < SFF8472_A0_CC_EXT; i++) {
		cc += a0[i];
	}
	a0[SFF8472_A0_CC_EXT] = cc;

	/* Diagnostics page: live values with a few alarm and warning flags. */
	sys_put_be16(0x2D80U, &a2[SFF8472_A2_TEMP]);   /* +45.5 C */
	sys_put_be16(33000U, &a2[SFF8472_A2_VCC]);     /* 3.300 V */
	sys_put_be16(3250U, &a2[SFF8472_A2_TX_BIAS]);  /* 6.5 mA */
	sys_put_be16(5000U, &a2[SFF8472_A2_TX_POWER]); /* 500 uW */
	sys_put_be16(4000U, &a2[SFF8472_A2_RX_POWER]); /* 400 uW */
	a2[SFF8472_A2_ALARM_FLAGS] = BIT(7) | BIT(3);  /* temp high, tx bias high */
	a2[SFF8472_A2_ALARM_FLAGS + 1U] = BIT(6);      /* rx power low */
	a2[SFF8472_A2_WARNING_FLAGS] = BIT(4);         /* vcc low */
}

static int sff8472_emul_transfer(const struct emul *target, struct i2c_msg *msgs, int num_msgs,
				 int addr)
{
	struct sff8472_emul_data *data = target->data;
	const uint8_t *page;
	uint8_t offset;

	if (addr == (int)SFF8472_EMUL_DIAG_ADDR) {
		page = data->page_a2;
	} else {
		page = data->page_a0;
	}

	/* Model an EEPROM read: a one-byte offset write followed by a read. */
	if (num_msgs != 2) {
		return -EIO;
	}
	if (((msgs[0].flags & I2C_MSG_READ) != 0U) || (msgs[0].len != 1U)) {
		return -EIO;
	}
	if ((msgs[1].flags & I2C_MSG_READ) == 0U) {
		return -EIO;
	}

	offset = msgs[0].buf[0];
	if (((size_t)offset + msgs[1].len) > SFF8472_PAGE_SIZE) {
		return -EIO;
	}

	(void)memcpy(msgs[1].buf, &page[offset], msgs[1].len);

	return 0;
}

static const struct i2c_emul_api sff8472_emul_api = {
	.transfer = sff8472_emul_transfer,
};

void emul_sff8472_reset(const struct emul *target)
{
	emul_build_module(target->data);
}

int emul_sff8472_set_byte(const struct emul *target, bool diag_page, uint8_t offset,
			  uint8_t value)
{
	struct sff8472_emul_data *data = target->data;

	if (diag_page) {
		data->page_a2[offset] = value;
	} else {
		data->page_a0[offset] = value;
	}

	return 0;
}

static int sff8472_emul_init(const struct emul *target, const struct device *parent)
{
	struct sff8472_emul_data *data = target->data;

	emul_build_module(data);

	data->diag_emul.target = target;
	data->diag_emul.api = &sff8472_emul_api;
	data->diag_emul.addr = SFF8472_EMUL_DIAG_ADDR;

	return i2c_emul_register(parent, &data->diag_emul);
}

#define SFF8472_EMUL(n)                                                                            \
	BUILD_ASSERT(DT_INST_REG_ADDR(n) == 0x50, "an SFF-8472 module answers at 0x50");          \
	static struct sff8472_emul_data sff8472_emul_data_##n;                                     \
	EMUL_DT_INST_DEFINE(n, sff8472_emul_init, &sff8472_emul_data_##n, NULL, &sff8472_emul_api, \
			    NULL)

DT_INST_FOREACH_STATUS_OKAY(SFF8472_EMUL)

/* The module node has no driver of its own: the sff,sfp cage driver reads it. */
DT_INST_FOREACH_STATUS_OKAY(EMUL_STUB_DEVICE)
