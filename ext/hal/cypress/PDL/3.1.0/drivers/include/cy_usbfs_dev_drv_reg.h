/***************************************************************************//**
* \file cy_usbfs_dev_drv_reg.h
* \version 1.0
*
* Provides register access API implementation of the USBFS driver.
*
********************************************************************************
* \copyright
* Copyright 2018, Cypress Semiconductor Corporation.  All rights reserved.
* SPDX-License-Identifier: Apache-2.0



*******************************************************************************/

/**
* \addtogroup group_usbfs_dev_drv_reg
* \{
*
* Register access API for the USBFS Device block.
*
* This is the API that provides an interface to the USBFS Device hardware.
* These API are intended to be used by the USBFS Device driver to access
* hardware. You can use the these API to implement a custom driver based on
* the USBFS Device hardware.
*
* \defgroup group_usbfs_dev_drv_reg_macros Macros
* \{
*   \defgroup group_usbfs_dev_drv_reg_macros_hardware       Hardware-specific Constants
*   \defgroup group_usbfs_dev_drv_reg_macros_sie_intr       SIE Interrupt Sources
*   \defgroup group_usbfs_dev_drv_reg_macros_sie_mode       SIE Endpoint Modes
*   \defgroup group_usbfs_dev_drv_reg_macros_arb_ep_intr    Arbiter Endpoint Interrupt Sources
* \}
*
* \defgroup group_usbfs_drv_drv_reg_functions Functions
* \{
*   \defgroup group_usbfs_drv_drv_reg_interrupt_sources     SIE Interrupt Sources Registers Access
*   \defgroup group_usbfs_drv_drv_reg_sie_access            SIE Data Endpoint Registers Access
*   \defgroup group_usbfs_drv_drv_reg_arbiter               Arbiter Endpoint Registers Access
*   \defgroup group_usbfs_drv_drv_reg_arbiter_data          Arbiter Endpoint Data Registers Access
* \}
*
* \defgroup group_usbfs_drv_drv_reg_data_structures Data Structures
*
* \}
*/


#if !defined(CY_USBFS_DEV_DRV_REG_H)
#define CY_USBFS_DEV_DRV_REG_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#include "cy_device_headers.h"
#include "cy_syslib.h"

#ifdef CY_IP_MXUSBFS

#if defined(__cplusplus)
extern "C" {
#endif

/*******************************************************************************
*                       Hardware-specific Constants
*******************************************************************************/

/**
* \addtogroup group_usbfs_dev_drv_reg_macros_hardware
* \{
*/
/** Number of data endpoints supported by the hardware */
#define CY_USBFS_DEV_DRV_NUM_EPS_MAX         (8UL)

/** The hardware buffer size utilized for data endpoint buffers */
#define CY_USBFS_DEV_DRV_HW_BUFFER_SIZE     (512UL)

/** The hardware buffer for endpoint 0 */
#define CY_USBFS_DEV_DRV_EP0_BUFFER_SIZE    (8UL)
/** \} group_usbfs_dev_drv_reg_macros_hardware */


/*******************************************************************************
*                            Type Definitions
*******************************************************************************/

/**
* \addtogroup group_usbfs_drv_drv_reg_data_structures
* \{
*/

/** Layout of the SIE data endpoint registers */
typedef struct
{
  __IOM uint32_t SIE_EP_CNT0;   /**< SIE Data Endpoint Count 0 register */
  __IOM uint32_t SIE_EP_CNT1;   /**< SIE Data Endpoint Count 1 register */
  __IOM uint32_t SIE_EP_CR0;    /**< SIE Data Endpoint Control 0 register */
   __IM uint32_t RESERVED[13];  /**< RESERVED location */
} cy_stc_usbfs_dev_drv_sie_t;

/** Layout of the Arbiter data endpoint registers. There registers contain
* 8 significant bits.
*/
typedef struct
{
  __IOM uint32_t ARB_EP_CFG;    /**< Arbiter Configuration register */  
  __IOM uint32_t ARB_EP_INT_EN; /**< Arbiter Interrupt Enable register */  
  __IOM uint32_t ARB_EP_SR;     /**< Arbiter Interrupt Status register */  
   __IM uint32_t RESERVED1;     /**< RESERVED location */
  __IOM uint32_t ARB_RW_WA;     /**< Arbiter Write Address register */  
  __IOM uint32_t ARB_RW_WA_MSB; /**< Arbiter Write Address register MSB */ 
  __IOM uint32_t ARB_RW_RA;     /**< Arbiter Read Address register */ 
  __IOM uint32_t ARB_RW_RA_MSB; /**< Arbiter Write Address register MSB */ 
  __IOM uint32_t ARB_RW_DR;     /**< Arbiter Data register */
   __IM uint32_t RESERVED2[7];  /**< RESERVED location */
} cy_stc_usbfs_dev_drv_arb8_t;

/** Layout of the Arbiter data endpoint registers. There registers contain
* 16 significant bits.
*/
typedef struct
{
  __IOM uint32_t ARB_RW_WA16;   /**< Arbiter Write Address register (16 bits) */  
   __IM uint32_t RESERVED1;     /**< RESERVED location */
  __IOM uint32_t ARB_RW_RA16;   /**< Arbiter Read Address register (16 bits) */  
   __IM uint32_t RESERVED2;     /**< RESERVED location */
  __IOM uint32_t ARB_RW_DR16;   /**< Arbiter Data register (16 bits) */
   __IM uint32_t RESERVED3[11]; /**< RESERVED location */
} cy_stc_usbfs_dev_drv_arb16_t;


/** This structure maps SIE endpoints registers to USBFS_Type.
* It is used to access SIE endpoints registers.
*/
typedef struct
{
   __IM uint32_t RESERVED[12];                                  /**< RESERVED location */
   cy_stc_usbfs_dev_drv_sie_t EP[CY_USBFS_DEV_DRV_NUM_EPS_MAX];  /**< Data Endpoints SIE registers array */
} cy_stc_usbfs_dev_drv_acces_sie_t;


/** This structure maps Arbiter endpoints registers to USBFS_Type.
* It is used to access Arbiter endpoints registers.
*/
typedef struct
{
   __IM uint32_t RESERVED[128];                                     /**< RESERVED location */
   cy_stc_usbfs_dev_drv_arb8_t EP8[CY_USBFS_DEV_DRV_NUM_EPS_MAX];    /**< Data Endpoints Arbiter registers array */
   __IM uint32_t RESERVED1[900];                                    /**< RESERVED location */
   cy_stc_usbfs_dev_drv_arb16_t EP16[CY_USBFS_DEV_DRV_NUM_EPS_MAX];  /**< Data Endpoints Arbiter 16-bit registers array */
} cy_stc_usbfs_dev_drv_access_arb_t;

/** \} group_usbfs_drv_drv_reg_data_structures */


/*******************************************************************************
*                          Functions
*******************************************************************************/

/**
* \addtogroup group_usbfs_drv_drv_reg_interrupt_sources
* \{
*/
/* Access to LPM SIE interrupt sources */
__STATIC_INLINE uint32_t Cy_USBFS_Dev_Drv_GetSieInterruptStatus(USBFS_Type const *base);
__STATIC_INLINE     void Cy_USBFS_Dev_Drv_SetSieInterruptMask  (USBFS_Type *base, uint32_t mask);
__STATIC_INLINE uint32_t Cy_USBFS_Dev_Drv_GetSieInterruptMask  (USBFS_Type const *base);
__STATIC_INLINE uint32_t Cy_USBFS_Dev_Drv_GetSieInterruptStatusMasked(USBFS_Type const *base);
__STATIC_INLINE     void Cy_USBFS_Dev_Drv_ClearSieInterrupt    (USBFS_Type *base, uint32_t mask);
__STATIC_INLINE     void Cy_USBFS_Dev_Drv_SetSieInterrupt      (USBFS_Type *base, uint32_t mask);
/** \} group_usbfs_drv_drv_reg_interrupt_sources */

/**
* \addtogroup group_usbfs_drv_drv_reg_sie_access
* \{
*/
/* Access SIE data endpoints CR0.Mode registers */
__STATIC_INLINE void     Cy_USBFS_Dev_Drv_SetSieEpMode(USBFS_Type *base, uint32_t endpoint, uint32_t mode);
__STATIC_INLINE uint32_t Cy_USBFS_Dev_Drv_GetSieEpError(USBFS_Type *base, uint32_t endpoint);

/* Access SIE data endpoints CNT0 and CNT1 registers */
__STATIC_INLINE uint32_t Cy_USBFS_Dev_Drv_GetSieEpToggle  (USBFS_Type *base, uint32_t endpoint);
__STATIC_INLINE void     Cy_USBFS_Dev_Drv_ClearSieEpToggle(USBFS_Type *base, uint32_t endpoint);
__STATIC_INLINE uint32_t Cy_USBFS_Dev_Drv_GetSieEpCount(USBFS_Type *base, uint32_t endpoint);
__STATIC_INLINE void     Cy_USBFS_Dev_Drv_SetSieEpCount(USBFS_Type *base, uint32_t endpoint, uint32_t count, uint32_t toggle);

/* Access SIE data endpoints interrupt source registers */
__STATIC_INLINE uint32_t Cy_USBFS_Dev_Drv_GetSieAllEpsInterruptStatus(USBFS_Type const *base);
__STATIC_INLINE void     Cy_USBFS_Dev_Drv_EnableSieEpInterrupt (USBFS_Type *base, uint32_t endpoint);
__STATIC_INLINE void     Cy_USBFS_Dev_Drv_DisableSieEpInterrupt(USBFS_Type *base, uint32_t endpoint);
__STATIC_INLINE void     Cy_USBFS_Dev_Drv_ClearSieEpInterrupt  (USBFS_Type *base, uint32_t endpoint);
/** \} group_usbfs_drv_drv_reg_sie_access */

/**
* \addtogroup group_usbfs_drv_drv_reg_arbiter
* \{
*/
/* Access Arbiter data endpoints interrupt sources registers */
__STATIC_INLINE uint32_t Cy_USBFS_Dev_Drv_GetArbAllEpsInterruptStatus(USBFS_Type const *base);
__STATIC_INLINE void     Cy_USBFS_Dev_Drv_EnableArbEpInterrupt (USBFS_Type *base, uint32_t endpoint);
__STATIC_INLINE void     Cy_USBFS_Dev_Drv_DisableArbEpInterrupt(USBFS_Type *base, uint32_t endpoint);
__STATIC_INLINE void     Cy_USBFS_Dev_Drv_SetArbEpInterruptMask(USBFS_Type *base, uint32_t endpoint, uint32_t mask);
__STATIC_INLINE uint32_t Cy_USBFS_Dev_Drv_GetArbEpInterruptMask(USBFS_Type *base, uint32_t endpoint);
__STATIC_INLINE uint32_t Cy_USBFS_Dev_Drv_GetArbEpInterruptStatusMasked(USBFS_Type *base, uint32_t endpoint);
__STATIC_INLINE void     Cy_USBFS_Dev_Drv_ClearArbEpInterrupt(USBFS_Type *base, uint32_t endpoint, uint32_t mask);

/* Access Arbiter data endpoints configuration register */
__STATIC_INLINE void Cy_USBFS_Dev_Drv_SetArbEpConfig       (USBFS_Type *base, uint32_t endpoint, uint32_t cfg);
__STATIC_INLINE void Cy_USBFS_Dev_Drv_SetArbCfgEpInReady   (USBFS_Type *base, uint32_t endpoint);
__STATIC_INLINE void Cy_USBFS_Dev_Drv_ClearArbCfgEpInReady (USBFS_Type *base, uint32_t endpoint);
__STATIC_INLINE void Cy_USBFS_Dev_Drv_TriggerArbCfgEpDmaReq(USBFS_Type *base, uint32_t endpoint);

/* Access Arbiter data endpoints WA (Write Address and RA(Read Address) registers */
__STATIC_INLINE void     Cy_USBFS_Dev_Drv_SetArbWriteAddr (USBFS_Type *base, uint32_t endpoint, uint32_t wa);
__STATIC_INLINE void     Cy_USBFS_Dev_Drv_SetArbReadAddr  (USBFS_Type *base, uint32_t endpoint, uint32_t ra);
__STATIC_INLINE uint32_t Cy_USBFS_Dev_Drv_GetArbWriteAddr (USBFS_Type *base, uint32_t endpoint);
__STATIC_INLINE uint32_t Cy_USBFS_Dev_Drv_GetArbReadAddr  (USBFS_Type *base, uint32_t endpoint);
/** \} group_usbfs_drv_drv_reg_arbiter */

/**
* \addtogroup group_usbfs_drv_drv_reg_arbiter_data 
* \{
*/
/* Access data endpoints data registers. Used to get/put data into endpoint buffer. */
__STATIC_INLINE void     Cy_USBFS_Dev_Drv_WriteData  (USBFS_Type *base, uint32_t endpoint, uint8_t  byte);
__STATIC_INLINE void     Cy_USBFS_Dev_Drv_WriteData16(USBFS_Type *base, uint32_t endpoint, uint16_t halfword);
__STATIC_INLINE uint8_t  Cy_USBFS_Dev_Drv_ReadData   (USBFS_Type *base, uint32_t endpoint);
__STATIC_INLINE uint16_t Cy_USBFS_Dev_Drv_ReadData16 (USBFS_Type *base, uint32_t endpoint);
__STATIC_INLINE void *   Cy_USBFS_Dev_Drv_GetDataRegAddr  (USBFS_Type *base, uint32_t endpoint);
__STATIC_INLINE void *   Cy_USBFS_Dev_Drv_GetDataReg16Addr(USBFS_Type *base, uint32_t endpoint);
/** \} group_usbfs_drv_drv_reg_arbiter_data */


/** \cond INTERNAL */
__STATIC_INLINE uint32_t Cy_USBFS_Dev_Drv_GetEp0Count(USBFS_Type *base);
__STATIC_INLINE void     Cy_USBFS_Dev_Drv_SetEpType  (USBFS_Type *base, bool dir, uint32_t endpoint);
/** \endcond */

/*******************************************************************************
*                       API Constants
*******************************************************************************/

/** \cond INTERNAL */
/* Macro to access ODD offset registers: Cypress ID# 299773 */
#define CY_USBFS_DEV_DRV_WRITE_ODD(val)  ((val) | (((uint32_t) (val)) << 8U))
#define CY_USBFS_DEV_READ_ODD(reg)   (CY_LO8((reg) | ((reg) >> 8U)))
/** \endcond */

/**
* \addtogroup group_usbfs_dev_drv_reg_macros_sie_intr
* \{
*/
#define CY_USBFS_DEV_DRV_INTR_SIE_SOF       USBFS_USBLPM_INTR_SIE_SOF_INTR_Msk      /**< SOF frame detected */
#define CY_USBFS_DEV_DRV_INTR_SIE_BUS_RESET USBFS_USBLPM_INTR_SIE_BUS_RESET_INTR_Msk  /**< Bus Reset detected */
#define CY_USBFS_DEV_DRV_INTR_SIE_EP0       USBFS_USBLPM_INTR_SIE_EP0_INTR_Msk          /**< EP0 access detected */
#define CY_USBFS_DEV_DRV_INTR_SIE_LPM       USBFS_USBLPM_INTR_SIE_LPM_INTR_Msk          /**< Link Power Management request detected */
#define CY_USBFS_DEV_DRV_INTR_SIE_RESUME    USBFS_USBLPM_INTR_SIE_RESUME_INTR_Msk       /**< Resume condition detected */
/** \} group_usbfs_dev_drv_reg_macros_sie_intr */

/**
* \addtogroup group_usbfs_dev_drv_reg_macros_sie_mode
* \{
*/
/* Modes for endpoint 0 (control endpoint) */
#define CY_USBFS_DEV_DRV_EP_CR_DISABLE           (0UL)  /**< Data endpoint disabled */
#define CY_USBFS_DEV_DRV_EP_CR_NAK_INOUT         (1UL)  /**< Data endpoint NAK IN and OUT requests */
#define CY_USBFS_DEV_DRV_EP_CR_STALL_INOUT       (3UL)  /**< Data endpoint STALL IN and OUT requests */
#define CY_USBFS_DEV_DRV_EP_CR_STATUS_OUT_ONLY   (2UL)  /**< Data endpoint ACK only Status OUT requests */
#define CY_USBFS_DEV_DRV_EP_CR_STATUS_IN_ONLY    (6UL)  /**< Data endpoint ACK only Status IN requests */
#define CY_USBFS_DEV_DRV_EP_CR_ACK_OUT_STATUS_IN (11UL) /**< Data endpoint ACK only Data and Status OUT requests */
#define CY_USBFS_DEV_DRV_EP_CR_ACK_IN_STATUS_OUT (15UL) /**< Data endpoint ACK only Data and Status IN requests */

/* Modes for ISO data endpoints */
#define CY_USBFS_DEV_DRV_EP_CR_ISO_OUT          (5UL)   /**< Data endpoint is ISO OUT */
#define CY_USBFS_DEV_DRV_EP_CR_ISO_IN           (7UL)   /**< Data endpoint is ISO IN */

/* Modes for Control/Bulk/Interrupt OUT data endpoints */
#define CY_USBFS_DEV_DRV_EP_CR_NAK_OUT          (8UL)   /**< Data endpoint NAK OUT requests */
#define CY_USBFS_DEV_DRV_EP_CR_ACK_OUT          (9UL)   /**< Data endpoint ACK OUT requests */

/* Modes for Control/Bulk/Interrupt IN data endpoints */
#define CY_USBFS_DEV_DRV_EP_CR_NAK_IN           (12UL)   /**< Data endpoint NAK IN requests */
#define CY_USBFS_DEV_DRV_EP_CR_ACK_IN           (13UL)   /**< Data endpoint ACK IN requests */
/** \} group_usbfs_dev_drv_reg_macros_sie_mode */

/**
* \addtogroup group_usbfs_dev_drv_reg_macros_arb_ep_intr
* \{
*/
/* ARB_EP_SR 1-8 registers */
#define USBFS_USBDEV_ARB_EP_IN_BUF_FULL_Msk USBFS_USBDEV_ARB_EP1_SR_IN_BUF_FULL_Msk /**< Data endpoint IN buffer full interrupt source */
#define USBFS_USBDEV_ARB_EP_DMA_GNT_Msk     USBFS_USBDEV_ARB_EP1_SR_DMA_GNT_Msk     /**< Data endpoint grant interrupt source (DMA complete read/write) */
#define USBFS_USBDEV_ARB_EP_DMA_TERMIN_Msk  USBFS_USBDEV_ARB_EP1_SR_DMA_TERMIN_Msk  /**< Data endpoint terminate interrupt source (DMA complete reading) */
#define USBFS_USBDEV_ARB_EP_BUF_OVER_Msk    USBFS_USBDEV_ARB_EP1_SR_BUF_OVER_Msk    /**< Data endpoint overflow interrupt source (only applicable for Automatic DMA mode) */
#define USBFS_USBDEV_ARB_EP_BUF_UNDER_Msk   USBFS_USBDEV_ARB_EP1_SR_BUF_UNDER_Msk   /**< Data endpoint underflow interrupt source (only applicable for Automatic DMA mode) */
/** \} group_usbfs_dev_drv_reg_macros_arb_ep_intr */

/**
* \addtogroup group_usbfs_dev_drv_reg_macros
* \{
*/
/** Data toggle mask in CNT0 register */
#define USBFS_USBDEV_SIE_EP_DATA_TOGGLE_Msk USBFS_USBDEV_SIE_EP1_CNT0_DATA_TOGGLE_Msk
/** \} group_usbfs_dev_drv_reg_macros */

/** \cond INTERNAL */

/* Extended cyip_usbfs.h */

/* Count registers includes CRC size (2 bytes) */
#define CY_USBFS_DEV_DRV_EP_CRC_SIZE        (2UL)

/* DYN_RECONFIG register */
#define USBFS_USBDEV_DYN_RECONFIG_EN_Msk       USBFS_USBDEV_DYN_RECONFIG_DYN_CONFIG_EN_Msk
#define USBFS_USBDEV_DYN_RECONFIG_EPNO_Pos     USBFS_USBDEV_DYN_RECONFIG_DYN_RECONFIG_EPNO_Pos
#define USBFS_USBDEV_DYN_RECONFIG_EPNO_Msk     USBFS_USBDEV_DYN_RECONFIG_DYN_RECONFIG_EPNO_Msk
#define USBFS_USBDEV_DYN_RECONFIG_RDY_STS_Msk  USBFS_USBDEV_DYN_RECONFIG_DYN_RECONFIG_RDY_STS_Msk

/* LPM_CTL register */
#define USBFS_USBLPM_LPM_CTL_LPM_RESP_Pos   (USBFS_USBLPM_LPM_CTL_LPM_ACK_RESP_Pos)
#define USBFS_USBLPM_LPM_CTL_LPM_RESP_Msk   (USBFS_USBLPM_LPM_CTL_LPM_ACK_RESP_Msk | \
                                             USBFS_USBLPM_LPM_CTL_NYET_EN_Msk)

/* ARB_EP_CFG 1-8 registers (default configuration) */
#define USBFS_USBDEV_ARB_EP_CFG_CRC_BYPASS_Msk USBFS_USBDEV_ARB_EP1_CFG_CRC_BYPASS_Msk
#define USBFS_USBDEV_ARB_EP_CFG_RESET_PTR_Msk  USBFS_USBDEV_ARB_EP1_CFG_CRC_BYPASS_Msk
/** \endcond */


/*******************************************************************************
*                         In-line Function Implementation
*******************************************************************************/

/**
* \addtogroup group_usbfs_drv_drv_reg_interrupt_sources
* \{
*/

/*******************************************************************************
* Function Name: Cy_USBFS_Dev_Drv_GetSieInterruptStatus
****************************************************************************//**
*
* Returns the SIE interrupt request register.
* This register contains the current status of the SIE interrupt sources.
*
* \param base
* The pointer to the USBFS instance.
*
* \return
* The current status of the SIE interrupt sources.
* Each constant is a bit field value. The value returned may have multiple
* bits set to indicate the current status.
* See \ref group_usbfs_dev_drv_reg_macros_sie_intr for the set of constants.
*
*******************************************************************************/
__STATIC_INLINE uint32_t Cy_USBFS_Dev_Drv_GetSieInterruptStatus(USBFS_Type const *base)
{
    return base->USBLPM.INTR_SIE;
}


/*******************************************************************************
* Function Name: Cy_USBFS_Dev_Drv_SetSieInterruptMask
****************************************************************************//**
*
* Writes the SIE interrupt mask register.
* This register configures which bits from the SIE interrupt request register
* can trigger an interrupt event.
*
* \param base
* The pointer to the USBFS instance.
*
* \param mask
* Enabled SIE interrupt sources.
* See \ref group_usbfs_dev_drv_reg_macros_sie_intr for the set of constants.
*
*******************************************************************************/
__STATIC_INLINE void Cy_USBFS_Dev_Drv_SetSieInterruptMask(USBFS_Type *base, uint32_t mask)
{
    base->USBLPM.INTR_SIE_MASK = mask;
}


/*******************************************************************************
* Function Name: Cy_USBFS_Dev_Drv_GetSieInterruptMask
****************************************************************************//**
*
* Returns the SIE interrupt mask register.
* This register specifies which bits from the SIE interrupt request register
* trigger can an interrupt event.
*
* \param base
* The pointer to the USBFS instance.
*
* \return
* Enabled SIE interrupt sources.
* See \ref group_usbfs_dev_drv_reg_macros_sie_intr for the set of constants.
*
*******************************************************************************/
__STATIC_INLINE uint32_t Cy_USBFS_Dev_Drv_GetSieInterruptMask(USBFS_Type const *base)
{
    return base->USBLPM.INTR_SIE_MASK;
}


/*******************************************************************************
* Function Name: Cy_USBFS_Dev_Drv_GetSieInterruptStatusMasked
****************************************************************************//**
*
* Returns the SIE interrupt masked request register.
* This register contains a logical AND of corresponding bits from the SIE
* interrupt request and mask registers.
* This function is intended to be used in the interrupt service routine to
* identify which of the enabled SIE interrupt sources caused the interrupt
* event.
*
* \param base
* The pointer to the USBFS instance.
*
* \return
* The current status of enabled SIE interrupt sources.
* See \ref group_usbfs_dev_drv_reg_macros_sie_intr for the set of constants.
*
*******************************************************************************/
__STATIC_INLINE uint32_t Cy_USBFS_Dev_Drv_GetSieInterruptStatusMasked(USBFS_Type const *base)
{
    return base->USBLPM.INTR_SIE_MASKED;
}


/*******************************************************************************
* Function Name: Cy_USBFS_Dev_Drv_ClearSieInterrupt
****************************************************************************//**
*
* Clears the SIE interrupt sources in the interrupt request register.
*
* \param base
* The pointer to the USBFS instance.
*
* \param mask
* The SIE interrupt sources to be cleared.
* See \ref group_usbfs_dev_drv_reg_macros_sie_intr for the set of constants.
*
*******************************************************************************/
__STATIC_INLINE void Cy_USBFS_Dev_Drv_ClearSieInterrupt(USBFS_Type *base, uint32_t mask)
{
    base->USBLPM.INTR_SIE = mask;

    (void) base->USBLPM.INTR_SIE;
}


/*******************************************************************************
* Function Name: Cy_USBFS_Dev_Drv_SetSieInterrupt
****************************************************************************//**
*
* Sets the SIE interrupt sources in the interrupt request register.
*
* \param base
* The pointer to the USBFS instance.
*
* \param mask
* The SIE interrupt sources to be set in the SIE interrupt request register.
* See \ref group_usbfs_dev_drv_reg_macros_sie_intr for the set of constants.
*
*******************************************************************************/
__STATIC_INLINE void Cy_USBFS_Dev_Drv_SetSieInterrupt(USBFS_Type *base, uint32_t mask)
{
    base->USBLPM.INTR_SIE_SET = mask;
}
/** \} group_usbfs_drv_drv_reg_interrupt_sources */


/**
* \addtogroup group_usbfs_drv_drv_reg_sie_access
* \{
*/

/*******************************************************************************
* Function Name: Cy_USBFS_Dev_Drv_SetSieEpMode
****************************************************************************//**
*
* Writes SIE mode register of the data endpoint and clears other bits in this
* register.
*
* \param base
* The pointer to the USBFS instance.
*
* \param endpoint
* Data endpoint number.
* The starting number of data endpoint is 0 so pass 0 for data endpoint 1,
* 1 - for data endpoint 2 and so on up to (\ref CY_USBFS_DEV_DRV_NUM_EPS_MAX -
* 1).
*
* \param mode
* SIE mode defines data endpoint response to host request.
* See \ref group_usbfs_dev_drv_reg_macros_sie_mode for the set of constants.
*
*******************************************************************************/
__STATIC_INLINE void Cy_USBFS_Dev_Drv_SetSieEpMode(USBFS_Type *base, uint32_t endpoint, uint32_t mode)
{
    cy_stc_usbfs_dev_drv_acces_sie_t *sieRegs = (cy_stc_usbfs_dev_drv_acces_sie_t *) base;

    sieRegs->EP[endpoint].SIE_EP_CR0 = _CLR_SET_FLD32U(sieRegs->EP[endpoint].SIE_EP_CR0,
                                                        USBFS_USBDEV_SIE_EP1_CR0_MODE, mode);

    /* Push bufferable write to execute */
    (void) sieRegs->EP[endpoint].SIE_EP_CR0;
}

/*******************************************************************************
* Function Name: Cy_USBFS_Dev_Drv_GetSieEpError
****************************************************************************//**
*
* Returns value of data endpoint error in transaction bit.
*
* \param base
* The pointer to the USBFS instance.
*
* \param endpoint
* Data endpoint number.
*
* \return
* Value of data endpoint error in transaction bit.
*
*******************************************************************************/
__STATIC_INLINE uint32_t Cy_USBFS_Dev_Drv_GetSieEpError(USBFS_Type *base, uint32_t endpoint)
{
    cy_stc_usbfs_dev_drv_acces_sie_t *sieRegs = (cy_stc_usbfs_dev_drv_acces_sie_t *) base;

    return (sieRegs->EP[endpoint].SIE_EP_CR0 & USBFS_USBDEV_SIE_EP1_CR0_ERR_IN_TXN_Msk);
}


/*******************************************************************************
* Function Name: Cy_USBFS_Dev_Drv_GetSieEpToggle
****************************************************************************//**
*
* Returns current value of data endpoint toggle bit.
*
* \param base
* The pointer to the USBFS instance.
*
* \param endpoint
* Data endpoint number.
*
* \return
* Value of data endpoint toggle bit.
*
*******************************************************************************/
__STATIC_INLINE uint32_t Cy_USBFS_Dev_Drv_GetSieEpToggle(USBFS_Type *base, uint32_t endpoint)
{
    cy_stc_usbfs_dev_drv_acces_sie_t *sieRegs = (cy_stc_usbfs_dev_drv_acces_sie_t *) base;

    /* Return data toggle bit */
    return (sieRegs->EP[endpoint].SIE_EP_CNT0 & USBFS_USBDEV_SIE_EP1_CNT0_DATA_TOGGLE_Msk);
}


/*******************************************************************************
* Function Name: Cy_USBFS_Dev_Drv_ClearSieEpToggle
****************************************************************************//**
*
* Resets to zero data endpoint toggle bit.
*
* \param base
* The pointer to the USBFS instance.
*
* \param endpoint
* Data endpoint number.
* The starting number of data endpoint is 0 so pass 0 for data endpoint 1,
* 1 - for data endpoint 2 and so on up to (\ref CY_USBFS_DEV_DRV_NUM_EPS_MAX -
* 1).
* \return
* Number of bytes written by the host into the endpoint.
*
*******************************************************************************/
__STATIC_INLINE void Cy_USBFS_Dev_Drv_ClearSieEpToggle(USBFS_Type *base, uint32_t endpoint)
{
    cy_stc_usbfs_dev_drv_acces_sie_t *sieRegs = (cy_stc_usbfs_dev_drv_acces_sie_t *) base;

    /* Clear data toggle bit */
    sieRegs->EP[endpoint].SIE_EP_CNT0 &= ~USBFS_USBDEV_SIE_EP1_CNT0_DATA_TOGGLE_Msk;
}


/*******************************************************************************
* Function Name: Cy_USBFS_Dev_Drv_GetSieEpCount
****************************************************************************//**
*
* Returns the number of data bytes written into the OUT data endpoint
* by the host.
*
* \param base
* The pointer to the USBFS instance.
*
* \param endpoint
* Data endpoint number.
* The starting number of data endpoint is 0 so pass 0 for data endpoint 1,
* 1 - for data endpoint 2 and so on up to (\ref CY_USBFS_DEV_DRV_NUM_EPS_MAX -
* 1).
*
* \return
* Number of bytes written by the host into the endpoint.
*
*******************************************************************************/
__STATIC_INLINE uint32_t Cy_USBFS_Dev_Drv_GetSieEpCount(USBFS_Type *base, uint32_t endpoint)
{
    uint32_t size;
    cy_stc_usbfs_dev_drv_acces_sie_t *sieRegs = (cy_stc_usbfs_dev_drv_acces_sie_t *) base;

    /* Get number of bytes transmitted or received by SIE */
    size = _FLD2VAL(USBFS_USBDEV_SIE_EP1_CNT0_DATA_COUNT_MSB, sieRegs->EP[endpoint].SIE_EP_CNT0);
    size = (size << 8U) | CY_USBFS_DEV_READ_ODD(sieRegs->EP[endpoint].SIE_EP_CNT1);

    /* Exclude CRC size */
    return (size - CY_USBFS_DEV_DRV_EP_CRC_SIZE);
}


/*******************************************************************************
* Function Name: Cy_USBFS_Dev_Drv_SetSieEpCount
****************************************************************************//**
*
* Configures number of bytes and toggle bit to return on the host read request
* to the IN data endpoint.
*
* \param base
* The pointer to the USBFS instance.
*
* \param endpoint
* Data endpoint number.
* The starting number of data endpoint is 0 so pass 0 for data endpoint 1,
* 1 - for data endpoint 2 and so on up to (\ref CY_USBFS_DEV_DRV_NUM_EPS_MAX -
* 1).
*
* \param count
* The nubmer of bytes to return on the host read request.
*
* \param toggle
* The data toggle bit.
* The range of valid values: 0 and \ref USBFS_USBDEV_SIE_EP_DATA_TOGGLE_Msk.
*
*******************************************************************************/
__STATIC_INLINE void Cy_USBFS_Dev_Drv_SetSieEpCount(USBFS_Type *base, uint32_t endpoint,
                                                    uint32_t count, uint32_t toggle)
{
    cy_stc_usbfs_dev_drv_acces_sie_t *sieRegs = (cy_stc_usbfs_dev_drv_acces_sie_t *) base;

    sieRegs->EP[endpoint].SIE_EP_CNT1 = (uint32_t) CY_USBFS_DEV_DRV_WRITE_ODD(CY_LO8(count));
    sieRegs->EP[endpoint].SIE_EP_CNT0 = (uint32_t) CY_HI8(count) | toggle;
}


/*******************************************************************************
* Function Name: Cy_USBFS_Dev_Drv_GetSieAllEpsInterruptStatus
****************************************************************************//**
*
* Returns the SIE data endpoints interrupt request register.
* This register contains the current status of the SIE data endpoints transfer
* completion interrupt.
*
* \param base
* The pointer to the USBFS instance.
*
* \return
* The current status of the SIE interrupt sources.
* The returned status specifies for which endpoint interrupt is active as
* follows: bit 0 corresponds to data endpoint 1, bit 1 data endpoint 2 and so
* on up to \ref CY_USBFS_DEV_DRV_NUM_EPS_MAX.
*
*******************************************************************************/
__STATIC_INLINE uint32_t Cy_USBFS_Dev_Drv_GetSieAllEpsInterruptStatus(USBFS_Type const *base)
{
    return base->USBDEV.SIE_EP_INT_SR;
}


/*******************************************************************************
* Function Name: Cy_USBFS_Dev_Drv_EnableSieEpInterrupt
****************************************************************************//**
*
* Enables SIE data endpoint transfer completion interrupt.
*
* \param base
* The pointer to the USBFS instance.
*
* \param endpoint
* Data endpoint number.
* The starting number of data endpoint is 0 so pass 0 for data endpoint 1,
* 1 - for data endpoint 2 and so on up to \ref CY_USBFS_DEV_DRV_NUM_EPS_MAX.
*
*******************************************************************************/
__STATIC_INLINE void Cy_USBFS_Dev_Drv_EnableSieEpInterrupt(USBFS_Type *base, uint32_t endpoint)
{
    base->USBDEV.SIE_EP_INT_EN |= (uint32_t)(1UL << endpoint);
}


/*******************************************************************************
* Function Name: Cy_USBFS_Dev_Drv_DisableSieEpInterrupt
****************************************************************************//**
*
* Disables SIE data endpoint transfer completion interrupt.
*
* \param base
* The pointer to the USBFS instance.
*
* \param endpoint
* Data endpoint number.
* The starting number of data endpoint is 0 so pass 0 for data endpoint 1,
* 1 - for data endpoint 2 and so on up to \ref CY_USBFS_DEV_DRV_NUM_EPS_MAX.
*
*******************************************************************************/
__STATIC_INLINE void Cy_USBFS_Dev_Drv_DisableSieEpInterrupt(USBFS_Type *base, uint32_t endpoint)
{
    base->USBDEV.SIE_EP_INT_EN &= ~ (uint32_t)(1UL << endpoint);
}


/*******************************************************************************
* Function Name: Cy_USBFS_Dev_Drv_ClearSieEpInterrupt
****************************************************************************//**
*
* Clears the SIE EP interrupt sources in the interrupt request register.
*
* \param base
* The pointer to the USBFS instance.
*
* \param endpoint
* Data endpoint number.
* The starting number of data endpoint is 0 so pass 0 for data endpoint 1,
* 1 - for data endpoint 2 and so on up to (\ref CY_USBFS_DEV_DRV_NUM_EPS_MAX -
* 1).
*
*******************************************************************************/
__STATIC_INLINE void Cy_USBFS_Dev_Drv_ClearSieEpInterrupt(USBFS_Type *base, uint32_t endpoint)
{
    base->USBDEV.SIE_EP_INT_SR = (uint32_t)(1UL << endpoint);

    /* Push bufferable write to execute */
    (void)  base->USBDEV.SIE_EP_INT_SR;
}
/** \} group_usbfs_drv_drv_reg_sie_access */


/**
* \addtogroup group_usbfs_drv_drv_reg_arbiter
* \{
*/

/*******************************************************************************
* Function Name: Cy_USBFS_Dev_Drv_GetArbAllEpsInterruptStatus
****************************************************************************//**
*
* Returns the arbiter interrupt request register.
* This register contains the current status of the data endpoints arbiter
* interrupt.
*
* \param base
* The pointer to the USBFS instance.
*
* \return
* The current status of the SIE interrupt sources.
* The returned status specifies for which endpoint interrupt is active as
* follows: bit 0 corresponds to data endpoint 1, bit 1 data endpoint 2 and so
* on up to \ref CY_USBFS_DEV_DRV_NUM_EPS_MAX.
*
*******************************************************************************/
__STATIC_INLINE uint32_t Cy_USBFS_Dev_Drv_GetArbAllEpsInterruptStatus(USBFS_Type const *base)
{
    return CY_USBFS_DEV_READ_ODD(base->USBDEV.ARB_INT_SR);
}


/*******************************************************************************
* Function Name: Cy_USBFS_Dev_Drv_EnableArbEpInterrupt
****************************************************************************//**
*
* Enables the arbiter interrupt for the specified data endpoint.
*
* \param base
* The pointer to the USBFS instance.
*
* \param endpoint
* Data endpoint number.
* The starting number of data endpoint is 0 so pass 0 for data endpoint 1,
* 1 - for data endpoint 2 and so on up to \ref CY_USBFS_DEV_DRV_NUM_EPS_MAX.
*
*******************************************************************************/
__STATIC_INLINE void Cy_USBFS_Dev_Drv_EnableArbEpInterrupt(USBFS_Type *base, uint32_t endpoint)
{
    base->USBDEV.ARB_INT_EN |= (uint32_t)(1UL << endpoint);
}


/*******************************************************************************
* Function Name: Cy_USBFS_Dev_Drv_DisableArbEpInterrupt
****************************************************************************//**
*
* Disabled arbiter interrupt for the specified data endpoint.
*
* \param base
* The pointer to the USBFS instance.
*
* \param endpoint
* Data endpoint number.
* The starting number of data endpoint is 0 so pass 0 for data endpoint 1,
* 1 - for data endpoint 2 and so on up to \ref CY_USBFS_DEV_DRV_NUM_EPS_MAX.
*
*******************************************************************************/
__STATIC_INLINE void Cy_USBFS_Dev_Drv_DisableArbEpInterrupt(USBFS_Type *base, uint32_t endpoint)
{
    base->USBDEV.ARB_INT_EN &= ~(uint32_t)(1UL << endpoint);
}


/*******************************************************************************
* Function Name: Cy_USBFS_Dev_Drv_SetArbEpInterruptMask
****************************************************************************//**
*
* Enables the arbiter interrupt sources which trigger the arbiter interrupt for
* the specified data endpoint.
*
* \param base
* The pointer to the USBFS instance.
*
* \param endpoint
* Data endpoint number.
* The starting number of data endpoint is 0 so pass 0 for data endpoint 1,
* 1 - for data endpoint 2 and so on up to \ref CY_USBFS_DEV_DRV_NUM_EPS_MAX.
*
* \param mask
* The arbiter interrupt sources.
* See \ref group_usbfs_dev_drv_reg_macros_arb_ep_intr for the set of constants.
*
*******************************************************************************/
__STATIC_INLINE void Cy_USBFS_Dev_Drv_SetArbEpInterruptMask(USBFS_Type *base, uint32_t endpoint, uint32_t mask)
{
    cy_stc_usbfs_dev_drv_access_arb_t *arbRegs = (cy_stc_usbfs_dev_drv_access_arb_t *) base;

    arbRegs->EP8[endpoint].ARB_EP_INT_EN = mask;
}


/*******************************************************************************
* Function Name: Cy_USBFS_Dev_Drv_GetArbEpInterruptMask
****************************************************************************//**
*
* Returns the arbiter interrupt sources which trigger the arbiter interrupt for
* the specified data endpoint.
*
* \param base
* The pointer to the USBFS instance.
*
* \param endpoint
* Data endpoint number.
* The starting number of data endpoint is 0 so pass 0 for data endpoint 1,
* 1 - for data endpoint 2 and so on up to \ref CY_USBFS_DEV_DRV_NUM_EPS_MAX.
*
* \return
* The arbiter interrupt sources.
* See \ref group_usbfs_dev_drv_reg_macros_arb_ep_intr for the set of constants.
*
*******************************************************************************/
__STATIC_INLINE uint32_t Cy_USBFS_Dev_Drv_GetArbEpInterruptMask(USBFS_Type *base, uint32_t endpoint)
{
    cy_stc_usbfs_dev_drv_access_arb_t *arbRegs = (cy_stc_usbfs_dev_drv_access_arb_t *) base;

    return (arbRegs->EP8[endpoint].ARB_EP_INT_EN);
}


/*******************************************************************************
* Function Name: Cy_USBFS_Dev_Drv_GetArbEpInterruptStatusMasked
****************************************************************************//**
*
* Returns the current status of the enabled arbiter interrupt sources for
* the specified data endpoint.
*
* \param base
* The pointer to the USBFS instance.
*
* \param endpoint
* Data endpoint number.
* The starting number of data endpoint is 0 so pass 0 for data endpoint 1,
* 1 - for data endpoint 2 and so on up to \ref CY_USBFS_DEV_DRV_NUM_EPS_MAX.
*
* \return
* The current status of the enabled arbiter interrupt sources
* See \ref group_usbfs_dev_drv_reg_macros_arb_ep_intr for the set of constants.
*
*******************************************************************************/
__STATIC_INLINE uint32_t Cy_USBFS_Dev_Drv_GetArbEpInterruptStatusMasked(USBFS_Type *base, uint32_t endpoint)
{
    cy_stc_usbfs_dev_drv_access_arb_t *arbRegs = (cy_stc_usbfs_dev_drv_access_arb_t *) base;
    
    uint32_t mask = CY_USBFS_DEV_READ_ODD(arbRegs->EP8[endpoint].ARB_EP_INT_EN);
    return (arbRegs->EP8[endpoint].ARB_EP_SR & mask);
}


/*******************************************************************************
* Function Name: Cy_USBFS_Dev_Drv_ClearArbEpInterruptStatus
****************************************************************************//**
*
* Clears the current status of the arbiter interrupt sources for the specified
* data endpoint.
*
* \param base
* The pointer to the USBFS instance.
*
* \param endpoint
* Data endpoint number.
* The starting number of data endpoint is 0 so pass 0 for data endpoint 1,
* 1 - for data endpoint 2 and so on up to \ref CY_USBFS_DEV_DRV_NUM_EPS_MAX.
*
* \param mask
* The arbiter interrupt sources to be cleared.
* See \ref group_usbfs_dev_drv_reg_macros_arb_ep_intr for the set of constants.
*
*******************************************************************************/
__STATIC_INLINE void Cy_USBFS_Dev_Drv_ClearArbEpInterrupt(USBFS_Type *base, uint32_t endpoint, uint32_t mask)
{
    cy_stc_usbfs_dev_drv_access_arb_t *arbRegs = (cy_stc_usbfs_dev_drv_access_arb_t *) base;

    arbRegs->EP8[endpoint].ARB_EP_SR = mask;

    /* Push bufferable write to execute */
    (void) arbRegs->EP8[endpoint].ARB_EP_SR;
}


/*******************************************************************************
* Function Name: Cy_USBFS_Dev_Drv_SetArbEpConfig
****************************************************************************//**
*
* Writes configuration register for the specified data endpoint.
*
* \param base
* The pointer to the USBFS instance.
*
* \param endpoint
* Data endpoint number.
* The starting number of data endpoint is 0 so pass 0 for data endpoint 1,
* 1 - for data endpoint 2 and so on up to \ref CY_USBFS_DEV_DRV_NUM_EPS_MAX.
*
* \param cfg
* The value written into the data endpoint configuration register.
*
*******************************************************************************/
__STATIC_INLINE void Cy_USBFS_Dev_Drv_SetArbEpConfig(USBFS_Type *base, uint32_t endpoint, uint32_t cfg)
{
    cy_stc_usbfs_dev_drv_access_arb_t *arbRegs = (cy_stc_usbfs_dev_drv_access_arb_t *) base;

    arbRegs->EP8[endpoint].ARB_EP_CFG = cfg;
}


/*******************************************************************************
* Function Name: Cy_USBFS_Dev_Drv_SetArbCfgEpInReady
****************************************************************************//**
*
* Notifies hardware that IN endpoint data buffer is read to be loaded in
* the hardware buffer.
*
* \param base
* The pointer to the USBFS instance.
*
* \param endpoint
* Data endpoint number.
* The starting number of data endpoint is 0 so pass 0 for data endpoint 1,
* 1 - for data endpoint 2 and so on up to \ref CY_USBFS_DEV_DRV_NUM_EPS_MAX.
*
*******************************************************************************/
__STATIC_INLINE void Cy_USBFS_Dev_Drv_SetArbCfgEpInReady(USBFS_Type *base, uint32_t endpoint)
{
    cy_stc_usbfs_dev_drv_access_arb_t *arbRegs = (cy_stc_usbfs_dev_drv_access_arb_t *) base;

    arbRegs->EP8[endpoint].ARB_EP_CFG |= USBFS_USBDEV_ARB_EP1_CFG_IN_DATA_RDY_Msk;
}


/*******************************************************************************
* Function Name: Cy_USBFS_Dev_Drv_ClearArbCfgEpInReady
****************************************************************************//**
*
* Clears hardware notification that IN endpoint data buffer is read to be loaded
* in the hardware buffer. This function needs to be called after buffer was
* copied into the hardware buffer.
*
* \param base
* The pointer to the USBFS instance.
*
* \param endpoint
* Data endpoint number.
* The starting number of data endpoint is 0 so pass 0 for data endpoint 1,
* 1 - for data endpoint 2 and so on up to (\ref CY_USBFS_DEV_DRV_NUM_EPS_MAX -
* 1).
*
*******************************************************************************/
__STATIC_INLINE void Cy_USBFS_Dev_Drv_ClearArbCfgEpInReady(USBFS_Type *base, uint32_t endpoint)
{
    cy_stc_usbfs_dev_drv_access_arb_t *arbRegs = (cy_stc_usbfs_dev_drv_access_arb_t *) base;

    arbRegs->EP8[endpoint].ARB_EP_CFG &= ~USBFS_USBDEV_ARB_EP1_CFG_IN_DATA_RDY_Msk;
}


/*******************************************************************************
* Function Name: Cy_USBFS_Dev_Drv_TriggerArbCfgEpDmaReq
****************************************************************************//**
*
* Triggers DMA request to read from or write data into the hardware buffer.
*
* \param base
* The pointer to the USBFS instance.
*
* \param endpoint
* Data endpoint number.
* The starting number of data endpoint is 0 so pass 0 for data endpoint 1,
* 1 - for data endpoint 2 and so on up to (\ref CY_USBFS_DEV_DRV_NUM_EPS_MAX -
* 1).
*
*******************************************************************************/
__STATIC_INLINE void Cy_USBFS_Dev_Drv_TriggerArbCfgEpDmaReq(USBFS_Type *base, uint32_t endpoint)
{
    cy_stc_usbfs_dev_drv_access_arb_t *arbRegs = (cy_stc_usbfs_dev_drv_access_arb_t *) base;

    /* Generates DMA request */
    arbRegs->EP8[endpoint].ARB_EP_CFG |=  USBFS_USBDEV_ARB_EP1_CFG_DMA_REQ_Msk;

    /* Cypress ID#295549: Handle back-to-back access */
    (void) arbRegs->EP8[endpoint].ARB_EP_CFG;

    arbRegs->EP8[endpoint].ARB_EP_CFG &= ~USBFS_USBDEV_ARB_EP1_CFG_DMA_REQ_Msk;

    /* Push buffer-able write to execute */
    (void) arbRegs->EP8[endpoint].ARB_EP_CFG;
}


/*******************************************************************************
* Function Name: Cy_USBFS_Dev_Drv_SetArbWriteAddr
****************************************************************************//**
*
* Sets write address in the hardware buffer for the specified endpoint.
* This is the start address of the endpoint buffer.
*
* \param base
* The pointer to the USBFS instance.
*
* \param endpoint
* Data endpoint number.
* The starting number of data endpoint is 0 so pass 0 for data endpoint 1,
* 1 - for data endpoint 2 and so on up to (\ref CY_USBFS_DEV_DRV_NUM_EPS_MAX -
* 1).
*
* \param wa
* Write address value.
*
*******************************************************************************/
__STATIC_INLINE void Cy_USBFS_Dev_Drv_SetArbWriteAddr(USBFS_Type *base, uint32_t endpoint, uint32_t wa)
{
    cy_stc_usbfs_dev_drv_access_arb_t *arbRegs = (cy_stc_usbfs_dev_drv_access_arb_t *) base;

    arbRegs->EP16[endpoint].ARB_RW_WA16 = wa;
}

/*******************************************************************************
* Function Name: Cy_USBFS_Dev_Drv_SetArbReadAddr
****************************************************************************//**
*
* Sets read address in the hardware buffer for the specified endpoint.
* This is the start address of the endpoint buffer.
*
* \param base
* The pointer to the USBFS instance.
*
* \param endpoint
* Data endpoint number.
* The starting number of data endpoint is 0 so pass 0 for data endpoint 1,
* 1 - for data endpoint 2 and so on up to (\ref CY_USBFS_DEV_DRV_NUM_EPS_MAX -
* 1).
*
* \param ra
* Read address value.
*
*******************************************************************************/
__STATIC_INLINE void Cy_USBFS_Dev_Drv_SetArbReadAddr(USBFS_Type *base, uint32_t endpoint, uint32_t ra)
{
    cy_stc_usbfs_dev_drv_access_arb_t *arbRegs = (cy_stc_usbfs_dev_drv_access_arb_t *) base;

    arbRegs->EP16[endpoint].ARB_RW_RA16 = ra;
}


/*******************************************************************************
* Function Name: Cy_USBFS_Dev_Drv_GetArbWriteAddr
****************************************************************************//**
*
* Returns write address in the hardware buffer for the specified endpoint.
* This is the start address of the endpoint buffer.
*
* \param base
* The pointer to the USBFS instance.
*
* \param endpoint
* Data endpoint number.
* The starting number of data endpoint is 0 so pass 0 for data endpoint 1,
* 1 - for data endpoint 2 and so on up to (\ref CY_USBFS_DEV_DRV_NUM_EPS_MAX -
* 1).
*
* \return
* Write address in the hardware buffer for the specified endpoint.
*
*******************************************************************************/
__STATIC_INLINE uint32_t Cy_USBFS_Dev_Drv_GetArbWriteAddr(USBFS_Type *base, uint32_t endpoint)
{
    cy_stc_usbfs_dev_drv_access_arb_t *arbRegs = (cy_stc_usbfs_dev_drv_access_arb_t *) base;

    return (arbRegs->EP16[endpoint].ARB_RW_WA16);
}


/*******************************************************************************
* Function Name: Cy_USBFS_Dev_Drv_GetArbReadAddr
****************************************************************************//**
*
* Returns read address in the hardware buffer for the specified endpoint.
* This is the start address of the endpoint buffer.
*
* \param base
* The pointer to the USBFS instance.
*
* \param endpoint
* Data endpoint number.
* The starting number of data endpoint is 0 so pass 0 for data endpoint 1,
* 1 - for data endpoint 2 and so on up to (\ref CY_USBFS_DEV_DRV_NUM_EPS_MAX -
* 1).
*
* \return
* Read address in the hardware buffer for the specified endpoint.
*
*******************************************************************************/
__STATIC_INLINE uint32_t Cy_USBFS_Dev_Drv_GetArbReadAddr(USBFS_Type *base, uint32_t endpoint)
{
    cy_stc_usbfs_dev_drv_access_arb_t *arbRegs = (cy_stc_usbfs_dev_drv_access_arb_t *) base;

    return (arbRegs->EP16[endpoint].ARB_RW_RA16);
}
/** \} group_usbfs_drv_drv_reg_arbiter */


/**
* \addtogroup group_usbfs_drv_drv_reg_arbiter_data
* \{
*/

/*******************************************************************************
* Function Name: Cy_USBFS_Dev_Drv_WriteData
****************************************************************************//**
*
* Writes a byte (8-bit) into the hardware buffer.
*
* \param base
* The pointer to the USBFS instance.
*
* \param endpoint
* Data endpoint number.
* The starting number of data endpoint is 0 so pass 0 for data endpoint 1,
* 1 - for data endpoint 2 and so on up to (\ref CY_USBFS_DEV_DRV_NUM_EPS_MAX -
* 1).
*
* \param byte
* The byte of data to be written into the hardware buffer.
*
*******************************************************************************/
__STATIC_INLINE void Cy_USBFS_Dev_Drv_WriteData(USBFS_Type *base, uint32_t endpoint, uint8_t byte)
{
    cy_stc_usbfs_dev_drv_access_arb_t *arbRegs = (cy_stc_usbfs_dev_drv_access_arb_t *) base;

    arbRegs->EP8[endpoint].ARB_RW_DR = (uint32_t) byte;
}


/*******************************************************************************
* Function Name: Cy_USBFS_Dev_Drv_WriteData16
****************************************************************************//**
*
* Writes a half-word (16-bits) into the hardware buffer.
*
* \param base
* The pointer to the USBFS instance.
*
* \param endpoint
* Data endpoint number.
* The starting number of data endpoint is 0 so pass 0 for data endpoint 1,
* 1 - for data endpoint 2 and so on up to (\ref CY_USBFS_DEV_DRV_NUM_EPS_MAX -
* 1).
*
* \param halfword
* The half-word of data to be written into the hardware buffer.
*
*******************************************************************************/
__STATIC_INLINE void Cy_USBFS_Dev_Drv_WriteData16(USBFS_Type *base, uint32_t endpoint, uint16_t halfword)
{
    cy_stc_usbfs_dev_drv_access_arb_t *arbRegs = (cy_stc_usbfs_dev_drv_access_arb_t *) base;

    arbRegs->EP16[endpoint].ARB_RW_DR16 = (uint32_t) halfword;
}


/*******************************************************************************
* Function Name: Cy_USBFS_Dev_Drv_ReadData
****************************************************************************//**
*
* Reads a byte (8-bit) from the hardware buffer.
*
* \param base
* The pointer to the USBFS instance.
*
* \param endpoint
* Data endpoint number.
* The starting number of data endpoint is 0 so pass 0 for data endpoint 1,
* 1 - for data endpoint 2 and so on up to (\ref CY_USBFS_DEV_DRV_NUM_EPS_MAX -
* 1).
*
* \return
* The byte of data to be read from the hardware buffer.
*
*******************************************************************************/
__STATIC_INLINE uint8_t Cy_USBFS_Dev_Drv_ReadData(USBFS_Type *base, uint32_t endpoint)
{
    cy_stc_usbfs_dev_drv_access_arb_t *arbRegs = (cy_stc_usbfs_dev_drv_access_arb_t *) base;

    return ((uint8_t) arbRegs->EP8[endpoint].ARB_RW_DR);
}


/*******************************************************************************
* Function Name: Cy_USBFS_Dev_Drv_ReadData16
****************************************************************************//**
*
* Reads a half-word (16-bit) from the hardware buffer.
*
* \param base
* The pointer to the USBFS instance.
*
* \param endpoint
* Data endpoint number.
* The starting number of data endpoint is 0 so pass 0 for data endpoint 1,
* 1 - for data endpoint 2 and so on up to (\ref CY_USBFS_DEV_DRV_NUM_EPS_MAX -
* 1).
*
* \return
* The half-word of data to be read from the hardware buffer.
*
*******************************************************************************/
__STATIC_INLINE uint16_t Cy_USBFS_Dev_Drv_ReadData16(USBFS_Type *base, uint32_t endpoint)
{
    cy_stc_usbfs_dev_drv_access_arb_t *arbRegs = (cy_stc_usbfs_dev_drv_access_arb_t *) base;

    return ((uint16_t) arbRegs->EP16[endpoint].ARB_RW_DR16);
}


/*******************************************************************************
* Function Name: Cy_USBFS_Dev_Drv_GetDataRegAddr
****************************************************************************//**
*
* Returns pointer to the 8-bit data register for the specified endpoint.
*
* \param base
* The pointer to the USBFS instance.
*
* \param endpoint
* Data endpoint number.
* The starting number of data endpoint is 0 so pass 0 for data endpoint 1,
* 1 - for data endpoint 2 and so on up to (\ref CY_USBFS_DEV_DRV_NUM_EPS_MAX -
* 1).
*
* \return
* The pointer to the 8-bit data register for the specified endpoint.
*
*******************************************************************************/
__STATIC_INLINE void * Cy_USBFS_Dev_Drv_GetDataRegAddr(USBFS_Type *base, uint32_t endpoint)
{
    cy_stc_usbfs_dev_drv_access_arb_t *arbRegs = (cy_stc_usbfs_dev_drv_access_arb_t *) base;

    return ((void *) &arbRegs->EP8[endpoint].ARB_RW_DR);
}


/*******************************************************************************
* Function Name: Cy_USBFS_Dev_Drv_GetDataReg16Addr
****************************************************************************//**
*
* Returns pointer to the 16-bit data register for the specified endpoint.
*
* \param base
* The pointer to the USBFS instance.
*
* \param endpoint
* Data endpoint number.
* The starting number of data endpoint is 0 so pass 0 for data endpoint 1,
* 1 - for data endpoint 2 and so on up to (\ref CY_USBFS_DEV_DRV_NUM_EPS_MAX -
* 1).
*
* \return
* The pointer to the 16-bit data register for the specified endpoint.
*
*******************************************************************************/
__STATIC_INLINE void * Cy_USBFS_Dev_Drv_GetDataReg16Addr(USBFS_Type *base, uint32_t endpoint)
{
    cy_stc_usbfs_dev_drv_access_arb_t *arbRegs = (cy_stc_usbfs_dev_drv_access_arb_t *) base;

    return ((void *) &arbRegs->EP16[endpoint].ARB_RW_DR16);
}
/** \} group_usbfs_drv_drv_reg_arbiter_data */


/*******************************************************************************
* Function Name: Cy_USBFS_Dev_Drv_GetEp0Count
****************************************************************************//**
*
* Returns the number of data bytes written into the endpoint 0 by the host.
*
* \param base
* The pointer to the USBFS instance.
*
* \return
* Number of bytes written by the host into the endpoint.
*
*******************************************************************************/
__STATIC_INLINE uint32_t Cy_USBFS_Dev_Drv_GetEp0Count(USBFS_Type *base)
{
    /* Exclude CRC size */
    uint32_t ep0Cnt = CY_USBFS_DEV_READ_ODD(base->USBDEV.EP0_CNT);
    
    return (_FLD2VAL(USBFS_USBDEV_EP0_CNT_BYTE_COUNT, ep0Cnt) - CY_USBFS_DEV_DRV_EP_CRC_SIZE);
}


/*******************************************************************************
* Function Name: Cy_USBFS_Dev_Drv_SetEpType
****************************************************************************//**
*
* Returns the number of data bytes written into the endpoint 0 by the host.
*
* \param base
* The pointer to the USBFS instance.
*
* \return
* Number of bytes written by the host into the endpoint.
*
*******************************************************************************/
__STATIC_INLINE void Cy_USBFS_Dev_Drv_SetEpType(USBFS_Type *base, bool inDir, uint32_t endpoint)
{
    uint32_t mask     = (uint32_t) (0x1UL << endpoint);
    uint32_t regValue = CY_USBFS_DEV_READ_ODD(base->USBDEV.EP_TYPE);
    
    if (inDir)
    {
        /* IN direction: clear bit */
        regValue &= ~mask;
    }
    else
    {
        /* OUT direction: set bit */
        regValue |= mask;
    }
    
    base->USBDEV.EP_TYPE = CY_USBFS_DEV_DRV_WRITE_ODD(regValue);
}

/** \} group_usbfs_drv */

#if defined(__cplusplus)
}
#endif

#endif /* CY_IP_MXUSBFS */

#endif /* (CY_USBFS_DEV_DRV_REG_H) */


/* [] END OF FILE */
