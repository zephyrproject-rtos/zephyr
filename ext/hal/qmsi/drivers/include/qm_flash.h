/*
 * Copyright (c) 2016, Intel Corporation
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
 * Flash controller.
 *
 * @defgroup groupFlash Flash
 * @{
 */

/**
 * Flash region enum.
 */
typedef enum {
	QM_FLASH_REGION_OTP = 0, /**< Flash OTP region. */
	QM_FLASH_REGION_SYS,     /**< Flash System region. */
#if (QUARK_D2000)
	QM_FLASH_REGION_DATA, /**< Flash Data region (Quark D2000 only). */
#endif
	QM_FLASH_REGION_NUM /**< Total number of flash regions. */
} qm_flash_region_t;

/**
 * Flash write disable / enable enum.
 */
typedef enum {
	QM_FLASH_WRITE_ENABLE = 0, /**< Flash write enable. */
	QM_FLASH_WRITE_DISABLE     /**< Flash write disable. */
} qm_flash_disable_t;

/**
 * Flash configuration structure.
 */
typedef struct {
	uint8_t wait_states; /**< Read wait state. */

	/**
	 * Number of clocks in a microsecond. Needed for program/erase
	 * operations.
	 */
	uint8_t us_count;
	qm_flash_disable_t write_disable; /**< Write disable. */
} qm_flash_config_t;

/**
 * Configure a Flash controller.
 *
 * The configuration includes timing and behavioral settings.
 *
 * @note: when switching SoC to a higher frequency, flash controllers must be
 * reconfigured to reflect settings associated with higher frequency BEFORE SoC
 * frequency is changed. On the other hand, when switching SoC to a lower
 * frequency, flash controller must be reconfigured only 6 NOP instructions
 * AFTER the SoC frequency has been updated. Otherwise, flash timings will be
 * violated.
 *
 * @param[in] flash Flash controller index.
 * @param[in] cfg   Flash configuration. It must not be NULL.
 *
 * @return Standard errno return type for QMSI.
 * @retval 0 on success.
 * @retval Negative @ref errno for possible error codes.
 */
int qm_flash_set_config(const qm_flash_t flash,
			const qm_flash_config_t *const cfg);

/**
 * Write 4 bytes of data to Flash.
 *
 * @note: this function performs a write operation only; page erase may be
 * needed if the page is already programmed.
 *
 * @param[in] flash  Flash controller index.
 * @param[in] region Flash region to address.
 * @param[in] f_addr Address within Flash physical address space.
 * @param[in] data   Data word to write.
 *
 * @return Standard errno return type for QMSI.
 * @retval 0 on success.
 * @retval Negative @ref errno for possible error codes.
 */
int qm_flash_word_write(const qm_flash_t flash, const qm_flash_region_t region,
			uint32_t f_addr, const uint32_t data);

/**
 * Write multiple of 4 bytes of data to Flash.
 *
 * The page is erased, and then written to.
 *
 * @note: Since this operation may take some time to complete, the caller is
 * responsible for ensuring that the watchdog timer does not elapse in the
 * meantime (e.g., by restarting it before calling this function).
 *
 * @param[in] flash    Flash controller index.
 * @param[in] reg      Which Flash region to address.
 * @param[in] f_addr   Address within Flash physical address space.
 * @param[in] page_buf Page buffer to store page during update. Must be at
 *		       least QM_FLASH_PAGE_SIZE words big and must not be NULL.
 * @param[in] data     Data to write (array of words). This must not be NULL.
 * @param[in] len      Length of data to write (number of words).
 *
 * @return Standard errno return type for QMSI.
 * @retval 0 on success.
 * @retval Negative @ref errno for possible error codes.
 */
int qm_flash_page_update(const qm_flash_t flash, const qm_flash_region_t reg,
			 uint32_t f_addr, uint32_t *const page_buf,
			 const uint32_t *const data, uint32_t len);

/**
 * Write a flash page.
 *
 * The page is erased, and then written to.
 *
 * @note: Since this operation may take some time to complete, the caller is
 * responsible for ensuring that the watchdog timer does not elapse in the
 * meantime (e.g., by restarting it before calling this function).
 *
 * @param[in] flash    Flash controller index.
 * @param[in] region   Which Flash region to address.
 * @param[in] page_num Which page of flash to overwrite.
 * @param[in] data     Data to write (array of words). This must not be NULL.
 * @param[in] len      Length of data to write (number of words).
 *
 * @return Standard errno return type for QMSI.
 * @retval 0 on success.
 * @retval Negative @ref errno for possible error codes.
 */
int qm_flash_page_write(const qm_flash_t flash, const qm_flash_region_t region,
			uint32_t page_num, const uint32_t *data, uint32_t len);

/**
 * Erase one page of Flash.
 *
 * @param[in] flash    Flash controller index.
 * @param[in] region   Flash region to address.
 * @param[in] page_num Page within the Flash controller to erase.
 *
 * @return Standard errno return type for QMSI.
 * @retval 0 on success.
 * @retval Negative @ref errno for possible error codes.
 */
/* Having page be 32-bits, saves 6 bytes over using 8 / 16-bits. */
int qm_flash_page_erase(const qm_flash_t flash, const qm_flash_region_t region,
			uint32_t page_num);

/**
 * Perform mass erase.
 *
 * Perform mass erase on the specified flash controller. The mass erase may
 * include the ROM region, if present and unlocked. Note: it is not possible
 * to mass-erase the ROM portion separately.
 *
 * @note: Since this operation may take some time to complete, the caller is
 * responsible for ensuring that the watchdog timer does not elapse in the
 * meantime (e.g., by restarting it before calling this function).
 *
 * @param[in] flash       Flash controller index.
 * @param[in] include_rom If set, it also erases the ROM region.
 *
 * @return Standard errno return type for QMSI.
 * @retval 0 on success.
 * @retval Negative @ref errno for possible error codes.
 */
int qm_flash_mass_erase(const qm_flash_t flash, const uint8_t include_rom);

#if (ENABLE_RESTORE_CONTEXT)
/**
 * Save flash context.
 *
 * Save the configuration of the specified flash controller.
 *
 * @param[in] flash Flash controller index.
 * @param[out] ctx Flash context structure. This must not be NULL.
 *
 * @return Standard errno return type for QMSI.
 * @retval 0 on success.
 * @retval Negative @ref errno for possible error codes.
 */
int qm_flash_save_context(const qm_flash_t flash,
			  qm_flash_context_t *const ctx);

/**
 * Restore flash context.
 *
 * Restore the configuration of the specified flash controller.
 * If the system clock frequency is lowered, the flash timings need to be
 * restored. Otherwise, reading from flash will not be optimal
 * (there will be 2 wait states instead of 0 wait states.)
 *
 * @param[in] flash Flash controller index.
 * @param[in] ctx Flash context structure. This must not be NULL.
 *
 * @return Standard errno return type for QMSI.
 * @retval 0 on success.
 * @retval Negative @ref errno for possible error codes.
 */
int qm_flash_restore_context(const qm_flash_t flash,
			     const qm_flash_context_t *const ctx);
#endif /* ENABLE_RESTORE_CONTEXT */

/**
 * @}
 */

#endif /* __QM_FLASH_H__ */
