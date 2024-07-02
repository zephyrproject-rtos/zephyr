/*
 * Copyright (c) 2024 Renesas Electronics Corporation
 * SPDX-License-Identifier: Apache-2.0
 */

#include <sys/types.h>
#include <zephyr/device.h>
#include <zephyr/drivers/flash/ra_flash_api_extensions.h>
#include <zephyr/kernel.h>

#ifdef CONFIG_USERSPACE
#include <zephyr/syscall.h>
#include <zephyr/syscall_handler.h>
#endif

#include <soc.h>
#include "flash_hp_ra.h"

#define FLASH_HP_FMEPROT_LOCK   (0xD901)
#define FLASH_HP_FMEPROT_UNLOCK (0xD900)

/* The maximum timeout for commands is 100usec when FCLK is 16 MHz i.e. 1600 FCLK cycles. */
#define FLASH_HP_FRDY_CMD_TIMEOUT            (19200)
#define FLASH_HP_FENTRYR_READ_MODE           (0xAA00U)

/* Bits indicating that CodeFlash is in P/E mode. */
#define FLASH_HP_FENTRYR_CF_PE_MODE          (0x0001U)
/* Key Code to transition to CF P/E mode. */
#define FLASH_HP_FENTRYR_TRANSITION_TO_CF_PE (0xAA01U)

#define FLASH_HP_FACI_CMD_CONFIG_SET_1 (0x40U)
#define FLASH_HP_FACI_CMD_CONFIG_SET_2 (0x08U)
#define FLASH_HP_FACI_CMD_FORCED_STOP  (0xB3U)
#define FLASH_HP_FACI_CMD_FINAL        (0xD0U)

#define FLASH_HP_CONFIG_SET_ACCESS_WORD_CNT (8U)

#if (FLASH_HP_CFG_CODE_FLASH_PROGRAMMING_ENABLE == 1)
static uint16_t g_configuration_area_data[FLASH_HP_CONFIG_SET_ACCESS_WORD_CNT] = {UINT16_MAX};
#endif

static fsp_err_t flash_hp_enter_pe_cf_mode(flash_hp_instance_ctrl_t *const p_ctrl)
				PLACE_IN_RAM_SECTION;

static fsp_err_t flash_hp_stop(void) PLACE_IN_RAM_SECTION;

static fsp_err_t flash_hp_configuration_area_write(flash_hp_instance_ctrl_t *p_ctrl,
						   uint32_t fsaddr) PLACE_IN_RAM_SECTION;

static fsp_err_t flash_hp_check_errors(fsp_err_t previous_error, uint32_t error_bits,
				       fsp_err_t return_error) PLACE_IN_RAM_SECTION;

static fsp_err_t flash_hp_pe_mode_exit(void) PLACE_IN_RAM_SECTION;

static fsp_err_t flash_hp_set_block_protect_ns(flash_hp_instance_ctrl_t *p_ctrl,
					       uint8_t *bps_val_ns, uint8_t *pbps_val_ns,
					       uint32_t size) PLACE_IN_RAM_SECTION;

static fsp_err_t flash_hp_set_block_protect_sec(flash_hp_instance_ctrl_t *p_ctrl,
						uint8_t *bps_val_sec, uint8_t *pbps_val_sec,
						uint32_t size) PLACE_IN_RAM_SECTION;

static fsp_err_t flash_hp_set_block_protect_sel(flash_hp_instance_ctrl_t *p_ctrl,
						uint8_t *bps_sel_val,
						uint32_t size) PLACE_IN_RAM_SECTION;

/********************************************************************************************/ /**
 * This function switches the peripheral to P/E mode for Code Flash.
 * @param[in]  p_ctrl              Pointer to the Flash control block.
 * @retval     FSP_SUCCESS         Successfully entered Code Flash P/E mode.
 * @retval     FSP_ERR_PE_FAILURE  Failed to enter Code Flash P/E mode.
 ********************************************************************************************/
static fsp_err_t flash_hp_enter_pe_cf_mode(flash_hp_instance_ctrl_t *const p_ctrl)
{
	fsp_err_t err = FSP_SUCCESS;

	/* See "Transition to Code Flash P/E Mode": Section 47.9.3.3 of the RA6M4 manual */
	/* Timeout counter. */
	volatile uint32_t wait_count = FLASH_HP_FRDY_CMD_TIMEOUT;

	/* While the Flash API is in use we will disable the flash cache. */
#if BSP_FEATURE_BSP_FLASH_CACHE_DISABLE_OPM
	R_BSP_FlashCacheDisable();
#elif BSP_FEATURE_BSP_HAS_CODE_SYSTEM_CACHE

	/* Disable the C-Cache. */
	R_CACHE->CCACTL = 0U;
#endif

	/* If interrupts are being used then disable interrupts. */
	if (p_ctrl->p_cfg->data_flash_bgo == true) {
		/* We are not supporting Flash Rdy interrupts for Code Flash operations */
		R_BSP_IrqDisable(p_ctrl->p_cfg->irq);

		/* We are not supporting Flash Err interrupts for Code Flash operations */
		R_BSP_IrqDisable(p_ctrl->p_cfg->err_irq);
	}

#if BSP_FEATURE_FLASH_HP_HAS_FMEPROT
	R_FACI_HP->FMEPROT = FLASH_HP_FMEPROT_UNLOCK;
#endif

	/* Enter code flash PE mode */
	R_FACI_HP->FENTRYR = FLASH_HP_FENTRYR_TRANSITION_TO_CF_PE;

	/* Wait for the operation to complete or timeout. If timeout return error. */
	/* Read FENTRYR until it has been set to 0x0001 so that we have entered CF P/E mode */
	while (R_FACI_HP->FENTRYR != FLASH_HP_FENTRYR_CF_PE_MODE) {
		/* Wait until FENTRYR is 0x0001UL unless timeout occurs. */
		if (wait_count == 0U) {

			/* if FENTRYR is not set after max timeout, FSP_ERR_PE_FAILURE*/
			return FSP_ERR_PE_FAILURE;
		}

		wait_count--;
	}

	return err;
}

/********************************************************************************************/ /**
 * Execute the Set Configuration sequence using the g_configuration_area_data structure
 * set up the caller.
 * @param      p_ctrl           Pointer to the control block
 * @param[in]  fsaddr           Flash address to be written to.
 *
 * @retval     FSP_SUCCESS      Set Configuration successful
 * @retval     FSP_ERR_TIMEOUT  Timed out waiting for the FCU to become ready.
 ********************************************************************************************/
static fsp_err_t flash_hp_configuration_area_write(flash_hp_instance_ctrl_t *p_ctrl,
						   uint32_t fsaddr)
{
	volatile uint32_t timeout = p_ctrl->timeout_write_config;

	/* See "Configuration Set Command": Section 47.9.3.15 of the RA6M4 manual R01UH0890EJ0100.
	 */
	R_FACI_HP->FSADDR = fsaddr;
	R_FACI_HP_CMD->FACI_CMD8 = FLASH_HP_FACI_CMD_CONFIG_SET_1;
	R_FACI_HP_CMD->FACI_CMD8 = FLASH_HP_FACI_CMD_CONFIG_SET_2;

	/* Write the configuration data. */
	for (uint32_t index = 0U; index < FLASH_HP_CONFIG_SET_ACCESS_WORD_CNT; index++) {
		/* There are 8 16 bit words that must be written to accomplish a configuration set
		 */
		R_FACI_HP_CMD->FACI_CMD16 = g_configuration_area_data[index];
	}

	R_FACI_HP_CMD->FACI_CMD8 = FLASH_HP_FACI_CMD_FINAL;

	while (R_FACI_HP->FSTATR_b.FRDY != 1U) {
		if (timeout <= 0U) {
			return FSP_ERR_TIMEOUT;
		}

		timeout--;
	}

	return FSP_SUCCESS;
}

/********************************************************************************************/ /**
 * This function is used to force stop the flash during an ongoing operation.
 *
 * @retval     FSP_SUCCESS         Successful stop.
 * @retval     FSP_ERR_TIMEOUT     Timeout executing flash_stop.
 * @retval     FSP_ERR_CMD_LOCKED  Peripheral in command locked state.
 ********************************************************************************************/
static fsp_err_t flash_hp_stop(void)
{
	/* See "Forced Stop Command": Section 47.9.3.13 of the RA6M4 manual R01UH0890EJ0100. */
	/* If the CMDLK bit is still set after issuing the force stop command return an error. */
	volatile uint32_t wait_count = FLASH_HP_FRDY_CMD_TIMEOUT;

	R_FACI_HP_CMD->FACI_CMD8 = (uint8_t)FLASH_HP_FACI_CMD_FORCED_STOP;

	while (R_FACI_HP->FSTATR_b.FRDY != 1U) {
		if (wait_count == 0U) {
			return FSP_ERR_TIMEOUT;
		}

		wait_count--;
	}

	if (R_FACI_HP->FASTAT_b.CMDLK != 0U) {
		return FSP_ERR_CMD_LOCKED;
	}

	return FSP_SUCCESS;
}

/********************************************************************************************/ /**
 * Check for errors after a flash operation
 *
 * @param[in]  previous_error      The previous error
 * @param[in]  error_bits          The error bits
 * @param[in]  return_error        The return error
 *
 * @retval     FSP_SUCCESS         Command completed successfully
 * @retval     FSP_ERR_CMD_LOCKED  Flash entered command locked state
 *********************************************************************************************/
static fsp_err_t flash_hp_check_errors(fsp_err_t previous_error, uint32_t error_bits,
				       fsp_err_t return_error)
{
	/* See "Recovery from the Command-Locked State" on Hardware manual. */
	fsp_err_t err = FSP_SUCCESS;

	if (R_FACI_HP->FASTAT_b.CMDLK == 1U) {
		/* If an illegal error occurred read and clear CFAE and DFAE in FASTAT. */
		if (R_FACI_HP->FSTATR_b.ILGLERR == 1U) {
			R_FACI_HP->FASTAT;
			R_FACI_HP->FASTAT = 0;
		}

		err = FSP_ERR_CMD_LOCKED;
	}

	/* Check if status is indicating a Programming error */
	/* If a programming error occurred return error. */
	if (R_FACI_HP->FSTATR & error_bits) {
		err = return_error;
	}

	/* Return the first error code that was encountered. */
	if (previous_error != FSP_SUCCESS) {
		err = previous_error;
	}

	if (err != FSP_SUCCESS) {
		/* Stop the flash and return the previous error. */
		(void)flash_hp_stop();
	}

	return err;
}

/********************************************************************************************/ /**
 * This function switches the peripheral from P/E mode for Code Flash or Data Flash to Read mode.
 *
 * @retval     FSP_SUCCESS         Successfully entered P/E mode.
 * @retval     FSP_ERR_PE_FAILURE  Failed to exited P/E mode
 * @retval     FSP_ERR_CMD_LOCKED  Flash entered command locked state.
 ********************************************************************************************/
static fsp_err_t flash_hp_pe_mode_exit(void)
{
	/* See "Transition to Read Mode": Section 47.9.3.5 of the RA6M4 manual R01UH0890EJ0100. */
	/* FRDY and CMDLK are checked after the previous commands complete. */
	fsp_err_t err = FSP_SUCCESS;
	fsp_err_t temp_err = FSP_SUCCESS;
	uint32_t pe_mode = R_FACI_HP->FENTRYR;

	uint32_t wait_count = FLASH_HP_FRDY_CMD_TIMEOUT;

	/* Transition to Read mode */
	R_FACI_HP->FENTRYR = FLASH_HP_FENTRYR_READ_MODE;

	/* Wait until the flash is in read mode or timeout. If timeout return error. */
	/* Read FENTRYR until it has been set to 0x0000 so we have successfully exited P/E mode.*/
	while (R_FACI_HP->FENTRYR != 0U) {
		/* Wait until FENTRYR is 0x0000 unless timeout occurs. */
		if (wait_count == 0U) {
			/* If FENTRYR is not set after max timeout, FSP_ERR_PE_FAILURE*/
			err = FSP_ERR_PE_FAILURE;
		}

		wait_count--;
	}

	/* If the device is coming out of code flash p/e mode restore the flash cache state. */
	if (pe_mode == FLASH_HP_FENTRYR_CF_PE_MODE) {
#if BSP_FEATURE_FLASH_HP_HAS_FMEPROT
		R_FACI_HP->FMEPROT = FLASH_HP_FMEPROT_LOCK;
#endif

		R_BSP_FlashCacheEnable();
	}

#if BSP_FEATURE_BSP_HAS_CODE_SYSTEM_CACHE
	else if (pe_mode == FLASH_HP_FENTRYR_DF_PE_MODE) {
		/* Flush the C-CACHE. */
		R_CACHE->CCAFCT = 1U;
		FSP_HARDWARE_REGISTER_WAIT(R_CACHE->CCAFCT, 0U);
	} else {
		/* Do nothing. */
	}
#endif

	/* If a command locked state was detected earlier, then return that error. */
	if (temp_err == FSP_ERR_CMD_LOCKED) {
		err = temp_err;
	}

	return err;
}

/********************************************************************************************/ /**
 * Set BPS register with new value.
 *
 * @param      p_ctrl                Pointer to the control block
 * @param[in]  bps_val_ns            new bps value for non-secure
 * @param[in]  pbps_val_ns           new pbps value for non-secure
 *
 * @retval     FSP_SUCCESS           Set Configuration successful
 ********************************************************************************************/
static fsp_err_t flash_hp_set_block_protect_ns(flash_hp_instance_ctrl_t *p_ctrl,
					       uint8_t *bps_val_ns, uint8_t *pbps_val_ns,
					       uint32_t size)
{
	/* Disable interrupts to prevent vector table access while code flash is in P/E mode. */
	int key = irq_lock();

	/* Update Flash state and enter Code Flash P/E mode */
	fsp_err_t err = flash_hp_enter_pe_cf_mode(p_ctrl);

	FSP_ERROR_RETURN(err == FSP_SUCCESS, err);

	memset(g_configuration_area_data, UINT8_MAX, sizeof(g_configuration_area_data));
	if (bps_val_ns != NULL) {
		memcpy(&g_configuration_area_data[FLASH_HP_FCU_CONFIG_SET_BPS_OFFSET], bps_val_ns,
		       size);
		err = flash_hp_configuration_area_write(p_ctrl, FLASH_HP_FCU_CONFIG_SET_BPS);
		err = flash_hp_check_errors(err, 0, FSP_ERR_WRITE_FAILED);
	}

	memset(g_configuration_area_data, UINT8_MAX, sizeof(g_configuration_area_data));
	if (pbps_val_ns != NULL) {
		memcpy(&g_configuration_area_data[FLASH_HP_FCU_CONFIG_SET_BPS_OFFSET], pbps_val_ns,
		       size);
		err = flash_hp_configuration_area_write(p_ctrl, FLASH_HP_FCU_CONFIG_SET_PBPS);
		err = flash_hp_check_errors(err, 0, FSP_ERR_WRITE_FAILED);
	}

	/* Return to read mode*/
	fsp_err_t pe_exit_err = flash_hp_pe_mode_exit();

	if (err == FSP_SUCCESS) {
		err = pe_exit_err;
	}

	/* Enable interrupts after code flash operations are complete. */
	irq_unlock(key);

	return err;
}

/********************************************************************************************/ /**
 * Set BPS_SEC register with new value.
 *
 * @param      p_ctrl                Pointer to the control block
 * @param[in]  bps_val_sec           new bps value for secure
 * @param[in]  pbps_val_sec          new pbps value for secure
 *
 * @retval     FSP_SUCCESS           Set Configuration successful
 ********************************************************************************************/
static fsp_err_t flash_hp_set_block_protect_sec(flash_hp_instance_ctrl_t *p_ctrl,
						uint8_t *bps_val_sec, uint8_t *pbps_val_sec,
						uint32_t size)
{
	/* Disable interrupts to prevent vector table access while code flash is in P/E mode. */
	int key = irq_lock();
	/* Update Flash state and enter Code Flash P/E mode */
	fsp_err_t err = flash_hp_enter_pe_cf_mode(p_ctrl);

	FSP_ERROR_RETURN(err == FSP_SUCCESS, err);

	memset(g_configuration_area_data, UINT8_MAX, sizeof(g_configuration_area_data));
	if (bps_val_sec != NULL) {
		memcpy(&g_configuration_area_data[FLASH_HP_FCU_CONFIG_SET_BPS_OFFSET], bps_val_sec,
		       size);
		err = flash_hp_configuration_area_write(p_ctrl, FLASH_HP_FCU_CONFIG_SET_BPS_SEC);
		err = flash_hp_check_errors(err, 0, FSP_ERR_WRITE_FAILED);
	}

	memset(g_configuration_area_data, UINT8_MAX, sizeof(g_configuration_area_data));
	if (pbps_val_sec != NULL) {
		memcpy(&g_configuration_area_data[FLASH_HP_FCU_CONFIG_SET_BPS_OFFSET], pbps_val_sec,
		       size);
		err = flash_hp_configuration_area_write(p_ctrl, FLASH_HP_FCU_CONFIG_SET_PBPS_SEC);
		err = flash_hp_check_errors(err, 0, FSP_ERR_WRITE_FAILED);
	}

	/* Return to read mode*/
	fsp_err_t pe_exit_err = flash_hp_pe_mode_exit();

	if (err == FSP_SUCCESS) {
		err = pe_exit_err;
	}

	/* Enable interrupts after code flash operations are complete. */
	irq_unlock(key);

	return err;
}

/********************************************************************************************/ /**
 * Set BPS SEL register with new value.
 *
 * @param      p_ctrl                Pointer to the control block
 * @param[in]  bps_sel_val           new bps sel value (for secure)
 *
 * @retval     FSP_SUCCESS           Set Configuration successful
 ********************************************************************************************/
static fsp_err_t flash_hp_set_block_protect_sel(flash_hp_instance_ctrl_t *p_ctrl,
						uint8_t *bps_sel_val, uint32_t size)
{
	/* Disable interrupts to prevent vector table access while code flash is in P/E mode. */
	int key = irq_lock();

	/* Update Flash state and enter Code Flash P/E mode */
	fsp_err_t err = flash_hp_enter_pe_cf_mode(p_ctrl);

	FSP_ERROR_RETURN(err == FSP_SUCCESS, err);

	memset(g_configuration_area_data, UINT8_MAX, sizeof(g_configuration_area_data));
	memcpy(&g_configuration_area_data[FLASH_HP_FCU_CONFIG_SET_BPS_OFFSET], bps_sel_val, size);
	err = flash_hp_configuration_area_write(p_ctrl, FLASH_HP_FCU_CONFIG_SET_BPS_SEL);
	err = flash_hp_check_errors(err, 0, FSP_ERR_WRITE_FAILED);

	/* Return to read mode*/
	fsp_err_t pe_exit_err = flash_hp_pe_mode_exit();

	if (err == FSP_SUCCESS) {
		err = pe_exit_err;
	}

	/* Enable interrupts after code flash operations are complete. */
	irq_unlock(key);

	return err;
}

/********************************************************************************************/ /**
 * Set the block protection (BPS, PBPS) register. Implements @ref flash_api_t::blockProtectSet.
 *
 * Example:
 * @snippet r_flash_hp_example.c R_FLASH_HP_BlockProtectSet
 *
 * @retval     FSP_SUCCESS        FLASH peripheral is ready to use.
 * @retval     FSP_ERR_UNSUPPORTED   Code Flash Programming is not enabled.
 ********************************************************************************************/
fsp_err_t R_FLASH_HP_BlockProtectSet(flash_ctrl_t *const p_api_ctrl, uint8_t *bps_val_ns,
				     uint8_t *bps_val_sec, uint8_t *bps_val_sel,
				     uint8_t *pbps_val_ns, uint8_t *pbps_val_sec, uint32_t size)
{
	flash_hp_instance_ctrl_t *p_ctrl = (flash_hp_instance_ctrl_t *)p_api_ctrl;
	fsp_err_t err = FSP_SUCCESS;

#if (FLASH_HP_CFG_CODE_FLASH_PROGRAMMING_ENABLE == 1)

	/* if non-secure BPS (PBPS) buffers are not null and size is smaller than 16 bytes */
	if (((bps_val_ns != NULL) || (pbps_val_ns != NULL)) &&
	    (size <= (sizeof(uint16_t) * FLASH_HP_CONFIG_SET_ACCESS_WORD_CNT))) {
		err = flash_hp_set_block_protect_ns(p_ctrl, bps_val_ns, pbps_val_ns, size);
	}

	/* if secure  BPS (PBPS) buffers are not null and size is smaller than 16 bytes */
	if (((bps_val_sec != NULL) || (pbps_val_sec != NULL)) &&
	    (size <= (sizeof(uint16_t) * FLASH_HP_CONFIG_SET_ACCESS_WORD_CNT))) {
		err = flash_hp_set_block_protect_sec(p_ctrl, bps_val_sec, pbps_val_sec, size);
	}

	/* if BPS SEL buffer is not null and size is smaller than 16 bytes */
	if ((bps_val_sel != NULL) &&
	    (size <= (sizeof(uint16_t) * FLASH_HP_CONFIG_SET_ACCESS_WORD_CNT))) {
		err = flash_hp_set_block_protect_sel(p_ctrl, bps_val_sel, size);
	}
#else

	err = FSP_ERR_UNSUPPORTED; /* For consistency with _LP API we return error if Code Flash */

#endif

	return err;
}

/********************************************************************************************/ /**
 * Get the block protection (BPS, PBPS) register. Implements @ref flash_api_t::blockProtectGet.
 *
 * Example:
 * @snippet r_flash_hp_example.c R_FLASH_HP_BlockProtectGet
 *
 * @retval     FSP_SUCCESS        FLASH peripheral is ready to use.
 * @retval     FSP_ERR_UNSUPPORTED   Code Flash Programming is not enabled.
 ********************************************************************************************/
fsp_err_t R_FLASH_HP_BlockProtectGet(flash_ctrl_t *const p_api_ctrl, uint32_t *bps_val_ns,
				     uint32_t *bps_val_sec, uint8_t *bps_val_sel,
				     uint8_t *pbps_val_ns, uint8_t *pbps_val_sec, uint32_t *size)
{
	fsp_err_t err = FSP_ERR_UNSUPPORTED;

#if (FLASH_HP_CFG_CODE_FLASH_PROGRAMMING_ENABLE == 1)

	err = FSP_SUCCESS;

	if (bps_val_ns != NULL) {
		bps_val_ns[0] = *((uint32_t *)(FLASH_HP_FCU_CONFIG_SET_BPS + 0));
		bps_val_ns[1] = *((uint32_t *)(FLASH_HP_FCU_CONFIG_SET_BPS + 3));
		bps_val_ns[2] = *((uint32_t *)(FLASH_HP_FCU_CONFIG_SET_BPS + 7));
		bps_val_ns[3] = *((uint32_t *)(FLASH_HP_FCU_CONFIG_SET_BPS + 11));
	}

	if (bps_val_sec != NULL) {
		bps_val_sec[0] = *((uint32_t *)(FLASH_HP_FCU_CONFIG_SET_BPS_SEC + 0));
		bps_val_sec[1] = *((uint32_t *)(FLASH_HP_FCU_CONFIG_SET_BPS_SEC + 3));
		bps_val_sec[2] = *((uint32_t *)(FLASH_HP_FCU_CONFIG_SET_BPS_SEC + 7));
		bps_val_sec[3] = *((uint32_t *)(FLASH_HP_FCU_CONFIG_SET_BPS_SEC + 11));
	}

	if (bps_val_sel != NULL) {
		bps_val_sel[0] = *((uint32_t *)(FLASH_HP_FCU_CONFIG_SET_BPS_SEL + 0));
		bps_val_sel[1] = *((uint32_t *)(FLASH_HP_FCU_CONFIG_SET_BPS_SEL + 3));
		bps_val_sel[2] = *((uint32_t *)(FLASH_HP_FCU_CONFIG_SET_BPS_SEL + 7));
		bps_val_sel[3] = *((uint32_t *)(FLASH_HP_FCU_CONFIG_SET_BPS_SEL + 11));
	}

	if (pbps_val_ns != NULL) {
		pbps_val_ns[0] = *((uint32_t *)(FLASH_HP_FCU_CONFIG_SET_PBPS + 0));
		pbps_val_ns[1] = *((uint32_t *)(FLASH_HP_FCU_CONFIG_SET_PBPS + 3));
		pbps_val_ns[2] = *((uint32_t *)(FLASH_HP_FCU_CONFIG_SET_PBPS + 7));
		pbps_val_ns[3] = *((uint32_t *)(FLASH_HP_FCU_CONFIG_SET_PBPS + 11));
	}

	if (pbps_val_sec != NULL) {
		pbps_val_sec[0] = *((uint32_t *)(FLASH_HP_FCU_CONFIG_SET_PBPS_SEC + 0));
		pbps_val_sec[1] = *((uint32_t *)(FLASH_HP_FCU_CONFIG_SET_PBPS_SEC + 3));
		pbps_val_sec[2] = *((uint32_t *)(FLASH_HP_FCU_CONFIG_SET_PBPS_SEC + 7));
		pbps_val_sec[3] = *((uint32_t *)(FLASH_HP_FCU_CONFIG_SET_PBPS_SEC + 11));
	}

	if (size != NULL) {
		*size = 4;
	}
#endif

	FSP_PARAMETER_NOT_USED(p_api_ctrl);

	return err;
}

#if defined(CONFIG_FLASH_RA_WRITE_PROTECT)
int flash_ra_block_protect_get(const struct device *dev,
			       struct flash_ra_ex_write_protect_out_t *response)
{
	fsp_err_t err = FSP_ERR_ASSERTION;
	struct flash_hp_ra_data *data = dev->data;
	flash_ra_cf_block_map bps_ns;

	/* get the current non-secure BPS register values */
	err = R_FLASH_HP_BlockProtectGet(&data->flash_ctrl, (uint32_t *)&bps_ns, NULL, NULL, NULL,
				    NULL, NULL);
	memcpy(&response->protected_enabled, &bps_ns, sizeof(flash_ra_cf_block_map));

	return err;
}

int flash_ra_ex_op_write_protect(const struct device *dev, const uintptr_t in, void *out)
{
	const struct flash_ra_ex_write_protect_in_t *request =
		(const struct flash_ra_ex_write_protect_in_t *)in;
	struct flash_ra_ex_write_protect_out_t *result =
		(struct flash_ra_ex_write_protect_out_t *)out;

	int rc = 0, rc2 = 0;
#ifdef CONFIG_USERSPACE
	bool syscall_trap = z_syscall_trap();
#endif

	if (request != NULL) {
#ifdef CONFIG_USERSPACE
		struct flash_ra_ex_write_protect_in_t in_copy;

		if (syscall_trap) {
			Z_OOPS(z_user_from_copy(&in_copy, request, sizeof(in_copy)));
			request = &in_copy;
		}
#endif
		/* if both enable and disable are set  */
		if ((request->protect_enable.BPS[0] & request->protect_disable.BPS[0]) ||
		    (request->protect_enable.BPS[1] & request->protect_disable.BPS[1]) ||
		    (request->protect_enable.BPS[2] & request->protect_disable.BPS[2]) ||
		    (request->protect_enable.BPS[3] & request->protect_disable.BPS[3])) {
			return EINVAL;
		}

		rc = flash_ra_block_protect_set(dev, request);
	}

	if (result != NULL) {
#ifdef CONFIG_USERSPACE
		struct flash_ra_ex_write_protect_out_t out_copy;

		if (syscall_trap) {
			result = &out_copy;
		}
#endif
		rc2 = flash_ra_block_protect_get(dev, result);
		if (!rc) {
			rc = rc2;
		}

#ifdef CONFIG_USERSPACE
		if (syscall_trap) {
			Z_OOPS(z_user_to_copy(out, result, sizeof(out_copy)));
		}
#endif
	}

	return rc;
}

int flash_ra_block_protect_set(const struct device *dev,
			       const struct flash_ra_ex_write_protect_in_t *request)
{
	fsp_err_t err = FSP_ERR_ASSERTION;
	struct flash_hp_ra_data *data = dev->data;
	flash_ra_cf_block_map bps_ns;

	/* get the current non-secure BPS register values */
	err = R_FLASH_HP_BlockProtectGet(&data->flash_ctrl, (uint32_t *)&bps_ns, NULL, NULL, NULL,
				    NULL, NULL);

	if (err != FSP_SUCCESS) {
		__ASSERT(false, "flash: block get current value error =%d", err);
		return -EIO;
	}

	/* enable block protect */
	bps_ns.BPS[0] &= ~(request->protect_enable.BPS[0]);
	bps_ns.BPS[1] &= ~(request->protect_enable.BPS[1]);
	bps_ns.BPS[2] &= ~(request->protect_enable.BPS[2]);
	bps_ns.BPS[3] &= ~(request->protect_enable.BPS[3]);

	/* disable block protect */
	bps_ns.BPS[0] |= (request->protect_disable.BPS[0]);
	bps_ns.BPS[1] |= (request->protect_disable.BPS[1]);
	bps_ns.BPS[2] |= (request->protect_disable.BPS[2]);
	bps_ns.BPS[3] |= (request->protect_disable.BPS[3]);

	/* reset default all from non-secure */
	err = R_FLASH_HP_BlockProtectSet(&data->flash_ctrl, (uint8_t *)&bps_ns, NULL, NULL, NULL,
					 NULL, sizeof(bps_ns));

	if (err != FSP_SUCCESS) {
		__ASSERT(false, "flash: block protect error=%d", err);
		return -EIO;
	}
	return 0;
}

#endif /* CONFIG_FLASH_RA_WRITE_PROTECT */
