/*
 * SPDX-License-Identifier: MIT
 *
 * Copyright (c) 2023-2025 tinyVision.ai Inc.
 */

#include <string.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>

#include <zephyr/kernel.h>
#include <zephyr/drivers/usb/udc.h>
#include <zephyr/sys/util.h>

/* TRB memory buffer fields */
#define DWC3_TRB_STATUS_BUFSIZ_MASK				GENMASK(23, 0)
#define DWC3_TRB_STATUS_PCM1_MASK				GENMASK(25, 24)
#define DWC3_TRB_STATUS_PCM1_1PKT				(0x0 << 24)
#define DWC3_TRB_STATUS_PCM1_2PKT				(0x1 << 24)
#define DWC3_TRB_STATUS_PCM1_3PKT				(0x2 << 24)
#define DWC3_TRB_STATUS_PCM1_4PKT				(0x3 << 24)
#define DWC3_TRB_STATUS_TRBSTS_MASK				GENMASK(31, 28)
#define DWC3_TRB_STATUS_TRBSTS_OK				(0x0 << 28)
#define DWC3_TRB_STATUS_TRBSTS_MISSEDISOC			(0x1 << 28)
#define DWC3_TRB_STATUS_TRBSTS_SETUPPENDING			(0x2 << 28)
#define DWC3_TRB_STATUS_TRBSTS_XFERINPROGRESS			(0x4 << 28)
#define DWC3_TRB_STATUS_TRBSTS_ZLPPENDING			(0xf << 28)
#define DWC3_TRB_CTRL_HWO					BIT(0)
#define DWC3_TRB_CTRL_LST					BIT(1)
#define DWC3_TRB_CTRL_CHN					BIT(2)
#define DWC3_TRB_CTRL_CSP					BIT(3)
#define DWC3_TRB_CTRL_TRBCTL_MASK				GENMASK(9, 4)
#define DWC3_TRB_CTRL_TRBCTL_NORMAL				(0x1 << 4)
#define DWC3_TRB_CTRL_TRBCTL_CONTROL_SETUP			(0x2 << 4)
#define DWC3_TRB_CTRL_TRBCTL_CONTROL_STATUS_2			(0x3 << 4)
#define DWC3_TRB_CTRL_TRBCTL_CONTROL_STATUS_3			(0x4 << 4)
#define DWC3_TRB_CTRL_TRBCTL_CONTROL_DATA			(0x5 << 4)
#define DWC3_TRB_CTRL_TRBCTL_ISOCHRONOUS_1			(0x6 << 4)
#define DWC3_TRB_CTRL_TRBCTL_ISOCHRONOUS_N			(0x7 << 4)
#define DWC3_TRB_CTRL_TRBCTL_LINK_TRB				(0x8 << 4)
#define DWC3_TRB_CTRL_TRBCTL_NORMAL_ZLP				(0x9 << 4)
#define DWC3_TRB_CTRL_ISP_IMI					BIT(10)
#define DWC3_TRB_CTRL_IOC					BIT(11)
#define DWC3_TRB_CTRL_PCM1_MASK					GENMASK(25, 24)
#define DWC3_TRB_CTRL_SPR					26
#define DWC3_TRB_CTRL_SIDSOFN_MASK				GENMASK(29, 14)

/* Incomplete coverage of all fields, but suited for what this driver supports */
#define DWC3_EVT_MASK						GENMASK(11, 0)
#define DWC3_DEPEVT_EPN_MASK					GENMASK(5, 1)
#define DWC3_DEPEVT_XFERCOMPLETE(epn)				(((epn) << 1) | (0x01 << 6))
#define DWC3_DEPEVT_XFERINPROGRESS(epn)				(((epn) << 1) | (0x02 << 6))
#define DWC3_DEPEVT_XFERNOTREADY(epn)				(((epn) << 1) | (0x03 << 6))
#define DWC3_DEPEVT_RXTXFIFOEVT(epn)				(((epn) << 1) | (0x04 << 6))
#define DWC3_DEPEVT_STREAMEVT(epn)				(((epn) << 1) | (0x06 << 6))
#define DWC3_DEPEVT_EPCMDCMPLT(epn)				(((epn) << 1) | (0x07 << 6))
/* For XferNotReady */
#define DWC3_DEPEVT_STATUS_B3_MASK				GENMASK(2, 0)
#define DWC3_DEPEVT_STATUS_B3_CONTROL_SETUP			(0x0 << 0)
#define DWC3_DEPEVT_STATUS_B3_CONTROL_DATA			(0x1 << 0)
#define DWC3_DEPEVT_STATUS_B3_CONTROL_STATUS			(0x2 << 0)
/* For XferComplete or XferInProgress */
#define DWC3_DEPEVT_STATUS_BUSERR				BIT(0)
#define DWC3_DEPEVT_STATUS_SHORT				BIT(1)
#define DWC3_DEPEVT_STATUS_IOC					BIT(2)
/* For XferComplete */
#define DWC3_DEPEVT_STATUS_LST					BIT(3)
/* For XferInProgress */
#define DWC3_DEPEVT_STATUS_MISSED_ISOC				BIT(3)
/* For StreamEvt */
#define DWC3_DEPEVT_STATUS_STREAMFOUND				0x1
#define DWC3_DEPEVT_STATUS_STREAMNOTFOUND			0x2
#define DWC3_DEVT_DISCONNEVT					(BIT(0) | (0x0 << 8))
#define DWC3_DEVT_USBRST					(BIT(0) | (0x1 << 8))
#define DWC3_DEVT_CONNECTDONE					(BIT(0) | (0x2 << 8))
#define DWC3_DEVT_ULSTCHNG					(BIT(0) | (0x3 << 8))
#define DWC3_DEVT_WKUPEVT					(BIT(0) | (0x4 << 8))
#define DWC3_DEVT_SUSPEND					(BIT(0) | (0x6 << 8))
#define DWC3_DEVT_SOF						(BIT(0) | (0x7 << 8))
#define DWC3_DEVT_ERRTICERR					(BIT(0) | (0x9 << 8))
#define DWC3_DEVT_CMDCMPLT					(BIT(0) | (0xa << 8))
#define DWC3_DEVT_EVNTOVERFLOW					(BIT(0) | (0xb << 8))
#define DWC3_DEVT_VNDRDEVTSTRCVED				(BIT(0) | (0xc << 8))

/* Device Endpoint Commands and Parameters */
#define DWC3_DEPCMDPAR2(n)					(0xc800 + 16 * (n))
#define DWC3_DEPCMDPAR1(n)					(0xc804 + 16 * (n))
#define DWC3_DEPCMDPAR0(n)					(0xc808 + 16 * (n))
#define DWC3_DEPCMD(n)						(0xc80c + 16 * (n))
/* Common fields to DEPCMD */
#define DWC3_DEPCMD_HIPRI_FORCERM				(1 << 11)
#define DWC3_DEPCMD_STATUS_MASK					GENMASK(15, 12)
#define DWC3_DEPCMD_STATUS_OK					(0 << 12)
#define DWC3_DEPCMD_STATUS_CMDERR				(1 << 12)
#define DWC3_DEPCMD_XFERRSCIDX_MASK				GENMASK(22, 16)
/* DEPCFG Command and Parameters */
#define DWC3_DEPCMD_DEPCFG					(1 << 0)
#define DWC3_DEPCMDPAR0_DEPCFG_EPTYPE_MASK			GENMASK(2, 1)
#define DWC3_DEPCMDPAR0_DEPCFG_EPTYPE_CTRL			(0x0 << 1)
#define DWC3_DEPCMDPAR0_DEPCFG_EPTYPE_ISOC			(0x1 << 1)
#define DWC3_DEPCMDPAR0_DEPCFG_EPTYPE_BULK			(0x2 << 1)
#define DWC3_DEPCMDPAR0_DEPCFG_EPTYPE_INT			(0x3 << 1)
#define DWC3_DEPCMDPAR0_DEPCFG_MPS_MASK				GENMASK(13, 3)
#define DWC3_DEPCMDPAR0_DEPCFG_FIFONUM_MASK			GENMASK(21, 17)
#define DWC3_DEPCMDPAR0_DEPCFG_BRSTSIZ_MASK			GENMASK(25, 22)
#define DWC3_DEPCMDPAR0_DEPCFG_ACTION_MASK			GENMASK(31, 30)
#define DWC3_DEPCMDPAR0_DEPCFG_ACTION_INIT			(0x0 << 30)
#define DWC3_DEPCMDPAR0_DEPCFG_ACTION_RESTORE			(0x1 << 30)
#define DWC3_DEPCMDPAR0_DEPCFG_ACTION_MODIFY			(0x2 << 30)
#define DWC3_DEPCMDPAR1_DEPCFG_INTRNUM_MASK			GENMASK(4, 0)
#define DWC3_DEPCMDPAR1_DEPCFG_XFERCMPLEN			BIT(8)
#define DWC3_DEPCMDPAR1_DEPCFG_XFERINPROGEN			BIT(9)
#define DWC3_DEPCMDPAR1_DEPCFG_XFERNRDYEN			BIT(10)
#define DWC3_DEPCMDPAR1_DEPCFG_RXTXFIFOEVTEN			BIT(11)
#define DWC3_DEPCMDPAR1_DEPCFG_STREAMEVTEN			BIT(13)
#define DWC3_DEPCMDPAR1_DEPCFG_LIMITTXDMA			BIT(15)
#define DWC3_DEPCMDPAR1_DEPCFG_BINTERVAL_MASK			GENMASK(23, 16)
#define DWC3_DEPCMDPAR1_DEPCFG_STRMCAP				BIT(24)
#define DWC3_DEPCMDPAR1_DEPCFG_EPNUMBER_MASK			GENMASK(29, 25)
#define DWC3_DEPCMDPAR1_DEPCFG_BULKBASED			BIT(30)
#define DWC3_DEPCMDPAR1_DEPCFG_FIFOBASED			BIT(31)
#define DWC3_DEPCMDPAR2_DEPCFG_EPSTATE_MASK			GENMASK(31, 0)
/* DEPXFERCFG Command and Parameters */
#define DWC3_DEPCMD_DEPXFERCFG					(0x2 << 0)
#define DWC3_DEPCMDPAR0_DEPXFERCFG_NUMXFERRES_MASK		GENMASK(15, 0)
/* Other Commands */
#define DWC3_DEPCMD_DEPGETSTATE					(0x3 << 0)
#define DWC3_DEPCMD_DEPSETSTALL					(0x4 << 0)
#define DWC3_DEPCMD_DEPCSTALL					(0x5 << 0)
#define DWC3_DEPCMD_DEPSTRTXFER					(0x6 << 0)
#define DWC3_DEPCMD_DEPUPDXFER					(0x7 << 0)
#define DWC3_DEPCMD_DEPENDXFER					(0x8 << 0)
#define DWC3_DEPCMD_DEPSTARTCFG					(0x9 << 0)
#define DWC3_DEPCMD_CMDACT					BIT(10)

/* Global USB2 (UTMI/ULPI) PHY configuration */
#define DWC3_GUSB2PHYCFG					0xC200
#define DWC3_GUSB2PHYCFG_PHYSOFTRST				BIT(31)
#define DWC3_GUSB2PHYCFG_ULPIEXTVBUSINDICATOR			BIT(18)
#define DWC3_GUSB2PHYCFG_ULPIEXTVBUSDRV				BIT(17)
#define DWC3_GUSB2PHYCFG_ULPICLKSUSM				BIT(16)
#define DWC3_GUSB2PHYCFG_ULPIAUTORES				BIT(15)
#define DWC3_GUSB2PHYCFG_USBTRDTIM_MASK				GENMASK(13, 10)
#define DWC3_GUSB2PHYCFG_USBTRDTIM_16BIT			(5 << 10)
#define DWC3_GUSB2PHYCFG_USBTRDTIM_8BIT				(9 << 10)
#define DWC3_GUSB2PHYCFG_ENBLSLPM				BIT(8)
#define DWC3_GUSB2PHYCFG_PHYSEL					BIT(7)
#define DWC3_GUSB2PHYCFG_SUSPHY					BIT(6)
#define DWC3_GUSB2PHYCFG_FSINTF					BIT(5)
#define DWC3_GUSB2PHYCFG_ULPI_UTMI_SEL				BIT(4)
#define DWC3_GUSB2PHYCFG_PHYIF					BIT(3)
#define DWC3_GUSB2PHYCFG_TOUTCAL_MASK				GENMASK(2, 0)

/* Global USB 3.0 PIPE Control Register */
#define DWC3_GUSB3PIPECTL					0xc2c0
#define DWC3_GUSB3PIPECTL_PHYSOFTRST				BIT(31)
#define DWC3_GUSB3PIPECTL_UX_EXIT_IN_PX				BIT(27)
#define DWC3_GUSB3PIPECTL_PING_ENHANCEMENT_EN			BIT(26)
#define DWC3_GUSB3PIPECTL_U1U2EXITFAIL_TO_RECOV			BIT(25)
#define DWC3_GUSB3PIPECTL_REQUEST_P1P2P3			BIT(24)
#define DWC3_GUSB3PIPECTL_STARTXDETU3RXDET			BIT(23)
#define DWC3_GUSB3PIPECTL_DISRXDETU3RXDET			BIT(22)
#define DWC3_GUSB3PIPECTL_P1P2P3DELAY_MASK			GENMASK(21, 19)
#define DWC3_GUSB3PIPECTL_DELAYP0TOP1P2P3			BIT(18)
#define DWC3_GUSB3PIPECTL_SUSPENDENABLE				BIT(17)
#define DWC3_GUSB3PIPECTL_DATWIDTH_MASK				GENMASK(16, 15)
#define DWC3_GUSB3PIPECTL_ABORTRXDETINU2			BIT(14)
#define DWC3_GUSB3PIPECTL_SKIPRXDET				BIT(13)
#define DWC3_GUSB3PIPECTL_LFPSP0ALGN				BIT(12)
#define DWC3_GUSB3PIPECTL_P3P2TRANOK				BIT(11)
#define DWC3_GUSB3PIPECTL_P3EXSIGP2				BIT(10)
#define DWC3_GUSB3PIPECTL_LFPSFILT				BIT(9)
#define DWC3_GUSB3PIPECTL_TXSWING				BIT(6)
#define DWC3_GUSB3PIPECTL_TXMARGIN_MASK				GENMASK(5, 3)
#define DWC3_GUSB3PIPECTL_TXDEEMPHASIS_MASK			GENMASK(2, 1)
#define DWC3_GUSB3PIPECTL_ELASTICBUFFERMODE			BIT(0)

/* USB Device Configuration Register */
#define DWC3_DCFG						0xc700
#define DWC3_DCFG_IGNORESTREAMPP				BIT(23)
#define DWC3_DCFG_LPMCAP					BIT(22)
#define DWC3_DCFG_NUMP_MASK					GENMASK(21, 17)
#define DWC3_DCFG_INTRNUM_MASK					GENMASK(16, 12)
#define DWC3_DCFG_PERFRINT_MASK					GENMASK(11, 10)
#define DWC3_DCFG_PERFRINT_80					(0x0 << 10)
#define DWC3_DCFG_PERFRINT_85					(0x1 << 10)
#define DWC3_DCFG_PERFRINT_90					(0x2 << 10)
#define DWC3_DCFG_PERFRINT_95					(0x3 << 10)
#define DWC3_DCFG_DEVADDR_MASK					GENMASK(9, 3)
#define DWC3_DCFG_DEVSPD_MASK					GENMASK(2, 0)
#define DWC3_DCFG_DEVSPD_SUPER_SPEED				(0x4 << 0)
#define DWC3_DCFG_DEVSPD_HIGH_SPEED				(0x0 << 0)
#define DWC3_DCFG_DEVSPD_FULL_SPEED				(0x1 << 0)

/* Global SoC Bus Configuration Register */
#define DWC3_GSBUSCFG0						0xc100
#define DWC3_GSBUSCFG0_DATRDREQINFO				GENMASK(31, 28)
#define DWC3_GSBUSCFG0_DESRDREQINFO				GENMASK(27, 24)
#define DWC3_GSBUSCFG0_DATWRREQINFO				GENMASK(23, 20)
#define DWC3_GSBUSCFG0_DESWRREQINFO				GENMASK(19, 16)
#define DWC3_GSBUSCFG0_DATBIGEND				BIT(11)
#define DWC3_GSBUSCFG0_DESBIGEND				BIT(10)
#define DWC3_GSBUSCFG0_INCR256BRSTENA				BIT(7)
#define DWC3_GSBUSCFG0_INCR128BRSTENA				BIT(6)
#define DWC3_GSBUSCFG0_INCR64BRSTENA				BIT(5)
#define DWC3_GSBUSCFG0_INCR32BRSTENA				BIT(4)
#define DWC3_GSBUSCFG0_INCR16BRSTENA				BIT(3)
#define DWC3_GSBUSCFG0_INCR8BRSTENA				BIT(2)
#define DWC3_GSBUSCFG0_INCR4BRSTENA				BIT(1)
#define DWC3_GSBUSCFG0_INCRBRSTENA				BIT(0)

/* Global Tx Threshold Control Register */
#define DWC3_GTXTHRCFG						0xc108
#define DWC3_GTXTHRCFG_USBTXPKTCNTSEL				BIT(29)
#define DWC3_GTXTHRCFG_USBTXPKTCNT_MASK				GENMASK(27, 24)
#define DWC3_GTXTHRCFG_USBMAXTXBURSTSIZE_MASK			GENMASK(23, 16)

/* Global control register */
#define DWC3_GCTL						0xc110
#define DWC3_GCTL_PWRDNSCALE_MASK				GENMASK(31, 19)
#define DWC3_GCTL_MASTERFILTBYPASS				BIT(18)
#define DWC3_GCTL_BYPSSETADDR					BIT(17)
#define DWC3_GCTL_U2RSTECN					BIT(16)
#define DWC3_GCTL_FRMSCLDWN_MASK				GENMASK(15, 14)
#define DWC3_GCTL_PRTCAPDIR_MASK				GENMASK(13, 12)
#define DWC3_GCTL_CORESOFTRESET					BIT(11)
#define DWC3_GCTL_DEBUGATTACH					BIT(8)
#define DWC3_GCTL_RAMCLKSEL_MASK				GENMASK(7, 6)
#define DWC3_GCTL_SCALEDOWN_MASK				GENMASK(5, 4)
#define DWC3_GCTL_DISSCRAMBLE					BIT(3)
#define DWC3_GCTL_DSBLCLKGTNG					BIT(0)

/* Global User Control Register */
#define DWC3_GUCTL						0xc12c
#define DWC3_GUCTL_NOEXTRDL					BIT(21)
#define DWC3_GUCTL_PSQEXTRRESSP_MASK				GENMASK(20, 18)
#define DWC3_GUCTL_PSQEXTRRESSP_EN				(0x1 << 18)
#define DWC3_GUCTL_SPRSCTRLTRANSEN				BIT(17)
#define DWC3_GUCTL_RESBWHSEPS					BIT(16)
#define DWC3_GUCTL_CMDEVADDR					BIT(15)
#define DWC3_GUCTL_USBHSTINAUTORETRYEN				BIT(14)
#define DWC3_GUCTL_DTCT_MASK					GENMASK(10, 9)
#define DWC3_GUCTL_DTFT_MASK					GENMASK(8, 0)

/* USB Device Control register */
#define DWC3_DCTL						0xc704
#define DWC3_DCTL_RUNSTOP					BIT(31)
#define DWC3_DCTL_CSFTRST					BIT(30)
#define DWC3_DCTL_HIRDTHRES_4					BIT(28)
#define DWC3_DCTL_HIRDTHRES_TIME_MASK				GENMASK(27, 24)
#define DWC3_DCTL_APPL1RES					BIT(23)
#define DWC3_DCTL_LPM_NYET_THRES_MASK				GENMASK(23, 20)
#define DWC3_DCTL_KEEPCONNECT					BIT(19)
#define DWC3_DCTL_L1HIBERNATIONEN				BIT(18)
#define DWC3_DCTL_CRS						BIT(17)
#define DWC3_DCTL_CSS						BIT(16)
#define DWC3_DCTL_INITU2ENA					BIT(12)
#define DWC3_DCTL_ACCEPTU2ENA					BIT(11)
#define DWC3_DCTL_INITU1ENA					BIT(10)
#define DWC3_DCTL_ACCEPTU1ENA					BIT(9)
#define DWC3_DCTL_ULSTCHNGREQ_MASK				GENMASK(8, 5)
#define DWC3_DCTL_ULSTCHNGREQ_REMOTEWAKEUP			(0x8 << 5)
#define DWC3_DCTL_TSTCTL_MASK					GENMASK(4, 1)

/* USB Device Event Enable Register */
#define DWC3_DEVTEN						0xc708
#define DWC3_DEVTEN_INACTTIMEOUTRCVEDEN				BIT(13)
#define DWC3_DEVTEN_VNDRDEVTSTRCVEDEN				BIT(12)
#define DWC3_DEVTEN_EVNTOVERFLOWEN				BIT(11)
#define DWC3_DEVTEN_CMDCMPLTEN					BIT(10)
#define DWC3_DEVTEN_ERRTICERREN					BIT(9)
#define DWC3_DEVTEN_SOFEN					BIT(7)
#define DWC3_DEVTEN_EOPFEN					BIT(6)
#define DWC3_DEVTEN_HIBERNATIONREQEVTEN				BIT(5)
#define DWC3_DEVTEN_WKUPEVTEN					BIT(4)
#define DWC3_DEVTEN_ULSTCNGEN					BIT(3)
#define DWC3_DEVTEN_CONNECTDONEEN				BIT(2)
#define DWC3_DEVTEN_USBRSTEN					BIT(1)
#define DWC3_DEVTEN_DISCONNEVTEN				BIT(0)

/* USB Device Event Register */

/* Endpoint Global Event Buffer Address (64-bit) */
#define DWC3_GEVNTADR(n)					(0xc400 + 16 * (n))
#define DWC3_GEVNTADR_LO(n)					(0xc400 + 16 * (n))
#define DWC3_GEVNTADR_HI(n)					(0xc404 + 16 * (n))

/* Endpoint Global Event Buffer Size */
#define DWC3_GEVNTSIZ(n)					(0xc408 + 16 * (n))
#define DWC3_GEVNTSIZ_EVNTINTRPTMASK				BIT(31)

/* Endpoint Global Event Buffer Count (of valid event) */
#define DWC3_GEVNTCOUNT(n)					(0xc40c + 16 * (n))

/* USB Device Active USB Endpoint Enable */
#define DWC3_DALEPENA						0xC720
#define DWC3_DALEPENA_USBACTEP(n)				(1 << (n))

/* USB Device Core Identification and Release Number Register */
#define DWC3_GCOREID						0xC120
#define DWC3_GCOREID_CORE_MASK					GENMASK(31, 16)
#define DWC3_GCOREID_REL_MASK					GENMASK(15, 0)

/* USB Globa Status register */
#define DWC3_GSTS						0xc118
#define DWC3_GSTS_CBELT_MASK					GENMASK(31, 20)
#define DWC3_GSTS_SSIC_IP					BIT(11)
#define DWC3_GSTS_OTG_IP					BIT(10)
#define DWC3_GSTS_BC_IP						BIT(9)
#define DWC3_GSTS_ADP_IP					BIT(8)
#define DWC3_GSTS_HOST_IP					BIT(7)
#define DWC3_GSTS_DEVICE_IP					BIT(6)
#define DWC3_GSTS_CSRTIMEOUT					BIT(5)
#define DWC3_GSTS_BUSERRADDRVLD					BIT(4)
#define DWC3_GSTS_CURMOD_MASK					GENMASK(1, 0)

/* USB Global TX FIFO Size register */
#define DWC3_GTXFIFOSIZ(n)					(0xc300 + 4 * (n))
#define DWC3_GTXFIFOSIZ_TXFSTADDR_MASK				GENMASK(31, 16)
#define DWC3_GTXFIFOSIZ_TXFDEP_MASK				GENMASK(15, 0)

/* USB Global RX FIFO Size register */
#define DWC3_GRXFIFOSIZ(n)					(0xc380 + 4 * (n))
#define DWC3_GRXFIFOSIZ_TXFSTADDR_MASK				GENMASK(31, 16)
#define DWC3_GRXFIFOSIZ_TXFDEP_MASK				GENMASK(15, 0)

/* USB Bus Error Address registers */
#define DWC3_GBUSERRADDR					0xc130
#define DWC3_GBUSERRADDR_LO					0xc130
#define DWC3_GBUSERRADDR_HI					0xc134

/* USB Controller Debug register */
#define DWC3_CTLDEBUG						0xe000
#define DWC3_CTLDEBUG_LO					0xe000
#define DWC3_CTLDEBUG_HI					0xe004

/* USB Analyzer Trace register */
#define DWC3_ANALYZERTRACE					0xe008

/* USB Global Debug Queue/FIFO Space Available register */
#define DWC3_GDBGFIFOSPACE					0xc160
#define DWC3_GDBGFIFOSPACE_AVAILABLE_MASK			GENMASK(31, 16)
#define DWC3_GDBGFIFOSPACE_QUEUETYPE_MASK			GENMASK(8, 5)
#define DWC3_GDBGFIFOSPACE_QUEUETYPE_TX				(0x0 << 5)
#define DWC3_GDBGFIFOSPACE_QUEUETYPE_RX				(0x1 << 5)
#define DWC3_GDBGFIFOSPACE_QUEUETYPE_TXREQ			(0x2 << 5)
#define DWC3_GDBGFIFOSPACE_QUEUETYPE_RXREQ			(0x3 << 5)
#define DWC3_GDBGFIFOSPACE_QUEUETYPE_RXINFO			(0x4 << 5)
#define DWC3_GDBGFIFOSPACE_QUEUETYPE_PROTOCOL			(0x5 << 5)
#define DWC3_GDBGFIFOSPACE_QUEUETYPE_DESCFETCH			(0x6 << 5)
#define DWC3_GDBGFIFOSPACE_QUEUETYPE_WREVENT			(0x7 << 5)
#define DWC3_GDBGFIFOSPACE_QUEUETYPE_AUXEVENT			(0x8 << 5)
#define DWC3_GDBGFIFOSPACE_QUEUENUM_MASK			GENMASK(4, 0)

/* USB Global Debug LTSSM register */
#define DWC3_GDBGLTSSM						0xc164

/* Global Debug LNMCC Register */
#define DWC3_GDBGLNMCC						0xc168

/* Global Debug BMU Register */
#define DWC3_GDBGBMU						0xc16c

/* Global Debug LSP MUX Register - Device*/
#define DWC3_GDBGLSPMUX_DEV					0xc170

/* Global Debug LSP MUX Register - Host */
#define DWC3_GDBGLSPMUX_HST					0xc170

/* Global Debug LSP Register */
#define DWC3_GDBGLSP						0xc174

/* Global Debug Endpoint Information Register 0 */
#define DWC3_GDBGEPINFO0					0xc178

/* Global Debug Endpoint Information Register 1 */
#define DWC3_GDBGEPINFO1					0xc17c

/* U3 Root Hub Debug Register */
#define DWC3_BU3RHBDBG0						0xd800

/* USB Device Status register */
#define DWC3_DSTS						0xC70C
#define DWC3_DSTS_DCNRD						BIT(29)
#define DWC3_DSTS_SRE						BIT(28)
#define DWC3_DSTS_RSS						BIT(25)
#define DWC3_DSTS_SSS						BIT(24)
#define DWC3_DSTS_COREIDLE					BIT(23)
#define DWC3_DSTS_DEVCTRLHLT					BIT(22)
#define DWC3_DSTS_USBLNKST_MASK					GENMASK(21, 18)
#define DWC3_DSTS_USBLNKST_USB3_U0				(0x0 << 18)
#define DWC3_DSTS_USBLNKST_USB3_U1				(0x1 << 18)
#define DWC3_DSTS_USBLNKST_USB3_U2				(0x2 << 18)
#define DWC3_DSTS_USBLNKST_USB3_U3				(0x3 << 18)
#define DWC3_DSTS_USBLNKST_USB3_SS_DIS				(0x4 << 18)
#define DWC3_DSTS_USBLNKST_USB3_RX_DET				(0x5 << 18)
#define DWC3_DSTS_USBLNKST_USB3_SS_INACT			(0x6 << 18)
#define DWC3_DSTS_USBLNKST_USB3_POLL				(0x7 << 18)
#define DWC3_DSTS_USBLNKST_USB3_RECOV				(0x8 << 18)
#define DWC3_DSTS_USBLNKST_USB3_HRESET				(0x9 << 18)
#define DWC3_DSTS_USBLNKST_USB3_CMPLY				(0xa << 18)
#define DWC3_DSTS_USBLNKST_USB3_LPBK				(0xb << 18)
#define DWC3_DSTS_USBLNKST_USB3_RESET_RESUME			(0xf << 18)
#define DWC3_DSTS_USBLNKST_USB2_ON_STATE			(0x0 << 18)
#define DWC3_DSTS_USBLNKST_USB2_SLEEP_STATE			(0x2 << 18)
#define DWC3_DSTS_USBLNKST_USB2_SUSPEND_STATE			(0x3 << 18)
#define DWC3_DSTS_USBLNKST_USB2_DISCONNECTED			(0x4 << 18)
#define DWC3_DSTS_USBLNKST_USB2_EARLY_SUSPEND			(0x5 << 18)
#define DWC3_DSTS_USBLNKST_USB2_RESET				(0xe << 18)
#define DWC3_DSTS_USBLNKST_USB2_RESUME				(0xf << 18)
#define DWC3_DSTS_RXFIFOEMPTY					BIT(17)
#define DWC3_DSTS_SOFFN_MASK					GENMASK(16, 3)
#define DWC3_DSTS_CONNECTSPD_MASK				GENMASK(2, 0)
#define DWC3_DSTS_CONNECTSPD_HS					(0x0 << 0)
#define DWC3_DSTS_CONNECTSPD_FS					(0x1 << 0)
#define DWC3_DSTS_CONNECTSPD_SS					(0x4 << 0)

/* Device Generic Command and Parameter */
#define DWC3_DGCMDPAR						0xc710
#define DWC3_DGCMD						0xc714
#define DWC3_DGCMD_STATUS_MASK					GENMASK(15, 12)
#define DWC3_DGCMD_STATUS_ERR					(1 << 12)
#define DWC3_DGCMD_STATUS_OK					(0 << 12)
#define DWC3_DGCMD_ACT						BIT(10)
#define DWC3_DGCMD_IOC						BIT(8)
#define DWC3_DGCMD_MASK						GENMASK(7, 0)
/* EXITLATENCY command and parameters */
#define DWC3_DGCMD_EXITLATENCY					(2 << 0)
/* Other Commands and Parameters */
#define DWC3_DGCMD_LINKFUNCTION					(1 << 0)
#define DWC3_DGCMD_WAKENOTIFNUM					(3 << 0)
#define DWC3_DGCMD_FIFOFLUSHONE					(9 << 0)
#define DWC3_DGCMD_FIFOFLUSHALL					(10 << 0)
#define DWC3_DGCMD_ENDPOINTNRDY					(12 << 0)
#define DWC3_DGCMD_LOOPBACKTEST					(16 << 0)
#define DWC3_DGCMD_ROLEREQUEST					(6 << 0)

/* USB 2.0 PHY External CSR Configuration register 0 */
#define DWC3_U2PHYCTRL0						0x00018000
#define DWC3_U2PHYCTRL0_LDO_1P0_ADJ_MASK			GENMASK(1, 0)
#define DWC3_U2PHYCTRL0_LS_CROSS_ADJ_MASK			GENMASK(4, 2)
#define DWC3_U2PHYCTRL0_HS_RISE_TUNE_MASK			GENMASK(6, 5)
#define DWC3_U2PHYCTRL0_CDR_RST_SEL				BIT(7)
#define DWC3_U2PHYCTRL0_GLOBAL_CONFIG_MASK			GENMASK(15, 8)
#define DWC3_U2PHYCTRL0_FS_CROSS_ADJ_MASK			GENMASK(18, 16)
#define DWC3_U2PHYCTRL0_REG14_ADJ_MASK				GENMASK(21, 19)
#define DWC3_U2PHYCTRL0_HS_REG0P8_ADJ_MASK			GENMASK(23, 22)
#define DWC3_U2PHYCTRL0_SQL_SP_ADJ_MASK				GENMASK(27, 24)
#define DWC3_U2PHYCTRL0_VBUS_VLD_ADJ_MASK			GENMASK(31, 28)

/* USB 2.0 PHY External CSR Configuration register 1 */
#define DWC3_U2PHYCTRL1						0x00018004
#define DWC3_U2PHYCTRL1_CALIB_ONCE_EN				BIT(0)
#define DWC3_U2PHYCTRL1_HS_EMP_ADJ_MASK				GENMASK(2, 1)
#define DWC3_U2PHYCTRL1_SQL_CUR_ADJ_MASK			GENMASK(4, 3)
#define DWC3_U2PHYCTRL1_PLLBW_SEL_MASK				GENMASK(7, 5)
#define DWC3_U2PHYCTRL1_BIST_EN_N				BIT(8)
#define DWC3_U2PHYCTRL1_SCP_EN					BIT(9)
#define DWC3_U2PHYCTRL1_SEL_12_24M				BIT(10)
#define DWC3_U2PHYCTRL1_HS_LP_MODE_EN				BIT(11)
#define DWC3_U2PHYCTRL1_SQL_VTH_ADJ_MASK			GENMASK(15, 12)
#define DWC3_U2PHYCTRL1_HS_EMP_EN				BIT(16)
#define DWC3_U2PHYCTRL1_RSTN_BYPASS				BIT(17)
#define DWC3_U2PHYCTRL1_CDR_BW_SEL_MASK				GENMASK(19, 18)
#define DWC3_U2PHYCTRL1_CDR_TIMING_SEL_MASK			GENMASK(23, 20)
#define DWC3_U2PHYCTRL1_SEL_INTERNALCLK				BIT(24)
#define DWC3_U2PHYCTRL1_XOSC_CUR_ADJ_MASK			GENMASK(27, 25)
#define DWC3_U2PHYCTRL1_DISC_ADJ_MASK				GENMASK(31, 28)

/* USB 2.0 PHY External CSR Configuration register 2 */
#define DWC3_U2PHYCTRL2						0x00018008
#define DWC3_U2PHYCTRL2_XCLK12MOUTEN				BIT(0)
#define DWC3_U2PHYCTRL2_TEST_LOOP_MASK				GENMASK(3, 1)
#define DWC3_U2PHYCTRL2_RX_LP_EN				BIT(4)
#define DWC3_U2PHYCTRL2_REG20_ADJ_MASK				GENMASK(7, 5)
#define DWC3_U2PHYCTRL2_CLK_SEL					BIT(8)
#define DWC3_U2PHYCTRL2_INTERNAL_RST				BIT(9)
#define DWC3_U2PHYCTRL2_REFCLK_SEL				BIT(10)
#define DWC3_U2PHYCTRL2_BIST_DONE				BIT(11)
#define DWC3_U2PHYCTRL2_BIST_ERR				BIT(12)

/* USB 3.0 PHY External Configuration registers (undocumented) */
#define DWC3_U3PHYCTRL0						0x000100C8
#define DWC3_U3PHYCTRL1						0x0001008C
#define DWC3_U3PHYCTRL2						0x00010090
#define DWC3_U3PHYCTRL3						0x00010094
#define DWC3_U3PHYCTRL4						0x00010040
#define DWC3_U3PHYCTRL4_INT_CLOCK				BIT(14)

/* USB 3.0 PHY Internal Configuration register (undocumented) */
#define DWC3_U3PHYCTRL5						0x00014010

/* Hardware parameters */
#define DWC3_GHWPARAMS0						0xc140
#define DWC3_GHWPARAMS1						0xc144
#define DWC3_GHWPARAMS2						0xc148
#define DWC3_GHWPARAMS3						0xc14c
#define DWC3_GHWPARAMS4						0xc150
#define DWC3_GHWPARAMS5						0xc154
#define DWC3_GHWPARAMS6						0xc158
#define DWC3_GHWPARAMS6_USB3_HSPHY_INTERFACE			GENMASK(5, 4)
#define DWC3_GHWPARAMS7						0xc15c
#define DWC3_GHWPARAMS8						0xc600

/* Helper macros */
#define LO32(n)			((uint32_t)((uint64_t)(n) & 0xffffffff))
#define HI32(n)			((uint32_t)((uint64_t)(n) >> 32))
#define LOG_EVENT(name)		LOG_DBG("--- %s ---", #name)
#define USB_EP_IS_CONTROL(addr)	((addr) == USB_CONTROL_EP_IN || (addr) == USB_CONTROL_EP_OUT)
#define NORMAL_EP(n, fn)	fn(n + 2)
#define EP_PHYS_NUMBER(addr)	((((addr) << 1) | ((addr) >> 7)) & 0xff)

/**
 * @brief One DMA transaction request passed from the CPU to the DWC3 core.
 *
 * This structure is described by the datasheet of DWC3 and shared between the hardware and
 * software driver. If the architecture involves cache, it must be flushed before accessing
 * this memory region.
 */
struct dwc3_trb {
	uint32_t addr_lo;
	uint32_t addr_hi;
	uint32_t status;
	uint32_t ctrl;
} __packed __aligned(16);

/**
 * @brief Controller configuration items that can remain in non-volatile memory
 */
struct dwc3_config {
	size_t num_in_eps;
	size_t num_out_eps;

	/* USB endpoints data */
	struct dwc3_ep_data *ep_data_in;
	struct dwc3_ep_data *ep_data_out;

	/* Pointer to the DMA-accessible buffer of TRBs and its size */
	struct dwc3_trb (*trb_buf_in)[CONFIG_UDC_DWC3_TRB_NUM];
	struct dwc3_trb (*trb_buf_out)[CONFIG_UDC_DWC3_TRB_NUM];

	/* USB device configuration */
	int maximum_speed_idx;

	/* Base address at which the DWC3 core registers are mapped */
	uintptr_t base;

	/* Pointers to event buffer fetched by DWC3 with DMA */
	volatile uint32_t *evt_buf;

	/* IRQ management functions */
	void (*irq_enable_func)(void);
	void (*irq_clear_func)(void);
};

/**
 * @brief All data specific to one endpoint for use by the driver.
 */
struct dwc3_ep_data {
	/* Allow to cast a pointer between ep_data and ep_cfg */
	struct udc_ep_config cfg;

	/* Endpoint number (physical address): the logical address is on ep_cfg */
	int epn;

	/* A work queue entry to process the buffers to submit on that endpoint */
	struct k_work work;

	/* Point back to the device for work queues */
	const struct device *dev;

	/* Buffer of pointers to net_buf, with index matching the position in the TRB buffers */
	struct net_buf *net_buf[CONFIG_UDC_DWC3_TRB_NUM];

	/* Buffer of TRB structures, with index matching the position in the net_buf buffers */
	struct dwc3_trb *trb_buf;

	/* Index of the next TRB to receive data in the TRB ring, Link TRB excluded */
	size_t head, tail;

	/* Total size sent from the device to the host for the ongoing transfer */
	size_t total;

	/* A flag to tell when the ring buffer is full */
	bool full;

	/* Given by the hardware for use in endpoint commands */
	uint32_t xferrscidx;
};

/**
 * @brief Data of each instance of the driver, that can be read and written to.
 *
 * Accessed via "udc_get_private(dev)".
 */
struct dwc3_data {
	/* Index within trb where to queue new TRBs */
	uint32_t evt_next;

	/* A work queue entry to process the events from the event buffer */
	struct k_work_delayable dwork;

	/* Back-reference to parent */
	const struct device *dev;

	/* Size of the data stage of control transaction */
	size_t data_stage_length;

	/* Check whether the USB stack is read */
	atomic_t flags;
#define DWC3_FLAG_INITIALIZED 1
};

/**
 * @brief Indexes matching the "device-speed" devicetree property values.
 */
enum {
	DWC3_SPEED_IDX_FULL_SPEED = 1,
	DWC3_SPEED_IDX_HIGH_SPEED = 2,
	DWC3_SPEED_IDX_SUPER_SPEED = 3,
};
