/*
 * Copyright (c) 2017, Intel Corporation
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 * 3. Neither the name of the Intel Corporation nor the names of its
 *    contributors may be used to endorse or promote products derived from this
 *    software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE INTEL CORPORATION OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include "qm_flash.h"

#ifndef UNIT_TEST
#if (QUARK_SE)
qm_flash_reg_t *qm_flash[QM_FLASH_NUM] = {(qm_flash_reg_t *)QM_FLASH_BASE_0,
					  (qm_flash_reg_t *)QM_FLASH_BASE_1};
#elif(QUARK_D2000)
qm_flash_reg_t *qm_flash[QM_FLASH_NUM] = {(qm_flash_reg_t *)QM_FLASH_BASE_0};
#endif
#endif

static __inline__ bool qm_flash_check_otp_locked(const uint32_t flash_stts)
{
	return (
	    (QM_FLASH_STTS_ROM_PROG == (flash_stts & QM_FLASH_STTS_ROM_PROG)));
}

int qm_flash_set_config(const qm_flash_t flash, const qm_flash_config_t *cfg)
{
	QM_CHECK(flash < QM_FLASH_NUM, -EINVAL);
	QM_CHECK(cfg != NULL, -EINVAL);
	QM_CHECK(cfg->wait_states <= QM_FLASH_MAX_WAIT_STATES, -EINVAL);
	QM_CHECK(cfg->us_count <= QM_FLASH_MAX_US_COUNT, -EINVAL);
	QM_CHECK(cfg->write_disable <= QM_FLASH_WRITE_DISABLE, -EINVAL);

	qm_flash_reg_t *const controller = QM_FLASH[flash];

	controller->tmg_ctrl =
	    (controller->tmg_ctrl & QM_FLASH_TMG_DEF_MASK) |
	    (cfg->us_count | (cfg->wait_states << QM_FLASH_WAIT_STATE_OFFSET));

	if (cfg->write_disable == QM_FLASH_WRITE_DISABLE) {
		controller->ctrl |= QM_FLASH_WRITE_DISABLE_VAL;
	}

	return 0;
}

int qm_flash_word_write(const qm_flash_t flash, const qm_flash_region_t region,
			uint32_t f_addr, const uint32_t data)
{
	QM_CHECK(flash < QM_FLASH_NUM, -EINVAL);
	QM_CHECK(region <= QM_FLASH_REGION_NUM, -EINVAL);
	QM_CHECK(f_addr < QM_FLASH_MAX_ADDR, -EINVAL);

	volatile uint32_t *p_wr_data, *p_wr_ctrl;

	qm_flash_reg_t *const controller = QM_FLASH[flash];

	/* Rom and flash write registers are laid out the same, but different */
	/* locations in memory, so point to those to have the same function to*/
	/* update page section based on main or rom. */
	switch (region) {

	case QM_FLASH_REGION_SYS:
		p_wr_data = &controller->flash_wr_data;
		p_wr_ctrl = &controller->flash_wr_ctrl;
#if (QUARK_D2000)
		/* Main flash memory starts after flash data section. */
		f_addr += QM_FLASH_REGION_DATA_0_SIZE;
#endif
		break;

#if (QUARK_D2000)
	case QM_FLASH_REGION_DATA:
		p_wr_data = &controller->flash_wr_data;
		p_wr_ctrl = &controller->flash_wr_ctrl;
		break;
#endif

	case QM_FLASH_REGION_OTP:

		if (qm_flash_check_otp_locked(controller->flash_stts)) {
			return -EACCES;
		}

		p_wr_data = &controller->rom_wr_data;
		p_wr_ctrl = &controller->rom_wr_ctrl;
		break;

	default:
		return -EINVAL;
		break;
	}
	/* Update address to include the write_address offset. */
	f_addr <<= WR_ADDR_OFFSET;

	*p_wr_data = data;
	*p_wr_ctrl = f_addr |= WR_REQ;
	/* Wait for write to finish. */
	while (!(controller->flash_stts & WR_DONE))
		;
	return 0;
}

int qm_flash_page_write(const qm_flash_t flash, const qm_flash_region_t region,
			uint32_t page_num, const uint32_t *const data,
			uint32_t len)
{
	QM_CHECK(flash < QM_FLASH_NUM, -EINVAL);
	QM_CHECK(region <= QM_FLASH_REGION_NUM, -EINVAL);
	QM_CHECK(page_num <= QM_FLASH_MAX_PAGE_NUM, -EINVAL);
	QM_CHECK(data != NULL, -EINVAL);
	QM_CHECK(len <= QM_FLASH_PAGE_SIZE_DWORDS, -EINVAL);

	uint32_t i;
	volatile uint32_t *p_wr_data, *p_wr_ctrl;

	qm_flash_reg_t *const controller = QM_FLASH[flash];

	/* Rom and flash write registers are laid out the same, but different */
	/* locations in memory, so point to those to have the same function to*/
	/* update page section based on main or rom. */
	switch (region) {

	case QM_FLASH_REGION_SYS:
#if (QUARK_D2000)
		page_num += QM_FLASH_REGION_DATA_0_PAGES;

	case QM_FLASH_REGION_DATA:
#endif
		p_wr_data = &controller->flash_wr_data;
		p_wr_ctrl = &controller->flash_wr_ctrl;
		break;

	case QM_FLASH_REGION_OTP:

		/* Check if OTP locked. */
		if (qm_flash_check_otp_locked(controller->flash_stts)) {
			return -EACCES;
		}

		p_wr_data = &controller->rom_wr_data;
		p_wr_ctrl = &controller->rom_wr_ctrl;
		break;

	default:
		return -EINVAL;
		break;
	}
	/* Update address to include the write_address offset. */
	page_num <<= (QM_FLASH_PAGE_SIZE_BITS + WR_ADDR_OFFSET);

	/* Erase the Flash page. */
	*p_wr_ctrl = page_num | ER_REQ;

	/* Wait for the erase to complete. */
	while (!(controller->flash_stts & ER_DONE))
		;

	/* Write bytes into Flash. */
	for (i = 0; i < len; i++) {
		*p_wr_data = data[i];
		*p_wr_ctrl = page_num;
		*p_wr_ctrl |= WR_REQ;
		page_num += QM_FLASH_ADDR_INC;
		/* Wait for write to finish. */
		while (!(controller->flash_stts & WR_DONE))
			;
	}
	return 0;
}

int qm_flash_page_update(const qm_flash_t flash, const qm_flash_region_t region,
			 uint32_t f_addr, uint32_t *const page_buffer,
			 const uint32_t *const data_buffer, uint32_t len)
{
	QM_CHECK(flash < QM_FLASH_NUM, -EINVAL);
	QM_CHECK(region <= QM_FLASH_REGION_NUM, -EINVAL);
	QM_CHECK(f_addr < QM_FLASH_MAX_ADDR, -EINVAL);
	QM_CHECK(page_buffer != NULL, -EINVAL);
	QM_CHECK(data_buffer != NULL, -EINVAL);
	QM_CHECK(len <= QM_FLASH_PAGE_SIZE_DWORDS, -EINVAL);

	uint32_t i, j;
	volatile uint32_t *p_flash = NULL, *p_wr_data, *p_wr_ctrl;

	qm_flash_reg_t *const controller = QM_FLASH[flash];

	/* Rom and flash write registers are laid out the same, but different */
	/* locations in memory, so point to those to have the same function to*/
	/* update page section based on main or rom. */
	switch (region) {

	case QM_FLASH_REGION_SYS:
		p_wr_data = &controller->flash_wr_data;
		p_wr_ctrl = &controller->flash_wr_ctrl;
#if (QUARK_D2000)
		p_flash = (uint32_t *)(QM_FLASH_REGION_SYS_0_BASE +
				       (f_addr & QM_FLASH_PAGE_MASK));
		/* Main flash memory starts after flash data section. */
		f_addr += QM_FLASH_REGION_DATA_0_SIZE;
#elif(QUARK_SE)
		if (flash == QM_FLASH_0) {
			p_flash = (uint32_t *)(QM_FLASH_REGION_SYS_0_BASE +
					       (f_addr & QM_FLASH_PAGE_MASK));
		} else {
			p_flash = (uint32_t *)(QM_FLASH_REGION_SYS_1_BASE +
					       (f_addr & QM_FLASH_PAGE_MASK));
		}
#else
#error("Unsupported / unspecified processor type")
#endif
		break;

#if (QUARK_D2000)
	case QM_FLASH_REGION_DATA:
		p_wr_data = &controller->flash_wr_data;
		p_wr_ctrl = &controller->flash_wr_ctrl;
		p_flash = (uint32_t *)(QM_FLASH_REGION_DATA_0_BASE +
				       (f_addr & QM_FLASH_PAGE_MASK));
		break;
#endif

	case QM_FLASH_REGION_OTP:

		/* Check if OTP locked. */
		if (qm_flash_check_otp_locked(controller->flash_stts)) {
			return -EACCES;
		}

		p_wr_data = &controller->rom_wr_data;
		p_wr_ctrl = &controller->rom_wr_ctrl;
		p_flash = (uint32_t *)(QM_FLASH_REGION_OTP_0_BASE +
				       (f_addr & QM_FLASH_PAGE_MASK));
		break;

	default:
		return -EINVAL;
		break;
	}

	/* Copy Flash Page, with location to be modified, to SRAM */
	for (i = 0; i < QM_FLASH_PAGE_SIZE_DWORDS; i++) {
		page_buffer[i] = *p_flash;
		p_flash++;
	}

	/* Erase the Flash page */
	*p_wr_ctrl = ((f_addr & QM_FLASH_PAGE_MASK) << WR_ADDR_OFFSET) | ER_REQ;

	/* Update sram data with new data */
	j = (f_addr & QM_FLASH_ADDRESS_MASK) >> 2;
	for (i = 0; i < len; i++, j++) {
		page_buffer[j] = data_buffer[i];
	}

	/* Wait for the erase to complete */
	while (!(controller->flash_stts & ER_DONE))
		;

	/* Update address to include the write_address offset. */
	f_addr &= QM_FLASH_PAGE_MASK;
	f_addr <<= WR_ADDR_OFFSET;
	/* Copy the modified page in SRAM into Flash. */
	for (i = 0; i < QM_FLASH_PAGE_SIZE_DWORDS; i++) {
		*p_wr_data = page_buffer[i];
		*p_wr_ctrl = f_addr |= WR_REQ;
		f_addr += QM_FLASH_ADDR_INC;
		/* Wait for write to finish. */
		while (!(controller->flash_stts & WR_DONE))
			;
	}
	return 0;
}

int qm_flash_page_erase(const qm_flash_t flash, const qm_flash_region_t region,
			uint32_t page_num)
{
	QM_CHECK(flash < QM_FLASH_NUM, -EINVAL);
	QM_CHECK(region <= QM_FLASH_REGION_NUM, -EINVAL);
	QM_CHECK(page_num <= QM_FLASH_MAX_PAGE_NUM, -EINVAL);

	qm_flash_reg_t *const controller = QM_FLASH[flash];

	switch (region) {

	case QM_FLASH_REGION_SYS:
#if (QUARK_D2000)
		page_num += QM_FLASH_REGION_DATA_0_PAGES;

	case QM_FLASH_REGION_DATA:
#endif
		controller->flash_wr_ctrl =
		    (page_num << (QM_FLASH_PAGE_SIZE_BITS + WR_ADDR_OFFSET)) |
		    ER_REQ;
		break;

	case QM_FLASH_REGION_OTP:

		/* Check if OTP locked. */
		if (qm_flash_check_otp_locked(controller->flash_stts)) {
			return -EACCES;
		}

		controller->rom_wr_ctrl =
		    (page_num << (QM_FLASH_PAGE_SIZE_BITS + WR_ADDR_OFFSET)) |
		    ER_REQ;
		break;
	default:
		return -EINVAL;
	}

	while (!(controller->flash_stts & ER_DONE))
		;

	return 0;
}

int qm_flash_mass_erase(const qm_flash_t flash, const uint8_t include_rom)
{
	QM_CHECK(flash < QM_FLASH_NUM, -EINVAL);

	qm_flash_reg_t *const controller = QM_FLASH[flash];

	/* Erase all the Flash pages */
	if (include_rom) {

		/* Check if OTP locked. */
		if (qm_flash_check_otp_locked(controller->flash_stts)) {
			return -EACCES;
		}

		controller->ctrl |= MASS_ERASE_INFO;
	}
	controller->ctrl |= MASS_ERASE;
	while (!(controller->flash_stts & ER_DONE))
		;
	return 0;
}

#if (ENABLE_RESTORE_CONTEXT)
int qm_flash_save_context(const qm_flash_t flash, qm_flash_context_t *const ctx)
{
	QM_CHECK(flash < QM_FLASH_NUM, -EINVAL);
	QM_CHECK(ctx != NULL, -EINVAL);

	qm_flash_reg_t *const controller = QM_FLASH[flash];

	ctx->tmg_ctrl = controller->tmg_ctrl;
	ctx->ctrl = controller->ctrl;

	return 0;
}

int qm_flash_restore_context(const qm_flash_t flash,
			     const qm_flash_context_t *const ctx)
{
	QM_CHECK(flash < QM_FLASH_NUM, -EINVAL);
	QM_CHECK(ctx != NULL, -EINVAL);

	qm_flash_reg_t *const controller = QM_FLASH[flash];

	controller->tmg_ctrl = ctx->tmg_ctrl;
	controller->ctrl = ctx->ctrl;

	return 0;
}
#else
int qm_flash_save_context(const qm_flash_t flash, qm_flash_context_t *const ctx)
{
	(void)flash;
	(void)ctx;

	return 0;
}

int qm_flash_restore_context(const qm_flash_t flash,
			     const qm_flash_context_t *const ctx)
{
	(void)flash;
	(void)ctx;

	return 0;
}
#endif /* ENABLE_RESTORE_CONTEXT */
