/*
 * Copyright (c) 2024 Renesas Electronics Corporation
 *
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

#define FLASH_HP_CONFIG_SET_ACCESS_WORD_CNT (8U)

#if (DT_PROP(DT_NODELABEL(flash0), renesas_programming_enable))
extern uint16_t g_configuration_area_data[FLASH_HP_CONFIG_SET_ACCESS_WORD_CNT];
#endif
extern fsp_err_t
flash_hp_enter_pe_cf_mode(flash_hp_instance_ctrl_t *const p_ctrl) PLACE_IN_RAM_SECTION;

extern fsp_err_t flash_hp_stop(void) PLACE_IN_RAM_SECTION;

extern fsp_err_t flash_hp_configuration_area_write(flash_hp_instance_ctrl_t *p_ctrl,
						   uint32_t fsaddr,
						   uint16_t *src_address) PLACE_IN_RAM_SECTION;

extern fsp_err_t flash_hp_check_errors(fsp_err_t previous_error, uint32_t error_bits,
				       fsp_err_t return_error) PLACE_IN_RAM_SECTION;

extern fsp_err_t flash_hp_pe_mode_exit(void) PLACE_IN_RAM_SECTION;

static fsp_err_t flash_hp_set_block_protect_ns(flash_hp_instance_ctrl_t *p_ctrl,
					       uint8_t *bps_val_ns, uint8_t *pbps_val_ns,
					       uint32_t size) PLACE_IN_RAM_SECTION;

static fsp_err_t flash_hp_set_block_protect_sec(flash_hp_instance_ctrl_t *p_ctrl,
						uint8_t *bps_val_sec, uint8_t *pbps_val_sec,
						uint32_t size) PLACE_IN_RAM_SECTION;

static fsp_err_t flash_hp_set_block_protect_sel(flash_hp_instance_ctrl_t *p_ctrl,
						uint8_t *bps_sel_val,
						uint32_t size) PLACE_IN_RAM_SECTION;

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
		err = flash_hp_configuration_area_write(p_ctrl, FLASH_HP_FCU_CONFIG_SET_BPS,
							&g_configuration_area_data);
		err = flash_hp_check_errors(err, 0, FSP_ERR_WRITE_FAILED);
	}

	memset(g_configuration_area_data, UINT8_MAX, sizeof(g_configuration_area_data));
	if (pbps_val_ns != NULL) {
		memcpy(&g_configuration_area_data[FLASH_HP_FCU_CONFIG_SET_BPS_OFFSET], pbps_val_ns,
		       size);
		err = flash_hp_configuration_area_write(p_ctrl, FLASH_HP_FCU_CONFIG_SET_PBPS,
							&g_configuration_area_data);
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
		err = flash_hp_configuration_area_write(p_ctrl, FLASH_HP_FCU_CONFIG_SET_BPS_SEC,
							&g_configuration_area_data);
		err = flash_hp_check_errors(err, 0, FSP_ERR_WRITE_FAILED);
	}

	memset(g_configuration_area_data, UINT8_MAX, sizeof(g_configuration_area_data));
	if (pbps_val_sec != NULL) {
		memcpy(&g_configuration_area_data[FLASH_HP_FCU_CONFIG_SET_BPS_OFFSET], pbps_val_sec,
		       size);
		err = flash_hp_configuration_area_write(p_ctrl, FLASH_HP_FCU_CONFIG_SET_PBPS_SEC,
							&g_configuration_area_data);
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
	err = flash_hp_configuration_area_write(p_ctrl, FLASH_HP_FCU_CONFIG_SET_BPS_SEL,
						&g_configuration_area_data);
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

static fsp_err_t R_FLASH_HP_BlockProtectSet(flash_ctrl_t *const p_api_ctrl, uint8_t *bps_val_ns,
					    uint8_t *bps_val_sec, uint8_t *bps_val_sel,
					    uint8_t *pbps_val_ns, uint8_t *pbps_val_sec,
					    uint32_t size);

static fsp_err_t R_FLASH_HP_BlockProtectGet(flash_ctrl_t *const p_api_ctrl, uint32_t *bps_val_ns,
					    uint32_t *bps_val_sec, uint8_t *bps_val_sel,
					    uint8_t *pbps_val_ns, uint8_t *pbps_val_sec,
					    uint32_t *size);

int flash_ra_block_protect_set(const struct device *dev,
			       const struct flash_ra_ex_write_protect_in *request);

int flash_ra_block_protect_get(const struct device *dev,
			       struct flash_ra_ex_write_protect_out *response);

static fsp_err_t R_FLASH_HP_BlockProtectSet(flash_ctrl_t *const p_api_ctrl, uint8_t *bps_val_ns,
					    uint8_t *bps_val_sec, uint8_t *bps_val_sel,
					    uint8_t *pbps_val_ns, uint8_t *pbps_val_sec,
					    uint32_t size)
{
	flash_hp_instance_ctrl_t *p_ctrl = (flash_hp_instance_ctrl_t *)p_api_ctrl;
	fsp_err_t err = FSP_SUCCESS;

#if (DT_PROP(DT_NODELABEL(flash0), renesas_programming_enable))

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

static fsp_err_t R_FLASH_HP_BlockProtectGet(flash_ctrl_t *const p_api_ctrl, uint32_t *bps_val_ns,
					    uint32_t *bps_val_sec, uint8_t *bps_val_sel,
					    uint8_t *pbps_val_ns, uint8_t *pbps_val_sec,
					    uint32_t *size)
{
	fsp_err_t err = FSP_ERR_UNSUPPORTED;

#if (DT_PROP(DT_NODELABEL(flash0), renesas_programming_enable))

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
			       struct flash_ra_ex_write_protect_out *response)
{
	fsp_err_t err = FSP_ERR_ASSERTION;
	struct flash_hp_ra_data *flash_data = dev->data;
	struct flash_hp_ra_controller *dev_ctrl = flash_data->controller;
	flash_ra_cf_block_map bps_ns;

	/* get the current non-secure BPS register values */
	err = R_FLASH_HP_BlockProtectGet(&dev_ctrl->flash_ctrl, (uint32_t *)&bps_ns, NULL, NULL,
					 NULL, NULL, NULL);
	memcpy(&response->protected_enabled, &bps_ns, sizeof(flash_ra_cf_block_map));

	return err;
}

int flash_ra_ex_op_write_protect(const struct device *dev, const uintptr_t in, void *out)
{
	const struct flash_ra_ex_write_protect_in *request =
		(const struct flash_ra_ex_write_protect_in *)in;
	struct flash_ra_ex_write_protect_out *result = (struct flash_ra_ex_write_protect_out *)out;

	int rc = 0, rc2 = 0;
#ifdef CONFIG_USERSPACE
	bool syscall_trap = z_syscall_trap();
#endif

	if (request != NULL) {
#ifdef CONFIG_USERSPACE
		struct flash_ra_ex_write_protect_in copy_in;

		if (syscall_trap) {
			Z_OOPS(z_user_from_copy(&copy_in, request, sizeof(copy_in)));
			request = &copy_in;
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
		struct flash_ra_ex_write_protect_out copy_out;

		if (syscall_trap) {
			result = &copy_out;
		}
#endif
		rc2 = flash_ra_block_protect_get(dev, result);
		if (!rc) {
			rc = rc2;
		}

#ifdef CONFIG_USERSPACE
		if (syscall_trap) {
			Z_OOPS(z_user_to_copy(out, result, sizeof(copy_out)));
		}
#endif
	}

	return rc;
}

int flash_ra_block_protect_set(const struct device *dev,
			       const struct flash_ra_ex_write_protect_in *request)
{
	fsp_err_t err = FSP_ERR_ASSERTION;
	struct flash_hp_ra_data *flash_data = dev->data;
	struct flash_hp_ra_controller *dev_ctrl = flash_data->controller;
	flash_ra_cf_block_map bps_ns;

	/* get the current non-secure BPS register values */
	err = R_FLASH_HP_BlockProtectGet(&dev_ctrl->flash_ctrl, (uint32_t *)&bps_ns, NULL, NULL,
					 NULL, NULL, NULL);

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
	err = R_FLASH_HP_BlockProtectSet(&dev_ctrl->flash_ctrl, (uint8_t *)&bps_ns, NULL, NULL,
					 NULL, NULL, sizeof(bps_ns));

	if (err != FSP_SUCCESS) {
		__ASSERT(false, "flash: block protect error=%d", err);
		return -EIO;
	}
	return 0;
}

#endif /* CONFIG_FLASH_RA_WRITE_PROTECT */
