/*
 * Copyright The Zephyr Project Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Emulator for a CMIS managed pluggable module. It serves a canned Page 00h
 * image over the single standard I2C address: Lower Memory is page-independent,
 * while Upper Memory (offset 0x80 and above) returns the page selected through
 * the page-select register. Only Page 00h is modeled.
 */

#define DT_DRV_COMPAT sff_cmis

#include <string.h>

#include <zephyr/cmis/cmis.h>
#include <zephyr/device.h>
#include <zephyr/drivers/emul.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/i2c_emul.h>
#include <zephyr/sys/byteorder.h>

/* CMIS page-select register in Lower Memory. */
#define CMIS_REG_PAGE_SELECT 0x7FU

struct cmis_emul_data {
	uint8_t mem[CMIS_PAGE_00H_SIZE]; /* Lower Memory plus Upper Page 00h. */
	uint8_t page;                    /* Currently selected upper page. */
};

static void emul_set_ascii(uint8_t *buf, size_t off, const char *str, size_t field_len)
{
	size_t len = strlen(str);

	for (size_t i = 0U; i < field_len; i++) {
		buf[off + i] = (i < len) ? (uint8_t)str[i] : (uint8_t)' ';
	}
}

static void emul_build_module(struct cmis_emul_data *data)
{
	uint8_t *m = data->mem;

	(void)memset(m, 0, sizeof(data->mem));
	data->page = 0U;

	/* Lower Memory: a fictional CMIS 5.0 QSFP-DD 400G single-mode module. */
	m[CMIS_LOWER_IDENTIFIER] = CMIS_ID_QSFP_DD;
	m[CMIS_LOWER_REVISION] = 0x50U;
	m[CMIS_LOWER_MEMORY_MODEL] = 0x00U;
	m[CMIS_LOWER_MODULE_STATE] = CMIS_MODULE_STATE_READY << 1U;
	m[CMIS_LOWER_MODULE_FLAGS] = BIT(0) | BIT(4); /* temp high alarm, vcc high alarm */
	sys_put_be16(0x2D80U, &m[CMIS_LOWER_TEMPERATURE]); /* +45.5 C */
	sys_put_be16(33000U, &m[CMIS_LOWER_VCC]);          /* 3.300 V */
	m[CMIS_LOWER_MEDIA_TYPE] = CMIS_MEDIA_OPTICAL_SMF;

	/* Upper Page 00h: administrative information. */
	emul_set_ascii(m, CMIS_PAGE00_VENDOR_NAME, "ACME PHOTONICS", CMIS_VENDOR_NAME_LEN);
	m[CMIS_PAGE00_VENDOR_OUI + 0U] = 0x00U;
	m[CMIS_PAGE00_VENDOR_OUI + 1U] = 0x90U;
	m[CMIS_PAGE00_VENDOR_OUI + 2U] = 0x65U;
	emul_set_ascii(m, CMIS_PAGE00_VENDOR_PN, "QSFP-DD-400G", CMIS_VENDOR_PN_LEN);
	emul_set_ascii(m, CMIS_PAGE00_VENDOR_REV, "B2", CMIS_VENDOR_REV_LEN);
	emul_set_ascii(m, CMIS_PAGE00_VENDOR_SN, "QD2400001234", CMIS_VENDOR_SN_LEN);
	emul_set_ascii(m, CMIS_PAGE00_DATE_CODE, "240620", CMIS_DATE_CODE_LEN);
	m[CMIS_PAGE00_POWER_CLASS] = 4U << 5U; /* power class 4 */
	m[CMIS_PAGE00_MAX_POWER] = 56U;        /* 56 * 0.25 W == 14 W */
}

static int cmis_emul_transfer(const struct emul *target, struct i2c_msg *msgs, int num_msgs,
			      int addr)
{
	struct cmis_emul_data *data = target->data;
	uint8_t offset;

	ARG_UNUSED(addr);

	/* A lone write is a register write such as a page select. */
	if (num_msgs == 1) {
		if (((msgs[0].flags & I2C_MSG_READ) != 0U) || (msgs[0].len != 2U)) {
			return -EIO;
		}
		if (msgs[0].buf[0] == CMIS_REG_PAGE_SELECT) {
			data->page = msgs[0].buf[1];
		}
		return 0;
	}

	/* Otherwise model an EEPROM read: a one-byte offset write, then a read. */
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
	if (((size_t)offset + msgs[1].len) > CMIS_PAGE_00H_SIZE) {
		return -EIO;
	}

	if (((size_t)offset + msgs[1].len <= CMIS_PAGE_SIZE) || (data->page == 0U)) {
		/* Lower Memory is page-independent; Upper Memory is modeled for page 0. */
		(void)memcpy(msgs[1].buf, &data->mem[offset], msgs[1].len);
	} else {
		(void)memset(msgs[1].buf, 0, msgs[1].len);
	}

	return 0;
}

static const struct i2c_emul_api cmis_emul_api = {
	.transfer = cmis_emul_transfer,
};

static int cmis_emul_init(const struct emul *target, const struct device *parent)
{
	struct cmis_emul_data *data = target->data;

	ARG_UNUSED(parent);

	emul_build_module(data);

	return 0;
}

#define CMIS_EMUL(n)                                                                               \
	static struct cmis_emul_data cmis_emul_data_##n;                                           \
	EMUL_DT_INST_DEFINE(n, cmis_emul_init, &cmis_emul_data_##n, NULL, &cmis_emul_api, NULL)

DT_INST_FOREACH_STATUS_OKAY(CMIS_EMUL)
