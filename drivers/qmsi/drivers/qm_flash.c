/*
 * Copyright (c) 2015, Intel Corporation
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

qm_rc_t qm_flash_set_config(const qm_flash_t flash, qm_flash_config_t *cfg)
{
	QM_CHECK(flash < QM_FLASH_NUM, QM_RC_EINVAL);
	QM_CHECK(cfg != NULL, QM_RC_EINVAL);
	QM_CHECK(cfg->wait_states <= QM_FLASH_MAX_WAIT_STATES, QM_RC_EINVAL);
	QM_CHECK(cfg->us_count <= QM_FLASH_MAX_US_COUNT, QM_RC_EINVAL);
	QM_CHECK(cfg->write_disable <= QM_FLASH_WRITE_DISABLE, QM_RC_EINVAL);

	QM_FLASH[flash].tmg_ctrl =
	    (QM_FLASH[flash].tmg_ctrl & QM_FLASH_TMG_DEF_MASK) |
	    (cfg->us_count | (cfg->wait_states << QM_FLASH_WAIT_STATE_OFFSET));

	if (QM_FLASH_WRITE_DISABLE == cfg->write_disable) {
		QM_FLASH[flash].ctrl |= QM_FLASH_WRITE_DISABLE_VAL;
	} else {
		QM_FLASH[flash].ctrl &= ~QM_FLASH_WRITE_DISABLE_VAL;
	}

	return QM_RC_OK;
}

qm_rc_t qm_flash_get_config(const qm_flash_t flash, qm_flash_config_t *cfg)
{
	QM_CHECK(flash < QM_FLASH_NUM, QM_RC_EINVAL);
	QM_CHECK(cfg != NULL, QM_RC_EINVAL);

	cfg->wait_states =
	    (QM_FLASH[flash].tmg_ctrl & QM_FLASH_WAIT_STATE_MASK) >>
	    QM_FLASH_WAIT_STATE_OFFSET;
	cfg->us_count =
	    QM_FLASH[flash].tmg_ctrl & QM_FLASH_MICRO_SEC_COUNT_MASK;
	cfg->write_disable =
	    (QM_FLASH[flash].ctrl & QM_FLASH_WRITE_DISABLE_VAL) >>
	    QM_FLASH_WRITE_DISABLE_OFFSET;

	return QM_RC_OK;
}

qm_rc_t qm_flash_word_write(const qm_flash_t flash, qm_flash_region_t region,
			    uint32_t f_addr, uint32_t data)
{
	QM_CHECK(flash < QM_FLASH_NUM, QM_RC_EINVAL);
	QM_CHECK(region <= QM_FLASH_REGION_NUM, QM_RC_EINVAL);
	QM_CHECK(f_addr < QM_FLASH_MAX_ADDR, QM_RC_EINVAL);

	volatile uint32_t *p_wr_data, *p_wr_ctrl;

	/* Rom and flash write registers are laid out the same, but different */
	/* locations in memory, so point to those to have the same function to*/
	/* update page section based on main or rom. */
	switch (region) {

	case QM_FLASH_REGION_SYS:
		p_wr_data = &QM_FLASH[flash].flash_wr_data;
		p_wr_ctrl = &QM_FLASH[flash].flash_wr_ctrl;
#if (QUARK_D2000)
		/* Main flash memory starts after flash data section. */
		f_addr += QM_FLASH_REGION_DATA_0_SIZE;
#endif
		break;

#if (QUARK_D2000)
	case QM_FLASH_REGION_DATA:
		p_wr_data = &QM_FLASH[flash].flash_wr_data;
		p_wr_ctrl = &QM_FLASH[flash].flash_wr_ctrl;
		break;
#endif

	case QM_FLASH_REGION_OTP:
		p_wr_data = &QM_FLASH[flash].rom_wr_data;
		p_wr_ctrl = &QM_FLASH[flash].rom_wr_ctrl;
		break;

	default:
		return QM_RC_ERROR;
		break;
	}
	/* Update address to include the write_address offset. */
	f_addr <<= WR_ADDR_OFFSET;

	*p_wr_data = data;
	*p_wr_ctrl = f_addr |= WR_REQ;
	/* Wait for write to finish. */
	while (!(QM_FLASH[flash].flash_stts & WR_DONE))
		;
	return QM_RC_OK;
}

qm_rc_t qm_flash_page_write(const qm_flash_t flash, qm_flash_region_t region,
			    uint32_t page_num, uint32_t *data, uint32_t len)
{
	QM_CHECK(flash < QM_FLASH_NUM, QM_RC_EINVAL);
	QM_CHECK(region <= QM_FLASH_REGION_NUM, QM_RC_EINVAL);
	QM_CHECK(page_num <= QM_FLASH_MAX_PAGE_NUM, QM_RC_EINVAL);
	QM_CHECK(data != NULL, QM_RC_EINVAL);
	QM_CHECK(len <= QM_FLASH_PAGE_SIZE, QM_RC_EINVAL);

	uint32_t i;
	volatile uint32_t *p_wr_data, *p_wr_ctrl;

	/* Rom and flash write registers are laid out the same, but different */
	/* locations in memory, so point to those to have the same function to*/
	/* update page section based on main or rom. */
	switch (region) {

	case QM_FLASH_REGION_SYS:
#if (QUARK_D2000)
		page_num += QM_FLASH_REGION_DATA_0_PAGES;

	case QM_FLASH_REGION_DATA:
#endif
		p_wr_data = &QM_FLASH[flash].flash_wr_data;
		p_wr_ctrl = &QM_FLASH[flash].flash_wr_ctrl;
		break;

	case QM_FLASH_REGION_OTP:
		p_wr_data = &QM_FLASH[flash].rom_wr_data;
		p_wr_ctrl = &QM_FLASH[flash].rom_wr_ctrl;
		break;

	default:
		return QM_RC_ERROR;
		break;
	}
	/* Update address to include the write_address offset. */
	page_num <<= (QM_FLASH_PAGE_SIZE_BITS + WR_ADDR_OFFSET);

	/* Erase the Flash page. */
	*p_wr_ctrl = page_num | ER_REQ;

	/* Wait for the erase to complete. */
	while (!(QM_FLASH[flash].flash_stts & ER_DONE))
		;

	/* Write bytes into Flash. */
	for (i = 0; i < len; i++) {
		*p_wr_data = data[i];
		*p_wr_ctrl = page_num;
		*p_wr_ctrl |= WR_REQ;
		page_num += QM_FLASH_ADDR_INC;
		/* Wait for write to finish. */
		while (!(QM_FLASH[flash].flash_stts & WR_DONE))
			;
	}
	return QM_RC_OK;
}

qm_rc_t qm_flash_page_update(const qm_flash_t flash, qm_flash_region_t region,
			     uint32_t f_addr, uint32_t *page_buffer,
			     uint32_t *data_buffer, uint32_t len)
{
	QM_CHECK(flash < QM_FLASH_NUM, QM_RC_EINVAL);
	QM_CHECK(region <= QM_FLASH_REGION_NUM, QM_RC_EINVAL);
	QM_CHECK(f_addr < QM_FLASH_MAX_ADDR, QM_RC_EINVAL);
	QM_CHECK(page_buffer != NULL, QM_RC_EINVAL);
	QM_CHECK(data_buffer != NULL, QM_RC_EINVAL);
	QM_CHECK(len <= QM_FLASH_PAGE_SIZE, QM_RC_EINVAL);

	uint32_t i, j;
	volatile uint32_t *p_flash = NULL, *p_wr_data, *p_wr_ctrl;

	/* Rom and flash write registers are laid out the same, but different */
	/* locations in memory, so point to those to have the same function to*/
	/* update page section based on main or rom. */
	switch (region) {

	case QM_FLASH_REGION_SYS:
		p_wr_data = &QM_FLASH[flash].flash_wr_data;
		p_wr_ctrl = &QM_FLASH[flash].flash_wr_ctrl;
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
		p_wr_data = &QM_FLASH[flash].flash_wr_data;
		p_wr_ctrl = &QM_FLASH[flash].flash_wr_ctrl;
		p_flash = (uint32_t *)(QM_FLASH_REGION_DATA_0_BASE +
				       (f_addr & QM_FLASH_PAGE_MASK));
		break;
#endif

	case QM_FLASH_REGION_OTP:
		p_wr_data = &QM_FLASH[flash].rom_wr_data;
		p_wr_ctrl = &QM_FLASH[flash].rom_wr_ctrl;
		p_flash = (uint32_t *)(QM_FLASH_REGION_OTP_0_BASE +
				       (f_addr & QM_FLASH_PAGE_MASK));
		break;

	default:
		return QM_RC_ERROR;
		break;
	}

	/* Copy Flash Page, with location to be modified, to SRAM */
	for (i = 0; i < QM_FLASH_PAGE_SIZE; i++) {
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
	while (!(QM_FLASH[flash].flash_stts & ER_DONE))
		;

	/* Update address to include the write_address offset. */
	f_addr &= QM_FLASH_PAGE_MASK;
	f_addr <<= WR_ADDR_OFFSET;
	/* Copy the modified page in SRAM into Flash. */
	for (i = 0; i < QM_FLASH_PAGE_SIZE; i++) {
		*p_wr_data = page_buffer[i];
		*p_wr_ctrl = f_addr |= WR_REQ;
		f_addr += QM_FLASH_ADDR_INC;
		/* Wait for write to finish. */
		while (!(QM_FLASH[flash].flash_stts & WR_DONE))
			;
	}
	return QM_RC_OK;
}

qm_rc_t qm_flash_page_erase(const qm_flash_t flash, qm_flash_region_t region,
			    uint32_t page_num)
{
	QM_CHECK(flash < QM_FLASH_NUM, QM_RC_EINVAL);
	QM_CHECK(region <= QM_FLASH_REGION_NUM, QM_RC_EINVAL);
	QM_CHECK(page_num <= QM_FLASH_MAX_PAGE_NUM, QM_RC_EINVAL);

	switch (region) {

	case QM_FLASH_REGION_SYS:
#if (QUARK_D2000)
		page_num += QM_FLASH_REGION_DATA_0_PAGES;

	case QM_FLASH_REGION_DATA:
#endif
		QM_FLASH[flash].flash_wr_ctrl =
		    (page_num << (QM_FLASH_PAGE_SIZE_BITS + WR_ADDR_OFFSET)) |
		    ER_REQ;
		break;

	case QM_FLASH_REGION_OTP:
		QM_FLASH[flash].rom_wr_ctrl =
		    (page_num << (QM_FLASH_PAGE_SIZE_BITS + WR_ADDR_OFFSET)) |
		    ER_REQ;
		break;
	default:
		return QM_RC_EINVAL;
	}

	while (!(QM_FLASH[flash].flash_stts & ER_DONE))
		;

	return QM_RC_OK;
}

qm_rc_t qm_flash_mass_erase(const qm_flash_t flash, uint8_t include_rom)
{
	QM_CHECK(flash < QM_FLASH_NUM, QM_RC_EINVAL);

	/* Erase all the Flash pages */
	if (include_rom) {
		QM_FLASH[flash].ctrl |= MASS_ERASE_INFO;
	}
	QM_FLASH[flash].ctrl |= MASS_ERASE;
	while (!(QM_FLASH[flash].flash_stts & ER_DONE))
		;
	return QM_RC_OK;
}
