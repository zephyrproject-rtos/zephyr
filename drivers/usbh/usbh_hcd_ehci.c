/*
*********************************************************************************************************
*                                             uC/USB-Host
*                                     The Embedded USB Host Stack
*
*                    Copyright 2004-2020 Silicon Laboratories Inc. www.silabs.com
*
*                                 SPDX-License-Identifier: APACHE-2.0
*
*               This software is subject to an open source license and is distributed by
*                Silicon Laboratories Inc. pursuant to the terms of the Apache License,
*                    Version 2.0 available at www.apache.org/licenses/LICENSE-2.0.
*
*********************************************************************************************************
*/

/*
*********************************************************************************************************
*
*                                         GENERIC EHCI DRIVER
*
* Filename : usbh_ehci.c
* Version  : V3.42.00
*********************************************************************************************************
* Note(s)  : (1) Streaming for Bulk and Interrupt transfers not implemented.
*
*            (2) With an appropriate BSP, this device driver also will support the EHCI module on
*                the following MCUs:
*
*                    Freescale i.MX6
*                    Freescale i.MX25
*                    NXP       LPC185x series
*                    NXP       LPC183x series
*                    NXP       LPC182x series
*                    NXP       LPC435x series
*                    NXP       LPC433x series
*                    NXP       LPC432x series
*                    Xilinx    Zynq-7000 Soc
*********************************************************************************************************
*/

/*
*********************************************************************************************************
*                                            INCLUDE FILES
*********************************************************************************************************
*/
#include <stdint.h>
#define DT_DRV_COMPAT nxp_kinetis_usbh

#define USBH_EHCI_MODULE
#define MICRIUM_SOURCE
#include <drivers/usbh/usbh_ll.h>
#include <usbh_hub.h>
#include <soc.h>
#include <zephyr.h>
#include <zephyr/types.h>
#include <sys/byteorder.h>
#include <sys/math_extras.h>
#ifdef CONFIG_HAS_MCUX_CACHE
#include <fsl_cache.h>
#endif

#include <logging/log.h>
LOG_MODULE_REGISTER(ehci_usbh, CONFIG_USBH_LOG_LEVEL);

/*
*********************************************************************************************************
*                                            LOCAL DEFINES
*********************************************************************************************************
*/
#define EHCI_MAX_ITD                                                           \
	10u /* Max nbr of iTD per EP for HS isoc xfer               */
#define EHCI_MAX_SITD                                                          \
	10u /* Max nbr of siTD per EP for FS isoc xfer              */

#define EHCI_MAX_PERIODIC_BW 90u
#define EHCI_PORT_POWERED_ALWAYS 0u
#define EHCI_PORT_POWERED_INDIVIDUAL 1u
#define EHCI_MAX_PERIODIC_LIST_SIZE (256u * 2u)

#define EHCI_TIMESTAMP_MICROSEC 1u
#define EHCI_TIMESTAMP_MILLISEC 2u

/* ----------- EHCI QH LIST NUMBER DEFINES ------------ */
#define EHCI_QH_LIST_256MS 0u
#define EHCI_QH_LIST_128MS 256u
#define EHCI_QH_LIST_64MS 384u
#define EHCI_QH_LIST_32MS 448u
#define EHCI_QH_LIST_16MS 480u
#define EHCI_QH_LIST_08MS 496u
#define EHCI_QH_LIST_04MS 504u
#define EHCI_QH_LIST_02MS 508u
#define EHCI_QH_LIST_01MS 510u
#define EHCI_INTR_QH_LIST_SIZE (EHCI_QH_LIST_01MS + 1u)

#define EHCI_MAX_BW_PER_MICRO_FRAME 3072u

#define EHCI_BW_FLAG_CONSUME 1u
#define EHCI_BW_FLAG_PRODUCE 2u

#ifndef EHCI_CFG_ONRESET_EN /* En callback to setup auxiliary registers on reset.   */
#define EHCI_CFG_ONRESET_EN 0
#endif

#define USBH_EHCI_CFG_PERIODIC_EN 1

/*
*********************************************************************************************************
*                                        MAXIMUM PACKET SIZES
*********************************************************************************************************
*/

#define MPS_HS 1024u /* Maximum Packet Size for High Speed device            */
#define MPS_FS 1023u /* Maximum Packet Size for Full Speed device            */
#define MPS_LS 64u /* Maximum Packet Size for Low  Speed device            */

/*
*********************************************************************************************************
*                                          TYPE DEFINITIONS
*********************************************************************************************************
*/
struct ehci_qh {
	volatile uint32_t QHHorLinkPtr;
	volatile uint32_t QHEpCapChar[2];
	volatile uint32_t QHCurQTDPtr;
	volatile uint32_t QHNxtQTDPtr;
	volatile uint32_t QHAltNxtQTDPtr;
	volatile uint32_t QHToken;
	volatile uint32_t QHBufPagePtrList[5];
	/* Fields not part of qH struct defined in EHCI spec    */
	struct usbh_ep *EPPtr;
	uint32_t QTDHead;
	uint8_t SMask;
	uint8_t BWStartFrame;
	uint16_t FrameInterval;
	uint8_t Rsvd[4]; /* Padding to align the struct on a 32-byte boundary    */
};

typedef struct ehci_isoc_ep_desc EHCI_ISOC_EP_DESC;
typedef struct ehci_isoc_ep_urb EHCI_ISOC_EP_URB;
typedef struct ehci_intr_info EHCI_INTR_INFO;
typedef struct ehci_qh EHCI_QH;

struct ehci_intr_info {
	uint8_t IntrPlaceholderIx; /* Index of Intr placeholder in QHLists array.          */
	uint16_t FrameInterval;
	struct usbh_ep *EpPtr;
	EHCI_INTR_INFO *NxtIntrInfo;
};

typedef struct ehci_qtd {
	volatile uint32_t QTDNxtPtr;
	volatile uint32_t QTDAltNxtPtr;
	volatile uint32_t QTDToken;
	volatile uint32_t QTDBufPagePtrList[5];
} EHCI_QTD;

typedef struct ehci_sitd {
	volatile uint32_t SITDNxtLinkPtr;
	volatile uint32_t SITDEpCapChar[2];
	volatile uint32_t SITDStsCtrl;
	volatile uint32_t SITDBufPagePtrList[2];
	volatile uint32_t SITDBackLinkPtr;
} EHCI_SITD;

typedef struct ehci_itd {
	volatile uint32_t ITDNxtLinkPtr;
	volatile uint32_t ITDStsAndCntrl[8];
	volatile uint32_t ITDBufPagePtrList[7];
} EHCI_ITD;

struct ehci_isoc_ep_desc {
	void *TDTailPtr;
	struct usbh_ep *EPPtr;
	uint8_t SMask;
	uint8_t CMask;
	uint8_t TCnt;
	uint8_t AppStartFrame;
	uint8_t NbrFrame;
	uint16_t FrameInterval;
	EHCI_ISOC_EP_DESC *NxtEPDesc;
};

struct ehci_isoc_ep_urb {
	uint32_t iTD_Addr;
	uint8_t AppStartFrame;
	uint8_t NbrFrame;
};

typedef struct ehci_cap {
	uint8_t CapLen;
	uint16_t HCIVersion;
	uint32_t HCSParams;
	uint32_t HCCParams;
	uint8_t HCSPPortRoute[15];
} EHCI_CAP;

typedef struct ehci_cap_reg {
	volatile uint32_t CapLen_HCIVersion;
	volatile uint32_t HCSParams;
	volatile uint32_t HCCParams;
	volatile uint8_t HCSPPortRoute[15];
} EHCI_CAP_REG;

typedef struct ehci_oper_reg {
	volatile uint32_t USBCmd;
	volatile uint32_t USBSts;
	volatile uint32_t USBIntr;
	volatile uint32_t FrameIx;
	volatile uint32_t CtrlDSSeg;
	volatile uint32_t PeriodicListBase;
	volatile uint32_t AsyncListAddr;
	volatile uint32_t Rsvd[9];
	volatile uint32_t CfgFlag;
	volatile uint32_t PortSC[1];
} EHCI_OPER_REG;

typedef struct ehci_dma {
	EHCI_QTD *QTDPtr; /* DMA memory CTRL,BULK and INTR QTD                    */
	EHCI_QH *QHPtr; /* DMA memory for Queue Head (QH)                       */
	EHCI_ITD *ITDPtr;
	uint8_t *BufPtr;
} EHCI_DMA;

typedef struct ehci_dev { /* -------------------- EHCI Device ------------------- */
	EHCI_CAP HcCap; /* Pointer to Capability structure                      */
	EHCI_DMA DMA_EHCI;
	uint8_t EHCI_HubBuf[sizeof(struct usbh_hub_desc)];
	EHCI_QH *AsyncQHHead; /* Asynchronous list head                               */
	uint8_t NbrPorts; /* Number of Ports in RootHub                           */

	struct k_mem_pool
		HC_QHPool; /* Memory pool for allocating HC QHs                    */
	struct k_mem_pool
		HC_QTDPool; /* Memory pool for allocating HC QTDs                   */
	struct k_mem_pool BufPool;

	uint32_t PortResetChng; /* Port Reset Change status variable                    */
	EHCI_CAP_REG *
		HcCapReg; /* Pointer to Host Controller Capability  Registers     */
	EHCI_OPER_REG *
		HcOperReg; /* Pointer to Host Controller Operational Registers     */

#if (USBH_EHCI_CFG_PERIODIC_EN == DEF_ENABLED)
	uint32_t *PeriodicListBase;

	EHCI_QH *QHLists
		[EHCI_INTR_QH_LIST_SIZE]; /* HCD qH placeholder array for Intr ep.                */

	struct k_mem_pool
		HC_ITDPool; /* Memory pool for allocating HC iTDs                   */

	struct k_mem_pool
		IntrInfoPool; /* Memory pool for allocating Intr info struct.         */
	struct k_mem_pool
		HC_Isoc_EP_DescPool; /* Memory pool for allocating HCD Isoc EP struct        */
	struct k_mem_pool HC_Isoc_EP_URBPool;

	uint16_t MaxPeriodicBWArr
		[256]
		[8]; /* Maximum Periodic Bandwidth                           */
	EHCI_ISOC_EP_DESC *
		HeadIsocEPDesc; /* Isochronous list head pointer                        */
	EHCI_INTR_INFO *
		HeadIntrInfo; /* Intr info list head pointer.                         */
#endif

	uint32_t FNOCnt; /* Counter for Frame List Rollover                      */
	bool HC_Started; /* Indicate if EHCI HC is started.                      */
	uint8_t DrvType; /* Indicate which EHCI drv type.                        */
} EHCI_DEV;

/*
*********************************************************************************************************
*                                          GLOBAL VARIABLES
*********************************************************************************************************
*/

// #ifdef   USBH_EHCI_MODULE
// #define  USBH_EHCI_EXT
// #else
// #define  USBH_EHCI_EXT  extern
// #endif

// USBH_EHCI_EXT  USBH_HC_DRV_API  EHCI_DrvAPI;
// USBH_EHCI_EXT  USBH_HC_DRV_API  EHCI_DrvAPI_Synopsys;
// USBH_EHCI_EXT  USBH_HC_RH_API   EHCI_RH_API;

#define EHCI_HCD_GENERIC 0u
#define EHCI_HCD_SYNOPSYS 1u

K_MEM_POOL_DEFINE(HC_QHPool, sizeof(EHCI_QH), sizeof(EHCI_QH),
		  (DT_INST_PROP(0, nbr_ep_bulk) + DT_INST_PROP(0, nbr_ep_intr) +
		   1),
		  sizeof(uint32_t));
K_MEM_POOL_DEFINE(HC_QTDPool, sizeof(EHCI_QTD), sizeof(EHCI_QTD),
		  (DT_INST_PROP(0, nbr_ep_intr) * DT_INST_PROP(0, buf_len) /
			   (20 * 1024) +
		   1),
		  4);
K_MEM_POOL_DEFINE(HC_ITDPool, sizeof(EHCI_ITD), sizeof(EHCI_ITD),
		  (DT_INST_PROP(0, nbr_isoc_ep) * EHCI_MAX_SITD * 8u), 4);
K_MEM_POOL_DEFINE(HC_Isoc_EP_DescPool, sizeof(EHCI_ISOC_EP_DESC),
		  sizeof(EHCI_ISOC_EP_DESC), DT_INST_PROP(0, nbr_isoc_ep), 4);
K_MEM_POOL_DEFINE(HC_Isoc_EP_URBPool, sizeof(EHCI_ISOC_EP_URB),
		  sizeof(EHCI_ISOC_EP_URB), DT_INST_PROP(0, nbr_isoc_ep) * 2u,
		  4);
K_MEM_POOL_DEFINE(BufPool, DT_INST_PROP(0, buf_len), DT_INST_PROP(0, buf_len),
		  USBH_CFG_MAX_NBR_DEVS + DT_INST_PROP(0, nbr_ep_bulk), 4);
K_MEM_POOL_DEFINE(IntrInfoPool, sizeof(EHCI_INTR_INFO), sizeof(EHCI_INTR_INFO),
		  DT_INST_PROP(0, nbr_ep_intr), 4);

/* 1 is added to take the ceiling value                 */

/*
*********************************************************************************************************
*                                     EHCI OPERATIONAL REGISTERS
*********************************************************************************************************
*/

#define USBCMD                                                                 \
	(p_ehci->HcOperReg                                                     \
		 ->USBCmd) /* USB Command Register                                 */
#define USBSTATUS                                                              \
	(p_ehci->HcOperReg                                                     \
		 ->USBSts) /* USB Status Register                                  */
#define USBINT                                                                 \
	(p_ehci->HcOperReg                                                     \
		 ->USBIntr) /* USB Interrupt Enable Register                        */
#define FRAMEIX                                                                \
	(p_ehci->HcOperReg                                                     \
		 ->FrameIx) /* Frame Index Register                                 */
#define CTRLDSSEG                                                              \
	(p_ehci->HcOperReg                                                     \
		 ->CtrlDSSeg) /* Control Data Structure Segment Register              */
#define PERIODICLISTBASE                                                       \
	(p_ehci->HcOperReg                                                     \
		 ->PeriodicListBase) /* Periodic Frame List Base Address Register            */
#define ASYNCLISTADDR                                                          \
	(p_ehci->HcOperReg                                                     \
		 ->AsyncListAddr) /* Current Asynchronous List Address Register           */
#define CFGFLAG                                                                \
	(p_ehci->HcOperReg                                                     \
		 ->CfgFlag) /* Configure Flag Register                              */
#define PORTSC(i)                                                              \
	(p_ehci->HcOperReg->PortSC[(                                           \
		i)]) /* Port Status and Control Register                     */

/*
*********************************************************************************************************
*                                  EHCI SYNOPSYS OPERATIONAL REGISTERS
*********************************************************************************************************
*/
/* USB mode Register                                    */
#define EHCI_SYNOPSYS_USBMODE                                                  \
	(*(volatile uint32_t *)(((volatile uint8_t *)p_ehci->HcCapReg) +       \
				0x00000A8))

/*
*********************************************************************************************************
*                            BITMASKS FOR EHCI CAPABILITY REGISTER FIELDS
*********************************************************************************************************
*/
/* ---------------- HCSPARAMS Register ---------------- */
#define EHCI_HCSPARAMS_RD_DPN                                                  \
	0x00F00000u /* Debug Port Number                                    */
#define EHCI_HCSPARAMS_RD_PI                                                   \
	0x00010000u /* Port Indicators                                      */
#define EHCI_HCSPARAMS_RD_NCC                                                  \
	0x0000F000u /* Number of Companion Controllers                      */
#define EHCI_HCSPARAMS_RD_NPCC                                                 \
	0x00000F00u /* Number of Ports per Companion Controller             */
#define EHCI_HCSPARAMS_RD_PRR                                                  \
	0x00000080u /* Port Routing Rules                                   */
#define EHCI_HCSPARAMS_RD_PPC                                                  \
	0x00000010u /* Port Power Control                                   */
#define EHCI_HCSPARAMS_RD_NP                                                   \
	0x0000000Fu /* Number of Ports                                      */

/* ---------------- HCCPARAMS Register ---------------- */
#define EHCI_HCCPARAMS_RD_EECP                                                 \
	0x0000FF00u /* EHCI Extended Capabilities Pointer                   */
#define EHCI_HCCPARAMS_RD_IST                                                  \
	0x000000F0u /* Isochronous Scheduling Threshold                     */
#define EHCI_HCCPARAMS_RD_ASPC                                                 \
	0x00000004u /* Asynchronous Schedule Park Capability                */
#define EHCI_HCCPARAMS_RD_PFLF                                                 \
	0x00000002u /* Programmable Frame List Flag                         */
#define EHCI_HCCPARAMS_RD_64BAC                                                \
	0x00000001u /* 64-Bit Addressing Capability                         */

/*
*********************************************************************************************************
*                         READ BITMASKS FOR EHCI OPERATIONAL REGISTER FIELDS
*********************************************************************************************************
*/
/* ------------------ USBCMD Register ----------------- */
#define EHCI_USBCMD_RD_ITC                                                     \
	0x00FF0000u /* Interrupt Threshold Control                          */
#define EHCI_USBCMD_RD_ASPME                                                   \
	0x00000800u /* Asynchronous Schedule Park Mode Enable               */
#define EHCI_USBCMD_RD_ACPMC                                                   \
	0x00000300u /* Asynchronous Schedule Park Mode Count                */
#define EHCI_USBCMD_RD_LHCR                                                    \
	0x00000080u /* Light Host Controller Reset                          */
#define EHCI_USBCMD_RD_IOAAD                                                   \
	0x00000040u /* Interrupt On Async Advance Doorbell                  */
#define EHCI_USBCMD_RD_ASE                                                     \
	0x00000020u /* Asynchronous Schedule Enable                         */
#define EHCI_USBCMD_RD_PSE                                                     \
	0x00000010u /* Periodic Schedule Enable                             */
#define EHCI_USBCMD_RD_FLS_1024                                                \
	0x00000000u /* 1024 Frame List Size                                 */
#define EHCI_USBCMD_RD_FLS_512                                                 \
	0x00000004u /* 512  Frame List Size                                 */
#define EHCI_USBCMD_RD_FLS_256                                                 \
	0x00000008u /* 256  Frame List Size                                 */
#define EHCI_USBCMD_RD_FLS                                                     \
	0x0000000Cu /* Frame List Size                                      */
#define EHCI_USBCMD_RD_HCR                                                     \
	0x00000002u /* Host Controller Reset                                */
#define EHCI_USBCMD_RD_RS                                                      \
	0x00000001u /* Run/Stop                                             */

/* ------------------ USBSTS Register ----------------- */
#define EHCI_USBSTS_RD_ASS                                                     \
	0x00008000u /* Asynchronous Schedule Status                         */
#define EHCI_USBSTS_RD_PSS                                                     \
	0x00004000u /* Periodic Schedule Status                             */
#define EHCI_USBSTS_RD_RECL                                                    \
	0x00002000u /* Reclamation                                          */
#define EHCI_USBSTS_RD_HC_HAL                                                  \
	0x00001000u /* HC Halted                                            */
#define EHCI_USBSTS_RD_IOAA                                                    \
	0x00000020u /* Interrupt On Async Advance                           */
#define EHCI_USBSTS_RD_HSE                                                     \
	0x00000010u /* Host System Error                                    */
#define EHCI_USBSTS_RD_FLR                                                     \
	0x00000008u /* Frame List Rollover                                  */
#define EHCI_USBSTS_RD_PCD                                                     \
	0x00000004u /* Port Change Detect                                   */
#define EHCI_USBSTS_RD_USBEI                                                   \
	0x00000002u /* USB Error Interrupt                                  */
#define EHCI_USBSTS_RD_USBI                                                    \
	0x00000001u /* USB Interrupt                                        */

/* ----------------- USBINT Register -----------------  */
#define EHCI_USBINTR_RD_IOAAE                                                  \
	0x00000020u /* Interrupt On Async Advance Enable                    */
#define EHCI_USBINTR_RD_HSEE                                                   \
	0x00000010u /* Host System Error Enable                             */
#define EHCI_USBINTR_RD_FLRE                                                   \
	0x00000008u /* Frame List Rollover Enable                           */
#define EHCI_USBINTR_RD_PCIE                                                   \
	0x00000004u /* Port Change Interrupt enable                         */
#define EHCI_USBINTR_RD_USBEIE                                                 \
	0x00000002u /* USB Error Interrupt Enable                           */
#define EHCI_USBINTR_RD_USBIE                                                  \
	0x00000001u /* USB Interrupt Enable                                 */

/* ----------------- FRAMEIX Register ----------------- */
#define EHCI_FRINDEX_RD_FI                                                     \
	0x00003FFFu /* Frame Index                                          */

/* ------------- PERIODICLISTBASE Register ------------ */
#define EHCI_PERIODICLIST_RD_BA                                                \
	0xFFFFF000u /* Base Address                                         */

/* -------------- ASYNCLISTADDR Register -------------- */
#define EHCI_ASYNCLISTADDR_RD_LPL                                              \
	0xFFFFFFE0u /* Link Pointer Low                                     */

/* ---------------- CFGFLAG Register ---------------    */
#define EHCI_CONFIGFLAG_RD_CF                                                  \
	0x00000001u /* Configure Flag                                       */

/* ------------------ PORTSC Register ----------------- */
#define EHCI_PORTSC_WKOC_RD_E                                                  \
	0x00400000u /* Wake on Over Current Enable                          */
#define EHCI_PORTSC_WKDSCNNT_RD_E                                              \
	0x00200000u /* Wake on Disconnect Enable                            */
#define EHCI_PORTSC_WKCNNT_RD_E                                                \
	0x00100000u /* Wake on Connect Enable                               */
#define EHCI_PORTSC_RD_PTC                                                     \
	0x000F0000u /* Port Test Control                                    */
#define EHCI_PORTSC_RD_PIC                                                     \
	0x0000C000u /* Port Indicator Control                               */
#define EHCI_PORTSC_RD_PO                                                      \
	0x00002000u /* Port Owner                                           */
#define EHCI_PORTSC_RD_PP                                                      \
	0x00001000u /* Port Power                                           */
#define EHCI_PORTSC_RD_LS                                                      \
	0x00000C00u /* Line Status                                          */
#define EHCI_PORTSC_RD_PR                                                      \
	0x00000100u /* Port Reset                                           */
#define EHCI_PORTSC_RD_SUSP                                                    \
	0x00000080u /* Suspend                                              */
#define EHCI_PORTSC_RD_FPR                                                     \
	0x00000040u /* Force Port Resume                                    */
#define EHCI_PORTSC_RD_OCC                                                     \
	0x00000020u /* Over Current Change                                  */
#define EHCI_PORTSC_RD_OCA                                                     \
	0x00000010u /* Over Current Active                                  */
#define EHCI_PORTSC_RD_PEDC                                                    \
	0x00000008u /* Port Enable/Disable Change                           */
#define EHCI_PORTSC_RD_PED                                                     \
	0x00000004u /* Port Enabled/Disabled                                */
#define EHCI_PORTSC_RD_CSC                                                     \
	0x00000002u /* Connect Status Change                                */
#define EHCI_PORTSC_RD_CCS                                                     \
	0x00000001u /* Current Connect Status                               */

/* Port speed bit (specific to Synopsys USB 2.0 Host IP)*/
#define EHCI_SYNOPSYS_PORTSC_RD_PSPD_MASK (BIT(26) | BIT(27))
#define EHCI_SYNOPSYS_PORTSC_RD_PSPD_FS 0
#define EHCI_SYNOPSYS_PORTSC_RD_PSPD_HS BIT(27)
#define EHCI_SYNOPSYS_PORTSC_RD_PSPD_LS BIT(26)

/*
*********************************************************************************************************
*                         WRITE BITMASKS FOR EHCI OPERATIONAL REGISTER FIELDS
*********************************************************************************************************
*/
/* ------------------ USBCMD Register ----------------- */
#define EHCI_USBCMD_WR_ITC_1MF                                                 \
	0x00010000u /* Issue interrupts for every 1 Micro Frame             */
#define EHCI_USBCMD_WR_ITC_2MF                                                 \
	0x00020000u /* Issue interrupts for every 2 Micro Frames            */
#define EHCI_USBCMD_WR_ITC_4MF                                                 \
	0x00040000u /* Issue interrupts for every 4 Micro Frames            */
#define EHCI_USBCMD_WR_ITC_8MF                                                 \
	0x00080000u /* Issue interrupts for every 8 Micro Frames            */
#define EHCI_USBCMD_WR_ITC_16MF                                                \
	0x00100000u /* Issue interrupts for every 16 Micro Frames           */
#define EHCI_USBCMD_WR_ITC_32MF                                                \
	0x00200000u /* Issue interrupts for every 32 Micro Frames           */
#define EHCI_USBCMD_WR_ITC_64MF                                                \
	0x00400000u /* Issue interrupts for every 64 Micro Frames           */
#define EHCI_USBCMD_WR_ASPME                                                   \
	0x00000800u /* Asynchronous Park Mode Enable                        */
#define EHCI_USBCMD_WR_LHCR                                                    \
	0x00000080u /* Light Host Controller Reset                          */
#define EHCI_USBCMD_WR_IOAAD                                                   \
	0x00000040u /* Interrupt On Async Advance Doorbell                  */
#define EHCI_USBCMD_WR_ASE                                                     \
	0x00000020u /* Asynchronous Schedule Enable                         */
#define EHCI_USBCMD_WR_PSE                                                     \
	0x00000010u /* Periodic Schedule Enable                             */
#define EHCI_USBCMD_WR_FLS_1024                                                \
	0x00000000u /* Frame List Size 1024 elements                        */
#define EHCI_USBCMD_WR_FLS_512                                                 \
	0x00000004u /* Frame List Size 512 elements                         */
#define EHCI_USBCMD_WR_FLS_256                                                 \
	0x00000008u /* Frame List Size 256 elements                         */
#define EHCI_USBCMD_WR_HCR                                                     \
	0x00000002u /* Host Controller Reset                                */
#define EHCI_USBCMD_WR_RS                                                      \
	0x00000001u /* Run/Stop                                             */

/* ------------------ USBSTS Register ----------------- */
#define EHCI_USBSTS_WR_ASS                                                     \
	0x00008000u /* Asynchronous Schedule Status                         */
#define EHCI_USBSTS_WR_PSS                                                     \
	0x00004000u /* Periodic Schedule Status                             */
#define EHCI_USBSTS_WR_RECL                                                    \
	0x00002000u /* Reclamation                                          */
#define EHCI_USBSTS_WR_HC_HAL                                                  \
	0x00001000u /* HC Halted                                            */
#define EHCI_USBSTS_WR_IOAA                                                    \
	0x00000020u /* Interrupt On Async Advance                           */
#define EHCI_USBSTS_WR_HSE                                                     \
	0x00000010u /* Host System Error                                    */
#define EHCI_USBSTS_WR_FLR                                                     \
	0x00000008u /* Frame List Roll over                                 */
#define EHCI_USBSTS_WR_PCD                                                     \
	0x00000004u /* Port Change Detect                                   */
#define EHCI_USBSTS_WR_USBEI                                                   \
	0x00000002u /* USB Error Interrupt                                  */
#define EHCI_USBSTS_WR_USBI                                                    \
	0x00000001u /* USB Interrupt                                        */

/* ----------------- USBINT Register ----------------- */
#define EHCI_USBINTR_WR_IOAAE                                                  \
	0x00000020u /* Interrupt On Async Advance Enable                    */
#define EHCI_USBINTR_WR_HSEE                                                   \
	0x00000010u /* Host System Error Enable                             */
#define EHCI_USBINTR_WR_FLRE                                                   \
	0x00000008u /* Frame List Rollover Enable                           */
#define EHCI_USBINTR_WR_PCIE                                                   \
	0x00000004u /* Port Change Interrupt enable                         */
#define EHCI_USBINTR_WR_USBEIE                                                 \
	0x00000002u /* USB Error Interrupt Enable                           */
#define EHCI_USBINTR_WR_USBIE                                                  \
	0x00000001u /* USB Interrupt Enable                                 */

/* ----------------- FRAMEIX Register ----------------- */
#define EHCI_FRINDEX_WR_FI_1024                                                \
	0x00000000u /* Frame Index                                          */
#define EHCI_FRINDEX_WR_FI_512                                                 \
	0x00001000u /* Frame Index                                          */
#define EHCI_FRINDEX_WR_FI_256                                                 \
	0x00002000u /* Frame Index                                          */

/* ---------------- CFGFLAG Register --------------- */
#define EHCI_CONFIGFLAG_WR_CF                                                  \
	0x00000001u /* Configure Flag                                       */

/* ------------------ PORTSC Register ----------------- */
#define EHCI_PORTSC_WR_WKOC_E                                                  \
	0x00400000u /* Wakeon Over Current Enable                           */
#define EHCI_PORTSC_WR_WKDSCNNT_E                                              \
	0x00200000u /* Wakeon Disconnect Enable                             */
#define EHCI_PORTSC_WR_WKCNNT_E                                                \
	0x00100000u /* Wakeon Connect Enable                                */
#define EHCI_PORTSC_WR_PTC_DIS                                                 \
	0x00000000u /* Port Test Control                                    */
#define EHCI_PORTSC_WR_PTC_J                                                   \
	0x00010000u /* Port Test Control                                    */
#define EHCI_PORTSC_WR_PTC_K                                                   \
	0x00020000u /* Port Test Control                                    */
#define EHCI_PORTSC_WR_PTC_SE0_NAK                                             \
	0x00030000u /* Port Test Control                                    */
#define EHCI_PORTSC_WR_PTC_P                                                   \
	0x00040000u /* Port Test Control                                    */
#define EHCI_PORTSC_WR_PTC_FE                                                  \
	0x00050000u /* Port Test Control                                    */
#define EHCI_PORTSC_WR_PIC_OFF                                                 \
	0x00000000u /* Port Indicator Control                               */
#define EHCI_PORTSC_WR_PIC_AMB                                                 \
	0x00004000u /* Port Indicator Control                               */
#define EHCI_PORTSC_WR_PIC_GRE                                                 \
	0x00008000u /* Port Indicator Control                               */
#define EHCI_PORTSC_WR_PO                                                      \
	0x00002000u /* Port Owner                                           */
#define EHCI_PORTSC_WR_PP_OFF                                                  \
	0x00000000u /* Port Power                                           */
#define EHCI_PORTSC_WR_PP_ON                                                   \
	0x00001000u /* Port Power                                           */
#define EHCI_PORTSC_WR_PR                                                      \
	0x00000100u /* Port Reset                                           */
#define EHCI_PORTSC_WR_SUSP                                                    \
	0x00000080u /* Suspend                                              */
#define EHCI_PORTSC_WR_FPR                                                     \
	0x00000040u /* Force Port Resume                                    */
#define EHCI_PORTSC_WR_OCC                                                     \
	0x00000020u /* Over Current Change                                  */
#define EHCI_PORTSC_WR_OCA                                                     \
	0x00000010u /* Over Current Active                                  */
#define EHCI_PORTSC_WR_PEDC                                                    \
	0x00000008u /* Port Enable/Disable Change                           */
#define EHCI_PORTSC_WR_PED                                                     \
	0x00000004u /* Port Enabled/Disabled                                */
#define EHCI_PORTSC_WR_CSC                                                     \
	0x00000002u /* Connect Status Change                                */

/* ------------------ USBMODE Register ---------------- */
#define EHCI_SYNOPSYS_USBMODE_WR_CM_HOST (BIT(0) | BIT(1))

/*
*********************************************************************************************************
*                                       OFFSETS FOR BIT FIELDS
*********************************************************************************************************
*/

#define O_ITD_T 0u /* Terminate                                            */
#define O_ITD_TYP 1u /* QH/iTD/siTD/FSTN Select                              */
#define O_ITD_LP 5u /* Link Pointer                                         */
#define O_ITD_OFFSET                                                           \
	0u /* Transaction Offset                                   */
#define O_ITD_PG 12u /* Page Select                                          */
#define O_ITD_IOC 15u /* Interrupt On Complete                                */
#define O_ITD_LENGTH                                                           \
	16u /* Transaction Length                                   */
#define O_ITD_STS 28u /* Status                                               */
#define O_ITD_STS_ACTIVE 0x8u
#define O_ITD_STS_DBE 0x4u
#define O_ITD_STS_BD 0x2u
#define O_ITD_STS_XACTERR 0x1u
#define O_ITD_DEVADD                                                           \
	0u /* Device Address                                       */
#define O_ITD_ENDPT                                                            \
	8u /* Endpoint Number                                      */
#define O_ITD_BUFPTR                                                           \
	12u /* Buffer Pointer                                       */
#define O_ITD_MPS 0u /* Maximum Packet Size                                  */
#define O_ITD_DIR 11u /* Direction                                            */
#define O_ITD_MULTI                                                            \
	0u /* Multi                                                */
#define O_SITD_NLP 5u
#define O_SITD_T 0u /* Terminate                                            */
#define O_SITD_TYP 2u /* QH/iTD/siTD/FSTN Select                              */
#define O_SITD_LP 5u /* Link Pointer                                         */
#define O_SITD_DEVADD                                                          \
	0u /* Device Address                                       */
#define O_SITD_ENDPT                                                           \
	8u /* Endpoint Number                                      */
#define O_SITD_HUBADD                                                          \
	16u /* Hub Address                                          */
#define O_SITD_PN 24u /* Port Number                                          */
#define O_SITD_DIR                                                             \
	31u /* Direction                                            */
#define O_SITD_SMASK                                                           \
	0u /* Split Complete Mask                                  */
#define O_SITD_CMASK                                                           \
	8u /* Split Start Mask                                     */
#define O_SITD_STS 0u /* Status of the transaction executed by the HC         */
#define O_SITD_TP 3u /* Transaction position                                 */
#define O_SITD_TCOUNT                                                          \
	0u /* Transaction count                                    */
#define O_SITD_STS_ACTIVE                                                      \
	0x80u /* Active                                               */
#define O_SITD_STS_ERR                                                         \
	0x40u /* Transaction translator error                         */
#define O_SITD_STS_DBE                                                         \
	0x20u /* Data buffer error                                    */
#define O_SITD_STS_BD                                                          \
	0x10u /* Babble detected                                      */
#define O_SITD_STS_XACT_ERR                                                    \
	0x08u /* Transaction error                                    */
#define O_SITD_STS_MMF                                                         \
	0x04u /* Missed micro frame                                   */
#define O_SITD_STS_STS                                                         \
	0x02u /* Split transaction state                              */
#define O_SITD_CSPMASK                                                         \
	8u /* Complete Split Progress Mask                         */
#define O_SITD_TBTT                                                            \
	16u /* Total Bytes To Transfer                              */
#define O_SITD_PS 30u /* Page Select                                          */
#define O_SITD_IOC                                                             \
	31u /* Interrupt On Complete                                */
#define O_SITD_CO 0u /* Current Offset                                       */
#define O_SITD_BPL                                                             \
	12u /* Buffer Pointer List                                  */
#define O_SITD_TP 3u /* Transaction Position                                 */
#define O_SITD_TC 0u /* Transaction Count                                    */
#define O_SITD_T 0u /* Terminate                                            */
#define O_SITD_BP 5u /* Back Pointer                                         */
#define O_QTD_T 0u /* Terminate                                            */
#define O_QTD_NTEP 5u /* Next Transfer Element Pointer                        */
#define O_QTD_ANTEP                                                            \
	5u /* Alternate Next Transfer Element Pointer              */
#define O_QTD_STS 0u /* Status                                               */
#define O_QTD_PID 8u /* PID Code                                             */
#define O_QTD_CERR                                                             \
	10u /* Error Counter                                        */
#define O_QTD_CP 12u /* Current Page                                         */
#define O_QTD_IOC 15u /* Interrupt On Complete                                */
#define O_QTD_TBTT                                                             \
	16u /* Total Bytes To Transfer                              */
#define O_QTD_DT 31u /* Data Toggle                                          */
#define O_QTD_SFD 0u /* Status Field Description                             */
#define O_QTD_CO 0u /* Current Offset                                       */
#define O_QTD_BPL 12u /* Buffer Pointer List                                  */
#define O_QH_T 0u /* Terminate                                            */
#define O_QH_TYP 1u /* QH/iTD/siTD/FSTN Select                              */
#define O_QH_QHHLP 5u /* Queue Head Horizontal Link Pointer                   */
#define O_QH_DEVADD                                                            \
	0u /* Device Address                                       */
#define O_QH_I 7u /* Inactive on Next Transaction                         */
#define O_QH_ENDPT 8u /* Endpoint Number                                      */
#define O_QH_EPS 12u /* Endpoint Speed                                       */
#define O_QH_DTC 14u /* Data Toggle Control                                  */
#define O_QH_H 15u /* Head of Reclamation List Flag                        */
#define O_QH_MPL 16u /* Maximum Packet Length                                */
#define O_QH_C 27u /* Control Endpoint Flag                                */
#define O_QH_RL 28u /* Next Count Reload                                    */
#define O_QH_SMASK 0u /* Interrupt Schedule Mask                              */
#define O_QH_CMASK 8u /* Split Completion Mask                                */
#define O_QH_HUBADD                                                            \
	16u /* Hub Address                                          */
#define O_QH_PN 23u /* Port Number                                          */
#define O_QH_HBPM 30u /* High Bandwidth Pipe Multifier                        */
#define O_QH_CETDLP                                                            \
	5u /* Current Element Transaction Descriptor Link Pointer  */
#define O_QH_NAKCNT                                                            \
	1u /* Nak Counter                                          */
#define O_QH_DT 31u /* Data Toggle                                          */
#define O_QH_IOC 15u /* Interrupt On Complete                                */
#define O_QH_EC 10u /* Error Counter                                        */
#define O_QH_PS 0u /* Ping State                                           */
#define O_QH_STCSP 0u /* Split Transaction Complete Split Progress            */
#define O_QH_STFT 0u /* Split Transaction Frame Tag                          */
#define O_QH_SBYTES                                                            \
	5u /* S-Bytes                                              */
#define O_QH_STS_ACTIVE                                                        \
	0x80u /* Active                                               */
#define O_QH_STS_HALTED                                                        \
	0x40u /* Halted                                               */
#define O_QH_STS_DBE                                                           \
	0x20u /* Data Buffer Error                                    */
#define O_QH_STS_BD                                                            \
	0x10u /* Babble Detected                                      */
#define O_QH_STS_XACT_ERR                                                      \
	0x08u /* Transaction Error                                    */
#define O_QH_STS_MMF                                                           \
	0x04u /* Missed Micro Frame                                   */
#define O_QH_STS_STS                                                           \
	0x02u /* Split Transaction State                              */
#define O_QH_STS_PE                                                            \
	0x01u /* Ping State                                           */
#define O_FSTN_T 0u /* Terminate                                            */
#define O_FSTN_TYP 1u /* QH/iTD/siTD/FSTN Select                              */
#define O_FSTN_NPLP                                                            \
	5u /* Normal Path Link Pointer                             */
#define O_FSTN_BPLP                                                            \
	5u /* Back Path Link Pointer                               */
#define S_MASK_1MICROFRM                                                       \
	0xFFu /* S mask for 1 Micro Frame interval                    */
#define S_MASK_2MICROFRM                                                       \
	0x55u /* S mask for 2 Micro Frame interval                    */
#define S_MASK_4MICROFRM                                                       \
	0x11u /* S mask for 3 Micro Frame interval                    */
#define S_MASK_8MICROFRM                                                       \
	0x01u /* S mask for 8 or > 8 Micro Frame interval             */

#define S_MASK_SPLIT_0_MICROFRM 0x01u
#define S_MASK_SPLIT_01_MICROFRM 0x03u
#define S_MASK_SPLIT_012_MICROFRM 0x07u
#define S_MASK_SPLIT_0123_MICROFRM 0x0Fu
#define S_MASK_SPLIT_01234_MICROFRM 0x1Fu
#define S_MASK_SPLIT_012345_MICROFRM 0x3Fu

#define C_MASK_SPLIT_0_MICROFRM 0xFEu
#define C_MASK_SPLIT_01_MICROFRM 0xF8u
#define C_MASK_SPLIT_012_MICROFRM 0xF0u
#define C_MASK_SPLIT_0123_MICROFRM 0xE0u
#define C_MASK_SPLIT_01234_MICROFRM 0xC0u
#define C_MASK_SPLIT_012345_MICROFRM 0x80u

/*
*********************************************************************************************************
*                                  DATA STRUCTURE FIELD DEFINITIONS
*********************************************************************************************************
*/
/* ------------------- Common Fields ------------------ */
#define DWORD1_T BIT(0)
#define DWORD1_T_VALID                                                         \
	0u /* T-bit Field in DWORD1 = 0 (Valid)                    */
#define DWORD1_T_INVALID                                                       \
	1u /* T-bit Field in DWORD1 = 1 (Invalid)                  */
#define DWORD1_TYP_ITD                                                         \
	0u /* Type Field in DWORD1 = 0 (iTD)                       */
#define DWORD1_TYP_QH                                                          \
	1u /* Type Field in DWORD1 = 1 (QH)                        */
#define DWORD1_TYP_SITD                                                        \
	2u /* Type Field in DWORD1 = 2 (siTD)                      */
#define DWORD1_TYP_FSTN                                                        \
	3u /* Type Field in DWORD1 = 3 (FSTN)                      */

/* ----------------- QueueHead Fields ----------------- */
#define DWORD2_QH_AS_I                                                         \
	1u /* I-Field in DWORD2 = 1, HC set 'Active' bit to 0. Used for Split transaction. */
#define DWORD2_QH_EPS_FS                                                       \
	0u /* EPS Field in DWORD2 = 0 for Full-Speed Device        */
#define DWORD2_QH_EPS_LS                                                       \
	1u /* EPS Field in DWORD2 = 1 for Low-Speed Device         */
#define DWORD2_QH_EPS_HS                                                       \
	2u /* EPS Field in DWORD2 = 2 for High-Speed Device        */
#define DWORD2_QH_DTC_QH                                                       \
	0u /* Preserve DT bit in QH.Ignore DT bit from qTD         */
#define DWORD2_QH_DTC_QTD                                                      \
	1u /* DT bit comes from qTD                                */
#define DWORD2_QH_R_H                                                          \
	1u /* QH is Head of Reclamation List                       */
#define DWORD2_QH_C                                                            \
	1u /* Split Transaction Control Endpoint Flag              */
#define DWORD3_QH_PS_CSPLIT_UFRAME_2345                                        \
	0x3Cu /* Split Transaction: CSPLIT sent in uFrame 2, 3, 4 ... */
/* ...or 5 from the Host-Frame (1ms)                    */
#define DWORD3_QH_PS_SSPLIT_UFRAME_0                                           \
	0x01u /* Split Transaction: SSPLIT sent in uFrame 0 from ...  */
/* ...the Host-Frame (1ms)                              */
#define DWORD3_QH_AS_SMASK                                                     \
	0u /* Interrupt Schedule Mask = 0: Asynchronous Schedule   */
#define DWORD3_QH_HBPM_1                                                       \
	1u /* One Transaction should be issued to the End-Point    */
#define DWORD3_QH_HBPM_2                                                       \
	2u /* Two Transactions should be issued to the End-Point   */
#define DWORD3_QH_HBPM_3                                                       \
	3u /* Three Transactions should be issued to the End-Point */

/* --- Queue Element Transfer Descriptor(qTD) Fields -- */
#define DWORD3_QTD_PIDC_OUT                                                    \
	0u /* PID Code Field in DWORD3 = 0 (OUT Packet)            */
#define DWORD3_QTD_PIDC_IN                                                     \
	1u /* PID Code Field in DWORD3 = 1 (IN Packet)             */
#define DWORD3_QTD_PIDC_SETUP                                                  \
	2u /* PID Code Field in DWORD3 = 2 (SETUP Packet)          */

/* ------- Isoc Transfer Descriptor (iTD) Fields ------ */
#define DWORDx_ITD_IOC BIT(15)
#define DWORDx_ITD_STATUS_ACTIVE BIT(31)

/* --- Split Transaction Isoc Transfer Descriptor (siTD) Fields -- */
#define DWORD3_SITD_STATUS_ACTIVE BIT(7)
#define DWORD3_SITD_IOC BIT(31)
#define DWORD1_SITD_IO_OUT 0u
#define DWORD1_SITD_IO_IN 1u
#define DWORD6_SITD_TP_ALL 0u
#define DWORD6_SITD_TP_BEGIN 1u
#define DWORD6_SITD_TP_MID 2u
#define DWORD6_SITD_TP_END 3u

#define DWORD1_ITD_IO_OUT 0u
#define DWORD1_ITD_IO_IN 1u

/*
*********************************************************************************************************
*                                     DATA STRUCTURE FIELD SHIFTS
*********************************************************************************************************
*/
/* Common Fields                                        */
#define HOR_LNK_PTR_PTR(x) ((x) << O_QH_QHHLP)
#define HOR_LNK_PTR_TYP(x) ((x) << O_QH_TYP)
#define HOR_LNK_PTR_T(x) ((x) << O_QH_T)

/* QueueHead Fields                                     */
#define QH_EPCHAR_DEVADD(x) ((x) << O_QH_DEVADD)
#define QH_EPCHAR_I(x) ((x) << O_QH_I)
#define QH_EPCHAR_ENDPT(x) ((x) << O_QH_ENDPT)
#define QH_EPCHAR_EPS(x) ((x) << O_QH_EPS)
#define QH_EPCHAR_DTC(x) ((x) << O_QH_DTC)
#define QH_EPCHAR_H(x) ((x) << O_QH_H)
#define QH_EPCHAR_MPL(x) ((x) << O_QH_MPL)
#define QH_EPCHAR_C(x) ((x) << O_QH_C)
#define QH_EPCHAR_RL(x) ((x) << O_QH_RL)
#define QH_EPCAP_SMASK(x) ((x) << O_QH_SMASK)
#define QH_EPCAP_CMASK(x) ((x) << O_QH_CMASK)
#define QH_EPCAP_HUBADD(x) ((x) << O_QH_HUBADD)
#define QH_EPCAP_PN(x) ((x) << O_QH_PN)
#define QH_EPCAP_HBPM(x) ((x) << O_QH_HBPM)
#define QH_CETDLP(x) ((x) << O_QH_CETDLP)
#define QH_OVERLAY_NAKCNT(x) ((x) << O_QH_NAKCNT)
#define QH_OVERLAY_PS(x) ((x) << O_QH_PS)
#define QH_OVERLAY_EC(x) ((x) << O_QH_EC)
#define QH_OVERLAY_IOC(x) ((x) << O_QH_IOC)
#define QH_OVERLAY_DT(x) ((x) << O_QH_DT)
#define QH_OVERLAY_STCSP(x) ((x) << O_QH_STCSP)
#define QH_OVERLAY_SBYTES(x) ((x) << O_QH_SBYTES)
#define QH_OVERLAY_STFT(x) ((x) << O_QH_STFT)

/* Queue Element Transfer Descriptor Fields             */
#define QTD_N_QTD_PTR_NTEP(x) ((x) << O_QTD_NTEP)
#define QTD_N_QTD_PTR_T(x) ((x) << O_QTD_T)
#define QTD_ALT_PTR_ANTEP(x) ((x) << O_QTD_ANTEP)
#define QTD_ALT_QTD_PTR_T(x) ((x) << O_QTD_T)
#define QTD_TOKEN_STS(x) ((x) << O_QTD_STS)
#define QTD_TOKEN_PID(x) ((x) << O_QTD_PID)
#define QTD_TOKEN_CERR(x) ((x) << O_QTD_CERR)
#define QTD_TOKEN_CP(x) ((x) << O_QTD_CP)
#define QTD_TOKEN_IOC(x) ((x) << O_QTD_IOC)
#define QTD_TOKEN_TBTT(x) ((x) << O_QTD_TBTT)
#define QTD_TOKEN_DT(x) ((x) << O_QTD_DT)
#define QTD_BPPL_CO(x) ((x) << O_QTD_CO)
#define QTD_BPPL_BPL(x) ((x) << O_QTD_BPL)

#define SITD_DWORD0_NXT_LINK_PTR(x) ((x) << O_SITD_NLP)
#define SITD_DWORD0_TYP(x) ((x) << O_SITD_TYP)
#define SITD_DWORD0_T(x) ((x) << O_SITD_T)

#define SITD_EPCHAR_DEVADD(x) ((x) << O_SITD_DEVADD)
#define SITD_EPCHAR_ENDPT(x) ((x) << O_SITD_ENDPT)
#define SITD_EPCHAR_HUBADD(x) ((x) << O_SITD_HUBADD)
#define SITD_EPCHAR_PN(x) ((x) << O_SITD_PN)
#define SITD_EPCHAR_DIR(x) ((x) << O_SITD_DIR)
#define SITD_EPCHAR_SMASK(x) ((x) << O_SITD_SMASK)
#define SITD_STSCTRL_IOC(x) ((x) << O_SITD_IOC)
#define SITD_STSCTRL_STS(x) ((x) << O_SITD_STS)
#define SITD_BUGPAGE1_TP(x) ((x) << O_SITD_TP)
#define SITD_BUGPAGE1_TCOUNT(x) ((x) << O_SITD_TCOUNT)

#define ITD_DWORD0_TYP(x) ((x) << O_ITD_TYP)
#define ITD_DWORD0_T(x) ((x) << O_ITD_T)
#define ITD_BUF_PG_PTR_LIST_DEVADD(x) ((x) << O_ITD_DEVADD)
#define ITD_BUF_PG_PTR_LIST_ENDPT(x) ((x) << O_ITD_ENDPT)
#define ITD_BUF_PG_PTR_LIST_MPS(x) ((x) << O_ITD_MPS)
#define ITD_BUF_PG_PTR_LIST_IO(x) ((x) << O_ITD_DIR)
#define ITD_BUF_PG_PTR_LIST_MULT(x) ((x) << O_ITD_MULTI)
#define ITD_BUF_PG_PTR_LIST_BUF_PTR(x) ((x) << O_ITD_BUFPTR)
#define ITD_STSCTRL_STS(x) ((x) << O_ITD_STS)
#define ITD_STSCTRL_XACT_LEN(x) ((x) << O_ITD_LENGTH)
#define ITD_STSCTRL_PG(x) ((x) << O_ITD_PG)
#define ITD_STSCTRL_XACT_OFFSET(x) ((x) << O_ITD_OFFSET)
#define ITD_STSCTRL_IOC(x) ((x) << O_ITD_IOC)

/* ----------------- ALIGNMENT MACROS ----------------- */
#define DEF_ALIGN(x, a)                                                        \
	((uint32_t)(x) % (a) ? (a) - ((uint32_t)(x) % (a)) + (uint32_t)(x) :   \
				     (uint32_t)(x))

#define USB_ALIGNED(x, a) (void *)DEF_ALIGN(((void *)(x)), (a))

#if (USBH_EHCI_CFG_PERIODIC_EN == DEF_ENABLED)
static uint32_t EHCI_BranchArray[256];
#endif

#if (CPU_CFG_CACHE_MGMT_EN == DEF_ENABLED)
extern uint32_t CPU_Cache_Linesize;
#endif

/*
**********************************************************************************************************
*                                      LOCAL FUNCTION PROTOTYPES
**********************************************************************************************************
*/

/* --------------- DRIVER API FUNCTIONS --------------- */
static void EHCI_SynopsysInit(struct usbh_hc_drv *p_hc_drv, int *p_err);

static void EHCI_Init(struct usbh_hc_drv *p_hc_drv, int *p_err);

static void EHCI_InitHandler(struct usbh_hc_drv *p_hc_drv,
			     uint8_t ehci_drv_type, int *p_err);

static void EHCI_Start(struct usbh_hc_drv *p_hc_drv, int *p_err);

static void EHCI_Stop(struct usbh_hc_drv *p_hc_drv, int *p_err);

static enum usbh_device_speed EHCI_SpdGet(struct usbh_hc_drv *p_hc_drv,
					  int *p_err);

static void EHCI_Suspend(struct usbh_hc_drv *p_hc_drv, int *p_err);

static void EHCI_Resume(struct usbh_hc_drv *p_hc_drv, int *p_err);

static uint32_t EHCI_FrameNbrGet(struct usbh_hc_drv *p_hc_drv, int *p_err);

static void EHCI_EP_Open(struct usbh_hc_drv *p_hc_drv, struct usbh_ep *p_ep,
			 int *p_err);

static void EHCI_EP_Close(struct usbh_hc_drv *p_hc_drv, struct usbh_ep *p_ep,
			  int *p_err);

static void EHCI_EP_Abort(struct usbh_hc_drv *p_hc_drv, struct usbh_ep *p_ep,
			  int *p_err);

static bool EHCI_IsHalt_EP(struct usbh_hc_drv *p_hc_drv, struct usbh_ep *p_ep,
			   int *p_err);

static void EHCI_URB_Submit(struct usbh_hc_drv *p_hc_drv,
			    struct usbh_urb *p_urb, int *p_err);

static void EHCI_URB_Complete(struct usbh_hc_drv *p_hc_drv,
			      struct usbh_urb *p_urb, int *p_err);

static void EHCI_URB_Abort(struct usbh_hc_drv *p_hc_drv, struct usbh_urb *p_urb,
			   int *p_err);

/* ---------------- INTERNAL FUNCTIONS ---------------- */
static void EHCI_ISR(void *p_data);

#if (EHCI_CFG_ONRESET_EN == DEF_ENABLED)
static void EHCI_OnReset(struct usbh_dev *p_dev);
#endif

#if (USBH_EHCI_CFG_PERIODIC_EN == DEF_ENABLED)
static void EHCI_PeriodicOrderPrepare(uint32_t idx, uint32_t power,
				      uint32_t list_size);
#endif

static int EHCI_DMA_Init(struct usbh_hc_drv *p_hc_drv);

static void EHCI_CapRegRead(EHCI_DEV *p_ehci, EHCI_CAP *p_cap);

static void EHCI_QTD_Clr(EHCI_QTD *p_qtd);

static void EHCI_QH_Clr(EHCI_QH *p_qh);

#if (USBH_EHCI_CFG_PERIODIC_EN == DEF_ENABLED)
static void EHCI_EP_DescClr(EHCI_ISOC_EP_DESC *p_ep_desc);

static void EHCI_SITD_Clr(EHCI_SITD *p_sitd);
#endif

static EHCI_QTD *EHCI_QTDListPrepare(struct usbh_hc_drv *p_hc_drv,
				     struct usbh_ep *p_ep,
				     struct usbh_urb *p_urb, uint8_t *p_buf,
				     uint32_t buf_len, int *p_err);

static uint32_t EHCI_QTDRemove(struct usbh_hc_drv *p_hc_drv, EHCI_QH *p_qh);

#if (USBH_EHCI_CFG_PERIODIC_EN == DEF_ENABLED)
static int EHCI_PeriodicListInit(struct usbh_hc_drv *p_hc_drv);
#endif

static int EHCI_AsyncListInit(struct usbh_hc_drv *p_hc_drv);

static int EHCI_AsyncEP_Open(struct usbh_hc_drv *p_hc_drv, struct usbh_ep *p_ep,
			     struct usbh_dev *p_dev);

#if (USBH_EHCI_CFG_PERIODIC_EN == DEF_ENABLED)
static int EHCI_IntrEP_Open(struct usbh_hc_drv *p_hc_drv, struct usbh_ep *p_ep,
			    struct usbh_dev *p_dev);

static int EHCI_IsocEP_Open(struct usbh_hc_drv *p_hc_drv, struct usbh_ep *p_ep);

static void EHCI_BW_Update(struct usbh_hc_drv *p_hc_drv, struct usbh_ep *p_ep,
			   void *p_data, bool bw_use);

static int EHCI_BW_Get(struct usbh_hc_drv *p_hc_drv, struct usbh_ep *p_ep,
		       void *p_data);

static int EHCI_SITDListPrepare(struct usbh_hc_drv *p_hc_drv,
				struct usbh_dev *p_dev, struct usbh_ep *p_ep,
				EHCI_ISOC_EP_DESC *p_ep_desc,
				struct usbh_urb *p_urb, uint8_t *p_buf);

static int EHCI_ITDListPrepare(struct usbh_hc_drv *p_hc_drv,
			       struct usbh_ep *p_ep,
			       EHCI_ISOC_EP_DESC *p_ep_desc,
			       struct usbh_urb *p_urb, uint8_t *p_buf,
			       uint32_t buf_len);

static void EHCI_ITD_Clr(EHCI_ITD *p_itd);
#endif

static int EHCI_AsyncEP_Close(struct usbh_hc_drv *p_hc_drv,
			      struct usbh_ep *p_ep);

#if (USBH_EHCI_CFG_PERIODIC_EN == DEF_ENABLED)
static uint32_t EHCI_SITDDone(struct usbh_hc_drv *p_hc_drv,
			      EHCI_ISOC_EP_DESC *p_ep_desc, uint8_t dev_addr,
			      uint8_t ep_addr, struct usbh_urb *p_urb);

static uint32_t EHCI_ITDDone(struct usbh_hc_drv *p_hc_drv,
			     EHCI_ISOC_EP_DESC *p_ep_desc, uint8_t dev_addr,
			     uint8_t ep_addr, struct usbh_urb *p_urb);
#endif

static void EHCI_QHDone(struct usbh_hc_drv *p_hc_drv, EHCI_QH *p_qh);

#if (USBH_EHCI_CFG_PERIODIC_EN == DEF_ENABLED)
static void EHCI_IntrEPInsert(struct usbh_hc_drv *p_hc_drv,
			      EHCI_QH *p_qh_to_insert);

static int EHCI_IntrEP_Close(struct usbh_hc_drv *p_hc_drv,
			     struct usbh_ep *p_ep);

static int EHCI_IsocEP_Close(struct usbh_hc_drv *p_hc_drv,
			     struct usbh_ep *p_ep);
#endif

/* -------------- ROOT HUB API FUNCTIONS -------------- */
static bool EHCI_PortStatusGet(struct usbh_hc_drv *p_hc_drv, uint8_t port_nbr,
			       struct usbh_hub_port_status *p_port_status);

static bool EHCI_HubDescGet(struct usbh_hc_drv *p_hc_drv, void *p_buf,
			    uint8_t buf_len);

static bool EHCI_PortEnSet(struct usbh_hc_drv *p_hc_drv, uint8_t port_nbr);

static bool EHCI_PortEnClr(struct usbh_hc_drv *p_hc_drv, uint8_t port_nbr);

static bool EHCI_PortEnChngClr(struct usbh_hc_drv *p_hc_drv, uint8_t port_nbr);

static bool EHCI_PortPwrSet(struct usbh_hc_drv *p_hc_drv, uint8_t port_nbr);

static bool EHCI_PortPwrClr(struct usbh_hc_drv *p_hc_drv, uint8_t port_nbr);

static bool EHCI_PortResetSet(struct usbh_hc_drv *p_hc_drv, uint8_t port_nbr);

static bool EHCI_PortResetChngClr(struct usbh_hc_drv *p_hc_drv,
				  uint8_t port_nbr);

static bool EHCI_PortSuspendClr(struct usbh_hc_drv *p_hc_drv, uint8_t port_nbr);

static bool EHCI_PortConnChngClr(struct usbh_hc_drv *p_hc_drv,
				 uint8_t port_nbr);

static bool EHCI_PCD_IntEn(struct usbh_hc_drv *p_hc_drv);

static bool EHCI_PCD_IntDis(struct usbh_hc_drv *p_hc_drv);

static bool EHCI_PortPwrModeGet(EHCI_DEV *p_ehci);

static int EHCI_PortSuspendSet(EHCI_DEV *p_ehci, uint32_t port_nbr);

/*
*********************************************************************************************************
*                                     LOCAL CONFIGURATION ERRORS
*********************************************************************************************************
*/

/*
*********************************************************************************************************
*                                    INITIALIZED GLOBAL VARIABLES
*********************************************************************************************************
*/

// const struct usbh_hc_drv_api  usbh_hcd_api = {
//     EHCI_Init,
//     EHCI_Start,
//     EHCI_Stop,
//     EHCI_SpdGet,
//     EHCI_Suspend,
//     EHCI_Resume,
//     EHCI_FrameNbrGet,

//     EHCI_EP_Open,
//     EHCI_EP_Close,
//     EHCI_EP_Abort,
//     EHCI_IsHalt_EP,

//     EHCI_URB_Submit,
//     EHCI_URB_Complete,
//     EHCI_URB_Abort
// };

const struct usbh_hc_drv_api usbh_hcd_api = {
	EHCI_SynopsysInit, EHCI_Start,	      EHCI_Stop,	EHCI_SpdGet,
	EHCI_Suspend,	   EHCI_Resume,	      EHCI_FrameNbrGet,

	EHCI_EP_Open,	   EHCI_EP_Close,     EHCI_EP_Abort,	EHCI_IsHalt_EP,

	EHCI_URB_Submit,   EHCI_URB_Complete, EHCI_URB_Abort
};

const struct usbh_hc_rh_api usbh_hcd_rh_api = {
	EHCI_PortStatusGet,  EHCI_HubDescGet,

	EHCI_PortEnSet,	     EHCI_PortEnClr,	    EHCI_PortEnChngClr,

	EHCI_PortPwrSet,     EHCI_PortPwrClr,

	EHCI_PortResetSet,   EHCI_PortResetChngClr,

	EHCI_PortSuspendClr, EHCI_PortConnChngClr,

	EHCI_PCD_IntEn,	     EHCI_PCD_IntDis
};

/*
*********************************************************************************************************
*                                             EHCI_Init()
*
* Description : Initialize generic EHCI host controller.
*
* Argument(s) : p_hc_drv     Pointer to Host controller driver structure
*
*               p_err        Pointer to variable that will receive the return error code from this function
*                                USBH_ERR_NONE           HCD initialized successfully.
*                                Specific error code     otherwise.
*
* Return(s)   : None
*
* Note(s)     : None
*********************************************************************************************************
*/

static void EHCI_Init(struct usbh_hc_drv *p_hc_drv, int *p_err)
{
	EHCI_InitHandler(p_hc_drv, EHCI_HCD_GENERIC, p_err);
}

/*
*********************************************************************************************************
*                                            EHCI_SynopsysInit()
*
* Description : Initialize EHCI host controller for MCUs that contain the Synopsys USB 2.0 Host Atlantic
*               IP.
*
* Argument(s) : p_hc_drv     Pointer to Host controller driver structure
*
*               p_err        Pointer to variable that will receive the return error code from this function
*                                USBH_ERR_NONE           HCD initialized successfully.
*                                Specific error code     otherwise.
*
* Return(s)   : None
*
* Note(s)     : None
*********************************************************************************************************
*/

static void EHCI_SynopsysInit(struct usbh_hc_drv *p_hc_drv, int *p_err)
{
	EHCI_InitHandler(p_hc_drv, EHCI_HCD_SYNOPSYS, p_err);
}

/*
*********************************************************************************************************
*                                          EHCI_InitHandler()
*
* Description : Initialize EHCI host controller, issue EHC hardware reset, initialize periodic
*               frame list size, initialize asynchronous and periodic lists, run host controller,
*               and enable interrupts
*
* Argument(s) : p_hc_drv          Pointer to Host controller driver structure
*
*               ehci_drv_type     EHCI driver type:
*
*                                 EHCI_HCD_GENERIC        Generic EHCI driver.
*                                 EHCI_HCD_SYNOPSYS       EHCI driver for MCU containing the Synopsys
*                                                         USB 2.0 Host Atlantic controller IP.
*
*               p_err             Pointer to variable that will receive the return error code from this function
*                                     USBH_ERR_NONE           HCD initialized successfully.
*                                     Specific error code     otherwise.
*
* Return(s)   : None
*
* Note(s)     : None
*********************************************************************************************************
*/

static void EHCI_InitHandler(struct usbh_hc_drv *p_hc_drv,
			     uint8_t ehci_drv_type, int *p_err)
{
	EHCI_DEV *p_ehci;
	volatile uint32_t usb_cmd;
	uint32_t base_addr;
#if (USBH_EHCI_CFG_PERIODIC_EN == DEF_ENABLED)
	uint16_t frame_nbr;
	uint8_t micro_frame_nbr;
#endif
	p_ehci = k_malloc(sizeof(EHCI_DEV));
	if (p_ehci == NULL) {
		*p_err = -ENOMEM;
		return;
	}

	memset(p_ehci, 0, sizeof(EHCI_DEV));

	p_hc_drv->data_ptr = (void *)p_ehci;
	p_ehci->HC_Started = false;

	base_addr = DT_INST_REG_ADDR(0);
	p_ehci->HcCapReg = (EHCI_CAP_REG *)
		base_addr; /* EHCI Capability  registers base address.             */
	EHCI_CapRegRead(p_ehci, &p_ehci->HcCap);
	/* EHCI Operational registers base address.             */
	p_ehci->HcOperReg =
		(EHCI_OPER_REG *)((uint32_t)base_addr + p_ehci->HcCap.CapLen);

	*p_err = EHCI_DMA_Init(
		p_hc_drv); /* Initialize memory pool.                              */
	if (*p_err != 0) {
		return;
	}
	LOG_INF("EHCI Applying Hardware Reset...");

	USBCMD =
		EHCI_USBCMD_RD_HCR; /* Apply hardware reset.                                */
	do {
		usb_cmd = USBCMD;
	} while ((usb_cmd & EHCI_USBCMD_RD_HCR) !=
		 0u); /* Wait for the reset completion.                       */

	p_ehci->DrvType = ehci_drv_type;
	if (p_ehci->DrvType ==
	    EHCI_HCD_SYNOPSYS) { /* Set ctrlr in host mode.                              */
		LOG_ERR("Synopsis");
		uint32_t reg_val;

		reg_val = EHCI_SYNOPSYS_USBMODE;
		// DEF_BIT_SET(reg_val, EHCI_SYNOPSYS_USBMODE_WR_CM_HOST);
		// WRITE_BIT(reg_val, EHCI_SYNOPSYS_USBMODE_WR_CM_HOST, 1);
		WRITE_BIT(reg_val, 0, 1);
		WRITE_BIT(reg_val, 1, 1);
		EHCI_SYNOPSYS_USBMODE = reg_val;
	}

	USBSTATUS = USBSTATUS;
	if ((USBSTATUS & EHCI_USBSTS_RD_HC_HAL) == 0u) {
		*p_err = -EIO;
		return;
	}

#if (USBH_EHCI_CFG_PERIODIC_EN == DEF_ENABLED)
	for (frame_nbr = 0u; frame_nbr < 256u;
	     frame_nbr++) { /* Initialize the array used for BW allocation          */
		for (micro_frame_nbr = 0u; micro_frame_nbr < 8u;
		     micro_frame_nbr++) {
			p_ehci->MaxPeriodicBWArr[frame_nbr][micro_frame_nbr] =
				3072u;
		}
	}

	EHCI_PeriodicOrderPrepare(0u, 7u, 256u);
	EHCI_PeriodicListInit(p_hc_drv);
#endif

	EHCI_AsyncListInit(p_hc_drv);

	p_ehci->NbrPorts = p_ehci->HcCap.HCSParams & EHCI_HCSPARAMS_RD_NP;

	USBINT = 0;

	*p_err = 0;
}

/*
*********************************************************************************************************
*                                            EHCI_Start()
*
* Description : Start EHCI Host controller
*
* Argument(s) : p_hc_drv     Pointer to host controller driver structure.
*
*               p_err        Pointer to variable that will receive the return error code from this function
*                                USBH_ERR_NONE           HCD start successful.
*                                Specific error code     otherwise.
*
* Return(s)   : None
*
* Note(s)     : None
*********************************************************************************************************
*/
static struct usbh_hc_drv *p_hc_drv_local;

static void EHCI_Start(struct usbh_hc_drv *p_hc_drv, int *p_err)
{
	EHCI_DEV *p_ehci;
	volatile uint32_t usb_cmd;

	printk("EHCI Enabling interrupts...\r");

	p_ehci = (EHCI_DEV *)p_hc_drv->data_ptr;
	p_hc_drv_local = p_hc_drv;

	//     if ((p_bsp_api          != (USBH_HC_BSP_API *)0) &&

	//         (p_bsp_api->ISR_Reg !=                    0)) {
	//         p_bsp_api->ISR_Reg(EHCI_ISR, p_err);

	//         if (*p_err != USBH_ERR_NONE) {
	//             return;
	//         }
	//     }

	IRQ_CONNECT(112, 1, EHCI_ISR, 0, 0);
	irq_enable(112);

	p_ehci->HC_Started = true;

	CFGFLAG =
		EHCI_CONFIGFLAG_WR_CF; /* Route all ports to EHCI                              */

	usb_cmd = USBCMD;
	// DEF_BIT_SET(usb_cmd, EHCI_USBCMD_RD_FLS_256);
	// DEF_BIT_SET(usb_cmd, BIT(9) | BIT(8));
	// DEF_BIT_SET(usb_cmd, EHCI_USBCMD_RD_RS);

	WRITE_BIT(usb_cmd, EHCI_USBCMD_RD_FLS_256, 1);
	WRITE_BIT(usb_cmd, 8, 1);
	WRITE_BIT(usb_cmd, 9, 1);
	WRITE_BIT(usb_cmd, EHCI_USBCMD_RD_RS, 1);
	USBCMD = usb_cmd;

	/* Enable all the required interrupts.                  */
	USBINT |=
		EHCI_USBINTR_WR_USBIE | /* USB Interrupt Enable.                                */
		EHCI_USBINTR_WR_USBEIE | /* USB Error Interrupt Enable.                          */
		EHCI_USBINTR_WR_HSEE | /* Host System Error Enable.                            */
		EHCI_USBINTR_WR_FLRE | /* Frame List Rollover Enable.                          */
		EHCI_USBINTR_WR_IOAAE | /* Interrupt on Async Advance Enable.                   */
		EHCI_USBINTR_WR_PCIE;

	*p_err = 0;
}

/*
*********************************************************************************************************
*                                             EHCI_Stop()
*
* Description : Stop EHCI Host controller
*
* Argument(s) : p_hc_drv     Pointer to host controller driver structure.
*
*               p_err        Pointer to variable that will receive the return error code from this function
*                                USBH_ERR_NONE           HCD stopped successfully.
*                                Specific error code     otherwise.
*
* Return(s)   : USBH_ERR_NOT_SUPPORTED
*
* Note(s)     : None
*********************************************************************************************************
*/

static void EHCI_Stop(struct usbh_hc_drv *p_hc_drv, int *p_err)
{
	(void)p_hc_drv;

	*p_err = -ENOTSUP;
}

/*
*********************************************************************************************************
*                                            EHCI_SpdGet()
*
* Description : Returns Host Controller Speed
*
* Argument(s) : p_hc_drv     Pointer to host controller driver structure.
*
*               p_err        Pointer to variable that will receive the return error code from this function
*                                USBH_ERR_NONE           Host controller speed retrieved successfuly.
*                                Specific error code     otherwise.
*
* Return(s)   : Host controller speed.
*
* Note(s)     : None
*
*********************************************************************************************************
*/

static enum usbh_device_speed EHCI_SpdGet(struct usbh_hc_drv *p_hc_drv,
					  int *p_err)
{
	(void)p_hc_drv;

	*p_err = 0;
	return (USBH_HIGH_SPEED); /* EHCI controller supports HS.                         */
}

/*
*********************************************************************************************************
*                                           EHCI_Suspend()
*
* Description : Suspend Host controller
*
* Argument(s) : p_hc_drv     Pointer to host controller driver structure.
*
*               p_err        Pointer to variable that will receive the return error code from this function
*                                USBH_ERR_NONE           HCD suspend successful.
*                                Specific error code     otherwise.
*
* Return(s)   : None
*
* Note(s)     : None
*********************************************************************************************************
*/

static void EHCI_Suspend(struct usbh_hc_drv *p_hc_drv, int *p_err)
{
	EHCI_DEV *p_ehci;
	uint8_t port_nbr;

	p_ehci = (EHCI_DEV *)p_hc_drv->data_ptr;

	for (port_nbr = 1u; port_nbr <= p_ehci->NbrPorts; port_nbr++) {
		EHCI_PortSuspendSet(p_ehci, port_nbr);
	}

	USBCMD &= ~EHCI_USBCMD_RD_RS;

	*p_err = 0;
}

/*
*********************************************************************************************************
*                                            EHCI_Resume()
*
* Description : Resume Host controller
*
* Argument(s) : p_hc_drv     Pointer to host controller driver structure.
*
*               p_err        Pointer to variable that will receive the return error code from this function
*                                USBH_ERR_NONE           HCD resume successful.
*                                Specific error code     otherwise.
*
* Return(s)   : None
*
* Note(s)     : None
*********************************************************************************************************
*/

static void EHCI_Resume(struct usbh_hc_drv *p_hc_drv, int *p_err)
{
	uint8_t port_nbr;
	EHCI_DEV *p_ehci;

	p_ehci = (EHCI_DEV *)p_hc_drv->data_ptr;

	while ((USBSTATUS & EHCI_USBSTS_RD_HC_HAL) == 0u) {
		;
	}

	USBCMD |= EHCI_USBCMD_RD_RS;

	for (port_nbr = 1u; port_nbr <= p_ehci->NbrPorts; port_nbr++) {
		EHCI_PortSuspendClr(p_hc_drv, port_nbr);
	}

	*p_err = 0;
}

/*
*********************************************************************************************************
*                                         EHCI_FrameNbrGet()
*
* Description : Retrieve current frame number.
*
* Argument(s) : p_hc_drv     Pointer to host controller driver structure.
*
*               p_err        Pointer to variable that will receive the return error code from this function
*                                USBH_ERR_NONE           HC frame number retrieved successfuly.
*                                Specific error code     otherwise.
*
* Return(s)   : Frame number.
*
* Note(s)     : None.
*********************************************************************************************************
*/

static uint32_t EHCI_FrameNbrGet(struct usbh_hc_drv *p_hc_drv, int *p_err)
{
	EHCI_DEV *p_ehci;
	uint32_t frame_nbr;

	p_ehci = (EHCI_DEV *)p_hc_drv->data_ptr;
	frame_nbr = FRAMEIX;
	frame_nbr =
		(frame_nbr & 0x000007F8u) >>
		3u; /* Bit[10..3] = current frame number                    */

	*p_err = 0;

	return (frame_nbr);
}

/*
*********************************************************************************************************
*                                            EHCI_EP_Open()
*
* Description : Create queue head structure for the given endpoint
*
* Argument(s) : p_hc_drv     Pointer to host controller driver structure.
*
*               p_ep         Pointer to endpoint structure
*
*               p_err        Pointer to variable that will receive the return error code from this function
*                                USBH_ERR_NONE                Endpoint open successfully.
*                                USBH_ERR_EP_INVALID_TYPE,    If p_hc_drv, or p_ep is 0
*                                Specific error code          otherwise.
*
* Return(s)   : None
*
* Note(s)     : (1) Handle Cache Coherency for the p_ehci->AsyncQHHead data structure
*
*               (2) See USB2.0 specification, section 9.6.6. Interval for polling endpoint for data
*                   transfers can be obtained from the equataion 2 POW (bInterval - 1)
*********************************************************************************************************
*/

static void EHCI_EP_Open(struct usbh_hc_drv *p_hc_drv, struct usbh_ep *p_ep,
			 int *p_err)
{
	uint8_t ep_type;

	LOG_INF("EP_Open");
	ep_type = usbh_ep_type_get(p_ep);

	switch (ep_type) {
	case USBH_EP_TYPE_CTRL:
	case USBH_EP_TYPE_BULK:
		LOG_INF("EP_Type");
		*p_err = EHCI_AsyncEP_Open(p_hc_drv, p_ep, p_ep->dev_ptr);
		break;

#if (USBH_EHCI_CFG_PERIODIC_EN == DEF_ENABLED)
	case USBH_EP_TYPE_INTR:
		*p_err = EHCI_IntrEP_Open(p_hc_drv, p_ep, p_ep->dev_ptr);
		break;

	case USBH_EP_TYPE_ISOC:
		*p_err = EHCI_IsocEP_Open(p_hc_drv, p_ep);
		break;
#endif

	default:
		LOG_ERR("Invalid EP Type");
		*p_err = -EAGAIN;
		break;
	}
}

/*
*********************************************************************************************************
*                                           EHCI_EP_Close()
*
* Description : Close the endpoint by unlinking the EHCI queue head
*
* Argument(s) : p_hc_drv     Pointer to host controller driver structure.
*
*               p_ep         Pointer to endpoint structure.
*
*               p_err        Pointer to variable that will receive the return error code from this function
*                                USBH_ERR_NONE                Endpoint closed successfully.
*                                USBH_ERR_EP_INVALID_TYPE,    If p_hc_drv, or p_ep is 0
*                                Specific error code          otherwise.
*
* Return(s)   : None
*
* Note(s)     : None
*********************************************************************************************************
*/

static void EHCI_EP_Close(struct usbh_hc_drv *p_hc_drv, struct usbh_ep *p_ep,
			  int *p_err)
{
	uint8_t ep_type;
	int key;

	ep_type = usbh_ep_type_get(p_ep);

	key = irq_lock();

	switch (ep_type) {
	case USBH_EP_TYPE_CTRL:
	case USBH_EP_TYPE_BULK:
		*p_err = EHCI_AsyncEP_Close(p_hc_drv, p_ep);
		break;

#if (USBH_EHCI_CFG_PERIODIC_EN == DEF_ENABLED)
	case USBH_EP_TYPE_INTR:
		*p_err = EHCI_IntrEP_Close(p_hc_drv, p_ep);
		break;

	case USBH_EP_TYPE_ISOC:
		*p_err = EHCI_IsocEP_Close(p_hc_drv, p_ep);
		break;
#endif

	default:
		*p_err = -EAGAIN;
		break;
	}
	irq_unlock(key);
}

/*
*********************************************************************************************************
*                                           EHCI_EP_Abort()
*
* Description : Abort all pending URBs in the queue head.
*
* Argument(s) : p_hc_drv     Pointer to host controller driver structure.
*
*               p_ep         Pointer to endpoint structure.
*
*               p_err        Pointer to variable that will receive the return error code from this function
*                                USBH_ERR_NONE          Endpoint abort successfully.
*                                Specific error code    otherwise.
*
* Return(s)   : None
*
* Note(s)     : None
*********************************************************************************************************
*/

static void EHCI_EP_Abort(struct usbh_hc_drv *p_hc_drv, struct usbh_ep *p_ep,
			  int *p_err)
{
	(void)p_hc_drv;
	(void)p_ep;

	*p_err = 0;
}

/*
*********************************************************************************************************
*                                           EHCI_IsHaltEP()
*
* Description : Retrieve endpoint halt state.
*
* Argument(s) : p_hc_drv     Pointer to host controller driver structure.
*
*               p_ep         Pointer to endpoint structure.
*
*               p_err        Pointer to variable that will receive the return error code from this function
*                                USBH_ERR_NONE          If successful.
*                                Specific error code    otherwise.
*
* Return(s)   : DEF_TRUE,  if halted.
*               DEF_FALSE, otherwise.
*
* Note(s)     : None.
*********************************************************************************************************
*/

static bool EHCI_IsHalt_EP(struct usbh_hc_drv *p_hc_drv, struct usbh_ep *p_ep,
			   int *p_err)
{
	(void)p_hc_drv;
	(void)p_ep;

	*p_err = 0;

	return (false);
}

/*
*********************************************************************************************************
*                                          EHCI_URB_Submit()
*
* Description : Insert the QTD list head into the appropriate QH.
*
* Argument(s) : p_hc_drv     Pointer to host controller driver structure.
*
*               p_urb        Pointer to URB structure.
*
*               p_err        Pointer to variable that will receive the return error code from this function
*                                USBH_ERR_NONE           URB submitted successfuly.
*                                Specific error code     otherwise.
*
* Return(s)   : None
*
* Note(s)     : (1) When the CPU cache is enabled, this code ensures that the buffer start address is
*                   aligned on the cache line size. If not, the nearest address from the initial buffer
*                   start address is computed. This address is aligned on the cache line. The number of
*                   octets to flush or invalidate will be increased accordingly to take into account
*                   the buffer size plus the address adjustment.
*********************************************************************************************************
*/

static void EHCI_URB_Submit(struct usbh_hc_drv *p_hc_drv,
			    struct usbh_urb *p_urb, int *p_err)
{
	EHCI_DEV *p_ehci;
	EHCI_QH *p_qh;
	EHCI_QTD *p_head_qtd;
	uint8_t ep_type;
	struct usbh_ep *p_ep;
#if (USBH_EHCI_CFG_PERIODIC_EN == DEF_ENABLED)
	EHCI_ISOC_EP_DESC *p_ep_desc;
	struct usbh_dev *p_dev;
#endif
	int key;

	p_ehci = (EHCI_DEV *)p_hc_drv->data_ptr;
	// p_hc_cfg = p_hc_drv->HC_CfgPtr;
	p_ep = p_urb->ep_ptr;
#if (USBH_EHCI_CFG_PERIODIC_EN == DEF_ENABLED)
	p_dev = p_ep->dev_ptr;
#endif
	ep_type = usbh_ep_type_get(p_ep);

	/* ----------- DATA BUF FROM DEDICATED MEM ------------ */
	// if ((p_hc_cfg->DedicatedMemAddr    != (uint32_t)0) &&
	//     (p_hc_cfg->DataBufFromSysMemEn == 0)) {
	if (DT_INST_PROP(0, dma) == false) {
		if (ep_type == USBH_EP_TYPE_ISOC) {
			if (p_urb->uberbuf_len > DT_INST_PROP(0, buf_len)) {
				*p_err = -ENOMEM;
				return;
			}
		}

		if (p_urb->uberbuf_len != 0u) {
			// p_urb->dma_buf_ptr = Mem_PoolBlkGet(&BufPool,
			//                                     DT_INST_PROP(0, buf_len),
			//                                    &err_lib);
			p_urb->dma_buf_ptr = k_mem_pool_malloc(
				&BufPool, DT_INST_PROP(0, buf_len));
			if (p_urb->dma_buf_ptr == NULL) {
				*p_err = -ENOMEM;
				return;
			}

			p_urb->dma_buf_len = MIN(p_urb->uberbuf_len,
						 DT_INST_PROP(0, buf_len));

			if ((p_urb->token == USBH_TOKEN_OUT) ||
			    (p_urb->token == USBH_TOKEN_SETUP)) {
				// Mem_Copy(p_urb->dma_buf_ptr,
				//          p_urb->userbuf_ptr,
				//          p_urb->dma_buf_len);
				memcpy(p_urb->dma_buf_ptr, p_urb->userbuf_ptr,
				       p_urb->dma_buf_len);

				DCACHE_CleanByRange(
					(uint32_t)p_urb->dma_buf_ptr,
					p_urb->dma_buf_len);
			} else {
				DCACHE_CleanByRange(
					(uint32_t)p_urb->dma_buf_ptr,
					p_urb->dma_buf_len);
				DCACHE_InvalidateByRange(
					(uint32_t)p_urb->dma_buf_ptr,
					p_urb->dma_buf_len);
			}
		}
	} else { /* ------------- DATA BUF FROM SYSTEM MEM ------------- */
#if (CPU_CFG_CACHE_MGMT_EN == DEF_ENABLED)
		uint8_t *p_cache_aligned_buf_addr = 0;
		uint32_t len;
		uint8_t remainder;
#endif

		p_urb->dma_buf_ptr = p_urb->userbuf_ptr;
		p_urb->dma_buf_len = p_urb->uberbuf_len;

#if (CPU_CFG_CACHE_MGMT_EN ==                                                  \
     DEF_ENABLED) /* See Note #1.                                         */
		remainder = (uint8_t)(((uint32_t)p_urb->dma_buf_ptr) % 32u);
		if (remainder != 0u) {
			p_cache_aligned_buf_addr =
				((uint8_t *)p_urb->dma_buf_ptr) - remainder;
			len = p_urb->dma_buf_len + remainder;
		} else {
			p_cache_aligned_buf_addr =
				(uint8_t *)p_urb->dma_buf_ptr;
			len = p_urb->dma_buf_len;
		}

		if (((p_urb->token == USBH_TOKEN_OUT) ||
		     (p_urb->token == USBH_TOKEN_SETUP)) &&
		    (p_urb->dma_buf_len != 0u)) {
			DCACHE_CleanByRange((uint32_t)p_cache_aligned_buf_addr,
					    len);
		} else {
			DCACHE_CleanByRange((uint32_t)p_cache_aligned_buf_addr,
					    len);
			DCACHE_InvalidateByRange(
				(uint32_t)p_cache_aligned_buf_addr, len);
		}
#endif
	}

	if ((ep_type == USBH_EP_TYPE_CTRL) || (ep_type == USBH_EP_TYPE_BULK) ||
	    (ep_type == USBH_EP_TYPE_INTR)) {
		p_qh = (EHCI_QH *)p_ep->arg_ptr;

		p_head_qtd = EHCI_QTDListPrepare(
			p_hc_drv, p_ep, p_urb,
			(p_urb->dma_buf_len) ? (uint8_t *)(p_urb->dma_buf_ptr) :
						     NULL,
			p_urb->dma_buf_len, p_err);
		if (p_head_qtd == 0) {
			return;
		}

		key = irq_lock();
		p_qh->QTDHead = (uint32_t)p_head_qtd;
		p_qh->QHNxtQTDPtr = (uint32_t)p_head_qtd;
		DCACHE_CleanByRange((uint32_t)p_qh, sizeof(EHCI_QH));
		irq_unlock(key);

	}
#if (USBH_EHCI_CFG_PERIODIC_EN == DEF_ENABLED)
	else {
		p_ep_desc = (EHCI_ISOC_EP_DESC *)p_ep->arg_ptr;

		if (p_ep->dev_spd == USBH_FULL_SPEED) {
			*p_err = EHCI_SITDListPrepare(
				p_hc_drv, p_dev, p_ep, p_ep_desc, p_urb,
				(p_urb->dma_buf_len) ?
					      (uint8_t *)(p_urb->dma_buf_ptr) :
					      NULL);
		} else {
			*p_err = EHCI_ITDListPrepare(
				p_hc_drv, p_ep, p_ep_desc, p_urb,
				(p_urb->dma_buf_len) ?
					      (uint8_t *)(p_urb->dma_buf_ptr) :
					      NULL,
				p_urb->dma_buf_len);
		}
	}
#endif
}

/*
*********************************************************************************************************
*                                         EHCI_URB_Complete()
*
* Description : Transfer received data to application buffer, and release DMA buffer, if DMA is enabled.
*
* Argument(s) : p_hc_drv     Pointer to host controller driver structure.
*
*               p_urb        Pointer to URB structure.
*
*               p_err        Pointer to variable that will receive the return error code from this function
*                                USBH_ERR_NONE           URB completed successfuly.
*                                Specific error code     otherwise.
*
* Return(s)   : None
*
* Note(s)     : (1) See Note #1 in function 'EHCI_URB_Submit()'.
*********************************************************************************************************
*/

static void EHCI_URB_Complete(struct usbh_hc_drv *p_hc_drv,
			      struct usbh_urb *p_urb, int *p_err)
{
	EHCI_DEV *p_ehci;

	p_ehci = (EHCI_DEV *)p_hc_drv->data_ptr;
	// p_hc_cfg = p_hc_drv->HC_CfgPtr;
	/* ----------- DATA BUF FROM DEDICATED MEM ------------ */
	// if ((p_hc_cfg->DedicatedMemAddr    != (uint32_t)0) &&
	//     (p_hc_cfg->DataBufFromSysMemEn == 0)) {
	if (DT_INST_PROP(0, dma) == false) {
		if ((p_urb->userbuf_ptr != p_urb->dma_buf_ptr) &&
		    (p_urb->dma_buf_ptr != NULL)) {
			if ((p_urb->token == USBH_TOKEN_IN) &&
			    (p_urb->xfer_len != 0u)) {
#if (CPU_CFG_CACHE_MGMT_EN == DEF_ENABLED)
				uint8_t *p_cache_aligned_buf_addr = 0;
				uint32_t len;
				uint8_t remainder;
#endif

				// Mem_Copy(p_urb->userbuf_ptr,
				//          p_urb->dma_buf_ptr,
				//          p_urb->xfer_len);
				memcpy(p_urb->userbuf_ptr, p_urb->dma_buf_ptr,
				       p_urb->xfer_len);

#if (CPU_CFG_CACHE_MGMT_EN ==                                                  \
     DEF_ENABLED) /* See Note #1.                                         */
				remainder = (uint8_t)(
					((uint32_t)p_urb->userbuf_ptr) %
					CPU_Cache_Linesize);
				if (remainder != 0u) {
					p_cache_aligned_buf_addr =
						((uint8_t *)p_urb->userbuf_ptr) -
						remainder;
					len = p_urb->xfer_len + remainder;
				} else {
					p_cache_aligned_buf_addr =
						(uint8_t *)p_urb->userbuf_ptr;
					len = p_urb->xfer_len;
				}
				DCACHE_CleanByRange(
					(uint32_t)p_cache_aligned_buf_addr,
					len);

#endif
			}

			// Mem_PoolBlkFree(&BufPool,
			//                  p_urb->dma_buf_ptr,
			//                 &err_lib);
			k_free(p_urb->dma_buf_ptr);
		}

	} else { /* ------------- DATA BUF FROM SYSTEM MEM ------------- */

		if ((p_urb->token == USBH_TOKEN_IN) &&
		    (p_urb->xfer_len != 0u)) {
			DCACHE_InvalidateByRange((uint32_t)p_urb->dma_buf_ptr,
						 p_urb->xfer_len);
		}
	}

	*p_err = 0;
}

/*
*********************************************************************************************************
*                                           EHCI_URB_Abort()
*
* Description : Abort pending transfer.
*
* Argument(s) : p_hc_drv     Pointer to host controller driver structure.
*
*               p_urb        Pointer to URB structure.
*
*               p_err        Pointer to variable that will receive the return error code from this function
*                                USBH_ERR_NONE           URB aborted successfuly.
*                                Specific error code     otherwise.
*
* Return(s)   : None
*
* Note(s)     : None.
*********************************************************************************************************
*/

static void EHCI_URB_Abort(struct usbh_hc_drv *p_hc_drv, struct usbh_urb *p_urb,
			   int *p_err)
{
	EHCI_DEV *p_ehci;

	p_ehci = (EHCI_DEV *)p_hc_drv->data_ptr;
	p_urb->err = -EAGAIN;

	if (DT_INST_PROP(0, mem_addr) != (uint32_t)0) {
		if (p_urb->dma_buf_ptr != NULL) {
			// Mem_PoolBlkFree(&BufPool,
			//                  p_urb->dma_buf_ptr,
			//                 &err_lib);
			k_free(p_urb->dma_buf_ptr);
		}
	}

	*p_err = 0;
}

/*
*********************************************************************************************************
*                                         EHCI_AsyncEP_Open()
*
* Description : Open a Control or Bulk Endpoint
*
* Argument(s) : p_hc_drv     Pointer to host controller driver structure.
*
*               p_ep         Pointer to endpoint structure
*
*               p_dev        Pointer to device structure
*
* Return(s)   : USBH_ERR_NONE           If successful
*
* Note(s)     : qH structure and fields. For more details, see section 3.6 (EHCI spec).
*
*               ---------------------------------------------------------------------------------------
*               |31 30 29 28 27 26 25 24 23 22 21 20 19 18 17 16 15 14 13 12 11 10 9 8 7 6 5 4 3 2 1 0|
*               ---------------------------------------------------------------------------------------
*               |        Queue Head Horizontal Link Pointer                                 |  0    |T|
*               ---------------------------------------------------------------------------------------
*               |   RL      |C |    Maximum Packet Length       |H |dtc|EPS |   EndPt |I| Device Addr |
*               ---------------------------------------------------------------------------------------
*               |  Mult  |  Port Number    |     Hub Addr       |   uFrame C-mask     | uFrame S-mask |
*               ---------------------------------------------------------------------------------------
*               |        Current qTD Pointer                                                |  0      |
*               ---------------------------------------------------------------------------------------
*********************************************************************************************************
*/

static int EHCI_AsyncEP_Open(struct usbh_hc_drv *p_hc_drv, struct usbh_ep *p_ep,
			     struct usbh_dev *p_dev)
{
	EHCI_QH *p_new_qh;
	EHCI_DEV *p_ehci;
	uint8_t ep_nbr;
	uint8_t ep_type;
	uint16_t ep_max_pkt_size;
	struct usbh_dev *ptemp_dev;
	uint8_t retry;
	int err;

	p_ehci = (EHCI_DEV *)p_hc_drv->data_ptr;
	/* Allocate memory for a queue head                    */
	//     p_new_qh = (EHCI_QH *)Mem_PoolBlkGet(&HC_QHPool,
	//                                           sizeof(EHCI_QH),
	//                                          &err_lib);

	p_new_qh = k_mem_pool_malloc(&HC_QHPool, sizeof(EHCI_QH));
	if (p_new_qh == NULL) {
		LOG_ERR("could not get p_new_qh block");
		return (-ENOMEM);
	}

	EHCI_QH_Clr(p_new_qh);
	p_new_qh->EPPtr = p_ep;
	p_ep->arg_ptr = (void *)p_new_qh;
	ep_nbr = usbh_ep_log_nbr_get(p_ep);
	ep_type = usbh_ep_type_get(p_ep);
	ep_max_pkt_size = usbh_ep_max_pkt_size_get(p_ep);

	p_new_qh->QHHorLinkPtr = HOR_LNK_PTR_TYP(DWORD1_TYP_QH);
	p_new_qh->QHEpCapChar[0] = QH_EPCHAR_DEVADD(p_ep->dev_addr) |
				   QH_EPCHAR_I(0) | QH_EPCHAR_ENDPT(ep_nbr) |
				   ((ep_type == USBH_EP_TYPE_CTRL) ?
						  QH_EPCHAR_DTC(DWORD2_QH_DTC_QTD) :
						  QH_EPCHAR_DTC(DWORD2_QH_DTC_QH)) |
				   QH_EPCHAR_H(0u) |
				   QH_EPCHAR_MPL(ep_max_pkt_size) |
				   QH_EPCHAR_C(0u) | QH_EPCHAR_RL(0xFu);

	switch (p_ep->dev_spd) {
	case USBH_LOW_SPEED: /* For low speed devices, use split transactions       */
		if (ep_type == USBH_EP_TYPE_CTRL) {
			p_new_qh->QHEpCapChar[0] |= QH_EPCHAR_C(DWORD2_QH_C);
		}

		p_new_qh->QHEpCapChar[0] |= QH_EPCHAR_EPS(DWORD2_QH_EPS_LS);
		/* Search Hub 2.0 the nearest of LS dev.                */
		ptemp_dev = p_dev;
		while (ptemp_dev->hub_dev_ptr->dev_spd != USBH_HIGH_SPEED) {
			ptemp_dev = ptemp_dev->hub_dev_ptr;
		}
		/* Set Hub addr of the nearest Hub 2.0 and port nbr...  */
		/* ...of the Hub 2.0 to which the dev is attached.      */
		if (ptemp_dev->hub_dev_ptr->is_root_hub == 1) {
			p_new_qh->QHEpCapChar[1] =
				QH_EPCAP_HUBADD(
					ptemp_dev->hub_dev_ptr->dev_addr) |
				QH_EPCAP_PN(ptemp_dev->port_nbr);
		} else {
			p_new_qh->QHEpCapChar[1] =
				QH_EPCAP_HUBADD(0u) | QH_EPCAP_PN(0u);
		}
		break;

	case USBH_FULL_SPEED: /* For full speed devices, use split transactions      */
		if (ep_type == USBH_EP_TYPE_CTRL) {
			p_new_qh->QHEpCapChar[0] |= QH_EPCHAR_C(DWORD2_QH_C);
		}

		p_new_qh->QHEpCapChar[0] |= QH_EPCHAR_EPS(DWORD2_QH_EPS_FS);
		/* Search Hub 2.0 the nearest of FS dev.                */
		ptemp_dev = p_dev;
		while (ptemp_dev->hub_dev_ptr->dev_spd != USBH_HIGH_SPEED) {
			ptemp_dev = ptemp_dev->hub_dev_ptr;
		}
		/* Set Hub addr of the nearest Hub 2.0 and port nbr...  */
		/* ...of the Hub 2.0 to which the dev is attached.      */
		if (ptemp_dev->hub_dev_ptr->is_root_hub == 1) {
			p_new_qh->QHEpCapChar[1] =
				QH_EPCAP_HUBADD(
					ptemp_dev->hub_dev_ptr->dev_addr) |
				QH_EPCAP_PN(ptemp_dev->port_nbr);
		} else {
			p_new_qh->QHEpCapChar[1] =
				QH_EPCAP_HUBADD(0u) | QH_EPCAP_PN(0u);
		}
		break;

	case USBH_HIGH_SPEED:
		p_new_qh->QHEpCapChar[0] |= QH_EPCHAR_EPS(DWORD2_QH_EPS_HS);
		break;

	default:
		break;
	}

	p_new_qh->QHEpCapChar[1] |=
		QH_EPCAP_HBPM(DWORD3_QH_HBPM_1) | QH_EPCAP_SMASK(0u);
	p_new_qh->QHCurQTDPtr = (uint32_t)0;
	p_new_qh->QHNxtQTDPtr = (uint32_t)0x00000001;
	p_new_qh->QHAltNxtQTDPtr = (uint32_t)0x00000001;
	p_new_qh->QHToken = (uint32_t)0;
	p_new_qh->QHBufPagePtrList[0] = (uint32_t)0;
	p_new_qh->QHBufPagePtrList[1] = (uint32_t)0;
	p_new_qh->QHBufPagePtrList[2] = (uint32_t)0;
	p_new_qh->QHBufPagePtrList[3] = (uint32_t)0;
	p_new_qh->QHBufPagePtrList[4] = (uint32_t)0;
	DCACHE_InvalidateByRange((uint32_t)p_ehci->AsyncQHHead,
				 sizeof(EHCI_QH));
	p_new_qh->QHHorLinkPtr |=
		(uint32_t)(p_ehci->AsyncQHHead->QHHorLinkPtr & 0xFFFFFFE0);

	DCACHE_CleanByRange((uint32_t)p_new_qh, sizeof(EHCI_QH));

	USBCMD &=
		~EHCI_USBCMD_RD_ASE; /* Disable async list processing                        */
	retry = 100u;
	while ((USBSTATUS & EHCI_USBSTS_RD_ASS) !=
	       0u) { /* Wait until the async list processing is disabled     */
		retry--;
		if (retry == 0) {
			// Mem_PoolBlkFree(       &HC_QHPool,
			//                 (void *)p_new_qh,
			//                        &err_lib);
			k_free(p_new_qh);

			p_ep->arg_ptr = NULL;
			err = -EBUSY;
			return (err);
		}
		k_sleep(K_MSEC(1u));
	}
	/* Insert new queue head                                */
	p_ehci->AsyncQHHead->QHHorLinkPtr = (uint32_t)(p_new_qh) |
					    HOR_LNK_PTR_TYP(DWORD1_TYP_QH) |
					    HOR_LNK_PTR_T(DWORD1_T_VALID);

	DCACHE_CleanByRange((uint32_t)p_ehci->AsyncQHHead, sizeof(EHCI_QH));

	USBCMD |=
		EHCI_USBCMD_WR_ASE; /* Enable async list processing                         */

	retry = 100u;
	while ((USBSTATUS & EHCI_USBSTS_RD_ASS) ==
	       0u) { /* Wait till async list processing is enabled           */
		retry--;
		if (retry == 0u) {
			// Mem_PoolBlkFree(       &HC_QHPool,
			//                 (void *)p_new_qh,
			//                        &err_lib);
			k_free(p_new_qh);

			p_ep->arg_ptr = NULL;
			err = -EBUSY;
			return (err);
		}
		k_sleep(K_MSEC(1u));
	}

	USBCMD |= EHCI_USBCMD_WR_IOAAD;

	err = 0;
	return (err);
}

/*
*********************************************************************************************************
*                                         EHCI_IntrEP_Open()
*
* Description : Open an Interrupt endpoint
*
* Argument(s) : p_hc_drv     Pointer to host controller driver structure.
*
*               p_ep         Pointer to endpoint structure
*
*               p_dev        Pointer to device structure
*
* Return(s)   : USBH_ERR_NONE           If successful
*
* Note(s)     : qH structure and fields. For more details, see section 3.6 (EHCI spec).
*
*               ---------------------------------------------------------------------------------------
*               |31 30 29 28 27 26 25 24 23 22 21 20 19 18 17 16 15 14 13 12 11 10 9 8 7 6 5 4 3 2 1 0|
*               ---------------------------------------------------------------------------------------
*               |        Queue Head Horizontal Link Pointer                                 |  0    |T|
*               ---------------------------------------------------------------------------------------
*               |   RL      |C |    Maximum Packet Length       |H |dtc|EPS |   EndPt |I| Device Addr |
*               ---------------------------------------------------------------------------------------
*               |  Mult  |  Port Number    |     Hub Addr       |   uFrame C-mask     | uFrame S-mask |
*               ---------------------------------------------------------------------------------------
*               |        Current qTD Pointer                                                |  0      |
*               ---------------------------------------------------------------------------------------
*********************************************************************************************************
*/

#if (USBH_EHCI_CFG_PERIODIC_EN == DEF_ENABLED)
static int EHCI_IntrEP_Open(struct usbh_hc_drv *p_hc_drv, struct usbh_ep *p_ep,
			    struct usbh_dev *p_dev)
{
	EHCI_QH *p_new_qh;
	EHCI_DEV *p_ehci;
	uint8_t ep_nbr;
	uint16_t ep_max_pkt_size;
	int err;
	uint16_t nbr_of_transaction_per_uframe;
	struct usbh_dev *ptemp_dev;
	EHCI_INTR_INFO *p_intr_info;
	EHCI_INTR_INFO *p_temp_intr_info;
	int key;

	p_ehci = (EHCI_DEV *)p_hc_drv->data_ptr;
	/* Allocate memory for a queue head                     */
	// p_new_qh = (EHCI_QH *)Mem_PoolBlkGet(&HC_QHPool,
	//                                       sizeof(EHCI_QH),
	//                                      &err_lib);
	p_new_qh = k_mem_pool_malloc(&HC_QHPool, sizeof(EHCI_QH));
	if (p_new_qh == NULL) {
		return (-ENOMEM);
	}

	EHCI_QH_Clr(
		p_new_qh); /* Clear QH structure                                   */

	p_new_qh->EPPtr = p_ep;
	p_ep->arg_ptr = (void *)p_new_qh;
	ep_nbr = usbh_ep_log_nbr_get(p_ep);
	ep_max_pkt_size = usbh_ep_max_pkt_size_get(p_ep);

	p_new_qh->QHHorLinkPtr = HOR_LNK_PTR_TYP(DWORD1_TYP_QH) |
				 HOR_LNK_PTR_T(DWORD1_T_INVALID);

	p_new_qh->QHEpCapChar[0] =
		QH_EPCHAR_DEVADD(
			p_ep->dev_addr) | /* USB Device address                                   */
		QH_EPCHAR_ENDPT(
			ep_nbr) | /* Endpoint number                                      */
		QH_EPCHAR_DTC(
			DWORD2_QH_DTC_QH) | /* Use toggle bit from QH                               */
		QH_EPCHAR_H(
			0u) | /* For interrut endpoint H-bit must be zero             */
		QH_EPCHAR_MPL(
			ep_max_pkt_size) | /* Endpoint max packet size                             */
		QH_EPCHAR_C(
			0u) | /* C bit must be zero for non-control endpoints         */
		QH_EPCHAR_RL(
			0x00u); /* Reload NAK Cnt                                       */

	switch (p_ep->dev_spd) {
	case USBH_LOW_SPEED: /* For low speed devices, use split transactions        */
		/* Endpoint is low-speed.                               */
		p_new_qh->QHEpCapChar[0] |= QH_EPCHAR_EPS(DWORD2_QH_EPS_LS);

		/* Search Hub 2.0 the nearest of LS dev.                */
		ptemp_dev = p_dev;
		while (ptemp_dev->hub_dev_ptr->dev_spd != USBH_HIGH_SPEED) {
			ptemp_dev = ptemp_dev->hub_dev_ptr;
		}
		/* Set Hub addr of the nearest Hub 2.0 and port nbr...  */
		/* ...of the Hub 2.0 to which the dev is attached.      */
		if (ptemp_dev->hub_dev_ptr->is_root_hub == 1) {
			p_new_qh->QHEpCapChar[1] =
				QH_EPCAP_HUBADD(
					ptemp_dev->hub_dev_ptr->dev_addr) |
				QH_EPCAP_PN(ptemp_dev->port_nbr);
		} else {
			p_new_qh->QHEpCapChar[1] =
				QH_EPCAP_HUBADD(0u) | QH_EPCAP_PN(0u);
		}
		break;

	case USBH_FULL_SPEED: /* For full speed devices, use split transactions       */
		/* Endpoint is full-speed.                              */
		p_new_qh->QHEpCapChar[0] |= QH_EPCHAR_EPS(DWORD2_QH_EPS_FS);
		ptemp_dev = p_dev;
		/* Search Hub 2.0 the nearest of LS dev.                */
		while (ptemp_dev->hub_dev_ptr->dev_spd != USBH_HIGH_SPEED) {
			ptemp_dev = ptemp_dev->hub_dev_ptr;
		}
		/* Set Hub addr of the nearest Hub 2.0 and port nbr...  */
		/* ...of the Hub 2.0 to which the dev is attached.      */
		if (ptemp_dev->hub_dev_ptr->is_root_hub == 1) {
			p_new_qh->QHEpCapChar[1] =
				QH_EPCAP_HUBADD(
					ptemp_dev->hub_dev_ptr->dev_addr) |
				QH_EPCAP_PN(ptemp_dev->port_nbr);
		} else {
			p_new_qh->QHEpCapChar[1] =
				QH_EPCAP_HUBADD(0u) | QH_EPCAP_PN(0u);
		}
		break;

	case USBH_HIGH_SPEED:
		p_new_qh->QHEpCapChar[0] |= QH_EPCHAR_EPS(DWORD2_QH_EPS_HS);
		break;

	default:
		break;
	}

	nbr_of_transaction_per_uframe = (p_ep->desc.w_max_packet_size &
					 USBH_NBR_TRANSACTION_PER_UFRAME) >>
					11;

	if (nbr_of_transaction_per_uframe == USBH_3_TRANSACTION_PER_UFRAME) {
		p_new_qh->QHEpCapChar[1] |=
			(uint32_t)QH_EPCAP_HBPM(DWORD3_QH_HBPM_3);
	} else if (nbr_of_transaction_per_uframe ==
		   USBH_2_TRANSACTION_PER_UFRAME) {
		p_new_qh->QHEpCapChar[1] |=
			(uint32_t)QH_EPCAP_HBPM(DWORD3_QH_HBPM_2);
	} else {
		p_new_qh->QHEpCapChar[1] |=
			(uint32_t)QH_EPCAP_HBPM(DWORD3_QH_HBPM_1);
	}

	p_new_qh->QHCurQTDPtr = (uint32_t)0u;
	p_new_qh->QHNxtQTDPtr = (uint32_t)0x00000001u;
	p_new_qh->QHAltNxtQTDPtr = (uint32_t)0x00000001u;
	p_new_qh->QHToken = (uint32_t)0u;
	p_new_qh->QHBufPagePtrList[0] = (uint32_t)0u;
	p_new_qh->QHBufPagePtrList[1] = (uint32_t)0u;
	p_new_qh->QHBufPagePtrList[2] = (uint32_t)0u;
	p_new_qh->QHBufPagePtrList[3] = (uint32_t)0u;
	p_new_qh->QHBufPagePtrList[4] = (uint32_t)0u;

	DCACHE_CleanByRange((uint32_t)p_new_qh, sizeof(EHCI_QH));

	key = irq_lock();

	err = EHCI_BW_Get(p_hc_drv, p_ep, (void *)p_new_qh);

	if (err != 0) {
		// Mem_PoolBlkFree(       &HC_QHPool,
		//                 (void *)p_new_qh,
		//                        &err_lib);
		k_free(p_new_qh);
		p_ep->arg_ptr = NULL;

		irq_unlock(key);
		return (err);
	}

	p_new_qh->QHEpCapChar[1] |=
		p_new_qh->SMask | DWORD3_QH_PS_CSPLIT_UFRAME_2345 << 8;

	DCACHE_CleanByRange((uint32_t)p_new_qh, sizeof(EHCI_QH));

	EHCI_BW_Update(p_hc_drv, p_ep, (void *)p_new_qh, true);

	// p_intr_info = (EHCI_INTR_INFO *)Mem_PoolBlkGet(&IntrInfoPool,
	//                                                 sizeof(EHCI_INTR_INFO),
	//                                                &err_lib);
	p_intr_info = k_mem_pool_malloc(&IntrInfoPool, sizeof(EHCI_INTR_INFO));
	if (p_intr_info == NULL) {
		return (-ENOMEM);
	}

	DCACHE_InvalidateByRange((uint32_t)p_new_qh, sizeof(EHCI_QH));
	p_intr_info->IntrPlaceholderIx =
		p_new_qh->BWStartFrame; /* Save placeholder index in QHLists array.             */
	p_intr_info->FrameInterval =
		p_new_qh->FrameInterval; /* Save polling interval list to which belongs qH.      */
	p_intr_info->EpPtr = p_ep;
	p_intr_info->NxtIntrInfo = NULL;

	if (p_ehci->HeadIntrInfo ==
	    0) { /* First Intr EP opened.                                */
		p_ehci->HeadIntrInfo =
			p_intr_info; /* Init Intr info queue head ptr.                       */
	} else { /* Other Intr EP opened.                                */
		p_temp_intr_info =
			p_ehci->HeadIntrInfo; /* Retrieve the 1st Intr info.                          */
		while (p_temp_intr_info->NxtIntrInfo !=
		       0) { /* Find end of Intr info queue.                         */
			p_temp_intr_info = p_temp_intr_info->NxtIntrInfo;
		}
		p_temp_intr_info->NxtIntrInfo =
			p_intr_info; /* Insert new Intr info at end of the queue.            */
	}

	EHCI_IntrEPInsert(p_hc_drv, p_new_qh);

	irq_unlock(key);

	return (0);
}
#endif

/*
*********************************************************************************************************
*                                         EHCI_IsocEP_Open()
*
* Description : Open an Isochronous endpoint
*
* Argument(s) : p_hc_drv     Pointer to host controller driver structure.
*
*               p_ep         Pointer to endpoint structure
*
* Return(s)   : USBH_ERR_NONE           If successful
*
* Note(s)     : None.
*********************************************************************************************************
*/

#if (USBH_EHCI_CFG_PERIODIC_EN == DEF_ENABLED)
static int EHCI_IsocEP_Open(struct usbh_hc_drv *p_hc_drv, struct usbh_ep *p_ep)
{
	EHCI_ISOC_EP_DESC *p_ep_desc;
	EHCI_ISOC_EP_DESC *p_temp_ep_desc;
	EHCI_DEV *p_ehci;
	int err;
	int key;

	p_ehci = (EHCI_DEV *)p_hc_drv->data_ptr;
	// p_ep_desc = (EHCI_ISOC_EP_DESC *)Mem_PoolBlkGet(&HC_Isoc_EP_DescPool,
	//                                                  sizeof(EHCI_ISOC_EP_DESC),
	//                                                 &err_lib);
	p_ep_desc = k_mem_pool_malloc(&HC_Isoc_EP_DescPool,
				      sizeof(EHCI_ISOC_EP_DESC));
	if (p_ep_desc == NULL) {
		return (-ENOMEM);
	}

	EHCI_EP_DescClr(p_ep_desc);

	if (p_ehci->HeadIsocEPDesc ==
	    0) { /* First Isochronous EP opened                                  */
		p_ehci->HeadIsocEPDesc =
			p_ep_desc; /* Init the Isochronous queue Head pointer                      */
	} else { /* Other Isochronous EP opened                                  */
		p_temp_ep_desc =
			p_ehci->HeadIsocEPDesc; /* Retrieve the 1st Isoc EP                                     */
		while (p_temp_ep_desc
			       ->NxtEPDesc) { /* Find the end of the Isoc EP queue                            */
			p_temp_ep_desc = p_temp_ep_desc->NxtEPDesc;
		}
		p_temp_ep_desc->NxtEPDesc =
			p_ep_desc; /* Insert the new Isoc EP at the end of the queue               */
	}

	p_ep_desc->EPPtr = p_ep;
	p_ep->arg_ptr = (void *)p_ep_desc;
	p_ep_desc->FrameInterval =
		1u; /* Add the Isoc EP to the 1 ms list                             */

	key = irq_lock();
	err = EHCI_BW_Get(p_hc_drv, p_ep, (void *)p_ep_desc);

	if (err != 0) {
		// Mem_PoolBlkFree(       &HC_QHPool,
		//                 (void *)p_ep_desc,
		//                        &err_lib);
		k_free(p_ep_desc);

		p_ep->arg_ptr = NULL;

		irq_unlock(key);
		return (err);
	}

	EHCI_BW_Update(p_hc_drv, p_ep, (void *)p_ep_desc, true);
	irq_unlock(key);

	return (0);
}
#endif

/*
*********************************************************************************************************
*                                        EHCI_AsyncEP_Close()
*
* Description : Close the endpoint by unlinking the EHCI queue head
*
* Argument(s) : p_hc_drv     Pointer to host controller driver structure.
*
*               p_ep         Pointer to endpoint structure
*
* Return(s)   : USBH_ERR_NONE    If endpoint was closed
*
* Note(s)     : (1) Interrupt on Async Advance Doorbell bit in the USBCMD register allows software
*                   to inform the host controller that something has been removed from its asynchronous
*                   schedule. For more details about the doorbell mechanism, see section
*                   4.8.2 Removing Queue Heads from Asynchronous Schedule (EHCI spec document).
*********************************************************************************************************
*/

static int EHCI_AsyncEP_Close(struct usbh_hc_drv *p_hc_drv,
			      struct usbh_ep *p_ep)
{
	EHCI_DEV *p_ehci;
	EHCI_QH *p_qh_to_remove;
	EHCI_QH *p_temp_qh;
	uint32_t qh_hor_link_ptr_temp;
	uint32_t qh_bus_addr;
	uint32_t async_qh_head_bus_addr;
	uint32_t retry;
	int err;

	p_ehci = (EHCI_DEV *)p_hc_drv->data_ptr;
	/* ------------- (1) SEARCH QH TO REMOVE -------------- */
	/* Retrieve the QH associated with this EP.             */
	p_qh_to_remove = (EHCI_QH *)p_ep->arg_ptr;
	qh_bus_addr = (uint32_t)((void *)p_qh_to_remove);
	/* Retrieve the QH at the head of the Async Schedule.   */
	p_temp_qh = p_ehci->AsyncQHHead;
	async_qh_head_bus_addr = (uint32_t)((void *)p_temp_qh);

	DCACHE_InvalidateByRange((uint32_t)p_qh_to_remove, sizeof(EHCI_QH));
	DCACHE_InvalidateByRange((uint32_t)p_temp_qh, sizeof(EHCI_QH));

	/* Mask Typ bits-field and T-bit.                       */
	qh_hor_link_ptr_temp = p_temp_qh->QHHorLinkPtr & 0xFFFFFFE0;
	/* Find QH which references the QH to remove.           */
	while ((qh_hor_link_ptr_temp != (uint32_t)qh_bus_addr) &&
	       (qh_hor_link_ptr_temp != (uint32_t)async_qh_head_bus_addr)) {
		p_temp_qh = (EHCI_QH *)((qh_hor_link_ptr_temp));

		DCACHE_InvalidateByRange((uint32_t)p_temp_qh, sizeof(EHCI_QH));

		qh_hor_link_ptr_temp = p_temp_qh->QHHorLinkPtr & 0xFFFFFFE0;
	}

	if (qh_hor_link_ptr_temp == async_qh_head_bus_addr) {
		return (-EAGAIN); /* The QH to remove was not found in the Async Schedule.*/
	}

	/* --------- (2) REMOVE QH FROM ASYNC LIST ------------ */
	USBCMD &=
		~EHCI_USBCMD_RD_ASE; /* Disable the Async list processing.                   */

	retry = 100u;
	while ((USBSTATUS & EHCI_USBSTS_RD_ASS) !=
	       0u) { /* Wait until the async list processing is disabled     */
		retry--;
		if (retry == 0u) {
			err = -EAGAIN;
			return (err);
		}
		k_sleep(K_MSEC(1u));
	}

	p_temp_qh->QHHorLinkPtr =
		p_qh_to_remove
			->QHHorLinkPtr; /* Remove the QH from the Async list.                   */
	DCACHE_CleanByRange((uint32_t)p_temp_qh, sizeof(EHCI_QH));

	EHCI_QTDRemove(
		p_hc_drv,
		p_qh_to_remove); /* Remove all QTDs attached to QH to remove.            */
	/* Free the QH just removed.                            */
	// Mem_PoolBlkFree(       &HC_QHPool,
	//                 (void *)p_qh_to_remove,
	//                        &err_lib);
	k_free(p_qh_to_remove);
	p_ep->arg_ptr = NULL;

	USBCMD |=
		EHCI_USBCMD_WR_ASE; /* Enable async list processing                         */

	retry = 100u;
	while ((USBSTATUS & EHCI_USBSTS_RD_ASS) ==
	       0u) { /* Wait until async schedule is enabled.                */
		retry--;
		if (retry == 0u) {
			err = -EAGAIN;
			return (err);
		}
		k_sleep(K_MSEC(1u));
	}

	USBCMD |=
		EHCI_USBCMD_WR_IOAAD; /* Ring the doorbell. See Note #1.                      */

	err = 0;
	return (err);
}

/*
*********************************************************************************************************
*                                         EHCI_IntrEP_Close()
*
* Description : Close the endpoint by unlinking the EHCI queue head
*
* Argument(s) : p_hc_drv     Pointer to host controller driver structure.
*
*               p_ep         Pointer to endpoint structure
*
* Return(s)   : USBH_ERR_NONE    If endpoint was closed
*
* Note(s)     : None
*********************************************************************************************************
*/

#if (USBH_EHCI_CFG_PERIODIC_EN == DEF_ENABLED)
static int EHCI_IntrEP_Close(struct usbh_hc_drv *p_hc_drv, struct usbh_ep *p_ep)
{
	EHCI_DEV *p_ehci;
	EHCI_QH *p_qh_to_remove;
	EHCI_QH *p_parent_qh;
	uint8_t bw_start_frame;
	int err;
	EHCI_INTR_INFO *p_intr_info_to_remove;
	EHCI_INTR_INFO *p_prev_intr_info = 0;

	err = 0;
	p_ehci = (EHCI_DEV *)p_hc_drv->data_ptr;
	p_qh_to_remove = (EHCI_QH *)p_ep->arg_ptr;

	DCACHE_InvalidateByRange((uint32_t)p_qh_to_remove, sizeof(EHCI_QH));

	bw_start_frame = p_qh_to_remove->BWStartFrame;
	p_parent_qh = p_ehci->QHLists[bw_start_frame];
	p_parent_qh = (EHCI_QH *)p_parent_qh;

	DCACHE_InvalidateByRange((uint32_t)p_parent_qh, sizeof(EHCI_QH));

	while (((p_parent_qh->QHHorLinkPtr & 0x01u) == 0u) &&
	       ((p_parent_qh->QHHorLinkPtr & 0xFFFFFFE0u) !=
		(uint32_t)p_qh_to_remove)) {
		p_parent_qh =
			(EHCI_QH *)(p_parent_qh->QHHorLinkPtr & 0xFFFFFFE0u);

		DCACHE_InvalidateByRange((uint32_t)p_parent_qh,
					 sizeof(EHCI_QH));
	}

	if ((p_parent_qh->QHHorLinkPtr & 0x01u) != 0u) {
		err = -EAGAIN;
	} else {
		p_parent_qh->QHHorLinkPtr = p_qh_to_remove->QHHorLinkPtr;
		DCACHE_CleanByRange((uint32_t)p_parent_qh, sizeof(EHCI_QH));
	}

	EHCI_QTDRemove(
		p_hc_drv,
		p_qh_to_remove); /* Remove all QTDs attached to this QH                  */

	EHCI_BW_Update(
		p_hc_drv, /* Update bandwidth allocation.                         */
		p_ep, (void *)p_qh_to_remove, false);

	// Mem_PoolBlkFree(       &HC_QHPool,
	//                 (void *)p_qh_to_remove,
	//                        &err_lib);
	k_free(p_qh_to_remove);

	p_intr_info_to_remove =
		p_ehci->HeadIntrInfo; /* Find Intr info struct to remove from queue.          */
	/* Search until end of the Intr info queue.             */
	while (p_intr_info_to_remove != NULL) {
		if ((p_intr_info_to_remove->IntrPlaceholderIx ==
		     bw_start_frame) &&
		    (p_intr_info_to_remove->EpPtr == p_ep)) {
			break;
		}
		p_prev_intr_info =
			p_intr_info_to_remove; /* Keep ref to prev Intr info struct.                   */
		/* Get next Intr info struct.                           */
		p_intr_info_to_remove = p_intr_info_to_remove->NxtIntrInfo;
	}

	if (p_intr_info_to_remove != NULL) {
		if (p_intr_info_to_remove == p_ehci->HeadIntrInfo) {
			p_ehci->HeadIntrInfo =
				p_ehci->HeadIntrInfo->NxtIntrInfo;
		} else {
			p_prev_intr_info->NxtIntrInfo =
				p_intr_info_to_remove->NxtIntrInfo;
		}

		// Mem_PoolBlkFree(       &IntrInfoPool,
		//                 (void *)p_intr_info_to_remove,
		//                        &err_lib);
		k_free(p_intr_info_to_remove);
	}

	return (err);
}
#endif

/*
*********************************************************************************************************
*                                         EHCI_IsocEP_Close()
*
* Description : Close the endpoint by unlinking the EHCI queue head
*
* Argument(s) : p_hc_drv     Pointer to host controller driver structure.
*
*               p_ep         Pointer to endpoint structure
*
* Return(s)   : USBH_ERR_NONE    If endpoint was closed
*
* Note(s)     : None
*********************************************************************************************************
*/

#if (USBH_EHCI_CFG_PERIODIC_EN == DEF_ENABLED)
static int EHCI_IsocEP_Close(struct usbh_hc_drv *p_hc_drv, struct usbh_ep *p_ep)
{
	EHCI_ISOC_EP_DESC *p_ep_desc_to_close;
	EHCI_ISOC_EP_DESC *p_temp_ep_desc;
	EHCI_ITD *p_itd;
	EHCI_SITD *p_sitd;
	EHCI_DEV *p_ehci;
	uint8_t dev_addr;
	uint8_t ep_addr;
	bool isoc_ep_desc_found;
	struct usbh_urb *p_urb;
	EHCI_ISOC_EP_URB *p_urb_info;

	p_ehci = (EHCI_DEV *)p_hc_drv->data_ptr;
	p_ep_desc_to_close = (EHCI_ISOC_EP_DESC *)p_ep->arg_ptr;
	/* (1) Find the Isoc EP to close in the EHCI Isoc queue */
	p_temp_ep_desc = p_ehci->HeadIsocEPDesc;
	if (p_temp_ep_desc ==
	    p_ep_desc_to_close) { /* If the Isoc EP to close is the head of the queue...  */
		p_ehci->HeadIsocEPDesc =
			p_ep_desc_to_close
				->NxtEPDesc; /* Remove the Isoc EP to close from the Isoc queue      */

	} else { /* Search into the Isoc queue                           */
		isoc_ep_desc_found = false;
		while (p_temp_ep_desc->NxtEPDesc !=
		       0) { /* Search until the end of the Isoc queue               */
			if (p_temp_ep_desc->NxtEPDesc == p_ep_desc_to_close) {
				isoc_ep_desc_found = true;
				break;
			}
			p_temp_ep_desc =
				p_temp_ep_desc
					->NxtEPDesc; /* Get the next Isoc desc                               */
		}

		if (isoc_ep_desc_found == false) {
			return (-ENOENT);
		}
		/* Remove the Isoc EP to close from the Isoc queue      */
		p_temp_ep_desc->NxtEPDesc = p_ep_desc_to_close->NxtEPDesc;
	}

	/* (2) Clear any iTD or siTD scheduled for this EP      */
	p_urb = &p_ep->urb;

	while (p_urb !=
	       0) { /* Browse every URB scheduled for this EP               */

		if (p_urb->arg_ptr != 0) {
			p_urb_info = (EHCI_ISOC_EP_URB *)p_urb->arg_ptr;

			if (p_ep->dev_spd == USBH_HIGH_SPEED) {
				p_itd = (EHCI_ITD *)p_urb_info->iTD_Addr;

				DCACHE_InvalidateByRange((uint32_t)p_itd,
							 sizeof(EHCI_ITD));
				dev_addr = p_itd->ITDBufPagePtrList[0] &
					   0x0000007Fu;
				ep_addr = (p_itd->ITDBufPagePtrList[0] &
					   0x00000F00u) >>
					  8u;

				EHCI_ITDDone(
					p_hc_drv, /* Unschedule the iTD(s) of this URB                    */
					p_ep_desc_to_close, dev_addr, ep_addr,
					p_urb);
			} else {
				p_sitd = (EHCI_SITD *)p_urb_info->iTD_Addr;

				DCACHE_InvalidateByRange((uint32_t)p_sitd,
							 sizeof(EHCI_SITD));
				dev_addr =
					p_sitd->SITDEpCapChar[0] & 0x0000007Fu;
				ep_addr = (p_sitd->SITDEpCapChar[0] &
					   0x00000F00u) >>
					  8u;

				EHCI_SITDDone(
					p_hc_drv, /* Unschedule the siTD(s) of this URB                   */
					p_ep_desc_to_close, dev_addr, ep_addr,
					p_urb);
			}
		}
		p_urb = p_urb->async_urb_nxt_ptr; /* Get the next URB scheduled for this EP               */
	}

	EHCI_BW_Update(p_hc_drv, p_ep, (void *)p_ep_desc_to_close, false);

	// Mem_PoolBlkFree(       &HC_Isoc_EP_DescPool,
	//                 (void *)p_ep_desc_to_close,
	//                        &err_lib);
	k_free(p_ep_desc_to_close);

	return (0);
}
#endif

/*
*********************************************************************************************************
*                                        EHCI_QTDListPrepare()
*
* Description : Prepare a QTD list and fills the elements of each QTD with appropriate values.
*
* Argument(s) : p_hc_drv     Pointer to host controller driver structure.
*
*               p_ep         Pointer to endpoint structure
*
*               p_urb        Pointer to URB structure.
*
*               p_buf        Pointer to the buffer.
*
*               buf_len      Number of bytes to transfer.
*
*               p_err        Pointer to variable that will receive the return error code from this function
*                                USBH_ERR_NONE          Endpoint closed successfully.
*                                Specific error code    otherwise.
*
* Return(s)   : Pointer to the head of the QTD list.
*
* Note(s)     : qTD structure and fields. iTD is a 32-bytes structure which must be aligned on a 32-byte
*               boundary. For more details, see section 3.5 (EHCI spec)
*
*               ---------------------------------------------------------------------------------------
*               |31 30 29 28 27 26 25 24 23 22 21 20 19 18 17 16 15 14 13 12 11 10 9 8 7 6 5 4 3 2 1 0|
*               ---------------------------------------------------------------------------------------
*               |        Next qTD Pointer                                                   |  0    |T|
*               ---------------------------------------------------------------------------------------
*               |        Alternate Next qTD Pointer                                         |  0    |T|
*               ---------------------------------------------------------------------------------------
*               |dt| Total Bytes to Transfer                    |io| C_Page |Cerr |PID|    Status     |
*               ---------------------------------------------------------------------------------------
*               |  Buffer Pointer (page 0)                                  |    Current Offset       |
*               ---------------------------------------------------------------------------------------
*               |  Buffer Pointer (page 1)                                  |    Reserved             |
*               ---------------------------------------------------------------------------------------
*               |  Buffer Pointer (page 2)                                  |    Reserved             |
*               ---------------------------------------------------------------------------------------
*               |  Buffer Pointer (page 3)                                  |    Reserved             |
*               ---------------------------------------------------------------------------------------
*               |  Buffer Pointer (page 4)                                  |    Reserved             |
*               ---------------------------------------------------------------------------------------
*
*               (1) Section 3.5 (EHCI spec), one qTD structure is used to transfer up to 20480 (5*4096) bytes.
*
*               (2) Alternate Next qTD Pointer (2nd DWord of qTD) is used to support hardware-only advance
*                   of the data stream to the next client buffer on short packet. To be more explicit the
*                   host controller will always use this pointer when the current qTD is retired due
*                   to short packet. Alternate Next qTD Pointer applies to IN direction only.
*                   See Section 3.5.2 (EHCI spec) for more details about Alternate Next qTD Pointer.
*
*               (3) See section 4.10.6 for more details about the Buffer Pointer List use when the buffer
*                   associated with the transfer spans more than one physical page.
*********************************************************************************************************
*/

static EHCI_QTD *EHCI_QTDListPrepare(struct usbh_hc_drv *p_hc_drv,
				     struct usbh_ep *p_ep,
				     struct usbh_urb *p_urb, uint8_t *p_buf,
				     uint32_t buf_len, int *p_err)
{
	EHCI_DEV *p_ehci;
	EHCI_QTD *p_new_qtd;
	EHCI_QTD *p_head_qtd;
	EHCI_QTD *p_temp_qtd;
	uint32_t qtd_token;
	uint8_t ep_type;
	uint16_t ep_max_pkt_size;
	uint32_t qtd_toggle;
	uint32_t token;
	uint8_t *p_buf_page;
	uint8_t i;
	bool rtn_flag;
	uint32_t qtd_totbytes;
	uint32_t rem;
	uint32_t buf_page_max;
	uint32_t buf_page;

	p_ehci = (EHCI_DEV *)p_hc_drv->data_ptr;
	qtd_toggle = 0u;
	token = 0u;
	ep_type = usbh_ep_type_get(p_ep);
	ep_max_pkt_size = usbh_ep_max_pkt_size_get(p_ep);

	if (ep_type == USBH_EP_TYPE_CTRL) {
		if (p_urb->token == USBH_TOKEN_SETUP) {
			token = DWORD3_QTD_PIDC_SETUP;
		} else {
			qtd_toggle =
				O_QTD_DT; /* Data toggle is 1 for Data and Status phases          */
		}
	}

	if (p_urb->token ==
	    USBH_TOKEN_OUT) { /* Set the direction of the transfer                    */
		token = DWORD3_QTD_PIDC_OUT;
	} else if (p_urb->token == USBH_TOKEN_IN) {
		token = DWORD3_QTD_PIDC_IN;
	} else {
		/* Empty Else Statement                                 */
	}

	p_buf = (uint8_t *)((void *)p_buf);
	p_buf_page = p_buf;
	p_new_qtd = NULL;
	p_head_qtd = NULL;
	rtn_flag = false;

	while ((p_buf_page <
		(p_buf +
		 buf_len)) || /* Initialize one or several qTDs for total xfer's size */
	       (buf_len ==
		0u)) { /* See Note #1                                          */
		/* Get a qTD structure                                  */
		// p_temp_qtd = (EHCI_QTD *)Mem_PoolBlkGet(&HC_QTDPool,
		//                                          sizeof(EHCI_QTD),
		//                                         &err_lib);
		p_temp_qtd = k_mem_pool_malloc(&HC_QTDPool, sizeof(EHCI_QTD));

		if (p_temp_qtd == NULL) {
			*p_err = -ENOMEM;
			return (NULL);
		}

		EHCI_QTD_Clr(
			p_temp_qtd); /* Clear every field of the qTD to have a known state   */
		DCACHE_CleanByRange((uint32_t)p_temp_qtd, sizeof(EHCI_QTD));

		if (p_new_qtd) { /* Next qTD.                                            */
			/* Set Next qTD Pointer.                                */
			p_new_qtd->QTDNxtPtr = (uint32_t)(p_temp_qtd);
			p_new_qtd->QTDAltNxtPtr =
				0x00000001u; /* Set Alternate Next qTD Pointer (see Note #2).        */
			DCACHE_CleanByRange((uint32_t)p_new_qtd,
					    sizeof(EHCI_QTD));
			p_new_qtd =
				p_temp_qtd; /* qTD struct gotten.                                   */

		} else { /* 1st qTD.                                             */
			p_head_qtd = p_temp_qtd;
			p_new_qtd = p_temp_qtd;
		}

		/* Init Buffer Pointer (Page 0) + Current Offset        */
		p_new_qtd->QTDBufPagePtrList[0] = (uint32_t)p_buf_page;

		buf_page_max =
			(((uint32_t)p_buf_page + 0x1000u) & 0xFFFFF000u) -
			(uint32_t)p_buf_page;
		buf_page = (p_buf + buf_len) - p_buf_page;
		qtd_totbytes = MIN(buf_page, buf_page_max);

		/* Init Buffer Pointer List if buffer spans more ...    */
		/* ... than one physical page (see Note #3).            */
		for (i = 1u; i <= 4u;
		     i++) { /* Init Buffer Pointer (Page 1 to 4)                    */
			/* Find the next closest 4K-page boundary ahead.        */
			p_buf_page =
				(uint8_t *)(((uint32_t)p_buf_page + 0x1000u) &
					    0xFFFFF000u);

			if (p_buf_page <
			    (p_buf +
			     buf_len)) { /* If buffer spans a new 4K-page boundary.              */
				/* Set page ptr to ref start of the subsequent 4K page. */
				p_new_qtd->QTDBufPagePtrList[i] =
					(uint32_t)p_buf_page;
				qtd_totbytes +=
					MIN(((p_buf + buf_len) - p_buf_page),
					    0x1000);

			} else { /* All the transfer size has been described...          */
				rtn_flag = true;
				break; /* ... quit the loop.                                   */
			}
		}

		if (rtn_flag == true) {
			/* Init the qTD token                                   */
			qtd_token =
				QTD_TOKEN_STS(
					1
					<< 7u) | /* Status field. Active bit to '1'.                     */
				QTD_TOKEN_PID(
					token) | /* PID code                                             */
				QTD_TOKEN_CERR(
					3u) | /* Error Counter                                        */
				QTD_TOKEN_CP(
					0u) | /* Current Page                                         */
				QTD_TOKEN_TBTT(
					qtd_totbytes) | /* Total Bytes to Transfer                              */
				QTD_TOKEN_DT(
					qtd_toggle); /* Data Toggle                                          */

			if ((p_ep->dev_spd == USBH_HIGH_SPEED) &&
			    (p_urb->token == USBH_TOKEN_OUT)) {
				qtd_token |= QTD_TOKEN_STS(1u);
			}
			p_new_qtd->QTDToken =
				qtd_token; /* Prepare qTD with the parameters                      */
			break;

		} else { /* The transfer's size requires more qTDs, update the...*/
			/* ... size remaining to describe by qTD(s).            */
			p_buf_page += 0x1000;

			if (p_buf_page < (p_buf + buf_len)) {
				rem = ((uint32_t)p_buf_page -
				       p_new_qtd->QTDBufPagePtrList[0]) %
				      ep_max_pkt_size;
				qtd_totbytes -= rem;
				p_buf_page -= rem;
			}
		}
		/* Init the qTD token                                   */
		qtd_token =
			QTD_TOKEN_STS(
				1u
				<< 7u) | /* Status field. Active bit to '1'.                     */
			QTD_TOKEN_PID(
				token) | /* PID code                                             */
			QTD_TOKEN_CERR(
				3u) | /* Error Counter                                        */
			QTD_TOKEN_CP(
				0u) | /* Current Page                                         */
			QTD_TOKEN_TBTT(
				qtd_totbytes) | /* Total Bytes to Transfer                              */
			QTD_TOKEN_DT(
				qtd_toggle); /* Data Toggle                                          */

		p_new_qtd->QTDToken =
			qtd_token; /* Prepare qTD with the parameters                      */
		DCACHE_CleanByRange((uint32_t)p_new_qtd, sizeof(EHCI_QTD));
	}

	if (p_new_qtd == NULL) {
		*p_err = -ENOMEM;
		return (NULL);
	}
	/* Finalize init for last qTD.                          */
	p_new_qtd->QTDToken |= QTD_TOKEN_IOC(
		1u); /* Interrupt On Completion for last qTD                 */
	p_new_qtd->QTDNxtPtr |= QTD_N_QTD_PTR_T(
		1u); /* Set Terminate bit                                    */
	p_new_qtd->QTDAltNxtPtr |= QTD_ALT_QTD_PTR_T(1u);
	DCACHE_CleanByRange((uint32_t)p_new_qtd, sizeof(EHCI_QTD));

	*p_err = 0;

	return (p_head_qtd);
}

/*
*********************************************************************************************************
*                                        EHCI_SITDListPrepare()
*
* Description : Prepare a QTD list and fills the elements of each QTD with appropriate values.
*
* Argument(s) : p_hc_drv      Pointer to host controller driver structure.
*
*               p_dev         Pointer to device structure
*
*               p_ep          Pointer to endpoint structure
*
*               p_ep_desc     Pointer to endpoint descriptor
*
*               p_urb         Pointer to URB structure.
*
*               p_buf         Pointer to the buffer.
*
*
* Return(s)   : Pointer to the head of the QTD list.
*
* Note(s)     : siTD structure and fields. siTD is a 28-bytes structure which must be aligned on a 32-byte
*               boundary. For more details, see section 3.4 (EHCI spec)
*
*               ---------------------------------------------------------------------------------------
*               |31 30 29 28 27 26 25 24 23 22 21 20 19 18 17 16 15 14 13 12 11 10 9 8 7 6 5 4 3 2 1 0|
*               ---------------------------------------------------------------------------------------
*               |        Next qTD Pointer                                                   | 0 |Typ|T|
*               ---------------------------------------------------------------------------------------
*               |I/O|   Port Number     |R |     Hub Addr       |     R     | EndPt   |R| Device Addr |
*               ---------------------------------------------------------------------------------------
*               |                  Reserved                     |   uFrame C-mask     | uFrame S-mask |
*               ---------------------------------------------------------------------------------------
*               |ioc|P| Reserved |    Total Bytes to Transfer   | uFrame C-prog-mask  |    Status     |
*               ---------------------------------------------------------------------------------------
*               |                   Buffer Pointer (Page 0)                 |     Current Offset      |
*               ---------------------------------------------------------------------------------------
*               |                   Buffer Pointer (Page 1)                 |   Reserved    |TP |T-cnt|
*               ---------------------------------------------------------------------------------------
*               |        Back Pointer                                                       | 0     |T|
*               ---------------------------------------------------------------------------------------
*
*               (1) For a split transaction, any isochronous OUT full-speed transaction is subdivided into
*                   multiple start-splits, each with a data payload of 188 bytes or less.
*                   For more information, See USB 2.0 specfication:
*                   section 11.18.1 Best Case Full-Speed Budget
*                   section 11.18.4 Host Split Transaction Scheduling Requirements
*                   section 11.21.3 Isochronous OUT Sequencing
*
*                   Table 4-14 (EHCI spec) gives details about the initial conditions for OUT siTD's
*                   TP and T-count Fields
*********************************************************************************************************
*/

#if (USBH_EHCI_CFG_PERIODIC_EN == DEF_ENABLED)
static int EHCI_SITDListPrepare(struct usbh_hc_drv *p_hc_drv,
				struct usbh_dev *p_dev, struct usbh_ep *p_ep,
				EHCI_ISOC_EP_DESC *p_ep_desc,
				struct usbh_urb *p_urb, uint8_t *p_buf)
{
	EHCI_DEV *p_ehci;
	EHCI_SITD *p_new_sitd;
	uint32_t buf_page;
	uint8_t ep_nbr;
	uint32_t token;
	uint32_t i;
	uint16_t frame_len; /* Number of bytes to be transferred in this frame       */
	uint32_t frame_nbr; /* Frame number where this SITD to be placed             */
	uint16_t frame_interval; /* Interval of this endpoint                             */
	uint32_t *p_hw_desc;
	uint8_t t_count;
	EHCI_ISOC_EP_URB *p_urb_info;
	int err;
	int key;

	p_ehci = (EHCI_DEV *)p_hc_drv->data_ptr;
	ep_nbr = usbh_ep_log_nbr_get(p_ep);

	key = irq_lock();

	if (p_urb->token ==
	    USBH_TOKEN_OUT) { /* Set the EP direction                                 */
		token = DWORD1_SITD_IO_OUT;
	} else if (p_urb->token == USBH_TOKEN_IN) {
		token = DWORD1_SITD_IO_IN;
	} else {
		token = (uint32_t)0;
	}

	p_buf = (uint8_t *)((void *)p_buf);
	buf_page = (uint32_t)p_buf;
	frame_interval =
		p_ep_desc->FrameInterval; /* siTD belongs to the 1 ms Frame list                  */

	for (i = 0u; i < p_urb->isoc_desc_ptr->nbr_frm;
	     i++) { /* Initialize the frame error array                     */
		p_urb->isoc_desc_ptr->frm_err[i] = 0;
	}

	if (p_urb->isoc_desc_ptr->start_frm == 0u) {
		frame_nbr =
			EHCI_FrameNbrGet(p_hc_drv, &err) +
			8u; /* Start this xfer immediately after current frame nbr  */
	} else {
		frame_nbr =
			p_urb->isoc_desc_ptr->start_frm +
			8u; /* Start this xfer at the caller specified frame number */
	}

	frame_nbr %=
		256u; /* Keep the Periodic Frame List index between 0 and 255 */

	p_ep_desc->AppStartFrame =
		frame_nbr; /* Save the index                                       */
	p_ep_desc->NbrFrame = p_urb->isoc_desc_ptr->nbr_frm;

	// p_urb_info = (EHCI_ISOC_EP_URB *)Mem_PoolBlkGet(&HC_Isoc_EP_URBPool,
	//                                                  sizeof(EHCI_ISOC_EP_URB),
	//                                                 &err_lib);
	p_urb_info = k_mem_pool_malloc(&HC_Isoc_EP_URBPool,
				       sizeof(EHCI_ISOC_EP_URB));
	if (p_urb_info == NULL) {
		irq_unlock(key);
		return (-ENOMEM);
	}

	p_urb_info->AppStartFrame = p_ep_desc->AppStartFrame;
	p_urb_info->NbrFrame = p_ep_desc->NbrFrame;

	/* Prepare 1 or more siTD for this Isoc transfer        */
	for (i = 0u; i < p_urb->isoc_desc_ptr->nbr_frm; i++) {
		frame_nbr %=
			256u; /* Keep the Periodic Frame List index between 0 and 255 */

		/* Get a new siTD struct                                */
		// p_new_sitd = (EHCI_SITD *)Mem_PoolBlkGet(&HC_ITDPool,
		//                                           sizeof(EHCI_SITD),
		//                                          &err_lib)
		p_new_sitd = k_mem_pool_malloc(&HC_ITDPool, sizeof(EHCI_SITD));
		if (p_new_sitd == NULL) {
			irq_unlock(key);
			return (-ENOMEM);
		}

		EHCI_SITD_Clr(p_new_sitd);
		/* Init siTD EP Capabilities/Characteristics            */
		p_new_sitd->SITDEpCapChar[0] =
			(SITD_EPCHAR_DIR(token) |
			 SITD_EPCHAR_PN(p_dev->port_nbr) |
			 SITD_EPCHAR_HUBADD(p_dev->hub_dev_ptr->dev_addr) |
			 SITD_EPCHAR_ENDPT(ep_nbr) |
			 SITD_EPCHAR_DEVADD(p_ep->dev_addr));

		if (p_urb->token ==
		    USBH_TOKEN_IN) { /* Only isochronous in transfers have C-Mask            */
			p_new_sitd->SITDEpCapChar[1] = p_ep_desc->SMask;
			p_new_sitd->SITDEpCapChar[1] |=
				((uint32_t)p_ep_desc->CMask) << 8u;
		}

		frame_len =
			p_urb->isoc_desc_ptr->frm_len
				[i]; /* Size of transaction for this frame                   */
		p_new_sitd->SITDStsCtrl =
			(frame_len
			 << 16u) | /* Total bytes to transfer                              */
			O_SITD_STS_ACTIVE; /* Enable execution of Isoc split transaction by HC     */
		/* Set pointer to buffer data (Page 0)                  */
		p_new_sitd->SITDBufPagePtrList[0] = (uint32_t)buf_page;
		/* If buf data crosses 4K page, set Buffer Ptr (Page 1) */
		if (((buf_page + frame_len) & 0xFFFFF000u) ==
		    ((buf_page + 0x1000u) & 0xFFFFF000u)) {
			p_new_sitd->SITDBufPagePtrList[1] =
				(buf_page + frame_len) & 0xFFFFF000u;
		}

		if (p_urb->token ==
		    USBH_TOKEN_OUT) { /* For Isoc OUT, set TP and T-count fields. See Note#1  */
			if (frame_len <=
			    188) { /* If data payload for this transaction <= 188 bytes    */
				/* Only 1 SSPLIT required. Mark it with an ALL          */
				p_new_sitd->SITDBufPagePtrList[1] |=
					SITD_BUGPAGE1_TP(DWORD6_SITD_TP_ALL);
			} else {
				/* Several SSPLITs required. Mark the 1st one with BEGIN*/
				p_new_sitd->SITDBufPagePtrList[1] |=
					SITD_BUGPAGE1_TP(DWORD6_SITD_TP_BEGIN);
			}

			t_count =
				p_ep_desc->TCnt; /* Number of SSPLITs for this OUT transaction           */
			if (t_count <
			    7) { /* T-Count must NOT be larger than 6                    */
				p_new_sitd->SITDBufPagePtrList[1] |=
					SITD_BUGPAGE1_TCOUNT(t_count);
			}

			p_new_sitd->SITDEpCapChar[1] |=
				p_ep_desc->SMask; /* Set every bit required for the nbr of SSPLITs        */
		}

		if (i ==
		    (p_urb->isoc_desc_ptr->nbr_frm -
		     1u)) { /* If this is the last siTD for this Isoc xfer          */
			p_new_sitd->SITDStsCtrl |= (uint32_t)SITD_STSCTRL_IOC(
				1); /* Interrupt On Completion for last siTD        */
			p_ep_desc->TDTailPtr = (void *)p_new_sitd;
			p_urb_info->iTD_Addr = (uint32_t)
				p_new_sitd; /* Save the last siD associated with this URB           */
			p_urb->arg_ptr = (void *)p_urb_info;
		}

		/* Get the data struct at index 'frame_nbr' in PFL      */
		p_hw_desc = (uint32_t *)(p_ehci->PeriodicListBase[frame_nbr] &
					 0xFFFFFFE0u);
		DCACHE_InvalidateByRange((uint32_t)p_hw_desc, sizeof(uint32_t));
		/* Find the last siTD at this entry position            */
		while ((*p_hw_desc & 0x06u) != HOR_LNK_PTR_TYP(DWORD1_TYP_QH)) {
			p_hw_desc = (uint32_t *)(*p_hw_desc & 0xFFFFFFE0u);
			DCACHE_InvalidateByRange((uint32_t)p_hw_desc,
						 sizeof(uint32_t));
		}

		p_new_sitd->SITDNxtLinkPtr =
			*p_hw_desc; /* Store the gotten data struct in siTD Next Link Ptr   */
		DCACHE_CleanByRange((uint32_t)p_new_sitd, sizeof(EHCI_SITD));

		*p_hw_desc = HOR_LNK_PTR_T(
			DWORD1_T_INVALID); /* Invalidate siTD Next Link Ptr so HC ignores it       */
		/* Insert the new siTD after the gotten data struct     */
		*p_hw_desc |= (uint32_t)((void *)p_new_sitd) |
			      HOR_LNK_PTR_TYP(DWORD1_TYP_SITD);

		*p_hw_desc &=
			0xFFFFFFFEu; /* Validate Next Link Ptr pointed to siTD to insert     */
		DCACHE_CleanByRange((uint32_t)p_hw_desc, sizeof(uint32_t));

		buf_page += frame_len;
		frame_nbr += frame_interval;
	}

	irq_unlock(key);

	return (0);
}
#endif

/*
*********************************************************************************************************
*                                        EHCI_ITDListPrepare()
*
* Description : Prepare a ITD list and fills the elements of each ITD with appropriate values.
*
* Argument(s) : p_hc_drv      Pointer to host controller driver structure.
*
*               p_ep          Pointer to endpoint structure
*
*               p_ep_desc     Pointer to endpoint descriptor
*
*               p_urb         Pointer to URB structure.
*
*               p_buf         Pointer to the buffer.
*
*               buf_len       Number of bytes to transfer.
*
* Return(s)   : Pointer to the head of the ITD list.
*
* Note(s)     : iTD structure and fields. iTD is a 64-bytes structure which must be aligned on a 32-byte
*               boundary. For more details, see section 3.3 (EHCI spec)
*
*               ---------------------------------------------------------------------------------------
*               |31 30 29 28 27 26 25 24 23 22 21 20 19 18 17 16 15 14 13 12 11 10 9 8 7 6 5 4 3 2 1 0|
*               ---------------------------------------------------------------------------------------
*               |        Next qTD Pointer                                                   | 0 |Typ|T|
*               ---------------------------------------------------------------------------------------
*               |  Status   |       Transaction 0 Length        |io|  PG    |   Transaction 0 Offset  |
*               ---------------------------------------------------------------------------------------
*               |  Status   |       Transaction 1 Length        |io|  PG    |   Transaction 1 Offset  |
*               ---------------------------------------------------------------------------------------
*               |  Status   |       Transaction 2 Length        |io|  PG    |   Transaction 2 Offset  |
*               ---------------------------------------------------------------------------------------
*               |  Status   |       Transaction 3 Length        |io|  PG    |   Transaction 3 Offset  |
*               ---------------------------------------------------------------------------------------
*               |  Status   |       Transaction 4 Length        |io|  PG    |   Transaction 4 Offset  |
*               ---------------------------------------------------------------------------------------
*               |  Status   |       Transaction 5 Length        |io|  PG    |   Transaction 5 Offset  |
*               ---------------------------------------------------------------------------------------
*               |  Status   |       Transaction 6 Length        |io|  PG    |   Transaction 6 Offset  |
*               ---------------------------------------------------------------------------------------
*               |  Status   |       Transaction 7 Length        |io|  PG    |   Transaction 7 Offset  |
*               ---------------------------------------------------------------------------------------
*               |                   Buffer Pointer (Page 0)                 | EP Addr |R| Device Addr |
*               ---------------------------------------------------------------------------------------
*               |                   Buffer Pointer (Page 1)                 |I/O| Maximum Packet Size |
*               ---------------------------------------------------------------------------------------
*               |                   Buffer Pointer (Page 2)                 |       Reserved      |Mlt|
*               ---------------------------------------------------------------------------------------
*               |                   Buffer Pointer (Page 3)                 |       Reserved          |
*               ---------------------------------------------------------------------------------------
*               |                   Buffer Pointer (Page 4)                 |       Reserved          |
*               ---------------------------------------------------------------------------------------
*               |                   Buffer Pointer (Page 5)                 |       Reserved          |
*               ---------------------------------------------------------------------------------------
*               |                   Buffer Pointer (Page 6)                 |       Reserved          |
*               ---------------------------------------------------------------------------------------
*********************************************************************************************************
*/

#if (USBH_EHCI_CFG_PERIODIC_EN == DEF_ENABLED)
static int EHCI_ITDListPrepare(struct usbh_hc_drv *p_hc_drv,
			       struct usbh_ep *p_ep,
			       EHCI_ISOC_EP_DESC *p_ep_desc,
			       struct usbh_urb *p_urb, uint8_t *p_buf,
			       uint32_t buf_len)
{
	EHCI_DEV *p_ehci;
	EHCI_ITD *p_new_itd; /* Pointer to new iTD structure                         */
	uint32_t *p_hw_desc;
	uint8_t active_trans
		[10]; /* Array of active transactions                         */
	uint32_t array_index; /* Active Transactiona array index                      */
	uint32_t buf_page; /* Buffer Page                                          */
	uint32_t buf_ptr =
		0u; /* Buffer Pointer                                       */
	uint8_t buf_ptr_page_nbr; /* Buffer Pointer Page Number                           */
	uint32_t buf_start_addr =
		0u; /* Buffer Start Address                                 */
	uint8_t ep_nbr; /* Endpoint Number                                      */
	uint16_t ep_max_pkt_size; /* Endpoint Max Packet Size                             */
	uint32_t i; /* For loop index                                       */
	uint32_t j; /* For loop index                                       */
	uint32_t k; /* For loop index                                       */
	uint8_t ioc_bit =
		0u; /* Interrupt on Complete bit                            */
	uint16_t frame_interval; /* Interval of this endpoint                            */
	uint16_t frame_nbr; /* Frame number where this iTD is to be placed          */
	uint8_t micro_frame_nbr; /* Microframe number                                    */
	uint16_t mult_value; /* Mult value                                           */
	uint16_t nbr_of_transaction_per_uframe; /* Number of transactions per uframe from EP desc       */
	uint16_t nbr_of_transaction_per_iTD; /* Number of transactions per iTD                       */
	uint16_t nbr_of_transaction; /* Number of transactions                               */
	uint16_t nbr_of_iTDs_for_xfer; /* Number of iTDs for transfer                          */
	uint8_t page_nbr; /* Page Number                                          */
	uint32_t token; /* I/O Token                                            */
	uint8_t transaction_per_octo_mult_rem;
	uint8_t transaction_per_mult_rem;
	uint8_t transaction_shift;
	uint16_t xact_len; /* Exact Transaction Length                             */
	uint16_t xact_offset; /* Exact Transaction Offset                             */
	uint32_t xfer_remaining_len; /* Transfer Remaining Length                            */
	uint16_t max_transaction_len; /* Maximum Transaction Length                           */
	uint32_t xfer_elapsed_len; /* Transfer Elapsed Length                              */
	EHCI_ISOC_EP_URB *p_urb_info;
	int err;
	int key;

	p_ehci = (EHCI_DEV *)p_hc_drv->data_ptr;

	key = irq_lock();

	if (p_urb->token ==
	    USBH_TOKEN_OUT) { /* Determine I/O Token direction                       */
		token = DWORD1_ITD_IO_OUT;
	} else if (p_urb->token == USBH_TOKEN_IN) {
		token = DWORD1_ITD_IO_IN;
	} else {
		token = (uint32_t)0;
	}

	ep_nbr = usbh_ep_log_nbr_get(p_ep);
	ep_max_pkt_size = usbh_ep_max_pkt_size_get(p_ep);
	p_buf = (uint8_t *)((void *)p_buf);
	buf_page = (uint32_t)p_buf;
	xfer_remaining_len = buf_len;

	for (j = 0u; j < 10u; j++) {
		active_trans[j] =
			0x00u; /* Clear all values in active transaction array        */
	}

	nbr_of_transaction = p_urb->isoc_desc_ptr->nbr_frm;
	nbr_of_transaction_per_uframe = (p_ep->desc.w_max_packet_size &
					 USBH_NBR_TRANSACTION_PER_UFRAME) >>
					11u;
	mult_value = nbr_of_transaction_per_uframe +
		     1u; /* Determine the Mult field for iTD structure   */
	max_transaction_len = ep_max_pkt_size * mult_value;

	/* Determine the number of iTD for transfer            */
	transaction_per_octo_mult_rem = nbr_of_transaction % (mult_value * 8u);
	if (transaction_per_octo_mult_rem == 0) {
		nbr_of_iTDs_for_xfer = (nbr_of_transaction / (mult_value * 8u));
	} else {
		nbr_of_iTDs_for_xfer =
			(nbr_of_transaction / (mult_value * 8u)) + 1u;
	}
	/* Determine the number of transactions per iTD        */
	transaction_per_mult_rem = nbr_of_transaction % mult_value;
	if (transaction_per_mult_rem == 0) {
		nbr_of_transaction_per_iTD = nbr_of_transaction / mult_value;
	} else {
		nbr_of_transaction_per_iTD =
			(nbr_of_transaction / mult_value) + 1u;
	}

	/* Left shift nbr of trans in active transaction array */
	for (k = 0u; k < nbr_of_transaction_per_iTD; k++) {
		array_index = k / 8u;
		transaction_shift = k % 8u;
		active_trans[array_index] |= 1 << transaction_shift;
	}

	frame_interval =
		p_ep_desc->FrameInterval; /* iTD belongs to the 1 ms Frame list                   */

	if (p_urb->isoc_desc_ptr->start_frm == 0u) {
		frame_nbr =
			EHCI_FrameNbrGet(p_hc_drv, &err) +
			8u; /* Start this xfer immediately after current frame nbr  */
	} else {
		frame_nbr =
			p_urb->isoc_desc_ptr->start_frm +
			8u; /* Start this xfer at the caller specified frame number */
	}
	frame_nbr %=
		256; /* Keep the Periodic Frame List index between 0 and 255 */

	p_ep_desc->AppStartFrame = frame_nbr;
	p_ep_desc->NbrFrame = p_urb->isoc_desc_ptr->nbr_frm;

	// p_urb_info = (EHCI_ISOC_EP_URB *)Mem_PoolBlkGet(&HC_Isoc_EP_URBPool,
	//                                                  sizeof(EHCI_ISOC_EP_URB),
	//                                                 &err_lib);
	p_urb_info = k_mem_pool_malloc(&HC_Isoc_EP_URBPool,
				       sizeof(EHCI_ISOC_EP_URB));
	if (p_urb_info == NULL) {
		irq_unlock(key);
		return (-ENOMEM);
	}

	p_urb_info->AppStartFrame = p_ep_desc->AppStartFrame;
	p_urb_info->NbrFrame = p_ep_desc->NbrFrame;

	/* ----------- iTD Structure Initialization ---------- */
	/* Init each iTD that composed for the transfer        */
	for (i = 0u; i < nbr_of_iTDs_for_xfer; i++) {
		// p_new_itd = (EHCI_ITD *)Mem_PoolBlkGet(&HC_ITDPool,
		//                                         sizeof(EHCI_ITD),
		//                                        &err_lib);
		p_new_itd = k_mem_pool_malloc(&HC_ITDPool, sizeof(EHCI_ITD));
		if (p_new_itd == NULL) {
			irq_unlock(key);
			return (-ENOMEM);
		}

		EHCI_ITD_Clr(
			p_new_itd); /* Clear the new iTD structure                         */

		p_new_itd->ITDBufPagePtrList[0] =
			ITD_BUF_PG_PTR_LIST_DEVADD(p_ep->dev_addr) |
			ITD_BUF_PG_PTR_LIST_ENDPT(ep_nbr);

		p_new_itd->ITDBufPagePtrList[1] =
			ITD_BUF_PG_PTR_LIST_MPS(ep_max_pkt_size) |
			ITD_BUF_PG_PTR_LIST_IO(token);

		p_new_itd->ITDBufPagePtrList[2] =
			ITD_BUF_PG_PTR_LIST_MULT(mult_value);

		page_nbr = 0u;
		buf_ptr_page_nbr = 0u;
		xfer_elapsed_len = 0u;

		for (micro_frame_nbr = 0u; micro_frame_nbr < 8u;
		     micro_frame_nbr++) {
			if ((active_trans[i] & (1 << micro_frame_nbr)) != 0u) {
				/* Determine the Transaction Length for iTD structure  */
				if (xfer_remaining_len > max_transaction_len) {
					xact_len = max_transaction_len;
				} else {
					xact_len = xfer_remaining_len;
				}
				/* Calculate the Transaction Offset for iTD structure  */
				xact_offset = (buf_page + xfer_elapsed_len) &
					      0x00000FFFu;

				/* For the 1st microframe nbr, determine the Buffer    */
				/* Pointer for iTD structure and buffer start address  */
				if (micro_frame_nbr == 0) {
					buf_ptr = buf_page & 0xFFFFF000u;
					buf_start_addr = buf_ptr | xact_offset;
				}

				if (buf_start_addr >
				    (buf_ptr +
				     0x1000u)) { /* If buffer start addr is greater than 4096 boundary  */
					buf_ptr +=
						0x1000u; /* Increment Buffer Pointer by 4096                    */
					page_nbr++; /* Increment Page Number for iTD structure             */
					buf_ptr_page_nbr++; /* Increment buffer pointer page number                */
				}
				/* Store the Status, Transaction Length, Page Number   */
				/* and Transaction Offset in iTD structure's Status and*/
				/* Control field for this microframe                   */
				p_new_itd->ITDStsAndCntrl[micro_frame_nbr] =
					ITD_STSCTRL_STS(O_ITD_STS_ACTIVE) |
					ITD_STSCTRL_XACT_LEN(xact_len) |
					ITD_STSCTRL_PG(page_nbr) |
					ITD_STSCTRL_XACT_OFFSET(xact_offset);

				/* Store the Buffer Pointer in iTD structure's Buffer  */
				/* Page Pointer field for this page number ensuring it */
				if (buf_ptr_page_nbr <
				    7u) { /* doesn't go beyond the array limit                   */
					p_new_itd->ITDBufPagePtrList
						[buf_ptr_page_nbr] |= buf_ptr;
				} else {
					irq_unlock(key);
					return (-ENOMEM);
				}

				buf_start_addr += xact_len;
				ioc_bit = micro_frame_nbr;
				xfer_remaining_len -= xact_len;
				xfer_elapsed_len += xact_len;
			}
		}
		if (i ==
		    (nbr_of_iTDs_for_xfer -
		     1)) { /* For the itd is the last iTD for this transfer       */
			p_new_itd->ITDStsAndCntrl[ioc_bit] |=
				(uint32_t)ITD_STSCTRL_IOC(
					1); /* Set IOC bit for last iTD          */
			p_ep_desc->TDTailPtr = (void *)
				p_new_itd; /* Save the last iTD of this xfer    */
			p_urb_info->iTD_Addr = (uint32_t)
				p_new_itd; /* Save the last iTD associated with this URB */
			p_urb->arg_ptr = (void *)p_urb_info;
		}

		/* ----------- Isochronous EP Insertion  ------------- */
		p_hw_desc = (uint32_t *)(p_ehci->PeriodicListBase[frame_nbr] &
					 0xFFFFFFE0);
		DCACHE_InvalidateByRange((uint32_t)p_hw_desc, sizeof(uint32_t));

		/* While Type in Next Link Pointer is not QH           */
		/* go to next pointer                                  */
		while ((*p_hw_desc & 0x06u) != HOR_LNK_PTR_TYP(DWORD1_TYP_QH)) {
			p_hw_desc = (uint32_t *)p_hw_desc;
			DCACHE_InvalidateByRange((uint32_t)p_hw_desc,
						 sizeof(uint32_t));
		}

		p_new_itd->ITDNxtLinkPtr = *p_hw_desc;
		DCACHE_CleanByRange((uint32_t)p_new_itd, sizeof(EHCI_ITD));

		*p_hw_desc = HOR_LNK_PTR_T(
			DWORD1_T_INVALID); /* Set to invalid so that insertion is not compromised. */

		*p_hw_desc |=
			(uint32_t)((void *)p_new_itd) |
			HOR_LNK_PTR_TYP(
				DWORD1_TYP_ITD); /* Set insertion Type to iTD                            */

		*p_hw_desc &=
			0xFFFFFFFEu; /* Set to valid when insertion is done.                 */
		DCACHE_CleanByRange((uint32_t)p_hw_desc, sizeof(uint32_t));

		frame_nbr += frame_interval;
	}
	irq_unlock(key);

	return (0);
}
#endif

/*
*********************************************************************************************************
*                                             EHCI_ISR()
*
* Description : EHCI interrupt service routine.
*
* Argument(s) : p_data     Pointer to host controller driver structure.
*
* Return(s)   : None.
*
* Note(s)     : None.
*********************************************************************************************************
*/

static void EHCI_ISR(void *p_data)
{
	uint32_t int_status;
	uint32_t int_en;
	EHCI_DEV *p_ehci;
	EHCI_QH *p_qh;
	struct usbh_hc_drv *p_hc_drv;
#if (USBH_EHCI_CFG_PERIODIC_EN == DEF_ENABLED)
	uint32_t bytes_to_xfer;
	uint32_t frame_interval;
	uint16_t index;
	uint8_t ep_addr;
	uint8_t dev_addr;
	EHCI_ITD *p_itd;
	EHCI_SITD *p_sitd;
	EHCI_ISOC_EP_DESC *p_ep_desc;
	EHCI_ISOC_EP_URB *p_urb_info;
	EHCI_INTR_INFO *p_intr_info;
	EHCI_QH *p_intr_qh_placeholder;
	struct usbh_ep *p_ep;
	struct usbh_urb *p_urb;
	struct usbh_urb *p_urb_previous = 0;
#endif

	p_hc_drv = (struct usbh_hc_drv *)p_hc_drv_local;

	//     p_hc_drv    = (USBH_HC_DRV *)p_data;
	p_ehci = (EHCI_DEV *)p_hc_drv->data_ptr;
	int_status = USBSTATUS;
	int_en = USBINT;
	int_status &= int_en;
	USBSTATUS =
		int_status; /* Clear the interrupt status register                  */

	if (int_status == 0u) {
		return;
	}

	if ((int_status & EHCI_USBSTS_RD_HSE) !=
	    0u) { /* ----------- (1) HOST SYSTEM ERROR INT -------------- */

		printk("Host System Error => HC halted\r");
	}
	if ((int_status & EHCI_USBSTS_RD_PCD) !=
	    0u) { /* ----------- (2) PORT CHANGE DETECT INT ------------- */
		usbh_rh_event(p_hc_drv->rh_dev_ptr);
	}

	if (((int_status & EHCI_USBSTS_RD_USBI) !=
	     0u) || /* ---------- (3) USB INT or USB ERROR INT ------------ */
	    ((int_status & EHCI_USBSTS_RD_USBEI) != 0u)) {
		/* (1)       Control and Bulk qTD processing            */
		DCACHE_InvalidateByRange((uint32_t)p_ehci->AsyncQHHead,
					 sizeof(EHCI_QH));
		p_qh = (EHCI_QH *)(p_ehci->AsyncQHHead->QHHorLinkPtr &
				   0xFFFFFFE0);
		DCACHE_InvalidateByRange((uint32_t)p_qh, sizeof(EHCI_QH));

		while (p_qh !=
		       (EHCI_QH *)p_ehci
			       ->AsyncQHHead) { /* Search in the async list until  async head is found  */

			if (p_qh->QHCurQTDPtr != 0u) {
				EHCI_QHDone(p_hc_drv, p_qh);
			}

			p_qh = (EHCI_QH *)(p_qh->QHHorLinkPtr & 0xFFFFFFE0);
			DCACHE_InvalidateByRange((uint32_t)p_qh,
						 sizeof(EHCI_QH));
		}

#if (USBH_EHCI_CFG_PERIODIC_EN == DEF_ENABLED)
		/* (2)          Interrupt qTD processing                */
		p_intr_info = p_ehci->HeadIntrInfo;
		while (p_intr_info !=
		       0) { /* Browse Intr info list = active Intr placeholder.     */
			/* Get placeholder containing opened Intr ep(s).        */
			p_intr_qh_placeholder =
				p_ehci->QHLists[p_intr_info->IntrPlaceholderIx];
			/* Get polling interval of this placeholder.            */
			frame_interval = p_intr_info->FrameInterval;

			/* T-bit = 0 => QH Horizontal link ptr is valid          */
			if (DEF_BIT_IS_SET(p_intr_qh_placeholder->QHHorLinkPtr,
					   DWORD1_T) == 1) {
				DCACHE_InvalidateByRange(
					(uint32_t)p_intr_qh_placeholder,
					sizeof(EHCI_QH));
				p_qh = (EHCI_QH *)(p_intr_qh_placeholder
							   ->QHHorLinkPtr &
						   0xFFFFFFE0);
				DCACHE_InvalidateByRange((uint32_t)p_qh,
							 sizeof(EHCI_QH));

				/* Search polling interval list matching to open qH.    */
				while (p_qh->FrameInterval != frame_interval) {
					p_qh = (EHCI_QH *)(p_qh->QHHorLinkPtr &
							   0xFFFFFFE0);
					DCACHE_InvalidateByRange(
						(uint32_t)p_qh,
						sizeof(EHCI_QH));
				}
				/* From this placeholder, get all active Intr qH.       */
				while (p_qh->FrameInterval == frame_interval) {
					if (p_qh->QHCurQTDPtr !=
					    0u) { /* Are there qTDs completed for this active qH?...      */
						EHCI_QHDone(
							p_hc_drv,
							p_qh); /* ...Yes, process compeled qTD(s).                     */
					}

					if ((p_qh->QHHorLinkPtr & 0x01u) !=
					    0u) {
						break;
					} else { /* Get next active qH.                                  */
						p_qh = (EHCI_QH *)(p_qh->QHHorLinkPtr &
								   0xFFFFFFE0u);
						DCACHE_InvalidateByRange(
							(uint32_t)p_qh,
							sizeof(EHCI_QH));
					}
				}
			}

			p_intr_info =
				p_intr_info
					->NxtIntrInfo; /* Go to next placeholder containing opened Intr ep(s). */
		}
		/* ------------ ISOCHRONOUS XFER COMPLETION ----------- */
		p_ep_desc = p_ehci->HeadIsocEPDesc;

		while (p_ep_desc !=
		       0) { /* Browse the list of opened Isochronous EP             */
			p_ep = p_ep_desc->EPPtr;
			p_urb = &p_ep->urb;

			while (p_urb !=
			       0) { /* Search for every Isoc transfer completed             */

				if ((p_ep_desc->TDTailPtr != 0) &&
				    (p_urb->state ==
				     USBH_URB_STATE_SCHEDULED) &&
				    (p_urb->arg_ptr != 0)) {
					/* (1)                  iTD processing                  */
					if (p_ep->dev_spd == USBH_HIGH_SPEED) {
						/* Retrieve the last iTD associated with this URB       */
						p_urb_info =
							(EHCI_ISOC_EP_URB *)
								p_urb->arg_ptr;
						p_itd = (EHCI_ITD *)p_urb_info
								->iTD_Addr;
						DCACHE_InvalidateByRange(
							(uint32_t)p_itd,
							sizeof(EHCI_ITD));

						for (index = 0u; index < 8u;
						     index++) { /* Search the last transaction of the iTD               */

							/* Is Isoc transfer completed?                          */
							if ((DEF_BIT_IS_SET(
								     p_itd->ITDStsAndCntrl
									     [index],
								     DWORDx_ITD_IOC) ==
							     1) &&
							    (DEF_BIT_IS_SET(
								     p_itd->ITDStsAndCntrl
									     [index],
								     DWORDx_ITD_STATUS_ACTIVE) ==
							     1)) {
								/* Retrieve Device @ and EP @                           */
								dev_addr =
									(p_itd->ITDBufPagePtrList
										 [0] &
									 0x0000007Fu);
								ep_addr =
									(p_itd->ITDBufPagePtrList
										 [0] &
									 0x00000F00u) >>
									8u;

								bytes_to_xfer = EHCI_ITDDone(
									p_hc_drv,
									p_ep_desc,
									dev_addr,
									ep_addr,
									p_urb);
								p_urb->xfer_len =
									bytes_to_xfer;
								/* See Note #1.                                         */
								if (p_urb ==
								    &p_ep->urb) {
									usbh_urb_done(
										p_urb);
									p_urb->arg_ptr =
										NULL;
								} else if (
									p_urb ==
									p_ep->urb
										.async_urb_nxt_ptr) {
									usbh_urb_done(
										p_urb);
									p_urb->arg_ptr =
										NULL;
									p_urb->urb_done_signal =
										true;
								} else if (
									p_urb_previous
										->urb_done_signal ==
									true) {
									usbh_urb_done(
										p_urb);
									p_urb->arg_ptr =
										NULL;
									p_urb->urb_done_signal =
										true;
									p_urb_previous
										->urb_done_signal =
										false;
								} else {
									/* Empty Else Statement                                 */
								}
								break;
							}
						}

					} else { /* (2)                 siTD processing                  */
						/* Retrieve the last siTD associated with this URB      */
						p_urb_info =
							(EHCI_ISOC_EP_URB *)
								p_urb->arg_ptr;
						p_sitd = (EHCI_SITD *)p_urb_info
								 ->iTD_Addr;
						DCACHE_InvalidateByRange(
							(uint32_t)p_sitd,
							sizeof(EHCI_SITD));

						/* Is Isoc transfer completed?                          */
						if (DEF_BIT_IS_SET(
							    p_sitd->SITDStsCtrl,
							    DWORD3_SITD_STATUS_ACTIVE) ==
						    1) {
							/* Retrieve Device @ and EP @                           */
							dev_addr =
								(p_sitd->SITDEpCapChar
									 [0] &
								 0x0000007Fu);
							ep_addr =
								(p_sitd->SITDEpCapChar
									 [0] &
								 0x00000F00u) >>
								8u;

							bytes_to_xfer =
								EHCI_SITDDone(
									p_hc_drv,
									p_ep_desc,
									dev_addr,
									ep_addr,
									p_urb);
							p_urb->xfer_len =
								p_urb->dma_buf_len -
								bytes_to_xfer;
							usbh_urb_done(
								p_urb); /* Notify about URB completion                          */
							p_urb->arg_ptr = NULL;
						}
					}
				}
				p_urb_previous =
					p_urb; /* Keep ref of previous URB in progress.                */
				p_urb = p_urb->async_urb_nxt_ptr; /* Go to the next URB in progress                       */
			}
			p_ep_desc =
				p_ep_desc->NxtEPDesc; /* Go to the next opened Isoc EP                        */
		}
#endif
	}

	if ((int_status & EHCI_USBSTS_RD_FLR) !=
	    0u) { /* ----------- (4) FRAME LIST ROLLOVER INT ------------ */
		p_ehci->FNOCnt++; /* Count frame number overrun                           */
	}
}

/*
*********************************************************************************************************
*                                     EHCI_PeriodicOrderPrepare()
*
* Description : Initialize EHCI_BranchArray[] array following a scheduling pattern.
*
* Argument(s) : idx           Starting array index
*
*               power         bits to shift(Power of two)
*
*               list_size     Size of Periodic Frame List
*
* Return(s)   : None
*
* Note(s)     : None
*********************************************************************************************************
*/

#if (USBH_EHCI_CFG_PERIODIC_EN == DEF_ENABLED)
static void EHCI_PeriodicOrderPrepare(uint32_t idx, uint32_t power,
				      uint32_t list_size)
{
	EHCI_BranchArray[idx + (1u << power)] =
		EHCI_BranchArray[idx] + ((list_size / 2u) / (1u << power));

	if (power == 0u) {
		return;
	}

	power--;

	EHCI_PeriodicOrderPrepare(idx, power, list_size);

	EHCI_PeriodicOrderPrepare(idx + (1u << (power + 1u)), power, list_size);
}
#endif

/*
*********************************************************************************************************
*                                          EHCI_CapRegRead()
*
* Description : Read the EHCI capability registers.
*
* Argument(s) : p_ehci     Pointer to EHCI_DEV structure.
*
*               p_cap      Pointer to EHCI capability structure.
*
* Return(s)   : None.
*
* Note(s)     : None.
*********************************************************************************************************
*/

static void EHCI_CapRegRead(EHCI_DEV *p_ehci, EHCI_CAP *p_cap)
{
	uint8_t i;
	uint32_t reg;
	uint8_t cap_len;
	uint16_t hci_ver;

	reg = sys_get_le32((uint8_t *)&(p_ehci->HcCapReg->CapLen_HCIVersion));
	// cap_len = MEM_VAL_GET_INT08U(&reg);
	// hci_ver = MEM_VAL_GET_INT16U((uint8_t *)&reg + 2);
	cap_len = *(uint8_t *)&reg;
	hci_ver = sys_get_le16((uint8_t *)&reg + 2);

	p_cap->CapLen = cap_len;
	p_cap->HCIVersion = hci_ver;
	p_cap->HCSParams =
		p_ehci->HcCapReg
			->HCSParams; /* Structural Parameters                                */
	p_cap->HCCParams =
		p_ehci->HcCapReg
			->HCCParams; /* Capability Parameters                                */

	if ((p_cap->HCSParams & EHCI_HCSPARAMS_RD_PRR) != 0u) {
		for (i = 0u; i < 15u;
		     i++) { /* Companion Port Route Description                     */
			p_cap->HCSPPortRoute[i] =
				p_ehci->HcCapReg->HCSPPortRoute[i];
		}
	}
}

/*
*********************************************************************************************************
*                                           EHCI_DMA_Init()
*
* Description : Allocate all structures used by the EHCI driver.
*
* Argument(s) : p_hc_drv      Pointer to host controller driver structure.
*
* Return(s)   : USBH_ERR_NONE,          if successful.
*               Specific error code     otherwise.
*
* Note(s)     : (1) 8 represents eight frames. One siTD per frame is required. Multiplying per 8 frames
*                   is a safety margin which allows the applicatio to define an isochronous transfer
*                   which will span several frames, and so several siTD will be required.
*********************************************************************************************************
*/

static int EHCI_DMA_Init(struct usbh_hc_drv *p_hc_drv)
{
	uint32_t max_nbr_qh;
	uint32_t max_nbr_qh_alloc;
	uint32_t max_nbr_qtd;
	uint32_t max_nbr_itd = 0u;
	uint32_t max_data_buf;
	uint32_t total_mem_req;
	uint8_t *p_dedicated_mem;
	EHCI_DEV *p_ehci;
	const struct usbh_hc_cfg *p_hc_cfg;
#if (USBH_EHCI_CFG_PERIODIC_EN == DEF_ENABLED)
	uint32_t max_ep_desc;
#endif

	p_ehci = p_hc_drv->data_ptr;
	// p_hc_cfg     = p_hc_drv->HC_CfgPtr;

	max_nbr_qh =
		(USBH_CFG_MAX_NBR_DEVS + /* Total ctrl, bulk, intr EPs + 1 dummy async Q head EP.*/
		 DT_INST_PROP(0, nbr_ep_bulk) + 1u);
	max_nbr_qh_alloc = max_nbr_qh;

	max_data_buf = (USBH_CFG_MAX_NBR_DEVS + DT_INST_PROP(0, nbr_ep_bulk));
#if (USBH_EHCI_CFG_PERIODIC_EN == DEF_ENABLED)
	max_nbr_qh_alloc += EHCI_INTR_QH_LIST_SIZE;

	max_nbr_qh += DT_INST_PROP(0, nbr_ep_intr);
	max_nbr_qh_alloc += DT_INST_PROP(0, nbr_ep_intr);

	max_data_buf +=
		DT_INST_PROP(0, nbr_ep_intr) + DT_INST_PROP(0, nbr_isoc_ep);

	max_ep_desc = DT_INST_PROP(
		0,
		nbr_isoc_ep); /* Total Isochronous endpoints                          */

	max_nbr_itd = (max_ep_desc * EHCI_MAX_ITD) *
		      ((DT_INST_PROP(0, buf_len) / (8u * 3072u)) + 1u);
	max_nbr_itd +=
		256u; /* For dummy SITDs in the periodic list                 */
	max_nbr_itd +=
		max_ep_desc * EHCI_MAX_SITD *
		8u; /* See Note #1.                                         */
#endif

	max_nbr_qtd =
		max_nbr_qh * ((DT_INST_PROP(0, buf_len) / (20u * 1024u)) + 1u);

	if (DT_INST_PROP(0, mem_addr) !=
	    (uint32_t)0) { /* --------------- DEDICATED MEMORY ------------------- */

		// if (p_hc_cfg->DataBufFromSysMemEn == 0) {    /* Data buffers allocated from dedicated memory         */
		if (DT_INST_PROP(0, dma) == false) {
			total_mem_req =
				(max_nbr_qh_alloc * sizeof(EHCI_QH)) +
				((max_nbr_qtd + 1u) * sizeof(EHCI_QTD)) +
				(max_nbr_itd * sizeof(EHCI_ITD)) +
				(DT_INST_PROP(0, buf_len) * max_data_buf);
		} else { /* Data buffers allocated from main memory              */
			total_mem_req =
				(max_nbr_qh_alloc * sizeof(EHCI_QH)) +
				((max_nbr_qtd + 1u) * sizeof(EHCI_QTD)) +
				(max_nbr_itd * sizeof(EHCI_ITD));
		}

#if (USBH_EHCI_CFG_PERIODIC_EN == DEF_ENABLED)
		total_mem_req += (EHCI_MAX_PERIODIC_LIST_SIZE * sizeof(void *));

		p_dedicated_mem = (uint8_t *)USB_ALIGNED(
			(uint8_t *)DT_INST_PROP(0, mem_addr), 4096u);
#else
		p_dedicated_mem = (uint8_t *)USB_ALIGNED(
			(uint8_t *)DT_INST_PROP(0, mem_addr), 32u);
#endif

		if (total_mem_req >
		    ((DT_INST_PROP(0, mem_addr) + DT_INST_PROP(0, mem_size)) -
		     (uint32_t)p_dedicated_mem)) {
			return (-ENOMEM);
		}

		/* Align first byte of dedicated mem on 4096 for ...    */
		/* ... periodic list.                                   */
#if (USBH_EHCI_CFG_PERIODIC_EN == DEF_ENABLED)
		p_ehci->PeriodicListBase = (uint32_t *)p_dedicated_mem;
		p_dedicated_mem +=
			(EHCI_MAX_PERIODIC_LIST_SIZE * sizeof(void *));
#endif

		p_ehci->DMA_EHCI.QHPtr = (EHCI_QH *)p_dedicated_mem;
		p_dedicated_mem += (max_nbr_qh_alloc * sizeof(EHCI_QH));

		p_ehci->DMA_EHCI.QTDPtr = (EHCI_QTD *)p_dedicated_mem;
		p_dedicated_mem +=
			((max_nbr_qtd + 1u) *
			 sizeof(EHCI_QTD)); /* 1 for dummy head QTD                   */

		p_ehci->DMA_EHCI.ITDPtr = (EHCI_ITD *)p_dedicated_mem;
		p_dedicated_mem += (max_nbr_itd * sizeof(EHCI_ITD));

		p_ehci->DMA_EHCI.BufPtr = (void *)p_dedicated_mem;

		// Mem_PoolCreate(       &p_ehci->HC_QHPool,              /* Create DMA QH Pool                                   */
		//                (void *)p_ehci->DMA_EHCI.QHPtr,
		//                        max_nbr_qh_alloc * sizeof(EHCI_QH),
		//                        max_nbr_qh_alloc,
		//                        sizeof(EHCI_QH),
		//                        32u,                            /* qH must be aligned on 32-byte boundary               */
		//                       &octets_reqd,
		//                       &err_lib);
		// if (err_lib != LIB_MEM_ERR_NONE) {
		//     return (USBH_ERR_ALLOC);
		// }

		// Mem_PoolCreate(      &p_ehci->HC_QTDPool,             /* Create DMA QTD Pool                                  */
		//                (void *)p_ehci->DMA_EHCI.QTDPtr,
		//                        ((max_nbr_qtd + 1u) * sizeof(EHCI_QTD)),
		//                        max_nbr_qtd,
		//                        sizeof(EHCI_QTD),
		//                        32u,                            /* qTD must be aligned on 32-byte boundary              */
		//                       &octets_reqd,
		//                       &err_lib);
		// if (err_lib != LIB_MEM_ERR_NONE) {
		//     return (USBH_ERR_ALLOC);
		// }

#if (USBH_EHCI_CFG_PERIODIC_EN == DEF_ENABLED)

		// Mem_PoolCreate(      &p_ehci->HC_ITDPool,             /* Create DMA iTD/siTD Pool                             */
		//                (void *)p_ehci->DMA_EHCI.ITDPtr,
		//                        (max_nbr_itd * sizeof(EHCI_ITD)),
		//                        max_nbr_itd,
		//                        sizeof(EHCI_ITD),
		//                        32u,                            /* iTD must be aligned on a 32-byte boundary.           */
		//                       &octets_reqd,
		//                       &err_lib);
		// if (err_lib != LIB_MEM_ERR_NONE) {
		//     return (USBH_ERR_ALLOC);
		// }
#endif
	} else {
		/* ---------------- SYSTEM MEMORY --------------------- */
		// Mem_PoolCreate(       &p_ehci->HC_QHPool,               /* Create DMA QH Pool                                   */
		//                (void *)0,                               /* Allocate from HEAP region                            */
		//                        max_nbr_qh_alloc * sizeof(EHCI_QH),
		//                        max_nbr_qh_alloc,
		//                        sizeof(EHCI_QH),
		//                        32u,                             /* qH must be aligned on 32-byte boundary               */
		//                       &octets_reqd,
		//                       &err_lib);
		// if (err_lib != LIB_MEM_ERR_NONE) {
		//     return (USBH_ERR_ALLOC);
		// }

		// p_ehci->DMA_EHCI.QHPtr = (EHCI_QH *)p_ehci->HC_QHPool.PoolAddrStart;

		// Mem_PoolCreate (       &p_ehci->HC_QTDPool,             /* Create DMA QTD Pool                                  */
		//                 (void *)0,                              /* Allocate from HEAP region                            */
		//                         ((max_nbr_qtd + 1u) * sizeof(EHCI_QTD)),
		//                           max_nbr_qtd + 1u,
		//                         sizeof(EHCI_QTD),
		//                         32u,                            /* qTD must be aligned on 32-byte boundary              */
		//                        &octets_reqd,
		//                        &err_lib);
		// if (err_lib != LIB_MEM_ERR_NONE) {
		//     return (USBH_ERR_ALLOC);
	}

	// p_ehci->DMA_EHCI.QTDPtr = (EHCI_QTD *)p_ehci->HC_QTDPool.PoolAddrStart;

#if (USBH_EHCI_CFG_PERIODIC_EN == DEF_ENABLED)
	/* Create DMA iTD/siTD Pool                             */
	// Mem_PoolCreate (       &p_ehci->HC_ITDPool,
	//                 (void *)0,                              /* Allocate from HEAP region                            */
	//                         (max_nbr_itd * sizeof(EHCI_ITD)),
	//                          max_nbr_itd,
	//                         sizeof(EHCI_ITD),
	//                         32u,                            /* iTD must be aligned on a 32-byte boundary.           */
	//                        &octets_reqd,
	//                        &err_lib);
	// if (err_lib != LIB_MEM_ERR_NONE) {
	//     return (USBH_ERR_ALLOC);
	// }

	/* Get a mem block for Periodic Frame List              */
	// p_ehci->PeriodicListBase = (uint32_t *)Mem_HeapAlloc(512u * sizeof(uint32_t),
	//                                                        4096u,
	//                                                       &octets_reqd,
	//                                                       &err_lib);
	p_ehci->PeriodicListBase = k_malloc(512u * sizeof(uint32_t));
	if (p_ehci->PeriodicListBase == NULL) {
		return (-ENOMEM);
	}
#endif
	//     }

#if (USBH_EHCI_CFG_PERIODIC_EN == DEF_ENABLED)
	if (max_ep_desc > 0u) {
		// Mem_PoolCreate (       &p_ehci->HC_Isoc_EP_DescPool,        /* Create Isochronous EP Pool                           */
		//                 (void *)0,                                  /* Allocate from HEAP region                            */
		//                         (max_ep_desc * sizeof(EHCI_ISOC_EP_DESC)),
		//                          max_ep_desc,
		//                         sizeof(EHCI_ISOC_EP_DESC),
		//                         sizeof(CPU_ALIGN),
		//                        &octets_reqd,
		//                        &err_lib);
		// if (err_lib != LIB_MEM_ERR_NONE) {
		//     return (USBH_ERR_ALLOC);
		// }

		// Mem_PoolCreate (       &p_ehci->HC_Isoc_EP_URBPool,         /* Create pool for storing URB info during ongoing xfer */
		//                 (void *)0,                                  /* Allocate from HEAP region                            */
		//                         (max_ep_desc * 2u * sizeof(EHCI_ISOC_EP_URB)),
		//                         (max_ep_desc * 2u),
		//                         sizeof(EHCI_ISOC_EP_URB),
		//                         sizeof(CPU_ALIGN),
		//                        &octets_reqd,
		//                        &err_lib);
		// if (err_lib != LIB_MEM_ERR_NONE) {
		//     return (USBH_ERR_ALLOC);
		// }
	}
#endif

	if ((DT_INST_PROP(0, mem_addr) != (uint32_t)0) &&
	    (DT_INST_PROP(0, dma) ==
	     0)) { /* ----------- DATA BUF FROM DEDICATED MEM ------------ */

		//         Mem_PoolCreate (       &p_ehci->BufPool,                /* Create Data buffer Pool                              */
		//                         (void *)p_ehci->DMA_EHCI.BufPtr,
		//                                 (DT_INST_PROP(0, buf_len) * max_data_buf),
		//                                 max_data_buf,
		//                                 DT_INST_PROP(0, buf_len),
		// #if (CPU_CFG_CACHE_MGMT_EN == DEF_ENABLED)
		//                                 CPU_Cache_Linesize,
		// #else
		//                                 sizeof(CPU_ALIGN),
		// #endif
		//                                &octets_reqd,
		//                                &err_lib);
		//         if (err_lib != LIB_MEM_ERR_NONE) {
		//             return (USBH_ERR_ALLOC);
		//         }
	}

#if (USBH_EHCI_CFG_PERIODIC_EN == DEF_ENABLED)

	//     Mem_PoolCreate (       &p_ehci->IntrInfoPool,               /* Create Intr info pool used for completed Intr xfer.  */
	//                     (void *)0,                                  /* Allocate from HEAP region                            */
	//                             (p_hc_cfg->MaxNbrEP_IntrOpen * sizeof(EHCI_INTR_INFO)),
	//                             (p_hc_cfg->MaxNbrEP_IntrOpen),
	//                             sizeof(EHCI_INTR_INFO),
	//                             sizeof(CPU_ALIGN),
	//                            &octets_reqd,
	//                            &err_lib);
	//     if (err_lib != LIB_MEM_ERR_NONE) {
	//         return (USBH_ERR_ALLOC);
	//     }

	p_ehci->HeadIntrInfo = NULL;
#endif

	return (0);
}

/*
*********************************************************************************************************
*                                            EHCI_QH_Clr()
*
* Description : Clear the contents of a queue head structure.
*
* Argument(s) : p_qh     Pointer to the queue head structure.
*
* Return(s)   : None.
*
* Note(s)     : None.
*********************************************************************************************************
*/

static void EHCI_QH_Clr(EHCI_QH *p_qh)
{
	p_qh->QHHorLinkPtr = (uint32_t)0;
	p_qh->QHEpCapChar[0] = (uint32_t)0;
	p_qh->QHEpCapChar[1] = (uint32_t)0;
	p_qh->QHCurQTDPtr = (uint32_t)0;
	p_qh->QHNxtQTDPtr = (uint32_t)0;
	p_qh->QHAltNxtQTDPtr = (uint32_t)0;
	p_qh->QHToken = (uint32_t)0;
	p_qh->QHBufPagePtrList[0] = (uint32_t)0;
	p_qh->QHBufPagePtrList[1] = (uint32_t)0;
	p_qh->QHBufPagePtrList[2] = (uint32_t)0;
	p_qh->QHBufPagePtrList[3] = (uint32_t)0;
	p_qh->QHBufPagePtrList[4] = (uint32_t)0;
	p_qh->QTDHead = (uint32_t)0;
}

/*
*********************************************************************************************************
*                                           EHCI_QTD_Clr()
*
* Description : Clear the contents of a queue element transfer descriptor structure.
*
* Argument(s) : p_qtd     Pointer to the queue element transfer descriptor.
*
* Return(s)   : None.
*
* Note(s)     : None.
*********************************************************************************************************
*/

static void EHCI_QTD_Clr(EHCI_QTD *p_qtd)
{
	p_qtd->QTDNxtPtr = (uint32_t)0;
	p_qtd->QTDAltNxtPtr = (uint32_t)0;
	p_qtd->QTDToken = (uint32_t)0;
	p_qtd->QTDBufPagePtrList[0] = (uint32_t)0;
	p_qtd->QTDBufPagePtrList[1] = (uint32_t)0;
	p_qtd->QTDBufPagePtrList[2] = (uint32_t)0;
	p_qtd->QTDBufPagePtrList[3] = (uint32_t)0;
	p_qtd->QTDBufPagePtrList[4] = (uint32_t)0;
}

/*
*********************************************************************************************************
*                                           EHCI_SITD_Clr()
*
* Description : Clear the contents of a split transaction element transfer descriptor structure.
*
* Argument(s) : p_sitd     Pointer to the split transaction element transfer descriptor.
*
* Return(s)   : None.
*
* Note(s)     : None.
*********************************************************************************************************
*/

#if (USBH_EHCI_CFG_PERIODIC_EN == DEF_ENABLED)
static void EHCI_SITD_Clr(EHCI_SITD *p_sitd)
{
	p_sitd->SITDNxtLinkPtr = (uint32_t)0;
	p_sitd->SITDEpCapChar[0] = (uint32_t)0;
	p_sitd->SITDEpCapChar[1] = (uint32_t)0;
	p_sitd->SITDStsCtrl = (uint32_t)0;
	p_sitd->SITDBufPagePtrList[0] = (uint32_t)0;
	p_sitd->SITDBufPagePtrList[1] = (uint32_t)0;
	p_sitd->SITDBackLinkPtr = (uint32_t)0;
}
#endif

/*
*********************************************************************************************************
*                                           EHCI_ITD_Clr()
*
* Description : Clear the contents of a split transaction element transfer descriptor structure.
*
* Argument(s) : p_itd     Pointer to the split transaction element transfer descriptor.
*
* Return(s)   : None.
*
* Note(s)     : None.
*********************************************************************************************************
*/

#if (USBH_EHCI_CFG_PERIODIC_EN == DEF_ENABLED)
static void EHCI_ITD_Clr(EHCI_ITD *p_itd)
{
	uint8_t i;

	p_itd->ITDNxtLinkPtr = (uint32_t)0;

	for (i = 0u; i < 8u; i++) {
		p_itd->ITDStsAndCntrl[i] = (uint32_t)0;
	}

	for (i = 0u; i < 7u; i++) {
		p_itd->ITDBufPagePtrList[i] = (uint32_t)0;
	}
}
#endif

/*
*********************************************************************************************************
*                                           EHCI_EP_DescClr()
*
* Description : Clear the contents of a isochronous endpoint descriptor structure.
*
* Argument(s) : p_ep_desc     Pointer to isochronous endpoint descriptor structure
*
* Return(s)   : None.
*
* Note(s)     : None.
*********************************************************************************************************
*/

#if (USBH_EHCI_CFG_PERIODIC_EN == DEF_ENABLED)
static void EHCI_EP_DescClr(EHCI_ISOC_EP_DESC *p_ep_desc)
{
	p_ep_desc->TDTailPtr = NULL;
	p_ep_desc->EPPtr = NULL;
	p_ep_desc->SMask = (uint8_t)0;
	p_ep_desc->CMask = (uint8_t)0;
	p_ep_desc->AppStartFrame = (uint8_t)0;
	p_ep_desc->NbrFrame = (uint8_t)0;
	p_ep_desc->FrameInterval = (uint8_t)0;
	p_ep_desc->NxtEPDesc = NULL;
}
#endif

/*
*********************************************************************************************************
*                                          EHCI_QTDRemove()
*
* Description : Free the memory of all QTDs in the QTD list and calculate the total bytes transfered by
*               all QTDs.
*
* Argument(s) : p_hc_drv      Pointer to host controller driver structure.
*
*               p_qh          Pointer to EHCI_QH structure.
*
* Return(s)   : Total number of bytes transferred.
*
* Note(s)     : None
*********************************************************************************************************
*/

static uint32_t EHCI_QTDRemove(struct usbh_hc_drv *p_hc_drv, EHCI_QH *p_qh)
{
	EHCI_QTD *p_qtd;
	EHCI_QTD *p_qtd_next;
	EHCI_DEV *p_ehci;
	uint32_t rem_len;
	uint32_t terminate;

	DCACHE_InvalidateByRange((uint32_t)p_qh, sizeof(EHCI_QH));
	p_qtd = (EHCI_QTD *)p_qh->QTDHead;
	if (p_qtd == (uint32_t)0) {
		return (0u);
	}

	p_ehci = (EHCI_DEV *)p_hc_drv->data_ptr;
	p_qh->QTDHead = 0u;
	terminate = 0u;
	rem_len = 0u;

	DCACHE_CleanByRange((uint32_t)p_qh, sizeof(EHCI_QH));
	DCACHE_InvalidateByRange((uint32_t)p_qtd, sizeof(EHCI_QTD));

	while (terminate !=
	       1u) { /* Until QTD terminate bit set is found                 */
		rem_len +=
			((p_qtd->QTDToken >> 16u) &
			 0x7FFFu); /* Bits 16-30 represent, bytes that are not transferred */

		p_qtd_next = (EHCI_QTD *)((p_qtd->QTDNxtPtr & 0xFFFFFFE0));
		terminate = (p_qtd->QTDNxtPtr & 1u);
		/* Free the QTD                                         */
		// Mem_PoolBlkFree(&HC_QTDPool, (void *)p_qtd, (uint32_t *)&err_lib);
		k_free(p_qtd);

		if (terminate != 1u) {
			p_qtd = p_qtd_next;
			DCACHE_InvalidateByRange((uint32_t)p_qtd,
						 sizeof(EHCI_QTD));
		}
	}

	return (rem_len);
}

/*
*********************************************************************************************************
*                                       EHCI_PeriodicListInit()
*
* Description : Initialize the periodic list. This will create a dummy queue head, which is the head of
*               the all queue heads, writes the periodic list base address with appropriate value and
*               enables the periodic list processing.
*
* Argument(s) : p_hc_drv      Pointer to host controller driver structure.
*
* Return(s)   : USBH_ERR_NONE,          if successful.
*               Specific error code     otherwise.
*
* Note(s)     : (1) Interrupt qH are are organized into a tree structure with Periodic Frame List (PFL)
*                   entries being the leaf nodes. The desired polling rate of an Interrupt endpoint is
*                   achieved by scheduling the qH at the appropriate depth in the tree. The higher the
*                   polling rate, the closer to the tree root the qH will be placed since multiple lists
*                   will converge on it. The figure below illustrates the Interrupt qH structure (PFL of
*                   32 entries is shown for simplification):
*
*                   0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0     <-- 32 ms polling interval
*                   |_| |_| |_| |_| |_| |_| |_| |_| |_| |_| |_| |_| |_| |_| |_| |_|
*                    0   0   0   0   0   0   0   0   0   0   0   0   0   0   0   0      <-- 16 ms polling interval
*                    |___|   |___|   |___|   |___|   |___|   |___|   |___|   |___|
*                      0       0       0       0       0       0       0       0        <--  8 ms polling interval
*                      |_______|       |_______|       |_______|       |_______|
*                          0               0               0               0            <--  4 ms polling interval
*                          |_______________|               |_______________|
*                                  0                               0                    <--  2 ms polling interval
*                                  |_______________________________|
*                                                  0                                    <--  1 ms polling interval
*
*                   Current EHCI driver has a PFL of 256 entries. The above tree structure can be easily
*                   extented to a PFL of 256 entries. Each depth level in the tree represents a polling
*                   interval. Hence for a PFL of 256 entries, 9 polling intervals are defined: 256, 128,
*                   64, 32, 16, 8, 4, 2, 1 ms.
*                   The tree nodes are dummy disabled Interrupt qH acting as a placeholder where 0 or more
*                   active qH may be enqueued. The total number of dummy qH is:
*
*                   256 + 128 + 64 + 32 + 16 + 8 + 4 + 2 + 1 = 511.
*
*                   Hence, the EHCI driver has 511 different scheduling lists into which active qH can be
*                   scheduled. While browsing the PFL, the host controller will visit 1 dummy qH every
*                   frame, 2 dummy qH once every 2 frames until 256 dummy qH once every 256 frames.
*                   The entire binary tree is stored into the "QHLists" array.
*********************************************************************************************************
*/

#if (USBH_EHCI_CFG_PERIODIC_EN == DEF_ENABLED)
static int EHCI_PeriodicListInit(struct usbh_hc_drv *p_hc_drv)
{
	EHCI_SITD *p_new_sitd;
	EHCI_QH *p_new_qh;
	EHCI_DEV *p_ehci;
	uint32_t list_ix;
	uint32_t ix_prev;

	ix_prev = 0u;
	p_ehci = (EHCI_DEV *)p_hc_drv->data_ptr;
	/* Build Intr QH lists with disabled QH (see Note #1).  */
	for (list_ix = EHCI_QH_LIST_256MS; list_ix <= EHCI_QH_LIST_01MS;
	     list_ix++) {
		/* Get dummy qH used as placeholder for Intr xfer.      */
		// p_new_qh = (EHCI_QH *)Mem_PoolBlkGet(&HC_QHPool,
		//                                       sizeof(EHCI_QH),
		//                                      &err_lib);
		p_new_qh = k_mem_pool_malloc(&HC_QHPool, sizeof(EHCI_QH));
		if (p_new_qh == NULL) {
			return (-ENOMEM);
		}

		EHCI_QH_Clr(p_new_qh);
		p_ehci->QHLists[list_ix] = p_new_qh;

		p_new_qh->QHEpCapChar[1] = QH_EPCAP_HBPM(DWORD3_QH_HBPM_1);
		p_new_qh->QHNxtQTDPtr = (uint32_t)0x00000001;
		p_new_qh->QHAltNxtQTDPtr = (uint32_t)0x00000001;

		if (list_ix < EHCI_QH_LIST_128MS) {
			p_new_qh->FrameInterval = 256u;
		} else if ((list_ix >= EHCI_QH_LIST_128MS) &&
			   (list_ix < EHCI_QH_LIST_64MS)) {
			ix_prev = EHCI_QH_LIST_256MS +
				  ((list_ix - EHCI_QH_LIST_128MS) * 2u);
			p_new_qh->FrameInterval = 128u;
		} else if ((list_ix >= EHCI_QH_LIST_64MS) &&
			   (list_ix < EHCI_QH_LIST_32MS)) {
			ix_prev = EHCI_QH_LIST_128MS +
				  ((list_ix - EHCI_QH_LIST_64MS) * 2u);
			p_new_qh->FrameInterval = 64u;
		} else if ((list_ix >= EHCI_QH_LIST_32MS) &&
			   (list_ix < EHCI_QH_LIST_16MS)) {
			ix_prev = EHCI_QH_LIST_64MS +
				  ((list_ix - EHCI_QH_LIST_32MS) * 2u);
			p_new_qh->FrameInterval = 32u;
		} else if ((list_ix >= EHCI_QH_LIST_16MS) &&
			   (list_ix < EHCI_QH_LIST_08MS)) {
			ix_prev = EHCI_QH_LIST_32MS +
				  ((list_ix - EHCI_QH_LIST_16MS) * 2u);
			p_new_qh->FrameInterval = 16u;
		} else if ((list_ix >= EHCI_QH_LIST_08MS) &&
			   (list_ix < EHCI_QH_LIST_04MS)) {
			ix_prev = EHCI_QH_LIST_16MS +
				  ((list_ix - EHCI_QH_LIST_08MS) * 2u);
			p_new_qh->FrameInterval = 8u;
		} else if ((list_ix >= EHCI_QH_LIST_04MS) &&
			   (list_ix < EHCI_QH_LIST_02MS)) {
			ix_prev = EHCI_QH_LIST_08MS +
				  ((list_ix - EHCI_QH_LIST_04MS) * 2u);
			p_new_qh->FrameInterval = 4u;
		} else if ((list_ix >= EHCI_QH_LIST_02MS) &&
			   (list_ix < EHCI_QH_LIST_01MS)) {
			ix_prev = EHCI_QH_LIST_04MS +
				  ((list_ix - EHCI_QH_LIST_02MS) * 2u);
			p_new_qh->FrameInterval = 2u;
		} else if (list_ix == EHCI_QH_LIST_01MS) {
			/* 2MS list points to 1MS lists.                        */
			ix_prev = EHCI_QH_LIST_02MS +
				  ((list_ix - EHCI_QH_LIST_01MS) * 2u);
			p_new_qh->FrameInterval = 1u;
		} else {
			/* Empty Else Statement                                 */
		}

		p_new_qh->QHHorLinkPtr = HOR_LNK_PTR_TYP(DWORD1_TYP_QH);

		if (list_ix != EHCI_QH_LIST_01MS) {
			p_new_qh->QHHorLinkPtr |= HOR_LNK_PTR_T(DWORD1_T_VALID);
		} else {
			p_new_qh->QHHorLinkPtr |=
				HOR_LNK_PTR_T(DWORD1_T_INVALID);
		}

		DCACHE_CleanByRange((uint32_t)p_new_qh, sizeof(EHCI_QH));

		if ((list_ix >= EHCI_QH_LIST_128MS) &&
		    (list_ix < EHCI_QH_LIST_64MS)) {
			p_ehci->QHLists[EHCI_BranchArray[ix_prev]]
				->QHHorLinkPtr |= (uint32_t)((void *)p_new_qh);
			p_ehci->QHLists[EHCI_BranchArray[ix_prev + 1u]]
				->QHHorLinkPtr |= (uint32_t)((void *)p_new_qh);

			DCACHE_CleanByRange(
				(uint32_t)p_ehci
					->QHLists[EHCI_BranchArray[ix_prev]],
				sizeof(EHCI_QH));
			DCACHE_CleanByRange(
				(uint32_t)p_ehci
					->QHLists[EHCI_BranchArray[ix_prev + 1u]],
				sizeof(EHCI_QH));

		} else if ((list_ix >= EHCI_QH_LIST_64MS) &&
			   (list_ix <= EHCI_QH_LIST_01MS)) {
			p_ehci->QHLists[ix_prev]->QHHorLinkPtr |=
				(uint32_t)((void *)p_new_qh);
			p_ehci->QHLists[ix_prev + 1u]->QHHorLinkPtr |=
				(uint32_t)((void *)p_new_qh);

			DCACHE_CleanByRange((uint32_t)p_ehci->QHLists[ix_prev],
					    sizeof(EHCI_QH));
			DCACHE_CleanByRange(
				(uint32_t)p_ehci->QHLists[ix_prev + 1u],
				sizeof(EHCI_QH));
		} else {
			/* Empty Else Statement                                 */
		}
	}

	for (list_ix = EHCI_QH_LIST_256MS; list_ix < EHCI_QH_LIST_128MS;
	     list_ix++) {
		// p_new_sitd = (EHCI_SITD *)Mem_PoolBlkGet(&HC_ITDPool,
		//                                           sizeof(EHCI_SITD),
		//                                          &err_lib);
		p_new_sitd = k_mem_pool_malloc(&HC_ITDPool, sizeof(EHCI_SITD));
		if (p_new_sitd == NULL) {
			return (-ENOMEM);
		}

		EHCI_ITD_Clr(
			(EHCI_ITD *)
				p_new_sitd); /* Clear SiTD struct (overlay with iTD struct).         */

		p_new_sitd->SITDNxtLinkPtr =
			(uint32_t)(p_ehci->QHLists[list_ix]) |
			HOR_LNK_PTR_TYP(DWORD1_TYP_QH) |
			HOR_LNK_PTR_T(DWORD1_T_VALID);
		DCACHE_CleanByRange((uint32_t)p_new_sitd, sizeof(EHCI_SITD));

		/* Insert this SiTD into periodic frame list            */
		p_ehci->PeriodicListBase[list_ix] =
			(uint32_t)((void *)p_new_sitd) |
			HOR_LNK_PTR_TYP(DWORD1_TYP_SITD);
		DCACHE_CleanByRange(
			(uint32_t)&p_ehci->PeriodicListBase[list_ix],
			sizeof(uint32_t));
	}
	/* Update the periodic list base address                */
	PERIODICLISTBASE = (uint32_t)((void *)p_ehci->PeriodicListBase);

	USBCMD |=
		EHCI_USBCMD_WR_PSE; /* Enable periodic list processing                      */

	return (0);
}
#endif

/*
*********************************************************************************************************
*                                        EHCI_AsyncListInit()
*
* Description : Initialize the asynchronous list. This will create a dummy queue head, which is the head
*               of the all queue heads, writes the asynchronous list base address with appropriate value
*               and enables the asynchronous list processing.
*
* Argument(s) : p_hc_drv      Pointer to host controller driver structure.
*
* Return(s)   : USBH_ERR_NONE,          if successful.
*               Specific error code     otherwise.
*
* Note(s)     : (1) Mark a queue head as being the head of the reclamation list.
*                   See section "4.8.3 Empty Asynchronous Schedule Detection" of EHCI spec for more details
*********************************************************************************************************
*/

static int EHCI_AsyncListInit(struct usbh_hc_drv *p_hc_drv)
{
	EHCI_QH *p_new_qh;
	EHCI_DEV *p_ehci;

	p_ehci = (EHCI_DEV *)p_hc_drv->data_ptr;
	// p_new_qh = (EHCI_QH  *)Mem_PoolBlkGet(&HC_QHPool,
	//                                        sizeof(EHCI_QH),
	//                                       &err_lib);
	p_new_qh = k_mem_pool_malloc(&HC_QHPool, sizeof(EHCI_QH));
	if (p_new_qh == NULL) {
		return (-ENOMEM);
	}

	EHCI_QH_Clr(p_new_qh);
	/* Set Head of reclamation list flag                    */
	p_new_qh->QHHorLinkPtr =
		(uint32_t)((p_new_qh)) | HOR_LNK_PTR_TYP(DWORD1_TYP_QH);
	p_new_qh->QHEpCapChar[0] = QH_EPCHAR_H(
		DWORD2_QH_R_H); /* See Note #1                                          */
	/* One transaction per micro frame                      */
	p_new_qh->QHEpCapChar[1] = QH_EPCAP_HBPM(DWORD3_QH_HBPM_1);
	p_new_qh->QHCurQTDPtr = (uint32_t)0;
	p_new_qh->QHNxtQTDPtr = (uint32_t)0x00000001;
	p_new_qh->QHAltNxtQTDPtr = (uint32_t)0x00000001;
	p_new_qh->QHToken = (uint32_t)0;
	p_new_qh->QHBufPagePtrList[0] = (uint32_t)0;
	p_new_qh->QHBufPagePtrList[1] = (uint32_t)0;
	p_new_qh->QHBufPagePtrList[2] = (uint32_t)0;
	p_new_qh->QHBufPagePtrList[3] = (uint32_t)0;
	p_new_qh->QHBufPagePtrList[4] = (uint32_t)0;
	DCACHE_CleanByRange((uint32_t)p_new_qh, sizeof(EHCI_QH));
	p_ehci->AsyncQHHead = p_new_qh;
	/* Update the async list base address                   */
	ASYNCLISTADDR = (uint32_t)((p_new_qh));

	return (0);
}

/*
*********************************************************************************************************
*                                            EHCI_QHDone()
*
* Description : Process completed queue head.
*
* Argument(s) : p_hc_drv      Pointer to host controller driver structure.
*
*               p_qh          Pointer to EHCI_QH structure.
*
* Return(s)   : None.
*
* Note(s)     : None
*********************************************************************************************************
*/

static void EHCI_QHDone(struct usbh_hc_drv *p_hc_drv, EHCI_QH *p_qh)
{
	uint32_t err_sts;
	uint32_t bytes_to_xfer;
	uint32_t qtd_head_addr;
	uint32_t qtd_head_addr_tmp;
	struct usbh_urb *p_urb;
	struct usbh_ep *p_ep;

	DCACHE_InvalidateByRange((uint32_t)p_qh, sizeof(EHCI_QH));
	p_ep = p_qh->EPPtr;
	err_sts = (p_qh->QHToken) & 0x000000FFu;
	/* Search the URB associated with this transfer */
	qtd_head_addr = (uint32_t)p_qh->QTDHead;
	p_urb = &p_ep->urb;

	if (p_urb->async_urb_nxt_ptr !=
	    0) { /* Extra URB has been allocated for this EP             */

		while (p_urb->async_urb_nxt_ptr != 0) {
			qtd_head_addr_tmp = (uint32_t)p_urb->arg_ptr;
			if (qtd_head_addr_tmp ==
			    qtd_head_addr) { /* URB associated with this transfer found              */
				break;
			}

			p_urb = p_urb->async_urb_nxt_ptr; /* Get the next extra URB in the queue                  */
		}
	}

	if ((err_sts & O_QH_STS_HALTED) !=
	    0u) { /* If QTD status is halted, retrieve error.             */
		/* EP corresponding to this QH                          */
		if ((err_sts & O_QH_STS_DBE) !=
		    0u) { /* Data Buffer Error                                    */
			p_urb->err = -EIO;
		} else if (((err_sts & O_QH_STS_BD) != 0u) ||
			   ((err_sts & O_QH_STS_XACT_ERR) != 0u) ||
			   ((err_sts & O_QH_STS_MMF) != 0u) ||
			   ((err_sts & O_QH_STS_PE) !=
			    0u)) { /* Babble Detected                                      */
			p_urb->err = -EIO;
		} else { /* If not the above errors, then it is stall            */
			p_urb->err = -EBUSY;
		}

		bytes_to_xfer = EHCI_QTDRemove(
			p_hc_drv,
			p_qh); /* Remove and free all QTDs from the QTD list           */
		p_qh->QHCurQTDPtr = 0u;
		p_urb->xfer_len = p_urb->dma_buf_len - bytes_to_xfer;
		usbh_urb_done(p_urb);

	} else if ((err_sts & O_QH_STS_ACTIVE) ==
		   0u) { /* The transaction completed successfully               */
		/* EP corresponding to this QH                          */
		p_urb->err = 0;
		bytes_to_xfer = EHCI_QTDRemove(
			p_hc_drv,
			p_qh); /* Remove and free all QTDs from the QTD list           */
		p_qh->QHCurQTDPtr = 0u;
		p_urb->xfer_len = p_urb->dma_buf_len - bytes_to_xfer;
		usbh_urb_done(p_urb);
	} else {
		/* Empty Else Statement                                 */
	}

	DCACHE_CleanByRange((uint32_t)p_qh, sizeof(EHCI_QH));
}

/*
*********************************************************************************************************
*                                            EHCI_SITDDone()
*
* Description : Process completed queue head.
*
* Argument(s) : p_hc_drv      Pointer to host controller driver structure.
*
*               p_ep_desc     Pointer to endpoint descriptor
*
*               dev_addr      Device address
*
*               ep_addr       Endpoint address
*
*               p_urb         Pointer to URB structure.
*
* Return(s)   : Total number of bytes transfered
*
* Note(s)     : (1) Table 3-11 EHCI spec. Total Bytes To Transfer field of siTD. For an OUT, the host
*                   controller decrements this value. The number of bytes transferred is not written back.
*********************************************************************************************************
*/

#if (USBH_EHCI_CFG_PERIODIC_EN == DEF_ENABLED)
static uint32_t EHCI_SITDDone(struct usbh_hc_drv *p_hc_drv,
			      EHCI_ISOC_EP_DESC *p_ep_desc, uint8_t dev_addr,
			      uint8_t ep_addr, struct usbh_urb *p_urb)
{
	struct usbh_ep *p_ep;
	EHCI_SITD *p_sitd;
	uint32_t *p_hw_desc;
	uint16_t frame_nbr;
	uint8_t err_sts;
	uint32_t len_rem_per_frame;
	uint32_t total_len_rem;
	uint32_t i;
	int err;
	EHCI_ISOC_EP_URB *p_urb_info;
	uint8_t ep_dir;
	EHCI_DEV *p_ehci;

	p_ehci = (EHCI_DEV *)p_hc_drv->data_ptr;
	p_ep = p_ep_desc->EPPtr;
	ep_dir = usbh_ep_dir_get(p_ep);
	total_len_rem = 0u;
	p_urb_info = (EHCI_ISOC_EP_URB *)p_urb->arg_ptr;
	frame_nbr = p_urb_info->AppStartFrame;

	for (i = 0u; i < p_urb_info->NbrFrame; i++) {
		frame_nbr %=
			256; /* Keep the Periodic Frame List index between 0 and 255 */
		p_hw_desc = (uint32_t *)(p_ehci->PeriodicListBase[frame_nbr] &
					 0xFFFFFFE0);
		DCACHE_InvalidateByRange((uint32_t)p_hw_desc, sizeof(uint32_t));

		while ((*p_hw_desc & 0x01u) == 0u) {
			if ((*p_hw_desc & 0x06u) ==
			    HOR_LNK_PTR_TYP(DWORD1_TYP_SITD)) {
				p_sitd = (EHCI_SITD *)(*p_hw_desc & 0xFFFFFFE0);
				DCACHE_InvalidateByRange((uint32_t)p_sitd,
							 sizeof(EHCI_SITD));

				if (((p_sitd->SITDEpCapChar[0] & 0x0000007Fu) ==
				     dev_addr) &&
				    (((p_sitd->SITDEpCapChar[0] & 0x00000F00u) >>
				      8u) == ep_addr)) {
					*p_hw_desc = p_sitd->SITDNxtLinkPtr;
					DCACHE_CleanByRange((uint32_t)p_hw_desc,
							    sizeof(uint32_t));

					err_sts = p_sitd->SITDStsCtrl &
						  0x000000F2u;

					if (((err_sts & O_SITD_STS_DBE) !=
					     0u) ||
					    ((err_sts & O_SITD_STS_ERR) !=
					     0u) ||
					    ((err_sts & O_SITD_STS_BD) != 0u) ||
					    ((err_sts & O_SITD_STS_XACT_ERR) !=
					     0u) ||
					    ((err_sts & O_SITD_STS_MMF) !=
					     0u)) {
						err = -EIO;
					} else {
						err = 0;
					}

					if (ep_dir == USBH_EP_DIR_IN) {
						/* Compute nbr of bytes remaining.                      */
						len_rem_per_frame =
							(p_sitd->SITDStsCtrl >>
							 16u) &
							0x3FFu;
						total_len_rem +=
							len_rem_per_frame;
						/* Nbr of received bytes per frame.                     */
						p_urb->isoc_desc_ptr
							->frm_len[i] -=
							len_rem_per_frame;
					} else {
						total_len_rem =
							0u; /* Nbr of bytes sent. See Note #1                       */
					}
					/* Free the siTD structure                              */
					// Mem_PoolBlkFree(       &HC_ITDPool,
					//                 (void *)p_sitd,
					//                        &err_lib);
					k_free(p_sitd);

					p_urb->isoc_desc_ptr->frm_err[i] = err;
					break;
				}
			}

			p_hw_desc = (uint32_t *)(*p_hw_desc & 0xFFFFFFE0u);
			DCACHE_InvalidateByRange((uint32_t)p_hw_desc,
						 sizeof(uint32_t));
		}

		frame_nbr += p_ep_desc->FrameInterval;
	}

	// Mem_PoolBlkFree(       &HC_Isoc_EP_URBPool,
	//                 (void *)p_urb_info,
	//                        &err_lib);
	k_free(p_urb_info);

	return (total_len_rem);
}
#endif

/*
*********************************************************************************************************
*                                            EHCI_ITDDone()
*
* Description : Process completed queue head.
*
* Argument(s) : p_hc_drv      Pointer to host controller driver structure.
*
*               p_ep_desc     Pointer to endpoint descriptor
*
*               dev_addr      Device address
*
*               ep_addr       Endpoint address
*
*               p_urb         Pointer to URB structure.
*
* Return(s)   : Number of bytes received
*
* Note(s)     : (1) Table 3-3 EHCI spec. Transaction X Length field of iTD structure: For an OUT, this
*                   field is the number of data bytes the host controller will send during the transaction.
*                   The HC is not required to update this field to reflect the actual number of bytes
*                   transferred during the transfer.
*********************************************************************************************************
*/

#if (USBH_EHCI_CFG_PERIODIC_EN == DEF_ENABLED)
static uint32_t EHCI_ITDDone(struct usbh_hc_drv *p_hc_drv,
			     EHCI_ISOC_EP_DESC *p_ep_desc, uint8_t dev_addr,
			     uint8_t ep_addr, struct usbh_urb *p_urb)
{
	struct usbh_ep *p_ep;
	EHCI_ITD *p_itd;
	uint32_t *p_hw_desc;
	uint16_t frame_nbr;
	uint8_t micro_frame_nbr;
	uint8_t err_sts;
	uint8_t i;
	uint32_t len_rxd_per_uframe;
	uint32_t total_len_rxd;
	uint8_t index;
	int err;
	EHCI_ISOC_EP_URB *p_urb_info;
	uint8_t ep_dir;
	EHCI_DEV *p_ehci;

	p_ehci = (EHCI_DEV *)p_hc_drv->data_ptr;
	p_ep = p_ep_desc->EPPtr;
	ep_dir = usbh_ep_dir_get(p_ep);
	total_len_rxd = 0u;
	p_urb_info = (EHCI_ISOC_EP_URB *)p_urb->arg_ptr;
	frame_nbr = p_urb_info->AppStartFrame;
	/* Search for iTD(s) associated to the Isoc EP          */
	for (i = 0u; i < p_urb_info->NbrFrame; i++) {
		frame_nbr %=
			256; /* Keep the Periodic Frame List index between 0 and 255 */
		/* Retrieve the 1st linked data structure at this frame */
		p_hw_desc = (uint32_t *)(p_ehci->PeriodicListBase[frame_nbr] &
					 0xFFFFFFE0);
		DCACHE_InvalidateByRange((uint32_t)p_hw_desc, sizeof(uint32_t));

		while ((*p_hw_desc & 0x01u) ==
		       0) { /* While Link Pointer is valid, browse the linked list  */
			/* Is the data structure referenced by a iTD ?          */
			if (((*p_hw_desc & 0x06u) ==
			     HOR_LNK_PTR_TYP(DWORD1_TYP_ITD))) {
				/* Retrieve the physical addr of the iTD                */
				p_itd = (EHCI_ITD *)(*p_hw_desc & 0xFFFFFFE0u);
				DCACHE_InvalidateByRange((uint32_t)p_itd,
							 sizeof(EHCI_ITD));

				if (((p_itd->ITDBufPagePtrList[0] &
				      0x0000007Fu) == dev_addr) ||
				    (((p_itd->ITDBufPagePtrList[0] &
				       0x00000F00u) >>
				      8u) == ep_addr)) {
					*p_hw_desc =
						p_itd->ITDNxtLinkPtr; /* Remove the iTD from this Periodic Frame List location*/
					DCACHE_CleanByRange((uint32_t)p_hw_desc,
							    sizeof(uint32_t));

					/* Get the completion status for each microframe        */
					for (micro_frame_nbr = 0;
					     micro_frame_nbr < 8u;
					     micro_frame_nbr++) {
						err_sts =
							(p_itd->ITDStsAndCntrl
								 [micro_frame_nbr] &
							 0xF0000000u) >>
							28u;

						if (((err_sts &
						      O_ITD_STS_DBE) != 0u) ||
						    ((err_sts & O_ITD_STS_BD) !=
						     0u) ||
						    ((err_sts &
						      O_ITD_STS_XACTERR) !=
						     0u)) {
							err = -EIO;
						} else {
							err = 0;
						}

						if (ep_dir == USBH_EP_DIR_IN) {
							/* Compute nbr of byte received                         */
							len_rxd_per_uframe =
								((p_itd->ITDStsAndCntrl
									  [micro_frame_nbr] >>
								  16u) &
								 0x0FFFu);
							total_len_rxd +=
								len_rxd_per_uframe;
							/* Nbr of received bytes per microframe.                */
							index = ((i * 8u) +
								 micro_frame_nbr);
							p_urb->isoc_desc_ptr
								->frm_len[index] =
								len_rxd_per_uframe;

						} else {
							total_len_rxd =
								0u; /* Nbr of bytes sent. See Note #1                       */
						}
					}

					/* Save the completion status of the xfer               */
					p_urb->isoc_desc_ptr->frm_err[i] = err;
					/* Free the iTD structure                               */
					// Mem_PoolBlkFree(       &HC_ITDPool,
					//                 (void *)p_itd,
					//                        &err_lib);
					k_free(p_itd);
					break;
				}
			}

			p_hw_desc = (uint32_t *)(*p_hw_desc & 0xFFFFFFE0);
			DCACHE_InvalidateByRange((uint32_t)p_hw_desc,
						 sizeof(uint32_t));
		}

		frame_nbr += p_ep_desc->FrameInterval;
	}
	/* Free the HCD Isoc EP structure                       */
	// Mem_PoolBlkFree(       &HC_Isoc_EP_URBPool,
	//                 (void *)p_urb_info,
	//                        &err_lib);
	k_free(p_urb_info);

	return (total_len_rxd);
}
#endif

/*
*********************************************************************************************************
*                                            EHCI_BW_Get()
*
* Description : Get bandwidth allocation
*
* Argument(s) : p_hc_drv     Pointer to host controller driver structure.
*
*               p_ep         Pointer to endpoint structure
*
*               p_data       Pointer to EHCI_QH structure
*
* Return(s)   : USBH_ERR_NONE          If successful.
*               Specific error code    otherwise.
*
* Note(s)     : None
*********************************************************************************************************
*/

#if (USBH_EHCI_CFG_PERIODIC_EN == DEF_ENABLED)
static int EHCI_BW_Get(struct usbh_hc_drv *p_hc_drv, struct usbh_ep *p_ep,
		       void *p_data)
{
	uint32_t min_avail; /* Minimum available BW in a branch                     */
	uint32_t max_of_min_avail; /* Maximum value of all minimum available BW            */
	uint8_t mask_nbr;
	uint16_t branch_nbr;
	uint16_t frame_nbr;
	uint8_t micro_frame_nbr;
	uint8_t nbr_mask = 0u;
	uint16_t nbr_branch;
	uint16_t frames_per_branch;
	uint8_t s_mask = 0u;
	uint8_t c_mask = 0u;
	uint32_t interval;
	uint32_t frame_interval;
	bool enough_BW;
	uint16_t ep_max_pkt_size;
	uint8_t ep_type;
	uint8_t ep_dir;
	EHCI_QH *p_qh;
	EHCI_ISOC_EP_DESC *p_ep_desc;
	EHCI_DEV *p_ehci;
	uint16_t i;
	uint8_t j;

	ep_max_pkt_size = usbh_ep_max_pkt_size_get(p_ep);
	ep_type = usbh_ep_type_get(p_ep);
	ep_dir = usbh_ep_dir_get(p_ep);
	p_ehci = (EHCI_DEV *)p_hc_drv->data_ptr;

	if ((ep_type == USBH_EP_TYPE_INTR) &&
	    (p_ep->dev_spd != USBH_HIGH_SPEED)) {
		interval = p_ep->desc.b_interval;
		j = 0u;
		for (i = 0u; i < 8u; i++) {
			if (((0x01u << i) & interval) != 0u) {
				j = i;
			}
		}
		interval &= (0x01u << j);
	} else {
		interval = 1 << (p_ep->desc.b_interval - 1);
	}

	if ((p_ep->dev_spd ==
	     USBH_HIGH_SPEED) && /* For high speed devices, interval is in usec units    */
	    (interval < 8u)) {
		switch (interval) {
		case 1u:
			s_mask = (uint8_t)
				S_MASK_1MICROFRM; /* S-Mask for One Micro frame polling rate              */
			nbr_mask = 1u;
			break;

		case 2u:
			s_mask = (uint8_t)
				S_MASK_2MICROFRM; /* S-Mask for Two Micro frame polling rate              */
			nbr_mask = 2u;
			break;

		case 4u:
			s_mask = (uint8_t)
				S_MASK_4MICROFRM; /* S-Mask for Four Micro frame polling rate             */
			nbr_mask = 4u;
			break;

		default:
			break;
		}

		frame_interval =
			1u; /* EP should be inserted in Perodic frame list at frame interval rate 1 Ms   */

	} else { /* If the EP is Non-HS or HS with interval >= 8         */
		s_mask = (uint8_t)S_MASK_8MICROFRM;
		nbr_mask = 7u;

		if (p_ep->dev_spd == USBH_HIGH_SPEED) {
			frame_interval =
				interval /
				8u; /* Convert micro frame interval to frame interval       */
		} else {
			frame_interval = interval;
		}

		if (frame_interval > 256u) {
			frame_interval = 256u;
		}
	}

	if (ep_type == USBH_EP_TYPE_INTR) {
		p_qh = (EHCI_QH *)
			p_data; /* For intr EP p_data var points to EHCI_QH struct.     */
		p_qh->FrameInterval = frame_interval;
	} else {
		p_ep_desc = (EHCI_ISOC_EP_DESC *)
			p_data; /* For isoc EP p_data pts to EHCI_ISOC_EP_DESC struct.  */
		p_ep_desc->FrameInterval = frame_interval;
	}

	max_of_min_avail = 0u;
	nbr_branch = frame_interval;
	frames_per_branch = 256u / nbr_branch;

	if (ep_type == USBH_EP_TYPE_INTR) {
		/* For each possible S-Mask                             */
		for (mask_nbr = 0u; mask_nbr < nbr_mask; mask_nbr++) {
			/* Starting from a frame number                         */
			for (branch_nbr = 0u; branch_nbr < nbr_branch;
			     branch_nbr++) {
				enough_BW = 1u;
				min_avail = EHCI_MAX_BW_PER_MICRO_FRAME;
				/* For each frame after the interval                    */
				frame_nbr = branch_nbr;
				for (i = 0u; i < frames_per_branch; i++) {
					/* For each micro frame                                 */
					for (micro_frame_nbr = 0u;
					     micro_frame_nbr < 8u;
					     micro_frame_nbr++) {
						if ((s_mask &
						     (1u << micro_frame_nbr)) !=
						    0u) { /* If corresponding bit is set in S-Mask                */
							/* Take BW in this frame number and micro frame number  */
							min_avail = MIN(
								min_avail,
								p_ehci->MaxPeriodicBWArr
									[frame_nbr]
									[micro_frame_nbr]);

							if (min_avail <
							    ep_max_pkt_size) { /* If BW is not available                               */
								enough_BW = 0u;
								break;
							} else {
							}
						}
					}

					if (enough_BW ==
					    0) { /* If BW is not available, go to next starting frame nbr*/
						break;
					}

					frame_nbr += frame_interval;
				}

				if ((min_avail > max_of_min_avail) &&
				    (enough_BW != 0)) {
					max_of_min_avail =
						min_avail; /* Take maximum of all minimum available                */
					p_qh->BWStartFrame =
						branch_nbr; /* Update starting frame number                         */
					p_qh->SMask =
						s_mask; /* Update S-Mask                                        */
					DCACHE_CleanByRange((uint32_t)p_qh,
							    sizeof(EHCI_QH));
				}
			}

			s_mask = s_mask << 1u;
		}

		if (max_of_min_avail < ep_max_pkt_size) {
			return (-EIO);
		}

	} else if ((ep_type == USBH_EP_TYPE_ISOC) &&
		   (p_ep->dev_spd == USBH_FULL_SPEED)) {
		p_ep_desc->TCnt = (ep_max_pkt_size / 188) + 1;

		if (ep_dir == USBH_EP_DIR_IN) {
			s_mask = S_MASK_SPLIT_0_MICROFRM;

		} else if (ep_dir == USBH_EP_DIR_OUT) {
			c_mask = 0u;
		} else {
			/* Empty Else Statement                                 */
		}

		switch (p_ep_desc->TCnt) {
		case 1:
			if (ep_dir == USBH_EP_DIR_OUT) {
				s_mask = S_MASK_SPLIT_0_MICROFRM;
			} else if (ep_dir == USBH_EP_DIR_IN) {
				c_mask = C_MASK_SPLIT_0_MICROFRM;
			} else {
				/* Empty Else Statement                                 */
			}
			break;

		case 2:
			if (ep_dir == USBH_EP_DIR_OUT) {
				s_mask = S_MASK_SPLIT_01_MICROFRM;
			} else if (ep_dir == USBH_EP_DIR_IN) {
				c_mask = C_MASK_SPLIT_01_MICROFRM;
			} else {
				/* Empty Else Statement                                 */
			}
			break;

		case 3:
			if (ep_dir == USBH_EP_DIR_OUT) {
				s_mask = S_MASK_SPLIT_012_MICROFRM;
			} else if (ep_dir == USBH_EP_DIR_IN) {
				c_mask = C_MASK_SPLIT_012_MICROFRM;
			} else {
				/* Empty Else Statement                                 */
			}
			break;

		case 4:
			if (ep_dir == USBH_EP_DIR_OUT) {
				s_mask = S_MASK_SPLIT_0123_MICROFRM;
			} else if (ep_dir == USBH_EP_DIR_IN) {
				c_mask = C_MASK_SPLIT_0123_MICROFRM;
			} else {
				/* Empty Else Statement                                 */
			}
			break;

		case 5:
			if (ep_dir == USBH_EP_DIR_OUT) {
				s_mask = S_MASK_SPLIT_01234_MICROFRM;
			} else if (ep_dir == USBH_EP_DIR_IN) {
				c_mask = C_MASK_SPLIT_01234_MICROFRM;
			} else {
				/* Empty Else Statement                                 */
			}
			break;

		case 6:
			if (ep_dir == USBH_EP_DIR_OUT) {
				s_mask = S_MASK_SPLIT_012345_MICROFRM;
			} else if (ep_dir == USBH_EP_DIR_IN) {
				c_mask = C_MASK_SPLIT_012345_MICROFRM;
			} else {
				/* Empty Else Statement                                 */
			}
			break;

		default:
			break;
		}

		nbr_mask = 7 - p_ep_desc->TCnt;
		/* For each possible S-Mask                             */
		for (mask_nbr = 0u; mask_nbr < nbr_mask; mask_nbr++) {
			enough_BW = 1u;
			min_avail = EHCI_MAX_BW_PER_MICRO_FRAME;

			for (frame_nbr = 0u; frame_nbr < 256u; frame_nbr++) {
				for (micro_frame_nbr = 0u; micro_frame_nbr < 8u;
				     micro_frame_nbr++) {
					if ((s_mask & (1 << micro_frame_nbr)) !=
					    0u) { /* If corresponding bit is set in S-Mask                */
						min_avail = MIN(
							min_avail,
							p_ehci->MaxPeriodicBWArr
								[frame_nbr]
								[micro_frame_nbr]);

						if (min_avail <
						    ep_max_pkt_size) { /* If BW is not available                               */
							enough_BW = 0u;
							break;
						}
					}

					if ((c_mask &
					     (1u << micro_frame_nbr)) !=
					    0u) { /* If corresponding bit is set in S-Mask                */
						min_avail = MIN(
							min_avail,
							p_ehci->MaxPeriodicBWArr
								[frame_nbr]
								[micro_frame_nbr]);

						if (min_avail <
						    ep_max_pkt_size) { /* If BW is not available                               */
							enough_BW = 0u;
							break;
						}
					}
				}

				if (enough_BW ==
				    0u) { /* If BW is not available, go to next starting frame nbr*/
					break;
				}
			}

			if ((min_avail > max_of_min_avail) &&
			    (enough_BW != 0u)) {
				max_of_min_avail =
					min_avail; /* Take max of all min available                        */
				p_ep_desc->SMask =
					s_mask; /* Update S-Mask                                        */
				p_ep_desc->CMask = c_mask;
			}

			s_mask = s_mask << 1u;
			c_mask = c_mask << 1u;
		}

		if (max_of_min_avail < ep_max_pkt_size) {
			return (-EIO);
		}
	}

	return (0);
}
#endif

/*
*********************************************************************************************************
*                                          EHCI_BW_Update()
*
* Description : Update bandwidth allocation
*
* Argument(s) : p_hc_drv     Pointer to host controller driver structure.
*
*               p_ep         Pointer to endpoint structure
*
*               p_data       Pointer to EHCI_QH structure or isochronous endpoint structure
*
*               bw_use       Determine if bandwidth needs to be incremented/decremented in the array
*
* Return(s)   : None
*
* Note(s)     : None
*********************************************************************************************************
*/

#if (USBH_EHCI_CFG_PERIODIC_EN == DEF_ENABLED)
static void EHCI_BW_Update(struct usbh_hc_drv *p_hc_drv, struct usbh_ep *p_ep,
			   void *p_data, bool bw_use)
{
	EHCI_QH *p_qh;
	EHCI_ISOC_EP_DESC *p_ep_desc;
	EHCI_DEV *p_ehci;
	uint16_t frame_nbr;
	uint8_t micro_frame_nbr;
	uint8_t start_frame_nbr;
	uint16_t ep_max_pkt_size;
	uint8_t s_mask;
	uint8_t c_mask;
	uint16_t frame_interval;
	uint16_t frames_per_branch;
	uint16_t i;
	uint8_t ep_type;

	p_ehci = (EHCI_DEV *)p_hc_drv->data_ptr;
	ep_max_pkt_size = usbh_ep_max_pkt_size_get(p_ep);
	ep_type = usbh_ep_type_get(p_ep);

	if (ep_type == USBH_EP_TYPE_INTR) {
		p_qh = (EHCI_QH *)p_data;

		DCACHE_InvalidateByRange((uint32_t)p_qh, sizeof(EHCI_QH));
		s_mask = p_qh->SMask;
		frame_interval = p_qh->FrameInterval;
		start_frame_nbr = p_qh->BWStartFrame;
	} else {
		p_ep_desc = (EHCI_ISOC_EP_DESC *)p_data;
		s_mask = p_ep_desc->SMask;
		c_mask = p_ep_desc->CMask;
		frame_interval = p_ep_desc->FrameInterval;
		start_frame_nbr = 0u;
	}

	frames_per_branch = 256u / frame_interval;

	frame_nbr = start_frame_nbr;
	for (i = 0u; i < frames_per_branch; i++) {
		/* For each micro frame                                 */
		for (micro_frame_nbr = 0u; micro_frame_nbr < 8u;
		     micro_frame_nbr++) {
			if ((s_mask & (1 << micro_frame_nbr)) !=
			    0u) { /* If corresponding bit is set in S-Mask                */
				if (bw_use ==
				    true) { /* If BW is used, decrement BW in periodic BW array     */
					p_ehci->MaxPeriodicBWArr
						[frame_nbr][micro_frame_nbr] -=
						ep_max_pkt_size;
				} else { /* If BW is released, increment BW in periodic BW array */
					p_ehci->MaxPeriodicBWArr
						[frame_nbr][micro_frame_nbr] +=
						ep_max_pkt_size;
				}
			}

			if ((ep_type == USBH_EP_TYPE_ISOC) &&
			    (p_ep->dev_spd == USBH_FULL_SPEED)) {
				if ((c_mask & (1 << micro_frame_nbr)) !=
				    0u) { /* If corresponding bit is set in S-Mask                */
					if (bw_use == true) {
						/* If BW is used, decrement the BW in periodicBW array  */
						p_ehci->MaxPeriodicBWArr
							[frame_nbr]
							[micro_frame_nbr] -=
							ep_max_pkt_size;
					} else {
						/* If BW is released, increment the BW in periodic BW array*/
						p_ehci->MaxPeriodicBWArr
							[frame_nbr]
							[micro_frame_nbr] +=
							ep_max_pkt_size;
					}
				}
			}
		}

		frame_nbr += frame_interval;
	}
}
#endif

/*
*********************************************************************************************************
*                                         EHCI_IntrEPInsert()
*
* Description : Insert an Interrupt QH in the software QH list
*
* Argument(s) : p_hc_drv           Pointer to host controller driver structure.
*
*               p_qh_to_insert     Pointer to EHCI_QH structure.
*
* Return(s)   : None
*
* Note(s)     : None
*********************************************************************************************************
*/

#if (USBH_EHCI_CFG_PERIODIC_EN == DEF_ENABLED)
static void EHCI_IntrEPInsert(struct usbh_hc_drv *p_hc_drv,
			      EHCI_QH *p_qh_to_insert)

{
	EHCI_QH *p_prev_qh;
	EHCI_DEV *p_ehci;
	uint16_t frame_interval;

	p_ehci = (EHCI_DEV *)p_hc_drv->data_ptr;

	DCACHE_InvalidateByRange((uint32_t)p_qh_to_insert, sizeof(EHCI_QH));
	frame_interval =
		p_qh_to_insert
			->FrameInterval; /* Get ep polling interval to which belongs qH.         */
	/* Get a qH placeholder.                                */
	p_prev_qh = p_ehci->QHLists[p_qh_to_insert->BWStartFrame];

	DCACHE_InvalidateByRange((uint32_t)p_prev_qh, sizeof(EHCI_QH));
	while (p_prev_qh->FrameInterval !=
	       frame_interval) { /* Search polling interval list matching to qH to insert*/

		p_prev_qh = (EHCI_QH *)(p_prev_qh->QHHorLinkPtr & 0xFFFFFFE0);
		DCACHE_InvalidateByRange((uint32_t)p_prev_qh, sizeof(EHCI_QH));
	}

	/* Insert qH at the selected placeholder.               */
	p_qh_to_insert->QHHorLinkPtr = p_prev_qh->QHHorLinkPtr;
	p_prev_qh->QHHorLinkPtr = HOR_LNK_PTR_T(
		DWORD1_T_INVALID); /* Invalidate QH Next Link Ptr so that HC ignores it.   */
	p_prev_qh->QHHorLinkPtr = (uint32_t)((void *)p_qh_to_insert) |
				  HOR_LNK_PTR_TYP(DWORD1_TYP_QH);
	p_prev_qh->QHHorLinkPtr |= HOR_LNK_PTR_T(
		DWORD1_T_VALID); /* Validate Next Link Ptr pointed to QH to insert.      */

	DCACHE_CleanByRange((uint32_t)p_qh_to_insert, sizeof(EHCI_QH));
	DCACHE_CleanByRange((uint32_t)p_prev_qh, sizeof(EHCI_QH));
}
#endif

/*
*********************************************************************************************************
*********************************************************************************************************
*                                         ROOT HUB FUNCTIONS
*********************************************************************************************************
*********************************************************************************************************
*/

/*
*********************************************************************************************************
*                                        EHCI_PortStatusGet()
*
* Description : Get port status changes and port status.
*
* Argument(s) : p_hc_drv          Pointer to host controller driver structure.
*
*               port_nbr          Port number.
*
*               p_port_status     Pointer to the port status structure.
*
* Return(s)   : DEF_OK,           If successful.
*               DEF_FAIL,         otherwise.
*
* Note(s)     : None.
*********************************************************************************************************
*/

static bool EHCI_PortStatusGet(struct usbh_hc_drv *p_hc_drv, uint8_t port_nbr,
			       struct usbh_hub_port_status *p_port_status)
{
	EHCI_DEV *p_ehci;
	uint32_t portsc;
	uint16_t status;
	uint16_t chng;

	p_ehci = (EHCI_DEV *)p_hc_drv->data_ptr;

	if ((port_nbr ==
	     0) || /* Port number starts from 1                            */
	    (port_nbr > p_ehci->NbrPorts)) {
		return (0);
	}

	portsc = PORTSC(port_nbr - 1);
	status =
		((portsc &
		  EHCI_PORTSC_RD_CCS) | /* bit0  to bit15 indicate port status                  */
		 ((portsc & EHCI_PORTSC_RD_PED) >>
		  1) | /* bit16 to bit31 indicate port status chng             */
		 ((portsc & EHCI_PORTSC_RD_PP) >> 4));

	if (p_ehci->DrvType == EHCI_HCD_GENERIC) {
		if ((portsc & EHCI_PORTSC_RD_LS) ==
		    0x400u) { /* Line status K-state: Low-speed device.               */
			status |= USBH_HUB_STATUS_PORT_LOW_SPD;
		} else if ((portsc & EHCI_PORTSC_RD_PED) != 0u) {
			status |= USBH_HUB_STATUS_PORT_HIGH_SPD;
		} else {
			/* Empty Else Statement                                 */
		}

	} else { /* Port speed detection (Synopsys USB 2.0 Host IP).     */
		switch (portsc & EHCI_SYNOPSYS_PORTSC_RD_PSPD_MASK) {
		case EHCI_SYNOPSYS_PORTSC_RD_PSPD_LS:
			status |= USBH_HUB_STATUS_PORT_LOW_SPD;
			break;

		case EHCI_SYNOPSYS_PORTSC_RD_PSPD_FS:
			status |= USBH_HUB_STATUS_PORT_FULL_SPD;
			break;

		case EHCI_SYNOPSYS_PORTSC_RD_PSPD_HS:
		default:
			status |= USBH_HUB_STATUS_PORT_HIGH_SPD;
			break;
		}
	}

	chng = (((portsc & EHCI_PORTSC_RD_CSC) >> 1) |
		((portsc & EHCI_PORTSC_RD_PEDC) >> 2));

	if ((p_ehci->PortResetChng & (1u << (port_nbr - 1u))) != 0u) {
		chng |= USBH_HUB_STATUS_C_PORT_RESET;
	}

	p_port_status->w_port_change = sys_get_le16((uint8_t *)&chng);
	p_port_status->w_port_status = sys_get_le16((uint8_t *)&status);

	return (1);
}

/*
*********************************************************************************************************
*                                          EHCI_HubDescGet()
*
* Description : Return root hub descriptor.
*
* Argument(s) : p_hc_drv     Pointer to host controller driver structure.
*
*               p_buf        Pointer to buffer that will receive hub descriptor.
*
*               buf_len      Buffer length in octets.
*
* Return(s)   : DEF_OK,      If successful.
*               DEF_FAIL,    otherwise.
*
* Note(s)     : None.
*********************************************************************************************************
*/

static bool EHCI_HubDescGet(struct usbh_hc_drv *p_hc_drv, void *p_buf,
			    uint8_t buf_len)
{
	EHCI_DEV *p_ehci;
	uint32_t hc_rh_desc_a;
	bool port_pwr_mode;
	struct usbh_hub_desc hub_desc;

	p_ehci = (EHCI_DEV *)p_hc_drv->data_ptr;

	hc_rh_desc_a = p_ehci->HcCap.HCSParams;
	port_pwr_mode = EHCI_PortPwrModeGet(
		p_ehci); /* Get port power mode                                  */
	hub_desc.b_desc_length = USBH_HUB_LEN_HUB_DESC;
	hub_desc.b_desc_type = USBH_HUB_DESC_TYPE_HUB;
	hub_desc.b_nbr_ports = hc_rh_desc_a & EHCI_HCSPARAMS_RD_NP;
	hub_desc.w_hub_characteristics = port_pwr_mode;
	hub_desc.b_hub_contr_current = 0u;

	/* Write the structure in USB format                    */
	usbh_hub_fmt_hub_desc(&hub_desc, (void *)p_ehci->EHCI_HubBuf);

	if (buf_len > sizeof(struct usbh_hub_desc)) {
		buf_len = sizeof(struct usbh_hub_desc);
	}

	// Mem_Copy(        p_buf,
	//          (void *)p_ehci->EHCI_HubBuf,
	//                  buf_len);
	memcpy(p_buf, (void *)p_ehci->EHCI_HubBuf, buf_len);

	return (1);
}

/*
*********************************************************************************************************
*                                          EHCI_PortEnSet()
*
* Description : Enable given port.
*
* Argument(s) : p_hc_drv     Pointer to host controller driver structure.
*
*               port_nbr     Port Number.
*
* Return(s)   : DEF_OK,      If successful.
*               DEF_FAIL,    otherwise.
*
* Note(s)     : None.
*********************************************************************************************************
*/

static bool EHCI_PortEnSet(struct usbh_hc_drv *p_hc_drv, uint8_t port_nbr)
{
	(void)p_hc_drv;
	(void)port_nbr;

	return (1);
}

/*
*********************************************************************************************************
*                                        EHCI_PortEnClr()
*
* Description : Clear port enable status.
*
* Argument(s) : p_hc_drv     Pointer to host controller driver structure.
*
*               port_nbr     Port Number.
*
* Return(s)   : DEF_OK,      If successful.
*               DEF_FAIL,    otherwise.
*
* Note(s)     : None.
*********************************************************************************************************
*/

static bool EHCI_PortEnClr(struct usbh_hc_drv *p_hc_drv, uint8_t port_nbr)
{
	(void)p_hc_drv;
	(void)port_nbr;

	return (1);
}

/*
*********************************************************************************************************
*                                        EHCI_PortEnChngClr()
*
* Description : Clear port enable status change.
*
* Argument(s) : p_hc_drv     Pointer to host controller driver structure.
*
*               port_nbr     Port Number.
*
* Return(s)   : DEF_OK,      If successful.
*               DEF_FAIL,    otherwise.
*
* Note(s)     : None.
*********************************************************************************************************
*/

static bool EHCI_PortEnChngClr(struct usbh_hc_drv *p_hc_drv, uint8_t port_nbr)
{
	EHCI_DEV *p_ehci;

	p_ehci = (EHCI_DEV *)p_hc_drv->data_ptr;

	if ((port_nbr ==
	     0) || /* Port number starts from 1                            */
	    (port_nbr > p_ehci->NbrPorts)) {
		return (0);
	}

	PORTSC(port_nbr - 1) |=
		EHCI_PORTSC_WR_PEDC; /* Clear Port enable/disable status chng                */

	return (1);
}

/*
*********************************************************************************************************
*                                          EHCI_PortPwrSet()
*
* Description : Set port power based on port power mode.
*
* Argument(s) : p_hc_drv     Pointer to host controller driver structure.
*
*               port_nbr     Port Number.
*
* Return(s)   : DEF_OK,      If successful.
*               DEF_FAIL,    otherwise.
*
* Note(s)     : None.
*********************************************************************************************************
*/

static bool EHCI_PortPwrSet(struct usbh_hc_drv *p_hc_drv, uint8_t port_nbr)
{
	EHCI_DEV *p_ehci;
	uint8_t pwr_mode;

	p_ehci = (EHCI_DEV *)p_hc_drv->data_ptr;

	if ((port_nbr ==
	     0) || /* Port number start from 1.                            */
	    (port_nbr > p_ehci->NbrPorts)) {
		return (0);
	}

	pwr_mode = EHCI_PortPwrModeGet(
		p_ehci); /* Determine port power mode.                           */

	if (pwr_mode == EHCI_PORT_POWERED_INDIVIDUAL) {
		PORTSC(port_nbr - 1) |=
			EHCI_PORTSC_WR_PP_ON; /* Set Port Power                                       */
	}

	return (1);
}

/*
*********************************************************************************************************
*                                          EHCI_PortPwrClr()
*
* Description : Clear port power.
*
* Argument(s) : p_hc_drv     Pointer to host controller driver structure.
*
*               port_nbr     Port Number.
*
* Return(s)   : DEF_OK,      If successful.
*               DEF_FAIL,    otherwise.
*
* Note(s)     : None.
*********************************************************************************************************
*/

static bool EHCI_PortPwrClr(struct usbh_hc_drv *p_hc_drv, uint8_t port_nbr)
{
	(void)p_hc_drv;
	(void)port_nbr;

	return (1);
}

/*
*********************************************************************************************************
*                                         EHCI_PortResetSet()
*
* Description : Reset given port.
*
* Argument(s) : p_hc_drv     Pointer to host controller driver structure.
*
*               port_nbr     Port Number.
*
* Return(s)   : DEF_OK,      If successful.
*               DEF_FAIL,    otherwise.
*
* Note(s)     : None.
*********************************************************************************************************
*/

static bool EHCI_PortResetSet(struct usbh_hc_drv *p_hc_drv, uint8_t port_nbr)
{
	EHCI_DEV *p_ehci;
	uint32_t portsc;

	p_ehci = (EHCI_DEV *)p_hc_drv->data_ptr;

	if ((port_nbr ==
	     0) || /* Port number starts from 1.                           */
	    (port_nbr > p_ehci->NbrPorts)) {
		return (0);
	}

	if ((USBSTATUS & EHCI_USBSTS_RD_HC_HAL) !=
	    0u) { /* HC is in Halted state                                */
		return (0);
	}

	if (p_ehci->DrvType == EHCI_HCD_GENERIC) {
		/* Line status K-state: Low-speed device,               */
		/* ... release port ownership.                          */
		if ((PORTSC(port_nbr - 1u) & EHCI_PORTSC_RD_LS) == 0x400u) {
			PORTSC(port_nbr - 1) |= EHCI_PORTSC_WR_CSC |
						EHCI_PORTSC_WR_PEDC |
						EHCI_PORTSC_WR_OCC;
			PORTSC(port_nbr - 1) |= EHCI_PORTSC_WR_PO;
			return (0);
		}
	}

	portsc = PORTSC(
		port_nbr -
		1); /* Clear port enable bit                                */
	portsc &= ~EHCI_PORTSC_WR_PED;
	portsc |= EHCI_PORTSC_WR_PR;
	PORTSC(port_nbr - 1) = portsc;
	p_ehci->PortResetChng |= (1 << (port_nbr - 1));

	return (1);
}

/*
*********************************************************************************************************
*                                       EHCI_PortResetChngClr()
*
* Description : Clear port reset status change.
*
* Argument(s) : p_hc_drv      Pointer to host controller driver structure.
*
*               port_nbr      Port Number.
*
* Return(s)   : DEF_OK,       If successful.
*               DEF_FAIL,     otherwise.
*
* Note(s)     : None.
*********************************************************************************************************
*/

static bool EHCI_PortResetChngClr(struct usbh_hc_drv *p_hc_drv,
				  uint8_t port_nbr)
{
	EHCI_DEV *p_ehci;
	uint8_t cnt;

	p_ehci = (EHCI_DEV *)p_hc_drv->data_ptr;
	cnt = 0u;

	if ((port_nbr ==
	     0u) || /* Port number starts from 1                            */
	    (port_nbr > p_ehci->NbrPorts)) {
		return (0);
	}

	PORTSC(port_nbr - 1) &= ~EHCI_PORTSC_WR_PR;

	k_sleep(K_MSEC(
		100u)); /* Wait until port reset is cleared                     */

	while (((PORTSC(port_nbr - 1u) & EHCI_PORTSC_RD_PR) != 0u) &&
	       (cnt < 5u)) {
		k_sleep(K_MSEC(2u));
		cnt++;
	}

	if (cnt >= 5u) {
		return (0);
	}

	p_ehci->PortResetChng &= (~(1 << (port_nbr - 1)));

	if (p_ehci->DrvType == EHCI_HCD_GENERIC) {
		/* If port is not enabled after port reset completion,  */
		if ((PORTSC(port_nbr - 1u) & EHCI_PORTSC_RD_PED) == 0u) {
			PORTSC(port_nbr - 1u) |= EHCI_PORTSC_WR_CSC |
						 EHCI_PORTSC_WR_PEDC |
						 EHCI_PORTSC_WR_OCC;
			PORTSC(port_nbr - 1u) |=
				EHCI_PORTSC_WR_PO; /* Release port ownership.                              */

			return (0); /* Not a high speed device.                             */
		}
	}

	return (1);
}

/*
*********************************************************************************************************
*                                        EHCI_PortSuspendClr()
*
* Description : Resume given port if port is suspended.
*
* Argument(s) : p_hc_drv     Pointer to host controller driver structure.
*
*               port_nbr     Port Number.
*
* Return(s)   : DEF_OK,      If successful.
*               DEF_FAIL,    otherwise.
*
* Note(s)     : None.
*********************************************************************************************************
*/

static bool EHCI_PortSuspendClr(struct usbh_hc_drv *p_hc_drv, uint8_t port_nbr)
{
	EHCI_DEV *p_ehci;

	p_ehci = (EHCI_DEV *)p_hc_drv->data_ptr;

	if ((port_nbr ==
	     0u) || /* Port number starts from 1                            */
	    (port_nbr > p_ehci->NbrPorts)) {
		return (0);
	}

	if ((PORTSC(port_nbr - 1u) & EHCI_PORTSC_RD_SUSP) != 0u) {
		k_sleep(K_MSEC(100u));
		PORTSC(port_nbr - 1u) |= EHCI_PORTSC_WR_FPR;
		k_sleep(K_MSEC(200u));
		PORTSC(port_nbr - 1u) &= ~EHCI_PORTSC_WR_FPR;
	}

	return (1);
}

/*
*********************************************************************************************************
*                                       EHCI_PortConnChngClr()
*
* Description : Clear port connect status change.
*
* Argument(s) : p_hc_drv     Pointer to host controller driver structure.
*
*               port_nbr     Port Number.
*
* Return(s)   : DEF_OK,      If successful.
*               DEF_FAIL,    otherwise.
*
* Note(s)     : None.
*********************************************************************************************************
*/

static bool EHCI_PortConnChngClr(struct usbh_hc_drv *p_hc_drv, uint8_t port_nbr)
{
	EHCI_DEV *p_ehci;

	p_ehci = (EHCI_DEV *)p_hc_drv->data_ptr;

	if ((port_nbr ==
	     0) || /* Port number starts from 1                            */
	    (port_nbr > p_ehci->NbrPorts)) {
		return (0);
	}

	PORTSC(port_nbr - 1) |=
		EHCI_PORTSC_WR_CSC; /* Clear Port connection status chng                    */

	return (1);
}

/*
*********************************************************************************************************
*                                          EHCI_PCD_IntEn()
*
* Description : Enable root hub interrupt.
*
* Argument(s) : p_hc_drv     Pointer to host controller driver structure.
*
* Return(s)   : DEF_OK,      If successful.
*               DEF_FAIL,    otherwise.
*
* Note(s)     : None.
*********************************************************************************************************
*/

static bool EHCI_PCD_IntEn(struct usbh_hc_drv *p_hc_drv)
{
	EHCI_DEV *p_ehci;

	p_ehci = (EHCI_DEV *)p_hc_drv->data_ptr;

	if (p_ehci->HC_Started == true) {
		USBINT |= EHCI_USBINTR_WR_PCIE;
	}

	return (1);
}

/*
*********************************************************************************************************
*                                          EHCI_PCD_IntDis()
*
* Description : Disable root hub interrupt.
*
* Argument(s) : p_hc_drv     Pointer to host controller driver structure.
*
* Return(s)   : DEF_OK,      If successful.
*               DEF_FAIL,    otherwise.
*
* Note(s)     : None.
*********************************************************************************************************
*/

static bool EHCI_PCD_IntDis(struct usbh_hc_drv *p_hc_drv)
{
	EHCI_DEV *p_ehci;

	p_ehci = (EHCI_DEV *)p_hc_drv->data_ptr;
	USBINT &= ~EHCI_USBINTR_WR_PCIE;

	return (1);
}

/*
*********************************************************************************************************
*                                        EHCI_PortPwrModeGet()
*
* Description : Retrieve whether the given port is individually powered, globally powered or always powered.
*
* Argument(s) : p_ehci       Pointer to EHCI_DEV structure.
*
* Return(s)   : EHCI_PORT_POWERED_ALWAYS,     if port is always powered. No switching.
*               EHCI_PORT_POWERED_INDIVIDUAL, if port is individually powered.
*
* Note(s)     : None.
*********************************************************************************************************
*/

static bool EHCI_PortPwrModeGet(EHCI_DEV *p_ehci)
{
	if ((p_ehci->HcCap.HCSParams & EHCI_HCSPARAMS_RD_PPC) != 0u) {
		return ((
			bool)EHCI_PORT_POWERED_INDIVIDUAL); /* Ports are individually powered                       */
	}

	return ((
		bool)EHCI_PORT_POWERED_ALWAYS); /* Ports are always powered                             */
}

/*
*********************************************************************************************************
*                                        EHCI_PortSuspendSet()
*
* Description : Suspend the given port if the port is enabled
*
* Argument(s) : p_ehci       Pointer to EHCI_DEV structure
*
*               port_nbr     Port number
*
* Return(s)   : USBH_ERR_NONE.
*
* Note(s)     : See EHCI specification for USB, section 4.3.1. The software must wait at least 10 msec
*               after a port indicates that it is suspended before initiating port resume
*********************************************************************************************************
*/

static int EHCI_PortSuspendSet(EHCI_DEV *p_ehci, uint32_t port_nbr)
{
	if ((PORTSC(port_nbr - 1) & EHCI_PORTSC_RD_PED) != 0u) {
		PORTSC(port_nbr - 1) |= EHCI_PORTSC_RD_SUSP;
	}

	return (0);
}
