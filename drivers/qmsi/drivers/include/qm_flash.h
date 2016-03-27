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

#ifndef __QM_FLASH_H__
#define __QM_FLASH_H__

#include "qm_common.h"
#include "qm_soc_regs.h"

/**
 * Flash Controller for Quark Microcontrollers.
 *
 * @brief Flash Controller for QM.
 * @defgroup groupFlash Flash
 * @{
 */

#define QM_FLASH_TMG_DEF_MASK (0xFFFFFC00)
#define QM_FLASH_MICRO_SEC_COUNT_MASK (0x3F)
#define QM_FLASH_WAIT_STATE_MASK (0x3C0)
#define QM_FLASH_WAIT_STATE_OFFSET (6)
#define QM_FLASH_WRITE_DISABLE_OFFSET (4)
#define QM_FLASH_WRITE_DISABLE_VAL BIT(4)

#define QM_FLASH_PAGE_SIZE (0x200)
#define QM_FLASH_PAGE_SIZE_BITS (11)

#define ROM_PROG BIT(2)
#define ER_REQ BIT(1)
#define ER_DONE (1)
#define WR_REQ (1)
#define WR_DONE BIT(1)

#define WR_ADDR_OFFSET (2)
#define MASS_ERASE_INFO BIT(6)
#define MASS_ERASE BIT(7)

#define QM_FLASH_ADDRESS_MASK (0x7FF)
/* Increment by 4 bytes each time, but there is an offset of 2, so 0x10. */
#define QM_FLASH_ADDR_INC (0x10)

/**
 * Flash region enum
 */
typedef enum {
	QM_FLASH_REGION_OTP = 0,
	QM_FLASH_REGION_SYS,
#if (QUARK_D2000)
	QM_FLASH_REGION_DATA,
#endif
	QM_FLASH_REGION_NUM
} qm_flash_region_t;

/**
 * Flash write disable / enable enum
 */
typedef enum {
	QM_FLASH_WRITE_ENABLE,
	QM_FLASH_WRITE_DISABLE
} qm_flash_disable_t;

/**
 * Flash configuration structure
 */
typedef struct {
	uint8_t wait_states; /**< Read wait state */
	uint8_t us_count;    /**< Number of clocks in a microsecond */
	qm_flash_disable_t write_disable; /**< Write disable */
} qm_flash_config_t;

/**
 * Configure a Flash controller. This includes timing and behavioral settings.
 * When switching SoC to a higher frequency, this register must be updated first
 * to reflect settings associated with higher frequency BEFORE SoC frequency is
 * changed. On the other hand, when switching SoC to a lower frequency, this
 * register must be updated only 6 NOP instructions AFTER the SoC frequency has
 * been updated. Otherwise, flash timings will be violated.
 *
 * @brief Configure a Flash controller.
 * @param [in] flash Flash controller index.
 * @param [in] cfg Flash configuration.
 * @return qm_rc_t QM_RC_OK on success, error code otherwise.
 */
qm_rc_t qm_flash_set_config(const qm_flash_t flash, qm_flash_config_t *cfg);

/**
 * Retrieve Flash controller configuration. This will set the
 * cfg parameter to match the current configuration of the
 * given Flash controller.
 *
 * @brief Get Flash controller configuration.
 * @param [in] flash Flash controller index.
 * @param [out] cfg Flash configuration.
 * @return qm_rc_t QM_RC_OK on success, error code otherwise.
 */
qm_rc_t qm_flash_get_config(const qm_flash_t flash, qm_flash_config_t *cfg);

/**
 * Write 4 bytes of data to Flash. Check for brownout before initiating the
 * write. Note this function performs a write operation only; page erase
 * may be needed if the page is already programmed.
 *
 * @brief Write 4 bytes of data to Flash.
 * @param [in] flash Flash controller index.
 * @param [in] region Flash region to address.
 * @param [in] f_addr Address within Flash physical address space.
 * @param [in] data Data word to write.
 * @return qm_rc_t QM_RC_OK on success, error code otherwise.
 */
qm_rc_t qm_flash_word_write(const qm_flash_t flash, qm_flash_region_t region,
			    uint32_t f_addr, const uint32_t data);

/**
 * Write a multiple of 4 bytes of data to Flash.
 * Check for brownout before initiating the write.
 * The page is erased, and then written to.
 *
 * @brief Write multiple of 4 bytes of data to Flash.
 * @param [in] flash Flash controller index.
 * @param [in] region Which Flash region to address.
 * @param [in] f_addr Address within Flash physical address space.
 * @param [in] page_buffer Page buffer to store page, must be at least
 * 		QM_FLASH_PAGE_SIZE words big.
 * @param [in] data_buffer Data buffer to write.
 * @param [in] len Length of data to write.
 * @return qm_rc_t QM_RC_OK on success, error code otherwise.
 */
qm_rc_t qm_flash_page_update(const qm_flash_t flash, qm_flash_region_t region,
			     uint32_t f_addr, uint32_t *page_buffer,
			     uint32_t *data_buffer, uint32_t len);

/**
 * Write a 2KB page of Flash. Check for brownout before initiating the write.
 * The page is erased, and then written to.
 *
 * @brief Write s 2KB flash page.
 * @param [in] flash Flash controller index.
 * @param [in] region Which Flash region to address.
 * @param [in] page_num Which page of flash to overwrite.
 * @param [in] data Data buffer to write.
 * @param [in] len Length of data to write.
 * @return qm_rc_t QM_RC_OK on success, error code otherwise.
 */
qm_rc_t qm_flash_page_write(const qm_flash_t flash, qm_flash_region_t region,
			    uint32_t page_num, uint32_t *data, uint32_t len);

/**
 * Erase one page of Flash.
 * Check for brownout before initiating the write.
 *
 * @brief Erase one page of Flash.
 * @param [in] flash Flash controller index.
 * @param [in] region Flash region to address.
 * @param [in] page_num Page within the Flash controller to erase.
 * @return qm_rc_t QM_RC_OK on success, error code otherwise.
 */
/* Having page be 32-bits, saves 6 bytes over using 8 / 16-bits. */
qm_rc_t qm_flash_page_erase(const qm_flash_t flash, qm_flash_region_t region,
			    uint32_t page_num);

/**
 * Perform Flash mass erase.
 * Check for brownout before initiating the erase.
 * Performs mass erase on the Flash controller. The mass erase
 * may include the ROM region, if present and unlocked.
 * Note it is not possible to mass-erase the ROM portion separately.
 *
 * @brief Perform mass erase.
 * @param [in] flash Flash controller index.
 * @param [in] include_rom If set, it also erases the ROM region.
 * @return qm_rc_t QM_RC_OK on success, error code otherwise.
 */
qm_rc_t qm_flash_mass_erase(const qm_flash_t flash, uint8_t include_rom);

/**
 * @}
 */

#endif /* __QM_FLASH_H__ */
