/***************************************************************************//**
* \file cyip_ipc.h
*
* \brief
* IPC IP definitions
*
* \note
* Generator version: 1.3.0.1146
* Database revision: rev#1050929
*
********************************************************************************
* \copyright
* Copyright 2016-2018, Cypress Semiconductor Corporation. All rights reserved.
* SPDX-License-Identifier: Apache-2.0



*******************************************************************************/

#ifndef _CYIP_IPC_H_
#define _CYIP_IPC_H_

#include "cyip_headers.h"

/*******************************************************************************
*                                     IPC
*******************************************************************************/

#define IPC_STRUCT_SECTION_SIZE                 0x00000020UL
#define IPC_INTR_STRUCT_SECTION_SIZE            0x00000020UL
#define IPC_SECTION_SIZE                        0x00010000UL

/**
  * \brief IPC structure (IPC_STRUCT)
  */
typedef struct {
   __IM uint32_t ACQUIRE;                       /*!< 0x00000000 IPC acquire */
   __OM uint32_t RELEASE;                       /*!< 0x00000004 IPC release */
   __OM uint32_t NOTIFY;                        /*!< 0x00000008 IPC notification */
  __IOM uint32_t DATA;                          /*!< 0x0000000C IPC data */
   __IM uint32_t LOCK_STATUS;                   /*!< 0x00000010 IPC lock status */
   __IM uint32_t RESERVED[3];
} IPC_STRUCT_V1_Type;                           /*!< Size = 32 (0x20) */

/**
  * \brief IPC interrupt structure (IPC_INTR_STRUCT)
  */
typedef struct {
  __IOM uint32_t INTR;                          /*!< 0x00000000 Interrupt */
  __IOM uint32_t INTR_SET;                      /*!< 0x00000004 Interrupt set */
  __IOM uint32_t INTR_MASK;                     /*!< 0x00000008 Interrupt mask */
   __IM uint32_t INTR_MASKED;                   /*!< 0x0000000C Interrupt masked */
   __IM uint32_t RESERVED[4];
} IPC_INTR_STRUCT_V1_Type;                      /*!< Size = 32 (0x20) */

/**
  * \brief IPC (IPC)
  */
typedef struct {
        IPC_STRUCT_V1_Type STRUCT[16];          /*!< 0x00000000 IPC structure */
   __IM uint32_t RESERVED[896];
        IPC_INTR_STRUCT_V1_Type INTR_STRUCT[16]; /*!< 0x00001000 IPC interrupt structure */
} IPC_V1_Type;                                  /*!< Size = 4608 (0x1200) */


/* IPC_STRUCT.ACQUIRE */
#define IPC_STRUCT_ACQUIRE_P_Pos                0UL
#define IPC_STRUCT_ACQUIRE_P_Msk                0x1UL
#define IPC_STRUCT_ACQUIRE_NS_Pos               1UL
#define IPC_STRUCT_ACQUIRE_NS_Msk               0x2UL
#define IPC_STRUCT_ACQUIRE_PC_Pos               4UL
#define IPC_STRUCT_ACQUIRE_PC_Msk               0xF0UL
#define IPC_STRUCT_ACQUIRE_MS_Pos               8UL
#define IPC_STRUCT_ACQUIRE_MS_Msk               0xF00UL
#define IPC_STRUCT_ACQUIRE_SUCCESS_Pos          31UL
#define IPC_STRUCT_ACQUIRE_SUCCESS_Msk          0x80000000UL
/* IPC_STRUCT.RELEASE */
#define IPC_STRUCT_RELEASE_INTR_RELEASE_Pos     0UL
#define IPC_STRUCT_RELEASE_INTR_RELEASE_Msk     0xFFFFUL
/* IPC_STRUCT.NOTIFY */
#define IPC_STRUCT_NOTIFY_INTR_NOTIFY_Pos       0UL
#define IPC_STRUCT_NOTIFY_INTR_NOTIFY_Msk       0xFFFFUL
/* IPC_STRUCT.DATA */
#define IPC_STRUCT_DATA_DATA_Pos                0UL
#define IPC_STRUCT_DATA_DATA_Msk                0xFFFFFFFFUL
/* IPC_STRUCT.LOCK_STATUS */
#define IPC_STRUCT_LOCK_STATUS_P_Pos            0UL
#define IPC_STRUCT_LOCK_STATUS_P_Msk            0x1UL
#define IPC_STRUCT_LOCK_STATUS_NS_Pos           1UL
#define IPC_STRUCT_LOCK_STATUS_NS_Msk           0x2UL
#define IPC_STRUCT_LOCK_STATUS_PC_Pos           4UL
#define IPC_STRUCT_LOCK_STATUS_PC_Msk           0xF0UL
#define IPC_STRUCT_LOCK_STATUS_MS_Pos           8UL
#define IPC_STRUCT_LOCK_STATUS_MS_Msk           0xF00UL
#define IPC_STRUCT_LOCK_STATUS_ACQUIRED_Pos     31UL
#define IPC_STRUCT_LOCK_STATUS_ACQUIRED_Msk     0x80000000UL


/* IPC_INTR_STRUCT.INTR */
#define IPC_INTR_STRUCT_INTR_RELEASE_Pos        0UL
#define IPC_INTR_STRUCT_INTR_RELEASE_Msk        0xFFFFUL
#define IPC_INTR_STRUCT_INTR_NOTIFY_Pos         16UL
#define IPC_INTR_STRUCT_INTR_NOTIFY_Msk         0xFFFF0000UL
/* IPC_INTR_STRUCT.INTR_SET */
#define IPC_INTR_STRUCT_INTR_SET_RELEASE_Pos    0UL
#define IPC_INTR_STRUCT_INTR_SET_RELEASE_Msk    0xFFFFUL
#define IPC_INTR_STRUCT_INTR_SET_NOTIFY_Pos     16UL
#define IPC_INTR_STRUCT_INTR_SET_NOTIFY_Msk     0xFFFF0000UL
/* IPC_INTR_STRUCT.INTR_MASK */
#define IPC_INTR_STRUCT_INTR_MASK_RELEASE_Pos   0UL
#define IPC_INTR_STRUCT_INTR_MASK_RELEASE_Msk   0xFFFFUL
#define IPC_INTR_STRUCT_INTR_MASK_NOTIFY_Pos    16UL
#define IPC_INTR_STRUCT_INTR_MASK_NOTIFY_Msk    0xFFFF0000UL
/* IPC_INTR_STRUCT.INTR_MASKED */
#define IPC_INTR_STRUCT_INTR_MASKED_RELEASE_Pos 0UL
#define IPC_INTR_STRUCT_INTR_MASKED_RELEASE_Msk 0xFFFFUL
#define IPC_INTR_STRUCT_INTR_MASKED_NOTIFY_Pos  16UL
#define IPC_INTR_STRUCT_INTR_MASKED_NOTIFY_Msk  0xFFFF0000UL


#endif /* _CYIP_IPC_H_ */


/* [] END OF FILE */
