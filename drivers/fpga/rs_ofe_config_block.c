/****************************************************************************************
 * File Name: 		rs_ofe_config_block.c
 * Author: 			  Muhammad Junaid Aslam
 * Organization:	Rapid Silicon
 * Repository: 		EmbeddedCommon-Dev
 *
 * @brief: This file contains driver implementation for common interfaces of
 *FCB, ICB, CCB and PCB
 *
 * @copyright:  	<Not Available>
 *
 * History:
 * 11-10-2022: Created this file for One flow engine configuration.
 ****************************************************************************************/

#include "rs_ofe_config_block.h"

#include "mpaland_printf.h"
#include "rs_config_controllers.h"
#include "rs_util.h"

static volatile struct rs_ofe_cfg_status *s_OFE_Cfg_Status = NULL;

/* ******************************************
 * @brief: ofe_get_config_status will return
 * the status of configuration for FCB or ICB.
 *
 * @param [in]: OFE register element for
 * configuration status; e.g, FCB, ICB etc
 *
 * @return: true or false
 * ******************************************/
bool rs_ofe_get_config_status(enum RS_OFE_REG_ELEM inElem) {
  switch (inElem) {
    case (RS_OFE_REG_ELEM_FCB): {
      return s_OFE_Cfg_Status->fcb_cfg_status;
    } break;
    case (RS_OFE_REG_ELEM_ICB): {
      return s_OFE_Cfg_Status->icb_cfg_status;
    } break;
  }
  return false;
}

/* ******************************************
 * @brief: ofe_set_config_status is used to
 * either set true or false to the
 * config_done or config_error bits in OFE
 * statuc register.
 *
 * @param [in]: bool
 *
 * @return: void
 *
 * @note: Make sure,If config_done is 1 then
 * config_error should be 0. Similarly, if
 * config_done is 0 then config_error should
 * be 1.
 * ******************************************/
void rs_ofe_set_config_status(bool inValue, enum RS_OFE_CFG_DONE_ERROR inElem) {
  struct rs_ofe_cfg_status lv_ofe_cfg_status = {0};

  // Read
  lv_ofe_cfg_status = *s_OFE_Cfg_Status;

  // Modify
  switch (inElem) {
    case (RS_OFE_CFG_DONE): {
      lv_ofe_cfg_status.cfg_done = inValue;
    } break;
    case (RS_OFE_CFG_ERROR): {
      lv_ofe_cfg_status.cfg_error = inValue;
    } break;
  }

  // Write
  REG_WRITE_32((void *)s_OFE_Cfg_Status, (void *)&lv_ofe_cfg_status);

  return;
}

/* ******************************************
 * @brief: ofe_set_pcb_rstn toggles the pcb
 * reset bit in the ofe_status register.
 * @param [in]: bool to set or reset pcb
 *
 * @return: enum xCB_ERROR_CODE
 * ******************************************/
enum xCB_ERROR_CODE rs_ofe_pcb_rstn(bool value) {
  struct rs_ofe_cfg_status lv_ofe_cfg_status = {0};

  // Read
  lv_ofe_cfg_status = *s_OFE_Cfg_Status;

  // Modify
  lv_ofe_cfg_status.pcb_rstn = value;

  // Write
  REG_WRITE_32((void *)s_OFE_Cfg_Status, (void *)&lv_ofe_cfg_status);

  if (s_OFE_Cfg_Status->pcb_rstn != value) {
    return xCB_ERROR_WRITE_ERROR;
    RS_LOG_ERROR("OFE", "%s(%d):%s\r\n", __func__, __LINE__,
                 Err_to_Str(xCB_ERROR_WRITE_ERROR));
  }

  return xCB_SUCCESS;
}

/* ******************************************
 * @brief: ofe_eFPGA_rstn sets the
 * global reset bit for eFPGA in ofe_status
 * register.
 * @param [in]: bool to set or reset eFPGA
 *
 * @return: enum xCB_ERROR_CODE
 * ******************************************/
enum xCB_ERROR_CODE rs_ofe_eFPGA_rstn(bool value) {
  struct rs_ofe_cfg_status lv_ofe_cfg_status;

  // Read
  lv_ofe_cfg_status = *s_OFE_Cfg_Status;

  // Modify
  lv_ofe_cfg_status.global_reset_fpga = value;

  // Write
  REG_WRITE_32((void *)s_OFE_Cfg_Status, (void *)&lv_ofe_cfg_status);

  if (s_OFE_Cfg_Status->global_reset_fpga != value) {
    return xCB_ERROR_WRITE_ERROR;
    RS_LOG_ERROR("OFE", "%s(%d):%s\r\n", __func__, __LINE__,
                 Err_to_Str(xCB_ERROR_WRITE_ERROR));
  }

  return xCB_SUCCESS;
}

/* ******************************************
 * @brief: RS_ofe_init interface sets the
 * required parameters to work on specific
 * platform.
 *
 * @param [in]: uint32_t inBaseAddr.
 *
 * @return: xCB_ERROR_CODE
 * ******************************************/
enum xCB_ERROR_CODE rs_ofe_init(uint32_t inBaseAddr) {
  enum xCB_ERROR_CODE lvErrorCode = xCB_SUCCESS;

  if (inBaseAddr == 0x00) {
    lvErrorCode = xCB_ERROR_INVALID_DATA;
  } else {
    s_OFE_Cfg_Status = ((struct rs_ofe_cfg_status *)(inBaseAddr + 0x00));
  }

  return lvErrorCode;
}