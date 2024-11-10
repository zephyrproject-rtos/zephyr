/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/* @file
 * @brief NRF Wi-Fi radio FICR programming functions
 */

#include <zephyr/logging/log.h>
#include <zephyr/kernel.h>
#include "rpu_if.h"
#include "rpu_hw_if.h"
#include "ficr_prog.h"


LOG_MODULE_DECLARE(wifi_nrf, CONFIG_WIFI_NRF70_LOG_LEVEL);

static void write_word(unsigned int addr, unsigned int data)
{
	rpu_write(addr, &data, 4);
}

static void read_word(unsigned int addr, unsigned int *data)
{
	rpu_read(addr, data, 4);
}

unsigned int check_protection(unsigned int *buff, unsigned int off1, unsigned int off2,
		    unsigned int off3, unsigned int off4)
{
	if ((buff[off1] == OTP_PROGRAMMED) &&
		(buff[off2] == OTP_PROGRAMMED)  &&
		(buff[off3] == OTP_PROGRAMMED) &&
		(buff[off4] == OTP_PROGRAMMED))
		return OTP_PROGRAMMED;
	else if ((buff[off1] == OTP_FRESH_FROM_FAB) &&
		(buff[off2] == OTP_FRESH_FROM_FAB) &&
		(buff[off3] == OTP_FRESH_FROM_FAB) &&
		(buff[off4] == OTP_FRESH_FROM_FAB))
		return OTP_FRESH_FROM_FAB;
	else if ((buff[off1] == OTP_ENABLE_PATTERN) &&
		(buff[off2] == OTP_ENABLE_PATTERN) &&
		(buff[off3] == OTP_ENABLE_PATTERN) &&
		(buff[off4] == OTP_ENABLE_PATTERN))
		return OTP_ENABLE_PATTERN;
	else
		return OTP_INVALID;
}


static void set_otp_timing_reg_40mhz(void)
{
	write_word(OTP_TIMING_REG1_ADDR, OTP_TIMING_REG1_VAL);
	write_word(OTP_TIMING_REG2_ADDR, OTP_TIMING_REG2_VAL);
}

static int poll_otp_ready(void)
{
	int otp_mem_status = 0;
	int poll = 0;

	while (poll != 100) {
		read_word(OTP_POLL_ADDR, &otp_mem_status);

		if ((otp_mem_status & OTP_READY) == OTP_READY) {
			return 0;
		}
		poll++;
	}
	LOG_ERR("OTP is not ready");
	return -ENOEXEC;
}


static int req_otp_standby_mode(void)
{
	write_word(OTP_RWSBMODE_ADDR, 0x0);
	return poll_otp_ready();
}


static int otp_wr_voltage_2V5(void)
{
	int err;

	err = req_otp_standby_mode();

	if (err) {
		LOG_ERR("Failed Setting OTP voltage IOVDD to 2.5V");
		return -ENOEXEC;
	}
	write_word(OTP_VOLTCTRL_ADDR, OTP_VOLTCTRL_2V5);
	return 0;
}

static int poll_otp_read_valid(void)
{
	int otp_mem_status = 0;
	int poll = 0;

	while (poll < 100) {
		read_word(OTP_POLL_ADDR, &otp_mem_status);

		if ((otp_mem_status & OTP_READ_VALID) == OTP_READ_VALID) {
			return 0;
		}
		poll++;
	}
	LOG_ERR("%s: OTP read failed", __func__);
	return -ENOEXEC;
}

static int poll_otp_wrdone(void)
{
	int otp_mem_status = 0;
	int poll = 0;

	while (poll < 100) {
		read_word(OTP_POLL_ADDR, &otp_mem_status);

		if ((otp_mem_status & OTP_WR_DONE) == OTP_WR_DONE) {
			return 0;
		}
		poll++;
	}
	LOG_ERR("%s: OTP write done failed", __func__);
	return -ENOEXEC;
}

static int req_otp_read_mode(void)
{
	write_word(OTP_RWSBMODE_ADDR, OTP_READ_MODE);
	return poll_otp_ready();
}


static int req_otp_byte_write_mode(void)
{
	write_word(OTP_RWSBMODE_ADDR, OTP_BYTE_WRITE_MODE);
	return poll_otp_ready();
}

static unsigned int read_otp_location(unsigned int offset, unsigned int *read_val)
{
	int err;

	write_word(OTP_RDENABLE_ADDR,  offset);
	err = poll_otp_read_valid();
	if (err) {
		LOG_ERR("OTP read failed");
		return err;
	}
	read_word(OTP_READREG_ADDR, read_val);

	return 0;
}

static int  write_otp_location(unsigned int otp_location_offset, unsigned int otp_data)
{
	write_word(OTP_WRENABLE_ADDR,  otp_location_offset);
	write_word(OTP_WRITEREG_ADDR,  otp_data);

	return poll_otp_wrdone();
}


static int otp_rd_voltage_1V8(void)
{
	int err;

	err = req_otp_standby_mode();
	if (err) {
		LOG_ERR("error in %s", __func__);
		return err;
	}
	write_word(OTP_VOLTCTRL_ADDR, OTP_VOLTCTRL_1V8);

	return 0;
}

static int update_mac_addr(unsigned int index, unsigned int *write_val)
{
	int ret = 0;

	for (int i = 0; i < 2; i++) {
		ret = write_otp_location(MAC0_ADDR + 2 * index + i, write_val[i]);
		if (ret == -ENOEXEC) {
			LOG_ERR("FICR: Failed to update MAC ADDR%d", index);
			break;
		}
		LOG_INF("mac addr %d : Reg%d (0x%x) = 0x%04x",
					index, (i+1), (MAC0_ADDR + i) << 2, write_val[i]);
	}
	return ret;
}

int write_otp_memory(unsigned int otp_addr, unsigned int *write_val)
{
	int err = 0;
	int	mask_val;
	int ret = 0;
	int retrim_loc = 0;

	err = poll_otp_ready();
	if (err) {
		LOG_ERR("err in otp ready poll");
		return err;
	}

	set_otp_timing_reg_40mhz();

	err = otp_wr_voltage_2V5();
	if (err) {
		LOG_ERR("error in write_voltage 2V5");
		goto _exit_otp_write;
	}

	err = req_otp_byte_write_mode();
	if (err) {
		LOG_ERR("error in OTP byte write mode");
		goto _exit_otp_write;
	}

	switch (otp_addr) {
	case REGION_PROTECT:
		write_otp_location(REGION_PROTECT, write_val[0]);
		write_otp_location(REGION_PROTECT+1, write_val[0]);
		write_otp_location(REGION_PROTECT+2, write_val[0]);
		write_otp_location(REGION_PROTECT+3, write_val[0]);

		LOG_INF("Written REGION_PROTECT0 (0x%x) : 0x%04x",
						(REGION_PROTECT << 2), write_val[0]);
		LOG_INF("Written REGION_PROTECT1 (0x%x) : 0x%04x",
						(REGION_PROTECT+1) << 2, write_val[0]);
		LOG_INF("Written REGION_PROTECT2 (0x%x) : 0x%04x",
						(REGION_PROTECT+2) << 2, write_val[0]);
		LOG_INF("Written REGION_PROTECT3 (0x%x) : 0x%04x",
						(REGION_PROTECT+3) << 2, write_val[0]);
		break;
	case QSPI_KEY:
		mask_val = QSPI_KEY_FLAG_MASK;
		for (int i = 0; i < QSPI_KEY_LENGTH_BYTES / 4; i++) {
			ret = write_otp_location(QSPI_KEY + i, write_val[i]);
			if (ret == -ENOEXEC) {
				LOG_ERR("FICR: Failed to write QSPI key offset-%d", QSPI_KEY + i);
				goto _exit_otp_write;
			}
			LOG_INF("Written QSPI_KEY0 (0x%x) : 0x%04x",
						(QSPI_KEY + i) << 2, write_val[i]);
		}
		write_otp_location(REGION_DEFAULTS, mask_val);
		LOG_INF("Written REGION_DEFAULTS (0x%x) : 0x%04x",
						(REGION_DEFAULTS) << 2, mask_val);
		break;
	case MAC0_ADDR:
		mask_val = MAC0_ADDR_FLAG_MASK;
		ret = update_mac_addr(0, write_val);
		if (ret == -ENOEXEC) {
			goto _exit_otp_write;
		}

		write_otp_location(REGION_DEFAULTS, mask_val);
		LOG_INF("Written MAC address 0");
		LOG_INF("Written REGION_DEFAULTS (0x%x) : 0x%04x",
						(REGION_DEFAULTS) << 2, mask_val);
		break;
	case MAC1_ADDR:
		mask_val = MAC1_ADDR_FLAG_MASK;
		ret = update_mac_addr(1, write_val);
		if (ret == -ENOEXEC) {
			goto _exit_otp_write;
		}
		write_otp_location(REGION_DEFAULTS, mask_val);
		LOG_INF("Written MAC address 1");
		LOG_INF("Written REGION_DEFAULTS (0x%x) : 0x%04x",
						(REGION_DEFAULTS) << 2, mask_val);
		break;
	case CALIB_XO:
		mask_val = CALIB_XO_FLAG_MASK;
		ret = write_otp_location(CALIB_XO, write_val[0]);

		if (ret == -ENOEXEC) {
			LOG_ERR("XO_Update Exception");
			goto _exit_otp_write;
		} else {
			write_otp_location(REGION_DEFAULTS, mask_val);

			LOG_INF("Written CALIB_XO (0x%x) to 0x%04x",
						CALIB_XO << 2, write_val[0]);
			LOG_INF("Written REGION_DEFAULTS (0x%x) : 0x%04x",
						(REGION_DEFAULTS) << 2, mask_val);
		}
		break;
	case PRODRETEST_PROGVERSION:
		ret = write_otp_location(PRODRETEST_PROGVERSION, *write_val);

		if (ret == -ENOEXEC) {
			LOG_ERR("PRODRETEST.PROGVERSION_Update Exception");
			goto _exit_otp_write;
		} else {
			LOG_INF("Written PRODRETEST.PROGVERSION 0x%04x", *write_val);
		}
		break;
	case PRODRETEST_TRIM0:
	case PRODRETEST_TRIM1:
	case PRODRETEST_TRIM2:
	case PRODRETEST_TRIM3:
	case PRODRETEST_TRIM4:
	case PRODRETEST_TRIM5:
	case PRODRETEST_TRIM6:
	case PRODRETEST_TRIM7:
	case PRODRETEST_TRIM8:
	case PRODRETEST_TRIM9:
	case PRODRETEST_TRIM10:
	case PRODRETEST_TRIM11:
	case PRODRETEST_TRIM12:
	case PRODRETEST_TRIM13:
	case PRODRETEST_TRIM14:
		retrim_loc = otp_addr - PRODRETEST_TRIM0;
		ret = write_otp_location(otp_addr, *write_val);

		if (ret == -ENOEXEC) {
			LOG_ERR("PRODRETEST.TRIM_Update Exception");
			goto _exit_otp_write;
		} else {
			LOG_INF("Written PRODRETEST.TRIM%d 0x%04x",
						retrim_loc, *write_val);
		}
		break;
	case REGION_DEFAULTS:
		write_otp_location(REGION_DEFAULTS, write_val[0]);

		LOG_INF("Written REGION_DEFAULTS (0x%x) to 0x%04x",
						REGION_DEFAULTS << 2, write_val[0]);
		break;
	default:
		LOG_ERR("unknown field received: %d", otp_addr);

	}
	return ret;

_exit_otp_write:
	err  = req_otp_standby_mode();
	err |= otp_rd_voltage_1V8();
	return err;
}

int read_otp_memory(unsigned int otp_addr, unsigned int *read_val, int len)
{
	int	err;

	err = poll_otp_ready();
	if (err) {
		LOG_ERR("err in otp ready poll");
		return -ENOEXEC;
	}

	set_otp_timing_reg_40mhz();

	err = otp_rd_voltage_1V8();
	if (err) {
		LOG_ERR("error in read_voltage 1V8");
		return -ENOEXEC;
	}

	err = req_otp_read_mode();
	if (err) {
		LOG_ERR("error in req_otp_read_mode()");
		return -ENOEXEC;
	}

	for (int i = 0; i < len; i++) {
		read_otp_location(otp_addr + i, &read_val[i]);
	}

	return req_otp_standby_mode();
}
