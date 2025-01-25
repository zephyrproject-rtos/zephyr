/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-FileCopyrightText: Copyright tinyVision.ai Inc.
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT snps_dwc3

#include <string.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>

#include <zephyr/kernel.h>
#include <zephyr/drivers/usb/udc.h>
#include <zephyr/sys/device_mmio.h>
#include <zephyr/sys/util.h>
#include <zephyr/shell/shell.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(dwc3, CONFIG_UDC_DRIVER_LOG_LEVEL);

#include "udc_common.h"

/* TRB memory buffer fields */
#define UDC_DWC3_TRB_STATUS_BUFSIZ_MASK				GENMASK(23, 0)
#define UDC_DWC3_TRB_STATUS_PCM1_MASK				GENMASK(25, 24)
#define UDC_DWC3_TRB_STATUS_PCM1_1PKT				(0x0 << 24)
#define UDC_DWC3_TRB_STATUS_PCM1_2PKT				(0x1 << 24)
#define UDC_DWC3_TRB_STATUS_PCM1_3PKT				(0x2 << 24)
#define UDC_DWC3_TRB_STATUS_PCM1_4PKT				(0x3 << 24)
#define UDC_DWC3_TRB_STATUS_TRBSTS_MASK				GENMASK(31, 28)
#define UDC_DWC3_TRB_STATUS_TRBSTS_OK				(0x0 << 28)
#define UDC_DWC3_TRB_STATUS_TRBSTS_MISSEDISOC			(0x1 << 28)
#define UDC_DWC3_TRB_STATUS_TRBSTS_SETUPPENDING			(0x2 << 28)
#define UDC_DWC3_TRB_STATUS_TRBSTS_XFERINPROGRESS		(0x4 << 28)
#define UDC_DWC3_TRB_STATUS_TRBSTS_ZLPPENDING			(0xf << 28)
#define UDC_DWC3_TRB_CTRL_HWO					BIT(0)
#define UDC_DWC3_TRB_CTRL_LST					BIT(1)
#define UDC_DWC3_TRB_CTRL_CHN					BIT(2)
#define UDC_DWC3_TRB_CTRL_CSP					BIT(3)
#define UDC_DWC3_TRB_CTRL_TRBCTL_MASK				GENMASK(9, 4)
#define UDC_DWC3_TRB_CTRL_TRBCTL_NORMAL				(0x1 << 4)
#define UDC_DWC3_TRB_CTRL_TRBCTL_CONTROL_SETUP			(0x2 << 4)
#define UDC_DWC3_TRB_CTRL_TRBCTL_CONTROL_STATUS_2		(0x3 << 4)
#define UDC_DWC3_TRB_CTRL_TRBCTL_CONTROL_STATUS_3		(0x4 << 4)
#define UDC_DWC3_TRB_CTRL_TRBCTL_CONTROL_DATA			(0x5 << 4)
#define UDC_DWC3_TRB_CTRL_TRBCTL_ISOCHRONOUS_1			(0x6 << 4)
#define UDC_DWC3_TRB_CTRL_TRBCTL_ISOCHRONOUS_N			(0x7 << 4)
#define UDC_DWC3_TRB_CTRL_TRBCTL_LINK_TRB			(0x8 << 4)
#define UDC_DWC3_TRB_CTRL_TRBCTL_NORMAL_ZLP			(0x9 << 4)
#define UDC_DWC3_TRB_CTRL_ISP_IMI				BIT(10)
#define UDC_DWC3_TRB_CTRL_IOC					BIT(11)
#define UDC_DWC3_TRB_CTRL_PCM1_MASK				GENMASK(25, 24)
#define UDC_DWC3_TRB_CTRL_SPR					26
#define UDC_DWC3_TRB_CTRL_SIDSOFN_MASK				GENMASK(29, 14)

/* Incomplete coverage of all fields, but suited for what this driver supports */
#define UDC_DWC3_EVT_MASK					GENMASK(11, 0)
#define UDC_DWC3_DEPEVT_EPN_MASK				GENMASK(5, 1)
#define UDC_DWC3_DEPEVT_XFERCOMPLETE(epn)			(((epn) << 1) | (0x01 << 6))
#define UDC_DWC3_DEPEVT_XFERINPROGRESS(epn)			(((epn) << 1) | (0x02 << 6))
#define UDC_DWC3_DEPEVT_XFERNOTREADY(epn)			(((epn) << 1) | (0x03 << 6))
#define UDC_DWC3_DEPEVT_RXTXFIFOEVT(epn)			(((epn) << 1) | (0x04 << 6))
#define UDC_DWC3_DEPEVT_STREAMEVT(epn)				(((epn) << 1) | (0x06 << 6))
#define UDC_DWC3_DEPEVT_EPCMDCMPLT(epn)				(((epn) << 1) | (0x07 << 6))
/* For XferNotReady */
#define UDC_DWC3_DEPEVT_STATUS_B3_MASK				GENMASK(2, 0)
#define UDC_DWC3_DEPEVT_STATUS_B3_CONTROL_SETUP			(0x0 << 0)
#define UDC_DWC3_DEPEVT_STATUS_B3_CONTROL_DATA			(0x1 << 0)
#define UDC_DWC3_DEPEVT_STATUS_B3_CONTROL_STATUS		(0x2 << 0)
/* For XferComplete or XferInProgress */
#define UDC_DWC3_DEPEVT_STATUS_BUSERR				BIT(0)
#define UDC_DWC3_DEPEVT_STATUS_SHORT				BIT(1)
#define UDC_DWC3_DEPEVT_STATUS_IOC				BIT(2)
/* For XferComplete */
#define UDC_DWC3_DEPEVT_STATUS_LST				BIT(3)
/* For XferInProgress */
#define UDC_DWC3_DEPEVT_STATUS_MISSED_ISOC			BIT(3)
/* For StreamEvt */
#define UDC_DWC3_DEPEVT_STATUS_STREAMFOUND			0x1
#define UDC_DWC3_DEPEVT_STATUS_STREAMNOTFOUND			0x2
#define UDC_DWC3_DEVT_DISCONNEVT				(BIT(0) | (0x0 << 8))
#define UDC_DWC3_DEVT_USBRST					(BIT(0) | (0x1 << 8))
#define UDC_DWC3_DEVT_CONNECTDONE				(BIT(0) | (0x2 << 8))
#define UDC_DWC3_DEVT_ULSTCHNG					(BIT(0) | (0x3 << 8))
#define UDC_DWC3_DEVT_WKUPEVT					(BIT(0) | (0x4 << 8))
#define UDC_DWC3_DEVT_SUSPEND					(BIT(0) | (0x6 << 8))
#define UDC_DWC3_DEVT_SOF					(BIT(0) | (0x7 << 8))
#define UDC_DWC3_DEVT_ERRTICERR					(BIT(0) | (0x9 << 8))
#define UDC_DWC3_DEVT_CMDCMPLT					(BIT(0) | (0xa << 8))
#define UDC_DWC3_DEVT_EVNTOVERFLOW				(BIT(0) | (0xb << 8))
#define UDC_DWC3_DEVT_VNDRDEVTSTRCVED				(BIT(0) | (0xc << 8))

/* Device Endpoint Commands and Parameters */
#define UDC_DWC3_DEPCMDPAR2(n)					(0xc800 + 16 * (n))
#define UDC_DWC3_DEPCMDPAR1(n)					(0xc804 + 16 * (n))
#define UDC_DWC3_DEPCMDPAR0(n)					(0xc808 + 16 * (n))
#define UDC_DWC3_DEPCMD(n)					(0xc80c + 16 * (n))
/* Common fields to DEPCMD */
#define UDC_DWC3_DEPCMD_HIPRI_FORCERM				(1 << 11)
#define UDC_DWC3_DEPCMD_STATUS_MASK				GENMASK(15, 12)
#define UDC_DWC3_DEPCMD_STATUS_OK				(0 << 12)
#define UDC_DWC3_DEPCMD_STATUS_CMDERR				(1 << 12)
#define UDC_DWC3_DEPCMD_XFERRSCIDX_MASK				GENMASK(22, 16)
/* DEPCFG Command and Parameters */
#define UDC_DWC3_DEPCMD_DEPCFG					(1 << 0)
#define UDC_DWC3_DEPCMDPAR0_DEPCFG_EPTYPE_MASK			GENMASK(2, 1)
#define UDC_DWC3_DEPCMDPAR0_DEPCFG_EPTYPE_CTRL			(0x0 << 1)
#define UDC_DWC3_DEPCMDPAR0_DEPCFG_EPTYPE_ISOC			(0x1 << 1)
#define UDC_DWC3_DEPCMDPAR0_DEPCFG_EPTYPE_BULK			(0x2 << 1)
#define UDC_DWC3_DEPCMDPAR0_DEPCFG_EPTYPE_INT			(0x3 << 1)
#define UDC_DWC3_DEPCMDPAR0_DEPCFG_MPS_MASK			GENMASK(13, 3)
#define UDC_DWC3_DEPCMDPAR0_DEPCFG_FIFONUM_MASK			GENMASK(21, 17)
#define UDC_DWC3_DEPCMDPAR0_DEPCFG_BRSTSIZ_MASK			GENMASK(25, 22)
#define UDC_DWC3_DEPCMDPAR0_DEPCFG_ACTION_MASK			GENMASK(31, 30)
#define UDC_DWC3_DEPCMDPAR0_DEPCFG_ACTION_INIT			(0x0 << 30)
#define UDC_DWC3_DEPCMDPAR0_DEPCFG_ACTION_RESTORE		(0x1 << 30)
#define UDC_DWC3_DEPCMDPAR0_DEPCFG_ACTION_MODIFY		(0x2 << 30)
#define UDC_DWC3_DEPCMDPAR1_DEPCFG_INTRNUM_MASK			GENMASK(4, 0)
#define UDC_DWC3_DEPCMDPAR1_DEPCFG_XFERCMPLEN			BIT(8)
#define UDC_DWC3_DEPCMDPAR1_DEPCFG_XFERINPROGEN			BIT(9)
#define UDC_DWC3_DEPCMDPAR1_DEPCFG_XFERNRDYEN			BIT(10)
#define UDC_DWC3_DEPCMDPAR1_DEPCFG_RXTXFIFOEVTEN		BIT(11)
#define UDC_DWC3_DEPCMDPAR1_DEPCFG_STREAMEVTEN			BIT(13)
#define UDC_DWC3_DEPCMDPAR1_DEPCFG_LIMITTXDMA			BIT(15)
#define UDC_DWC3_DEPCMDPAR1_DEPCFG_BINTERVAL_MASK		GENMASK(23, 16)
#define UDC_DWC3_DEPCMDPAR1_DEPCFG_STRMCAP			BIT(24)
#define UDC_DWC3_DEPCMDPAR1_DEPCFG_EPNUMBER_MASK		GENMASK(29, 25)
#define UDC_DWC3_DEPCMDPAR1_DEPCFG_BULKBASED			BIT(30)
#define UDC_DWC3_DEPCMDPAR1_DEPCFG_FIFOBASED			BIT(31)
#define UDC_DWC3_DEPCMDPAR2_DEPCFG_EPSTATE_MASK			GENMASK(31, 0)
/* DEPXFERCFG Command and Parameters */
#define UDC_DWC3_DEPCMD_DEPXFERCFG				(0x2 << 0)
#define UDC_DWC3_DEPCMDPAR0_DEPXFERCFG_NUMXFERRES_MASK		GENMASK(15, 0)
/* Other Commands */
#define UDC_DWC3_DEPCMD_DEPGETSTATE				(0x3 << 0)
#define UDC_DWC3_DEPCMD_DEPSETSTALL				(0x4 << 0)
#define UDC_DWC3_DEPCMD_DEPCSTALL				(0x5 << 0)
#define UDC_DWC3_DEPCMD_DEPSTRTXFER				(0x6 << 0)
#define UDC_DWC3_DEPCMD_DEPUPDXFER				(0x7 << 0)
#define UDC_DWC3_DEPCMD_DEPENDXFER				(0x8 << 0)
#define UDC_DWC3_DEPCMD_DEPSTARTCFG				(0x9 << 0)
#define UDC_DWC3_DEPCMD_CMDACT					BIT(10)

/* Global USB2 (UTMI/ULPI) PHY configuration */
#define UDC_DWC3_GUSB2PHYCFG					0xC200
#define UDC_DWC3_GUSB2PHYCFG_PHYSOFTRST				BIT(31)
#define UDC_DWC3_GUSB2PHYCFG_ULPIEXTVBUSINDICATOR		BIT(18)
#define UDC_DWC3_GUSB2PHYCFG_ULPIEXTVBUSDRV			BIT(17)
#define UDC_DWC3_GUSB2PHYCFG_ULPICLKSUSM			BIT(16)
#define UDC_DWC3_GUSB2PHYCFG_ULPIAUTORES			BIT(15)
#define UDC_DWC3_GUSB2PHYCFG_USBTRDTIM_MASK			GENMASK(13, 10)
#define UDC_DWC3_GUSB2PHYCFG_USBTRDTIM_16BIT			(5 << 10)
#define UDC_DWC3_GUSB2PHYCFG_USBTRDTIM_8BIT			(9 << 10)
#define UDC_DWC3_GUSB2PHYCFG_ENBLSLPM				BIT(8)
#define UDC_DWC3_GUSB2PHYCFG_PHYSEL				BIT(7)
#define UDC_DWC3_GUSB2PHYCFG_SUSPHY				BIT(6)
#define UDC_DWC3_GUSB2PHYCFG_FSINTF				BIT(5)
#define UDC_DWC3_GUSB2PHYCFG_ULPI_UTMI_SEL			BIT(4)
#define UDC_DWC3_GUSB2PHYCFG_PHYIF				BIT(3)
#define UDC_DWC3_GUSB2PHYCFG_TOUTCAL_MASK			GENMASK(2, 0)

/* Global USB 3.0 PIPE Control Register */
#define UDC_DWC3_GUSB3PIPECTL					0xc2c0
#define UDC_DWC3_GUSB3PIPECTL_PHYSOFTRST			BIT(31)
#define UDC_DWC3_GUSB3PIPECTL_UX_EXIT_IN_PX			BIT(27)
#define UDC_DWC3_GUSB3PIPECTL_PING_ENHANCEMENT_EN		BIT(26)
#define UDC_DWC3_GUSB3PIPECTL_U1U2EXITFAIL_TO_RECOV		BIT(25)
#define UDC_DWC3_GUSB3PIPECTL_REQUEST_P1P2P3			BIT(24)
#define UDC_DWC3_GUSB3PIPECTL_STARTXDETU3RXDET			BIT(23)
#define UDC_DWC3_GUSB3PIPECTL_DISRXDETU3RXDET			BIT(22)
#define UDC_DWC3_GUSB3PIPECTL_P1P2P3DELAY_MASK			GENMASK(21, 19)
#define UDC_DWC3_GUSB3PIPECTL_DELAYP0TOP1P2P3			BIT(18)
#define UDC_DWC3_GUSB3PIPECTL_SUSPENDENABLE			BIT(17)
#define UDC_DWC3_GUSB3PIPECTL_DATWIDTH_MASK			GENMASK(16, 15)
#define UDC_DWC3_GUSB3PIPECTL_ABORTRXDETINU2			BIT(14)
#define UDC_DWC3_GUSB3PIPECTL_SKIPRXDET				BIT(13)
#define UDC_DWC3_GUSB3PIPECTL_LFPSP0ALGN			BIT(12)
#define UDC_DWC3_GUSB3PIPECTL_P3P2TRANOK			BIT(11)
#define UDC_DWC3_GUSB3PIPECTL_P3EXSIGP2				BIT(10)
#define UDC_DWC3_GUSB3PIPECTL_LFPSFILT				BIT(9)
#define UDC_DWC3_GUSB3PIPECTL_TXSWING				BIT(6)
#define UDC_DWC3_GUSB3PIPECTL_TXMARGIN_MASK			GENMASK(5, 3)
#define UDC_DWC3_GUSB3PIPECTL_TXDEEMPHASIS_MASK			GENMASK(2, 1)
#define UDC_DWC3_GUSB3PIPECTL_ELASTICBUFFERMODE			BIT(0)

/* USB Device Configuration Register */
#define UDC_DWC3_DCFG						0xc700
#define UDC_DWC3_DCFG_IGNORESTREAMPP				BIT(23)
#define UDC_DWC3_DCFG_LPMCAP					BIT(22)
#define UDC_DWC3_DCFG_NUMP_MASK					GENMASK(21, 17)
#define UDC_DWC3_DCFG_INTRNUM_MASK				GENMASK(16, 12)
#define UDC_DWC3_DCFG_PERFRINT_MASK				GENMASK(11, 10)
#define UDC_DWC3_DCFG_PERFRINT_80				(0x0 << 10)
#define UDC_DWC3_DCFG_PERFRINT_85				(0x1 << 10)
#define UDC_DWC3_DCFG_PERFRINT_90				(0x2 << 10)
#define UDC_DWC3_DCFG_PERFRINT_95				(0x3 << 10)
#define UDC_DWC3_DCFG_DEVADDR_MASK				GENMASK(9, 3)
#define UDC_DWC3_DCFG_DEVSPD_MASK				GENMASK(2, 0)
#define UDC_DWC3_DCFG_DEVSPD_SUPER_SPEED			(0x4 << 0)
#define UDC_DWC3_DCFG_DEVSPD_HIGH_SPEED				(0x0 << 0)
#define UDC_DWC3_DCFG_DEVSPD_FULL_SPEED				(0x1 << 0)

/* Global SoC Bus Configuration Register */
#define UDC_DWC3_GSBUSCFG0					0xc100
#define UDC_DWC3_GSBUSCFG0_DATRDREQINFO				GENMASK(31, 28)
#define UDC_DWC3_GSBUSCFG0_DESRDREQINFO				GENMASK(27, 24)
#define UDC_DWC3_GSBUSCFG0_DATWRREQINFO				GENMASK(23, 20)
#define UDC_DWC3_GSBUSCFG0_DESWRREQINFO				GENMASK(19, 16)
#define UDC_DWC3_GSBUSCFG0_DATBIGEND				BIT(11)
#define UDC_DWC3_GSBUSCFG0_DESBIGEND				BIT(10)
#define UDC_DWC3_GSBUSCFG0_INCR256BRSTENA			BIT(7)
#define UDC_DWC3_GSBUSCFG0_INCR128BRSTENA			BIT(6)
#define UDC_DWC3_GSBUSCFG0_INCR64BRSTENA			BIT(5)
#define UDC_DWC3_GSBUSCFG0_INCR32BRSTENA			BIT(4)
#define UDC_DWC3_GSBUSCFG0_INCR16BRSTENA			BIT(3)
#define UDC_DWC3_GSBUSCFG0_INCR8BRSTENA				BIT(2)
#define UDC_DWC3_GSBUSCFG0_INCR4BRSTENA				BIT(1)
#define UDC_DWC3_GSBUSCFG0_INCRBRSTENA				BIT(0)

/* Global Tx Threshold Control Register */
#define UDC_DWC3_GTXTHRCFG					0xc108
#define UDC_DWC3_GTXTHRCFG_USBTXPKTCNTSEL			BIT(29)
#define UDC_DWC3_GTXTHRCFG_USBTXPKTCNT_MASK			GENMASK(27, 24)
#define UDC_DWC3_GTXTHRCFG_USBMAXTXBURSTSIZE_MASK		GENMASK(23, 16)

/* Global control register */
#define UDC_DWC3_GCTL						0xc110
#define UDC_DWC3_GCTL_PWRDNSCALE_MASK				GENMASK(31, 19)
#define UDC_DWC3_GCTL_MASTERFILTBYPASS				BIT(18)
#define UDC_DWC3_GCTL_BYPSSETADDR				BIT(17)
#define UDC_DWC3_GCTL_U2RSTECN					BIT(16)
#define UDC_DWC3_GCTL_FRMSCLDWN_MASK				GENMASK(15, 14)
#define UDC_DWC3_GCTL_PRTCAPDIR_MASK				GENMASK(13, 12)
#define UDC_DWC3_GCTL_CORESOFTRESET				BIT(11)
#define UDC_DWC3_GCTL_DEBUGATTACH				BIT(8)
#define UDC_DWC3_GCTL_RAMCLKSEL_MASK				GENMASK(7, 6)
#define UDC_DWC3_GCTL_SCALEDOWN_MASK				GENMASK(5, 4)
#define UDC_DWC3_GCTL_DISSCRAMBLE				BIT(3)
#define UDC_DWC3_GCTL_DSBLCLKGTNG				BIT(0)

/* Global User Control Register */
#define UDC_DWC3_GUCTL						0xc12c
#define UDC_DWC3_GUCTL_NOEXTRDL					BIT(21)
#define UDC_DWC3_GUCTL_PSQEXTRRESSP_MASK			GENMASK(20, 18)
#define UDC_DWC3_GUCTL_PSQEXTRRESSP_EN				BIT(18)
#define UDC_DWC3_GUCTL_SPRSCTRLTRANSEN				BIT(17)
#define UDC_DWC3_GUCTL_RESBWHSEPS				BIT(16)
#define UDC_DWC3_GUCTL_CMDEVADDR				BIT(15)
#define UDC_DWC3_GUCTL_USBHSTINAUTORETRYEN			BIT(14)
#define UDC_DWC3_GUCTL_DTCT_MASK				GENMASK(10, 9)
#define UDC_DWC3_GUCTL_DTFT_MASK				GENMASK(8, 0)

/* USB Device Control register */
#define UDC_DWC3_DCTL						0xc704
#define UDC_DWC3_DCTL_RUNSTOP					BIT(31)
#define UDC_DWC3_DCTL_CSFTRST					BIT(30)
#define UDC_DWC3_DCTL_HIRDTHRES_4				BIT(28)
#define UDC_DWC3_DCTL_HIRDTHRES_TIME_MASK			GENMASK(27, 24)
#define UDC_DWC3_DCTL_APPL1RES					BIT(23)
#define UDC_DWC3_DCTL_LPM_NYET_THRES_MASK			GENMASK(23, 20)
#define UDC_DWC3_DCTL_KEEPCONNECT				BIT(19)
#define UDC_DWC3_DCTL_L1HIBERNATIONEN				BIT(18)
#define UDC_DWC3_DCTL_CRS					BIT(17)
#define UDC_DWC3_DCTL_CSS					BIT(16)
#define UDC_DWC3_DCTL_INITU2ENA					BIT(12)
#define UDC_DWC3_DCTL_ACCEPTU2ENA				BIT(11)
#define UDC_DWC3_DCTL_INITU1ENA					BIT(10)
#define UDC_DWC3_DCTL_ACCEPTU1ENA				BIT(9)
#define UDC_DWC3_DCTL_ULSTCHNGREQ_MASK				GENMASK(8, 5)
#define UDC_DWC3_DCTL_ULSTCHNGREQ_REMOTEWAKEUP			(0x8 << 5)
#define UDC_DWC3_DCTL_TSTCTL_MASK				GENMASK(4, 1)

/* USB Device Event Enable Register */
#define UDC_DWC3_DEVTEN						0xc708
#define UDC_DWC3_DEVTEN_INACTTIMEOUTRCVEDEN			BIT(13)
#define UDC_DWC3_DEVTEN_VNDRDEVTSTRCVEDEN			BIT(12)
#define UDC_DWC3_DEVTEN_EVNTOVERFLOWEN				BIT(11)
#define UDC_DWC3_DEVTEN_CMDCMPLTEN				BIT(10)
#define UDC_DWC3_DEVTEN_ERRTICERREN				BIT(9)
#define UDC_DWC3_DEVTEN_SOFEN					BIT(7)
#define UDC_DWC3_DEVTEN_EOPFEN					BIT(6)
#define UDC_DWC3_DEVTEN_HIBERNATIONREQEVTEN			BIT(5)
#define UDC_DWC3_DEVTEN_WKUPEVTEN				BIT(4)
#define UDC_DWC3_DEVTEN_ULSTCNGEN				BIT(3)
#define UDC_DWC3_DEVTEN_CONNECTDONEEN				BIT(2)
#define UDC_DWC3_DEVTEN_USBRSTEN				BIT(1)
#define UDC_DWC3_DEVTEN_DISCONNEVTEN				BIT(0)

/* USB Device Event Register */

/* Endpoint Global Event Buffer Address (64-bit) */
#define UDC_DWC3_GEVNTADR(n)					(0xc400 + 16 * (n))
#define UDC_DWC3_GEVNTADR_LO(n)					(0xc400 + 16 * (n))
#define UDC_DWC3_GEVNTADR_HI(n)					(0xc404 + 16 * (n))

/* Endpoint Global Event Buffer Size */
#define UDC_DWC3_GEVNTSIZ(n)					(0xc408 + 16 * (n))
#define UDC_DWC3_GEVNTSIZ_EVNTINTRPTMASK			BIT(31)

/* Endpoint Global Event Buffer Count (of valid event) */
#define UDC_DWC3_GEVNTCOUNT(n)					(0xc40c + 16 * (n))

/* USB Device Active USB Endpoint Enable */
#define UDC_DWC3_DALEPENA					0xC720
#define UDC_DWC3_DALEPENA_USBACTEP(n)				(1 << (n))

/* USB Device Core Identification and Release Number Register */
#define UDC_DWC3_GCOREID					0xC120
#define UDC_DWC3_GCOREID_CORE_MASK				GENMASK(31, 16)
#define UDC_DWC3_GCOREID_REL_MASK				GENMASK(15, 0)

/* USB Globa Status register */
#define UDC_DWC3_GSTS						0xc118
#define UDC_DWC3_GSTS_CBELT_MASK				GENMASK(31, 20)
#define UDC_DWC3_GSTS_SSIC_IP					BIT(11)
#define UDC_DWC3_GSTS_OTG_IP					BIT(10)
#define UDC_DWC3_GSTS_BC_IP					BIT(9)
#define UDC_DWC3_GSTS_ADP_IP					BIT(8)
#define UDC_DWC3_GSTS_HOST_IP					BIT(7)
#define UDC_DWC3_GSTS_DEVICE_IP					BIT(6)
#define UDC_DWC3_GSTS_CSRTIMEOUT				BIT(5)
#define UDC_DWC3_GSTS_BUSERRADDRVLD				BIT(4)
#define UDC_DWC3_GSTS_CURMOD_MASK				GENMASK(1, 0)

/* USB Global TX FIFO Size register */
#define UDC_DWC3_GTXFIFOSIZ(n)					(0xc300 + 4 * (n))
#define UDC_DWC3_GTXFIFOSIZ_TXFSTADDR_MASK			GENMASK(31, 16)
#define UDC_DWC3_GTXFIFOSIZ_TXFDEP_MASK				GENMASK(15, 0)

/* USB Global RX FIFO Size register */
#define UDC_DWC3_GRXFIFOSIZ(n)					(0xc380 + 4 * (n))
#define UDC_DWC3_GRXFIFOSIZ_RXFSTADDR_MASK			GENMASK(31, 16)
#define UDC_DWC3_GRXFIFOSIZ_RXFDEP_MASK				GENMASK(15, 0)

/* USB Bus Error Address registers */
#define UDC_DWC3_GBUSERRADDR					0xc130
#define UDC_DWC3_GBUSERRADDR_LO					0xc130
#define UDC_DWC3_GBUSERRADDR_HI					0xc134

/* USB Controller Debug register */
#define UDC_DWC3_CTLDEBUG					0xe000
#define UDC_DWC3_CTLDEBUG_LO					0xe000
#define UDC_DWC3_CTLDEBUG_HI					0xe004

/* USB Analyzer Trace register */
#define UDC_DWC3_ANALYZERTRACE					0xe008

/* USB Global Debug Queue/FIFO Space Available register */
#define UDC_DWC3_GDBGFIFOSPACE					0xc160
#define UDC_DWC3_GDBGFIFOSPACE_AVAILABLE_MASK			GENMASK(31, 16)
#define UDC_DWC3_GDBGFIFOSPACE_QUEUETYPE_MASK			GENMASK(8, 5)
#define UDC_DWC3_GDBGFIFOSPACE_QUEUETYPE_TX			(0x0 << 5)
#define UDC_DWC3_GDBGFIFOSPACE_QUEUETYPE_RX			(0x1 << 5)
#define UDC_DWC3_GDBGFIFOSPACE_QUEUETYPE_TXREQ			(0x2 << 5)
#define UDC_DWC3_GDBGFIFOSPACE_QUEUETYPE_RXREQ			(0x3 << 5)
#define UDC_DWC3_GDBGFIFOSPACE_QUEUETYPE_RXINFO			(0x4 << 5)
#define UDC_DWC3_GDBGFIFOSPACE_QUEUETYPE_PROTOCOL		(0x5 << 5)
#define UDC_DWC3_GDBGFIFOSPACE_QUEUETYPE_DESCFETCH		(0x6 << 5)
#define UDC_DWC3_GDBGFIFOSPACE_QUEUETYPE_WREVENT		(0x7 << 5)
#define UDC_DWC3_GDBGFIFOSPACE_QUEUETYPE_AUXEVENT		(0x8 << 5)
#define UDC_DWC3_GDBGFIFOSPACE_QUEUENUM_MASK			GENMASK(4, 0)

/* USB Global Debug LTSSM register */
#define UDC_DWC3_GDBGLTSSM					0xc164

/* Global Debug LNMCC Register */
#define UDC_DWC3_GDBGLNMCC					0xc168

/* Global Debug BMU Register */
#define UDC_DWC3_GDBGBMU					0xc16c

/* Global Debug LSP MUX Register - Device*/
#define UDC_DWC3_GDBGLSPMUX_DEV					0xc170

/* Global Debug LSP MUX Register - Host */
#define UDC_DWC3_GDBGLSPMUX_HST					0xc170

/* Global Debug LSP Register */
#define UDC_DWC3_GDBGLSP					0xc174

/* Global Debug Endpoint Information Register 0 */
#define UDC_DWC3_GDBGEPINFO0					0xc178

/* Global Debug Endpoint Information Register 1 */
#define UDC_DWC3_GDBGEPINFO1					0xc17c

/* U3 Root Hub Debug Register */
#define UDC_DWC3_BU3RHBDBG0					0xd800

/* USB Device Status register */
#define UDC_DWC3_DSTS						0xC70C
#define UDC_DWC3_DSTS_DCNRD					BIT(29)
#define UDC_DWC3_DSTS_SRE					BIT(28)
#define UDC_DWC3_DSTS_RSS					BIT(25)
#define UDC_DWC3_DSTS_SSS					BIT(24)
#define UDC_DWC3_DSTS_COREIDLE					BIT(23)
#define UDC_DWC3_DSTS_DEVCTRLHLT				BIT(22)
#define UDC_DWC3_DSTS_USBLNKST_MASK				GENMASK(21, 18)
#define UDC_DWC3_DSTS_USBLNKST_USB3_U0				(0x0 << 18)
#define UDC_DWC3_DSTS_USBLNKST_USB3_U1				(0x1 << 18)
#define UDC_DWC3_DSTS_USBLNKST_USB3_U2				(0x2 << 18)
#define UDC_DWC3_DSTS_USBLNKST_USB3_U3				(0x3 << 18)
#define UDC_DWC3_DSTS_USBLNKST_USB3_SS_DIS			(0x4 << 18)
#define UDC_DWC3_DSTS_USBLNKST_USB3_RX_DET			(0x5 << 18)
#define UDC_DWC3_DSTS_USBLNKST_USB3_SS_INACT			(0x6 << 18)
#define UDC_DWC3_DSTS_USBLNKST_USB3_POLL			(0x7 << 18)
#define UDC_DWC3_DSTS_USBLNKST_USB3_RECOV			(0x8 << 18)
#define UDC_DWC3_DSTS_USBLNKST_USB3_HRESET			(0x9 << 18)
#define UDC_DWC3_DSTS_USBLNKST_USB3_CMPLY			(0xa << 18)
#define UDC_DWC3_DSTS_USBLNKST_USB3_LPBK			(0xb << 18)
#define UDC_DWC3_DSTS_USBLNKST_USB3_RESET_RESUME		(0xf << 18)
#define UDC_DWC3_DSTS_USBLNKST_USB2_ON_STATE			(0x0 << 18)
#define UDC_DWC3_DSTS_USBLNKST_USB2_SLEEP_STATE			(0x2 << 18)
#define UDC_DWC3_DSTS_USBLNKST_USB2_SUSPEND_STATE		(0x3 << 18)
#define UDC_DWC3_DSTS_USBLNKST_USB2_DISCONNECTED		(0x4 << 18)
#define UDC_DWC3_DSTS_USBLNKST_USB2_EARLY_SUSPEND		(0x5 << 18)
#define UDC_DWC3_DSTS_USBLNKST_USB2_RESET			(0xe << 18)
#define UDC_DWC3_DSTS_USBLNKST_USB2_RESUME			(0xf << 18)
#define UDC_DWC3_DSTS_RXFIFOEMPTY				BIT(17)
#define UDC_DWC3_DSTS_SOFFN_MASK				GENMASK(16, 3)
#define UDC_DWC3_DSTS_CONNECTSPD_MASK				GENMASK(2, 0)
#define UDC_DWC3_DSTS_CONNECTSPD_HS				(0x0 << 0)
#define UDC_DWC3_DSTS_CONNECTSPD_FS				(0x1 << 0)
#define UDC_DWC3_DSTS_CONNECTSPD_SS				(0x4 << 0)

/* Device Generic Command and Parameter */
#define UDC_DWC3_DGCMDPAR					0xc710
#define UDC_DWC3_DGCMD						0xc714
#define UDC_DWC3_DGCMD_STATUS_MASK				GENMASK(15, 12)
#define UDC_DWC3_DGCMD_STATUS_ERR				(1 << 12)
#define UDC_DWC3_DGCMD_STATUS_OK				(0 << 12)
#define UDC_DWC3_DGCMD_ACT					BIT(10)
#define UDC_DWC3_DGCMD_IOC					BIT(8)
#define UDC_DWC3_DGCMD_MASK					GENMASK(7, 0)
/* EXITLATENCY command and parameters */
#define UDC_DWC3_DGCMD_EXITLATENCY				(2 << 0)
/* Other Commands and Parameters */
#define UDC_DWC3_DGCMD_LINKFUNCTION				(1 << 0)
#define UDC_DWC3_DGCMD_WAKENOTIFNUM				(3 << 0)
#define UDC_DWC3_DGCMD_FIFOFLUSHONE				(9 << 0)
#define UDC_DWC3_DGCMD_FIFOFLUSHALL				(10 << 0)
#define UDC_DWC3_DGCMD_ENDPOINTNRDY				(12 << 0)
#define UDC_DWC3_DGCMD_LOOPBACKTEST				(16 << 0)
#define UDC_DWC3_DGCMD_ROLEREQUEST				(6 << 0)

/* USB 2.0 PHY External CSR Configuration register 0 */
#define UDC_DWC3_U2PHYCTRL0					0x00018000
#define UDC_DWC3_U2PHYCTRL0_LDO_1P0_ADJ_MASK			GENMASK(1, 0)
#define UDC_DWC3_U2PHYCTRL0_LS_CROSS_ADJ_MASK			GENMASK(4, 2)
#define UDC_DWC3_U2PHYCTRL0_HS_RISE_TUNE_MASK			GENMASK(6, 5)
#define UDC_DWC3_U2PHYCTRL0_CDR_RST_SEL				BIT(7)
#define UDC_DWC3_U2PHYCTRL0_GLOBAL_CONFIG_MASK			GENMASK(15, 8)
#define UDC_DWC3_U2PHYCTRL0_FS_CROSS_ADJ_MASK			GENMASK(18, 16)
#define UDC_DWC3_U2PHYCTRL0_REG14_ADJ_MASK			GENMASK(21, 19)
#define UDC_DWC3_U2PHYCTRL0_HS_REG0P8_ADJ_MASK			GENMASK(23, 22)
#define UDC_DWC3_U2PHYCTRL0_SQL_SP_ADJ_MASK			GENMASK(27, 24)
#define UDC_DWC3_U2PHYCTRL0_VBUS_VLD_ADJ_MASK			GENMASK(31, 28)

/* USB 2.0 PHY External CSR Configuration register 1 */
#define UDC_DWC3_U2PHYCTRL1					0x00018004
#define UDC_DWC3_U2PHYCTRL1_CALIB_ONCE_EN			BIT(0)
#define UDC_DWC3_U2PHYCTRL1_HS_EMP_ADJ_MASK			GENMASK(2, 1)
#define UDC_DWC3_U2PHYCTRL1_SQL_CUR_ADJ_MASK			GENMASK(4, 3)
#define UDC_DWC3_U2PHYCTRL1_PLLBW_SEL_MASK			GENMASK(7, 5)
#define UDC_DWC3_U2PHYCTRL1_BIST_EN_N				BIT(8)
#define UDC_DWC3_U2PHYCTRL1_SCP_EN				BIT(9)
#define UDC_DWC3_U2PHYCTRL1_SEL_12_24M				BIT(10)
#define UDC_DWC3_U2PHYCTRL1_HS_LP_MODE_EN			BIT(11)
#define UDC_DWC3_U2PHYCTRL1_SQL_VTH_ADJ_MASK			GENMASK(15, 12)
#define UDC_DWC3_U2PHYCTRL1_HS_EMP_EN				BIT(16)
#define UDC_DWC3_U2PHYCTRL1_RSTN_BYPASS				BIT(17)
#define UDC_DWC3_U2PHYCTRL1_CDR_BW_SEL_MASK			GENMASK(19, 18)
#define UDC_DWC3_U2PHYCTRL1_CDR_TIMING_SEL_MASK			GENMASK(23, 20)
#define UDC_DWC3_U2PHYCTRL1_SEL_INTERNALCLK			BIT(24)
#define UDC_DWC3_U2PHYCTRL1_XOSC_CUR_ADJ_MASK			GENMASK(27, 25)
#define UDC_DWC3_U2PHYCTRL1_DISC_ADJ_MASK			GENMASK(31, 28)

/* USB 2.0 PHY External CSR Configuration register 2 */
#define UDC_DWC3_U2PHYCTRL2					0x00018008
#define UDC_DWC3_U2PHYCTRL2_XCLK12MOUTEN			BIT(0)
#define UDC_DWC3_U2PHYCTRL2_TEST_LOOP_MASK			GENMASK(3, 1)
#define UDC_DWC3_U2PHYCTRL2_RX_LP_EN				BIT(4)
#define UDC_DWC3_U2PHYCTRL2_REG20_ADJ_MASK			GENMASK(7, 5)
#define UDC_DWC3_U2PHYCTRL2_CLK_SEL				BIT(8)
#define UDC_DWC3_U2PHYCTRL2_INTERNAL_RST			BIT(9)
#define UDC_DWC3_U2PHYCTRL2_REFCLK_SEL				BIT(10)
#define UDC_DWC3_U2PHYCTRL2_BIST_DONE				BIT(11)
#define UDC_DWC3_U2PHYCTRL2_BIST_ERR				BIT(12)

/* Hardware parameters */
#define UDC_DWC3_GHWPARAMS0					0xc140
#define UDC_DWC3_GHWPARAMS1					0xc144
#define UDC_DWC3_GHWPARAMS2					0xc148
#define UDC_DWC3_GHWPARAMS3					0xc14c
#define UDC_DWC3_GHWPARAMS4					0xc150
#define UDC_DWC3_GHWPARAMS5					0xc154
#define UDC_DWC3_GHWPARAMS6					0xc158
#define UDC_DWC3_GHWPARAMS6_USB3_HSPHY_INTERFACE		GENMASK(5, 4)
#define UDC_DWC3_GHWPARAMS7					0xc15c
#define UDC_DWC3_GHWPARAMS8					0xc600

/* Helper macros */
#define LO32(n)			((uint32_t)((uint64_t)(n) & 0xffffffff))
#define HI32(n)			((uint32_t)((uint64_t)(n) >> 32))

/*
 * One DMA transaction request passed from the CPU to the DWC3 core.
 *
 * This structure is described by the datasheet of DWC3 and shared between the hardware and
 * software driver. If the architecture involves cache, it must be flushed before accessing
 * this memory region.
 */
struct udc_dwc3_trb {
	uint32_t addr_lo;
	uint32_t addr_hi;
	uint32_t status;
	uint32_t ctrl;
} __packed __aligned(16);

/*
 * Controller configuration items that can remain in non-volatile memory
 */
struct udc_dwc3_config {
	DEVICE_MMIO_NAMED_ROM(base);

	/* USB endpoints data */
	struct udc_dwc3_ep_data *ep_data_in;
	struct udc_dwc3_ep_data *ep_data_out;
	/* Pointer to the DMA-accessible buffer of TRBs and its size */
	struct udc_dwc3_trb (*trb_buf_in)[CONFIG_UDC_DWC3_TRB_NUM];
	struct udc_dwc3_trb (*trb_buf_out)[CONFIG_UDC_DWC3_TRB_NUM];
	/* USB device configuration */
	int maximum_speed_idx;
	/* Pointers to event buffer fetched by DWC3 with DMA */
	volatile uint32_t *evt_buf;
	/* Data used by vendor-specific functions ("quirks") */
	const void *quirk_config;
	void *quirk_data;
	/* IRQ management functions */
	void (*irq_enable_func)(void);
	void (*irq_disable_func)(void);
	/* Number of hardware endpoint set for input or output */
	uint8_t num_in_eps;
	uint8_t num_out_eps;
};

/*
 * All data specific to one endpoint for use by the driver.
 */
struct udc_dwc3_ep_data {
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
	struct udc_dwc3_trb *trb_buf;
	/* Index of the next TRB to receive data in the TRB ring, Link TRB excluded */
	uint32_t head;
	uint32_t tail;
	/* Total size sent from the device to the host for the ongoing transfer */
	uint32_t total;
	/* A flag to tell when the ring buffer is full */
	bool full;
	/* Given by the hardware for use in endpoint commands */
	uint32_t xferrscidx;
};

/*
 * Data of each instance of the driver, that can be read and written to.
 *
 * Accessed via "udc_get_private(dev)".
 */
struct udc_dwc3_data {
	DEVICE_MMIO_NAMED_RAM(base);

	/* Index within trb where to queue new TRBs */
	uint32_t evt_next;
	/* A work queue entry to process the events from the event buffer */
	struct k_work_delayable dwork;
	/* Back-reference to parent */
	const struct device *dev;
	/* Size of the data stage of control transaction */
	uint32_t data_stage_length;
	/* Check whether the USB stack is read */
	atomic_t flags;
#define UDC_DWC3_FLAG_INITIALIZED BIT(0)
};

/*
 * Indexes matching the "device-speed" devicetree property values.
 */
enum {
	UDC_DWC3_SPEED_IDX_FULL_SPEED = 1,
	UDC_DWC3_SPEED_IDX_HIGH_SPEED = 2,
	UDC_DWC3_SPEED_IDX_SUPER_SPEED = 3,
};

/*
 * Vendor quirks
 *
 * Definition of vendor-specific functions that can be overwritten on a per-SoC basis.
 */

struct udc_dwc3_vendor_quirks {
	int (*preinit)(const struct device *const dev);
	int (*init)(const struct device *const dev);
	int (*enable)(const struct device *const dev);
	int (*disable)(const struct device *const dev);
	int (*shutdown)(const struct device *const dev);
};

#if DT_HAS_COMPAT_STATUS_OKAY(snps_dwc3 /* <- replace with your more specific compatible */)
#include "udc_dwc3_qemu.h"
#endif

/* Wrapper functions that fallback to returning 0 if no quirk is needed */
#define UDC_DWC3_QUIRK_FUNC_DEFINE(fn)						\
	static inline int udc_dwc3_quirk_##fn(const struct device *const dev)	\
	{									\
		if (udc_dwc3_vendor_quirks.fn != NULL) {			\
			return udc_dwc3_vendor_quirks.fn(dev);			\
		}								\
		return 0;							\
	}

UDC_DWC3_QUIRK_FUNC_DEFINE(preinit);
UDC_DWC3_QUIRK_FUNC_DEFINE(init);
UDC_DWC3_QUIRK_FUNC_DEFINE(enable);
UDC_DWC3_QUIRK_FUNC_DEFINE(disable);
UDC_DWC3_QUIRK_FUNC_DEFINE(shutdown);

#define DEV_CFG(dev) ((const struct udc_dwc3_config *)(dev->config))
#define DEV_DATA(dev) ((struct udc_dwc3_data *)udc_get_private(dev))

/* Shut down the controller completely  */
static int udc_dwc3_shutdown(const struct device *const dev)
{
	LOG_INF("api: shutdown");

	if (udc_ep_disable_internal(dev, USB_CONTROL_EP_OUT)) {
		LOG_ERR("Failed to disable control endpoint");
		return -EIO;
	}
	if (udc_ep_disable_internal(dev, USB_CONTROL_EP_IN)) {
		LOG_ERR("Failed to disable control endpoint");
		return -EIO;
	}
	return 0;
}

static void udc_dwc3_lock(const struct device *const dev)
{
	udc_lock_internal(dev, K_FOREVER);
}

static void udc_dwc3_unlock(const struct device *const dev)
{
	udc_unlock_internal(dev);
}

/*
 * Ring buffer
 *
 * Helpers to operate the TRB and event ring buffers, shared with the hardware.
 */

/* Increment the counter position "nump" of the TRBs/event ring buffer) */
void udc_dwc3_ring_inc(uint32_t *nump, uint32_t size)
{
	uint32_t num = *nump + 1;

	*nump = (num >= size) ? 0 : num;
}

static void udc_dwc3_push_trb(const struct device *const dev,
			      struct udc_dwc3_ep_data *const ep_data,
			   struct net_buf *buf, uint32_t ctrl)
{
	volatile struct udc_dwc3_trb *trb = &ep_data->trb_buf[ep_data->head];

	/* If the next TRB in the chain is still owned by the hardware, need
	 * to retry later when more resources become available.
	 */
	__ASSERT_NO_MSG(!ep_data->full);

	/* Associate an active buffer and a TRB together */
	ep_data->net_buf[ep_data->head] = buf;

	/* TRB# with one more chunk of data */
	trb->ctrl = ctrl;
	trb->addr_lo = LO32((uintptr_t)buf->data);
	trb->addr_hi = HI32((uintptr_t)buf->data);
	trb->status = ep_data->cfg.caps.in ? buf->len : buf->size;
	LOG_DBG("PUSH %u buf %p, data %p, size %u",
		ep_data->head, (void *)buf, (void *)buf->data, buf->size);

	/* Shift the head */
	udc_dwc3_ring_inc(&ep_data->head, CONFIG_UDC_DWC3_TRB_NUM - 1);

	/* If the head touches the tail after we add something, we are full */
	ep_data->full = (ep_data->head == ep_data->tail);
}

static struct net_buf *udc_dwc3_pop_trb(const struct device *const dev,
					struct udc_dwc3_ep_data *const ep_data)
{
	struct net_buf *buf = ep_data->net_buf[ep_data->tail];

	/* Clear the last TRB */
	ep_data->net_buf[ep_data->tail] = NULL;

	/* Move to the next position in the ring buffer */
	udc_dwc3_ring_inc(&ep_data->tail, CONFIG_UDC_DWC3_TRB_NUM - 1);

	if (buf == NULL) {
		LOG_ERR("pop: the next TRB is emtpy");
		return NULL;
	}

	LOG_DBG("POP %u EP 0x%02x, buf %p, data %p",
		ep_data->tail, ep_data->cfg.addr, (void *)buf, (void *)buf->data);

	/* If we just pulled a TRB, we know we made one hole and we are not full anymore */
	ep_data->full = false;

	return buf;
}

/*
 * Commands
 *
 * The DEPCMD register acts as a command interface, where a command number
 * is written along with parameters, an action is performed and a CMDACT bit
 * is reset whenever the command completes.
 */

static uint32_t udc_dwc3_depcmd(const struct device *const dev, uint32_t addr, uint32_t cmd)
{
	mm_reg_t base = DEVICE_MMIO_NAMED_GET(dev, base);
	uint32_t reg;

	sys_write32(cmd | UDC_DWC3_DEPCMD_CMDACT, base + addr);
	do {
		reg = sys_read32(base + addr);
	} while ((reg & UDC_DWC3_DEPCMD_CMDACT) != 0);

	switch (reg & UDC_DWC3_DEPCMD_STATUS_MASK) {
	case UDC_DWC3_DEPCMD_STATUS_OK:
		break;
	case UDC_DWC3_DEPCMD_STATUS_CMDERR:
		LOG_ERR("cmd: endpoint command failed");
		break;
	default:
		LOG_ERR("cmd: command failed with unknown status: 0x%08x", reg);
	}

	return FIELD_GET(UDC_DWC3_DEPCMD_XFERRSCIDX_MASK, reg);
}

static void udc_dwc3_depcmd_ep_config(const struct device *const dev,
				      struct udc_dwc3_ep_data *const ep_data)
{
	mm_reg_t base = DEVICE_MMIO_NAMED_GET(dev, base);
	uint32_t param0 = 0, param1 = 0;

	LOG_INF("cmd: configuring endpoint 0x%02x with wMaxPacketSize=%u", ep_data->cfg.addr,
		ep_data->cfg.mps);

	if (ep_data->cfg.stat.enabled) {
		param0 |= UDC_DWC3_DEPCMDPAR0_DEPCFG_ACTION_MODIFY;
	} else {
		param0 |= UDC_DWC3_DEPCMDPAR0_DEPCFG_ACTION_INIT;
	}

	switch (ep_data->cfg.attributes & USB_EP_TRANSFER_TYPE_MASK) {
	case USB_EP_TYPE_CONTROL:
		param0 |= UDC_DWC3_DEPCMDPAR0_DEPCFG_EPTYPE_CTRL;
		break;
	case USB_EP_TYPE_BULK:
		param0 |= UDC_DWC3_DEPCMDPAR0_DEPCFG_EPTYPE_BULK;
		break;
	case USB_EP_TYPE_INTERRUPT:
		param0 |= UDC_DWC3_DEPCMDPAR0_DEPCFG_EPTYPE_INT;
		break;
	case USB_EP_TYPE_ISO:
		param0 |= UDC_DWC3_DEPCMDPAR0_DEPCFG_EPTYPE_ISOC;
		break;
	default:
		CODE_UNREACHABLE;
	}

	/* Max Packet Size according to the USB descriptor configuration */
	param0 |= FIELD_PREP(UDC_DWC3_DEPCMDPAR0_DEPCFG_MPS_MASK, ep_data->cfg.mps);

	/* Burst Size of a single packet per burst (encoded as '0'): no burst */
	param0 |= FIELD_PREP(UDC_DWC3_DEPCMDPAR0_DEPCFG_BRSTSIZ_MASK, 15);

	/* Set the FIFO number, must be 0 for all OUT EPs */
	if (ep_data->cfg.caps.in) {
		param0 |= FIELD_PREP(UDC_DWC3_DEPCMDPAR0_DEPCFG_FIFONUM_MASK,
				     ep_data->cfg.addr & 0x7f);
	}

	/* Per-endpoint events */
	param1 |= UDC_DWC3_DEPCMDPAR1_DEPCFG_XFERINPROGEN;
	param1 |= UDC_DWC3_DEPCMDPAR1_DEPCFG_XFERCMPLEN;
	/* param1 |= UDC_DWC3_DEPCMDPAR1_DEPCFG_XFERNRDYEN; useful for debugging */

	/* This is the usb protocol endpoint number, but the data encoding
	 * we chose for physical endpoint number is the same as this
	 * register
	 */
	param1 |= FIELD_PREP(UDC_DWC3_DEPCMDPAR1_DEPCFG_EPNUMBER_MASK, ep_data->epn);

	sys_write32(param0, base + UDC_DWC3_DEPCMDPAR0(ep_data->epn));
	sys_write32(param1, base + UDC_DWC3_DEPCMDPAR1(ep_data->epn));
	udc_dwc3_depcmd(dev, UDC_DWC3_DEPCMD(ep_data->epn), UDC_DWC3_DEPCMD_DEPCFG);
}

static void udc_dwc3_depcmd_ep_xfer_config(const struct device *const dev,
					   struct udc_dwc3_ep_data *const ep_data)
{
	mm_reg_t base = DEVICE_MMIO_NAMED_GET(dev, base);
	uint32_t reg;

	LOG_DBG("cmd: DepXferConfig: ep=0x%02x", ep_data->cfg.addr);

	reg = FIELD_PREP(UDC_DWC3_DEPCMDPAR0_DEPXFERCFG_NUMXFERRES_MASK, 1);
	sys_write32(reg, base + UDC_DWC3_DEPCMDPAR0(ep_data->epn));
	udc_dwc3_depcmd(dev, UDC_DWC3_DEPCMD(ep_data->epn), UDC_DWC3_DEPCMD_DEPXFERCFG);
}

static void udc_dwc3_depcmd_set_stall(const struct device *const dev,
				      struct udc_dwc3_ep_data *const ep_data)
{
	LOG_WRN("cmd: DepSetStall: ep=0x%02x", ep_data->cfg.addr);
	udc_dwc3_depcmd(dev, UDC_DWC3_DEPCMD(ep_data->epn), UDC_DWC3_DEPCMD_DEPSETSTALL);
}

static void udc_dwc3_depcmd_clear_stall(const struct device *const dev,
					struct udc_dwc3_ep_data *const ep_data)
{
	LOG_INF("cmd: DepClearStall ep=0x%02x", ep_data->cfg.addr);
	udc_dwc3_depcmd(dev, UDC_DWC3_DEPCMD(ep_data->epn), UDC_DWC3_DEPCMD_DEPCSTALL);
}

static void udc_dwc3_depcmd_start_xfer(const struct device *const dev,
				       struct udc_dwc3_ep_data *const ep_data)
{
	mm_reg_t base = DEVICE_MMIO_NAMED_GET(dev, base);
	uint32_t reg;

	/* Make sure the device is in U0 state, assuming TX FIFO is empty */
	reg = sys_read32(base + UDC_DWC3_DCTL);
	reg &= ~UDC_DWC3_DCTL_ULSTCHNGREQ_MASK;
	reg |= UDC_DWC3_DCTL_ULSTCHNGREQ_REMOTEWAKEUP;
	sys_write32(reg, base + UDC_DWC3_DCTL);

	sys_write32(HI32((uintptr_t)ep_data->trb_buf),
		    base + UDC_DWC3_DEPCMDPAR0(ep_data->epn));

	sys_write32(LO32((uintptr_t)ep_data->trb_buf),
		    base + UDC_DWC3_DEPCMDPAR1(ep_data->epn));

	ep_data->xferrscidx =
		udc_dwc3_depcmd(dev, UDC_DWC3_DEPCMD(ep_data->epn), UDC_DWC3_DEPCMD_DEPSTRTXFER);
	LOG_DBG("cmd: DepStartXfer done ep=0x%02x xferrscidx=0x%x", ep_data->cfg.addr,
		ep_data->xferrscidx);
}

static void udc_dwc3_depcmd_update_xfer(const struct device *const dev,
					struct udc_dwc3_ep_data *const ep_data)
{
	uint32_t flags;

	flags = FIELD_PREP(UDC_DWC3_DEPCMD_XFERRSCIDX_MASK, ep_data->xferrscidx);
	udc_dwc3_depcmd(dev, UDC_DWC3_DEPCMD(ep_data->epn), UDC_DWC3_DEPCMD_DEPUPDXFER | flags);
	LOG_DBG("cmd: DepUpdateXfer done ep=0x%02x addr=0x%08x data=0x%08x", ep_data->cfg.addr,
		UDC_DWC3_DEPCMD(ep_data->epn), UDC_DWC3_DEPCMD_DEPUPDXFER | flags);
}

static void udc_dwc3_depcmd_end_xfer(const struct device *const dev,
				     struct udc_dwc3_ep_data *const ep_data,
				     uint32_t flags)
{
	flags |= FIELD_PREP(UDC_DWC3_DEPCMD_XFERRSCIDX_MASK, ep_data->xferrscidx);
	udc_dwc3_depcmd(dev, UDC_DWC3_DEPCMD(ep_data->epn),
			UDC_DWC3_DEPCMD_DEPENDXFER | flags);
	LOG_DBG("cmd: DepEndXfer done ep=0x%02x", ep_data->cfg.addr);

	ep_data->head = ep_data->tail = 0;
}

static void udc_dwc3_depcmd_start_config(const struct device *const dev,
					 struct udc_dwc3_ep_data *const ep_data)
{
	uint32_t flags;

	flags = FIELD_PREP(UDC_DWC3_DEPCMD_XFERRSCIDX_MASK, ep_data->cfg.caps.control ? 0 : 2);
	udc_dwc3_depcmd(dev, UDC_DWC3_DEPCMD(ep_data->epn), UDC_DWC3_DEPCMD_DEPSTARTCFG | flags);
	LOG_DBG("cmd: DepStartConfig done ep=0x%02x", ep_data->cfg.addr);
}

static int udc_dwc3_do_set_address(const struct device *const dev,
				   const uint8_t addr)
{
	const struct udc_dwc3_config *cfg = dev->config;
	mm_reg_t base = DEVICE_MMIO_NAMED_GET(dev, base);
	uint32_t reg;

	LOG_INF("addr: setting to %u", addr);

	/* Configure the new address */
	reg = sys_read32(base + UDC_DWC3_DCFG);
	reg &= ~UDC_DWC3_DCFG_DEVADDR_MASK;
	reg |= FIELD_PREP(UDC_DWC3_DCFG_DEVADDR_MASK, addr);
	sys_write32(reg, base + UDC_DWC3_DCFG);

	/* Re-apply the same endpoint configuration */
	udc_dwc3_depcmd_ep_config(dev, &cfg->ep_data_in[0]);
	udc_dwc3_depcmd_ep_config(dev, &cfg->ep_data_out[0]);

	return 0;
}

/*
 * Transfer Requests (TRB)
 *
 * DWC3 receives transfer requests from this driver through a shared memory
 * buffer, resubmitted upon every new transfer (through either Start or
 * Update command).
 */

static void udc_dwc3_trb_norm_init(const struct device *const dev,
				   struct udc_dwc3_ep_data *const ep_data)
{
	volatile struct udc_dwc3_trb *trb = ep_data->trb_buf;
	const uint32_t i = CONFIG_UDC_DWC3_TRB_NUM - 1;

	LOG_DBG("trb: normal: init");

	/* TRB0 that blocks the transfer from going further */
	trb[0].ctrl = 0;

	/* TRB LINK that loops the ring buffer back to the beginning */
	trb[i].ctrl = UDC_DWC3_TRB_CTRL_TRBCTL_LINK_TRB | UDC_DWC3_TRB_CTRL_HWO;
	trb[i].addr_lo = LO32((uintptr_t)ep_data->trb_buf);
	trb[i].addr_hi = HI32((uintptr_t)ep_data->trb_buf);

	/* Start the transfer now, update it later */
	udc_dwc3_depcmd_start_xfer(dev, ep_data);
}

static void udc_dwc3_trb_ctrl_in(const struct device *const dev, uint32_t ctrl)
{
	const struct udc_dwc3_config *cfg = dev->config;
	struct udc_dwc3_ep_data *const ep_data = &cfg->ep_data_in[0];
	struct net_buf *buf = ep_data->net_buf[0];
	struct udc_buf_info *bi = udc_get_buf_info(buf);
	volatile struct udc_dwc3_trb *trb = ep_data->trb_buf;

	__ASSERT_NO_MSG(buf != NULL);
	LOG_DBG("TRB_CONTROL_IN len=%u", buf->len);

	if (bi->zlp) {
		/* TRB0 sending the data */
		trb[0].addr_lo = LO32((uintptr_t)buf->data);
		trb[0].addr_hi = HI32((uintptr_t)buf->data);
		trb[0].status = buf->len;
		trb[0].ctrl = ctrl | UDC_DWC3_TRB_CTRL_CHN | UDC_DWC3_TRB_CTRL_HWO;

		/* TRB1 for the zero-length packet */
		trb[1].addr_lo = 0;
		trb[1].addr_hi = 0;
		trb[1].status = 0;
		trb[1].ctrl = ctrl | UDC_DWC3_TRB_CTRL_LST | UDC_DWC3_TRB_CTRL_HWO;
	} else {
		/* TRB0 sending the data */
		trb[0].addr_lo = LO32((uintptr_t)buf->data);
		trb[0].addr_hi = HI32((uintptr_t)buf->data);
		trb[0].status = buf->len;
		trb[0].ctrl = ctrl | UDC_DWC3_TRB_CTRL_LST | UDC_DWC3_TRB_CTRL_HWO;
	}

	/* Start a new transfer every time: no ring buffer */
	udc_dwc3_depcmd_start_xfer(dev, ep_data);
}

static void udc_dwc3_trb_ctrl_out(const struct device *const dev,
				  struct net_buf *buf, uint32_t ctrl)
{
	const struct udc_dwc3_config *cfg = dev->config;
	struct udc_dwc3_ep_data *const ep_data = &cfg->ep_data_out[0];
	volatile struct udc_dwc3_trb *trb = ep_data->trb_buf;

	__ASSERT_NO_MSG(buf != NULL);
	LOG_DBG("TRB_CONTROL_OUT size=%u", buf->size);

	/* Associate the buffer with the TRB for picking it up later */
	__ASSERT_NO_MSG(ep_data->net_buf[0] == NULL);
	ep_data->net_buf[0] = buf;

	/* TRB0 for recieinving the data */
	trb[0].addr_lo = LO32((uintptr_t)ep_data->net_buf[0]->data);
	trb[0].addr_hi = HI32((uintptr_t)ep_data->net_buf[0]->data);
	trb[0].status = buf->size;
	trb[0].ctrl = ctrl | UDC_DWC3_TRB_CTRL_LST | UDC_DWC3_TRB_CTRL_HWO;

	/* Start a new transfer every time: no ring buffer */
	udc_dwc3_depcmd_start_xfer(dev, ep_data);
}

static void udc_dwc3_trb_ctrl_setup_out(const struct device *const dev)
{
	struct net_buf *buf = udc_ctrl_alloc(dev, USB_CONTROL_EP_OUT, 8);

	LOG_DBG("TRB_CONTROL_SETUP ep=0x%02x", USB_CONTROL_EP_OUT);
	udc_dwc3_trb_ctrl_out(dev, buf, UDC_DWC3_TRB_CTRL_TRBCTL_CONTROL_SETUP);
}

static void udc_dwc3_trb_ctrl_data_out(const struct device *const dev)
{
	struct net_buf *buf = udc_ctrl_alloc(dev, USB_CONTROL_EP_OUT, 512);

	LOG_DBG("TRB_CONTROL_DATA_OUT ep=0x%02x", USB_CONTROL_EP_OUT);
	udc_dwc3_trb_ctrl_out(dev, buf, UDC_DWC3_TRB_CTRL_TRBCTL_CONTROL_DATA);
}

static void udc_dwc3_trb_ctrl_data_in(const struct device *const dev)
{
	LOG_DBG("TRB_CONTROL_DATA_IN ep=0x%02x", USB_CONTROL_EP_IN);
	udc_dwc3_trb_ctrl_in(dev, UDC_DWC3_TRB_CTRL_TRBCTL_CONTROL_DATA);
}

static void udc_dwc3_trb_ctrl_status_2_in(const struct device *const dev)
{
	LOG_DBG("TRB_CONTROL_STATUS_2_IN ep=0x%02x", USB_CONTROL_EP_IN);
	udc_dwc3_trb_ctrl_in(dev, UDC_DWC3_TRB_CTRL_TRBCTL_CONTROL_STATUS_2);
}

static void udc_dwc3_trb_ctrl_status_3_in(const struct device *const dev)
{
	LOG_DBG("TRB_CONTROL_STATUS_3_IN ep=0x%02x", USB_CONTROL_EP_IN);
	udc_dwc3_trb_ctrl_in(dev, UDC_DWC3_TRB_CTRL_TRBCTL_CONTROL_STATUS_3);
}

static void udc_dwc3_trb_ctrl_status_3_out(const struct device *const dev)
{
	struct net_buf *buf = udc_ctrl_alloc(dev, USB_CONTROL_EP_OUT, 0);

	LOG_DBG("TRB_CONTROL_STATUS_3_OUT ep=0x%02x", USB_CONTROL_EP_OUT);
	udc_dwc3_trb_ctrl_out(dev, buf, UDC_DWC3_TRB_CTRL_TRBCTL_CONTROL_STATUS_3);
}

static int udc_dwc3_trb_bulk(const struct device *const dev,
			     struct udc_dwc3_ep_data *const ep_data,
			     struct net_buf *buf)
{
	uint32_t ctrl = UDC_DWC3_TRB_CTRL_IOC | UDC_DWC3_TRB_CTRL_HWO | UDC_DWC3_TRB_CTRL_CSP;

	LOG_INF("TRB_BULK_EP_0x%02x, buf %p, data %p, size %u, len %u",
		ep_data->cfg.addr, (void *)buf, (void *)buf->data, buf->size, buf->len);

	if (ep_data->full) {
		return -EBUSY;
	}

	if (udc_ep_buf_has_zlp(buf)) {
		LOG_DBG("Buffer has a ZLP flag, terminating the transfer");
		ctrl |= UDC_DWC3_TRB_CTRL_TRBCTL_NORMAL_ZLP;
		ep_data->total = 0;
	} else {
		ctrl |= UDC_DWC3_TRB_CTRL_TRBCTL_NORMAL;
		ep_data->total += buf->len;

		if (ep_data->cfg.caps.in && ep_data->total % ep_data->cfg.mps == 0) {
			LOG_DBG("Buffer is a multiple of %d, continuing this transfer of %u bytes",
				ep_data->cfg.mps, ep_data->total);
			ctrl |= UDC_DWC3_TRB_CTRL_CHN;
		} else {
			LOG_DBG("End of USB transfer, %u bytes transferred", ep_data->total);
			ep_data->total = 0;
		}
	}

	udc_dwc3_push_trb(dev, ep_data, buf, ctrl);
	udc_dwc3_depcmd_update_xfer(dev, ep_data);

	return 0;
}

/*
 * Events
 *
 * Process the events from the event ring buffer. Interrupts gives us a
 * hint that an event is available, which we fetch from a ring buffer shared
 * with the hardware.
 */

static void udc_dwc3_on_soft_reset(const struct device *const dev)
{
	const struct udc_dwc3_config *cfg = dev->config;
	mm_reg_t base = DEVICE_MMIO_NAMED_GET(dev, base);
	uint32_t reg;

	/* Configure and reset the Device Controller */
	/* TODO confirm that DWC_USB3_EN_LPM_ERRATA == 1 */
	reg = UDC_DWC3_DCTL_CSFTRST;
	reg |= FIELD_PREP(UDC_DWC3_DCTL_LPM_NYET_THRES_MASK, 15);
	sys_write32(reg, base + UDC_DWC3_DCTL);
	while (sys_read32(base + UDC_DWC3_DCTL) & UDC_DWC3_DCTL_CSFTRST) {
		continue;
	}

	/* Enable AXI64 bursts for various sizes expected */
	reg = UDC_DWC3_GSBUSCFG0_INCR256BRSTENA;
	reg |= UDC_DWC3_GSBUSCFG0_INCR128BRSTENA;
	reg |= UDC_DWC3_GSBUSCFG0_INCR64BRSTENA;
	reg |= UDC_DWC3_GSBUSCFG0_INCR32BRSTENA;
	reg |= UDC_DWC3_GSBUSCFG0_INCR16BRSTENA;
	reg |= UDC_DWC3_GSBUSCFG0_INCR8BRSTENA;
	reg |= UDC_DWC3_GSBUSCFG0_INCR4BRSTENA;
	sys_set_bits(base + UDC_DWC3_GSBUSCFG0, reg);

	/* Letting GTXTHRCFG and GRXTHRCFG unchanged */

	/* Read the chip identification */
	reg = sys_read32(base + UDC_DWC3_GCOREID);
	LOG_INF("evt: coreid=0x%04lx rel=0x%04lx",
		FIELD_GET(UDC_DWC3_GCOREID_CORE_MASK, reg),
		FIELD_GET(UDC_DWC3_GCOREID_REL_MASK, reg));
	__ASSERT_NO_MSG(FIELD_GET(UDC_DWC3_GCOREID_CORE_MASK, reg) == 0x5533);

	/* Letting GUID unchanged */
	/* Letting GUSB2PHYCFG and GUSB3PIPECTL unchanged */
	/* Letting GRXFIFOSIZ unchanged */

	/* Setup the event buffer address, size and start event reception */
	memset((void *)cfg->evt_buf, 0, CONFIG_UDC_DWC3_EVENTS_NUM * sizeof(uint32_t));
	sys_write32(HI32((uintptr_t)cfg->evt_buf), base + UDC_DWC3_GEVNTADR_HI(0));
	sys_write32(LO32((uintptr_t)cfg->evt_buf), base + UDC_DWC3_GEVNTADR_LO(0));
	sys_write32(CONFIG_UDC_DWC3_EVENTS_NUM * sizeof(uint32_t),
		    base + UDC_DWC3_GEVNTSIZ(0));
	LOG_INF("Event buffer size is %u bytes",
		sys_read32(base + UDC_DWC3_GEVNTSIZ(0)));
	sys_write32(0, base + UDC_DWC3_GEVNTCOUNT(0));

	/* Letting GCTL unchanged */

	/* Set the USB device configuration, including max supported speed */
	sys_write32(UDC_DWC3_DCFG_PERFRINT_90, base + UDC_DWC3_DCFG);
	switch (cfg->maximum_speed_idx) {
	case UDC_DWC3_SPEED_IDX_SUPER_SPEED:
		LOG_DBG("UDC_DWC3_SPEED_IDX_SUPER_SPEED");
		sys_set_bits(base + UDC_DWC3_DCFG, UDC_DWC3_DCFG_DEVSPD_SUPER_SPEED);
		break;
	case UDC_DWC3_SPEED_IDX_HIGH_SPEED:
		LOG_DBG("UDC_DWC3_SPEED_IDX_HIGH_SPEED");
		sys_set_bits(base + UDC_DWC3_DCFG, UDC_DWC3_DCFG_DEVSPD_HIGH_SPEED);
		break;
	case UDC_DWC3_SPEED_IDX_FULL_SPEED:
		LOG_DBG("UDC_DWC3_SPEED_IDX_FULL_SPEED");
		sys_set_bits(base + UDC_DWC3_DCFG, UDC_DWC3_DCFG_DEVSPD_FULL_SPEED);
		break;
	default:
		CODE_UNREACHABLE;
	}

	/* Set the number of USB3 packets the device can receive at once */
	reg = sys_read32(base + UDC_DWC3_DCFG);
	reg &= ~UDC_DWC3_DCFG_NUMP_MASK;
	reg |= FIELD_PREP(UDC_DWC3_DCFG_NUMP_MASK, 15);
	sys_write32(reg, base + UDC_DWC3_DCFG);

	/* Enable reception of all USB events except UDC_DWC3_DEVTEN_ULSTCNGEN */
	reg = UDC_DWC3_DEVTEN_INACTTIMEOUTRCVEDEN;
	reg |= UDC_DWC3_DEVTEN_VNDRDEVTSTRCVEDEN;
	reg |= UDC_DWC3_DEVTEN_EVNTOVERFLOWEN;
	reg |= UDC_DWC3_DEVTEN_CMDCMPLTEN;
	reg |= UDC_DWC3_DEVTEN_ERRTICERREN;
	reg |= UDC_DWC3_DEVTEN_HIBERNATIONREQEVTEN;
	reg |= UDC_DWC3_DEVTEN_WKUPEVTEN;
	reg |= UDC_DWC3_DEVTEN_CONNECTDONEEN;
	reg |= UDC_DWC3_DEVTEN_USBRSTEN;
	reg |= UDC_DWC3_DEVTEN_DISCONNEVTEN;
	sys_write32(reg, base + UDC_DWC3_DEVTEN);

	/* Configure endpoint 0x00 and 0x80 only for now */
	udc_dwc3_depcmd_start_config(dev, &cfg->ep_data_in[0]);
	udc_dwc3_depcmd_start_config(dev, &cfg->ep_data_out[0]);
}

static void udc_dwc3_on_usb_reset(const struct device *const dev)
{
	const struct udc_dwc3_config *cfg = dev->config;

	LOG_DBG("Going through DWC3 reset logic");

	/* Reset all ongoing transfers on non-control IN endpoints */
	for (int epn = 1; epn < cfg->num_in_eps; epn++) {
		struct udc_dwc3_ep_data *const ep_data = &cfg->ep_data_in[epn];

		continue; /* TODO */
		udc_dwc3_depcmd_end_xfer(dev, ep_data, 0);
		udc_dwc3_depcmd_clear_stall(dev, ep_data);
	}

	/* Reset all ongoing transfers on non-control OUT endpoints */
	for (int epn = 1; epn < cfg->num_out_eps; epn++) {
		struct udc_dwc3_ep_data *const ep_data = &cfg->ep_data_out[epn];

		continue; /* TODO */
		udc_dwc3_depcmd_end_xfer(dev, ep_data, 0);
		udc_dwc3_depcmd_clear_stall(dev, ep_data);
	}

	/* Perform the USB reset operations manually to improve latency */
	udc_dwc3_do_set_address(dev, 0);

	/* Let Zephyr set the device address 0 */
	udc_submit_event(dev, UDC_EVT_RESET, 0);
}

static void udc_dwc3_on_connect_done(const struct device *const dev)
{
	const struct udc_dwc3_config *cfg = dev->config;
	mm_reg_t base = DEVICE_MMIO_NAMED_GET(dev, base);
	int mps = 0;

	/* Adjust parameters against the connection speed */
	switch (sys_read32(base + UDC_DWC3_DSTS) & UDC_DWC3_DSTS_CONNECTSPD_MASK) {
	case UDC_DWC3_DSTS_CONNECTSPD_FS:
	case UDC_DWC3_DSTS_CONNECTSPD_HS:
		mps = 64;
		/* TODO this is not suspending USB3, it enable suspend feature */
		/* sys_set_bits(base + UDC_DWC3_GUSB3PIPECTL, */
		/*		UDC_DWC3_GUSB3PIPECTL_SUSPENDENABLE); */
		break;
	case UDC_DWC3_DSTS_CONNECTSPD_SS:
		mps = 512;
		/* sys_set_bits(base + UDC_DWC3_GUSB2PHYCFG, UDC_DWC3_GUSB2PHYCFG_SUSPHY); */
		break;
	}
	__ASSERT_NO_MSG(mps != 0);

	/* Reconfigure control endpoints connection speed */
	udc_get_ep_cfg(dev, USB_CONTROL_EP_OUT)->mps = mps;
	udc_get_ep_cfg(dev, USB_CONTROL_EP_IN)->mps = mps;
	udc_dwc3_depcmd_ep_config(dev, &cfg->ep_data_in[0]);
	udc_dwc3_depcmd_ep_config(dev, &cfg->ep_data_out[0]);

	/* Letting GTXFIFOSIZn unchanged */
}

static void udc_dwc3_on_link_state_event(const struct device *const dev)
{
	mm_reg_t base = DEVICE_MMIO_NAMED_GET(dev, base);
	uint32_t reg;

	reg = sys_read32(base + UDC_DWC3_DSTS);

	switch (reg & UDC_DWC3_DSTS_CONNECTSPD_MASK) {
	case UDC_DWC3_DSTS_CONNECTSPD_SS:
		switch (reg & UDC_DWC3_DSTS_USBLNKST_MASK) {
		case UDC_DWC3_DSTS_USBLNKST_USB3_U0:
			LOG_DBG("--- DSTS_USBLNKST_USB3_U0 ---");
			break;
		case UDC_DWC3_DSTS_USBLNKST_USB3_U1:
			LOG_DBG("--- DSTS_USBLNKST_USB3_U1 ---");
			break;
		case UDC_DWC3_DSTS_USBLNKST_USB3_U2:
			LOG_DBG("--- DSTS_USBLNKST_USB3_U2 ---");
			break;
		case UDC_DWC3_DSTS_USBLNKST_USB3_U3:
			LOG_DBG("--- DSTS_USBLNKST_USB3_U3 ---");
			break;
		case UDC_DWC3_DSTS_USBLNKST_USB3_SS_DIS:
			LOG_DBG("--- DSTS_USBLNKST_USB3_SS_DIS ---");
			break;
		case UDC_DWC3_DSTS_USBLNKST_USB3_RX_DET:
			LOG_DBG("--- DSTS_USBLNKST_USB3_RX_DET ---");
			break;
		case UDC_DWC3_DSTS_USBLNKST_USB3_SS_INACT:
			LOG_DBG("--- DSTS_USBLNKST_USB3_SS_INACT ---");
			break;
		case UDC_DWC3_DSTS_USBLNKST_USB3_POLL:
			LOG_DBG("--- DSTS_USBLNKST_USB3_POLL ---");
			break;
		case UDC_DWC3_DSTS_USBLNKST_USB3_RECOV:
			LOG_DBG("--- DSTS_USBLNKST_USB3_RECOV ---");
			break;
		case UDC_DWC3_DSTS_USBLNKST_USB3_HRESET:
			LOG_DBG("--- DSTS_USBLNKST_USB3_HRESET ---");
			break;
		case UDC_DWC3_DSTS_USBLNKST_USB3_CMPLY:
			LOG_DBG("--- DSTS_USBLNKST_USB3_CMPLY ---");
			break;
		case UDC_DWC3_DSTS_USBLNKST_USB3_LPBK:
			LOG_DBG("--- DSTS_USBLNKST_USB3_LPBK ---");
			break;
		case UDC_DWC3_DSTS_USBLNKST_USB3_RESET_RESUME:
			LOG_DBG("--- DSTS_USBLNKST_USB3_RESET_RESUME ---");
			break;
		default:
			LOG_ERR("evt: unknown USB3 link state");
		}
		break;
	case UDC_DWC3_DSTS_CONNECTSPD_HS:
	case UDC_DWC3_DSTS_CONNECTSPD_FS:
		switch (reg & UDC_DWC3_DSTS_USBLNKST_MASK) {
		case UDC_DWC3_DSTS_USBLNKST_USB2_ON_STATE:
			LOG_DBG("--- DSTS_USBLNKST_USB2_ON_STATE ---");
			break;
		case UDC_DWC3_DSTS_USBLNKST_USB2_SLEEP_STATE:
			LOG_DBG("--- DSTS_USBLNKST_USB2_SLEEP_STATE ---");
			break;
		case UDC_DWC3_DSTS_USBLNKST_USB2_SUSPEND_STATE:
			LOG_DBG("--- DSTS_USBLNKST_USB2_SUSPEND_STATE ---");
			break;
		case UDC_DWC3_DSTS_USBLNKST_USB2_DISCONNECTED:
			LOG_DBG("--- DSTS_USBLNKST_USB2_DISCONNECTED ---");
			break;
		case UDC_DWC3_DSTS_USBLNKST_USB2_EARLY_SUSPEND:
			LOG_DBG("--- DSTS_USBLNKST_USB2_EARLY_SUSPEND ---");
			break;
		case UDC_DWC3_DSTS_USBLNKST_USB2_RESET:
			LOG_DBG("--- DSTS_USBLNKST_USB2_RESET ---");
			break;
		case UDC_DWC3_DSTS_USBLNKST_USB2_RESUME:
			LOG_DBG("--- DSTS_USBLNKST_USB2_RESUME ---");
			break;
		default:
			LOG_ERR("evt: unknown USB2 link state");
		}
		break;
	default:
		LOG_ERR("evt: unknown connection speed");
	}
}

/* Control Write */

/* OUT */
static void udc_dwc3_on_ctrl_write_setup(const struct device *const dev, struct net_buf *buf)
{
	LOG_DBG("evt: CTRL_WRITE_SETUP (out), data %p", (void *)buf->data);

	udc_dwc3_trb_ctrl_data_out(dev);
}

/* OUT */
static void udc_dwc3_on_ctrl_write_data(const struct device *const dev, struct net_buf *buf)
{
	int ret;

	LOG_DBG("evt: CTRL_WRITE_DATA (out), data %p", (void *)buf->data);

	udc_ctrl_update_stage(dev, buf);

	ret = udc_ctrl_submit_s_out_status(dev, buf);
	__ASSERT_NO_MSG(ret == 0);

	k_sleep(K_MSEC(1));
}

/* IN */
static void udc_dwc3_on_ctrl_write_status(const struct device *const dev, struct net_buf *buf)
{
	int ret;

	LOG_DBG("evt: CTRL_WRITE_STATUS (in), data %p", (void *)buf->data);

	udc_ctrl_update_stage(dev, buf);

	ret = udc_ctrl_submit_status(dev, buf);
	__ASSERT_NO_MSG(ret == 0);
}

/* Control Read */

/* OUT */
static void udc_dwc3_on_ctrl_read_setup(const struct device *const dev, struct net_buf *buf)
{
	int ret;

	LOG_DBG("evt: CTRL_READ_SETUP (out), data %p", (void *)buf->data);

	ret = udc_ctrl_submit_s_in_status(dev);
	__ASSERT_NO_MSG(ret == 0);
}

/* IN */
static void udc_dwc3_on_ctrl_read_data(const struct device *const dev, struct net_buf *buf)
{
	LOG_DBG("evt: CTRL_READ_DATA (in), data %p", (void *)buf->data);

	udc_dwc3_trb_ctrl_status_3_out(dev);

	udc_ctrl_update_stage(dev, buf);

	net_buf_unref(buf);
}

/* OUT */
static void udc_dwc3_on_ctrl_read_status(const struct device *const dev, struct net_buf *buf)
{
	int ret;

	LOG_DBG("evt: CTRL_READ_STATUS (out), data %p", (void *)buf->data);

	udc_ctrl_update_stage(dev, buf);

	ret = udc_ctrl_submit_status(dev, buf);
	__ASSERT_NO_MSG(ret == 0);
}

/* No-Data Control */

/* OUT */
static void udc_dwc3_on_ctrl_nodata_setup(const struct device *const dev, struct net_buf *buf)
{
	int ret;

	LOG_DBG("evt: CTRL_NODATA_SETUP (out), data %p", (void *)buf->data);

	ret = udc_ctrl_submit_s_status(dev);
	__ASSERT_NO_MSG(ret == 0);
}

/* IN */
static void udc_dwc3_on_ctrl_nodata_status(const struct device *const dev, struct net_buf *buf)
{
	int ret;

	LOG_DBG("evt: CTRL_NODATA_STATUS (in), data %p", (void *)buf->data);

	udc_ctrl_update_stage(dev, buf);

	ret = udc_ctrl_submit_status(dev, buf);
	__ASSERT_NO_MSG(ret == 0);
}

/*
 * We received a packet, we need to let the USB stack parse it to know what direction the next
 * transaction is expected to have.
 */
static void udc_dwc3_on_ctrl_setup_out(const struct device *const dev, struct net_buf *buf)
{
	struct udc_dwc3_data *priv = udc_get_private(dev);

	/* Only moment where this information is accessible */
	priv->data_stage_length = udc_data_stage_length(buf);

	/* To be able to differentiate the next stage*/
	udc_ep_buf_set_setup(buf);
	udc_ctrl_update_stage(dev, buf);

	if (udc_ctrl_stage_is_data_out(dev)) {
		udc_dwc3_on_ctrl_write_setup(dev, buf);
	} else if (udc_ctrl_stage_is_data_in(dev)) {
		udc_dwc3_on_ctrl_read_setup(dev, buf);
	} else if (udc_ctrl_stage_is_no_data(dev)) {
		udc_dwc3_on_ctrl_nodata_setup(dev, buf);
	} else {
		LOG_ERR("evt: unknown setup stage");
	}
}

/*
 * Handle completion of a CONTROL IN packet (device -> host).
 *
 * Further characterize which type of CONTROL IN packet that is.
 * Handle actions common to all CONTROL IN packets.
 */
static void udc_dwc3_on_ctrl_in(const struct device *const dev)
{
	const struct udc_dwc3_config *cfg = dev->config;
	struct udc_dwc3_ep_data *const ep_data = &cfg->ep_data_in[0];
	struct udc_dwc3_trb *trb = &ep_data->trb_buf[0];
	struct net_buf *buf = ep_data->net_buf[0];

	/* We are not expected to touch that buffer anymore */
	ep_data->net_buf[0] = NULL;
	__ASSERT_NO_MSG(buf != NULL);

	/* Continue to the next step */
	switch (trb->ctrl & UDC_DWC3_TRB_CTRL_TRBCTL_MASK) {
	case UDC_DWC3_TRB_CTRL_TRBCTL_CONTROL_DATA:
		udc_dwc3_on_ctrl_read_data(dev, buf);
		break;
	case UDC_DWC3_TRB_CTRL_TRBCTL_CONTROL_STATUS_2:
		udc_dwc3_on_ctrl_nodata_status(dev, buf);
		udc_dwc3_trb_ctrl_setup_out(dev);
		break;
	case UDC_DWC3_TRB_CTRL_TRBCTL_CONTROL_STATUS_3:
		udc_dwc3_on_ctrl_write_status(dev, buf);
		udc_dwc3_trb_ctrl_setup_out(dev);
		break;
	default:
		CODE_UNREACHABLE;
	}
}

/*
 * Handle completion of a CONTROL OUT packet (host -> device).
 *
 * Further characterize which type of CONTROL OUT packet that is.
 * Handle actions common to all CONTROL OUT packets.
 */
static void udc_dwc3_on_ctrl_out(const struct device *const dev)
{
	const struct udc_dwc3_config *cfg = dev->config;
	struct udc_dwc3_ep_data *const ep_data = &cfg->ep_data_out[0];
	struct udc_dwc3_trb *trb = &ep_data->trb_buf[0];
	struct net_buf *buf = ep_data->net_buf[0];

	__ASSERT_NO_MSG(buf != NULL);

	/* For buffers coming from the host, update the size actually received */
	buf->len = buf->size - FIELD_GET(UDC_DWC3_TRB_STATUS_BUFSIZ_MASK, trb->status);

	/* Latency optimization: set the address immediately to be able to be able to ACK/NAK the
	 * first packets from the host with the new address, otherwise the host issue a reset
	 */
	if (buf->len > 2 && buf->data[0] == 0x00 && buf->data[1] == 0x05) {
		udc_dwc3_do_set_address(dev, buf->data[2]);
	}

	/* We are not expected to touch that buffer anymore */
	ep_data->net_buf[0] = NULL;
	__ASSERT_NO_MSG(buf != NULL);

	/* Continue to the next step */
	switch (trb->ctrl & UDC_DWC3_TRB_CTRL_TRBCTL_MASK) {
	case UDC_DWC3_TRB_CTRL_TRBCTL_CONTROL_SETUP:
		udc_dwc3_on_ctrl_setup_out(dev, buf);
		break;
	case UDC_DWC3_TRB_CTRL_TRBCTL_CONTROL_DATA:
		udc_dwc3_on_ctrl_write_data(dev, buf);
		break;
	case UDC_DWC3_TRB_CTRL_TRBCTL_CONTROL_STATUS_3:
		udc_dwc3_on_ctrl_read_status(dev, buf);
		udc_dwc3_trb_ctrl_setup_out(dev);
		break;
	default:
		CODE_UNREACHABLE;
	}
}

static void udc_dwc3_on_xfer_not_ready(const struct device *const dev, uint32_t evt)
{
	switch (evt & UDC_DWC3_DEPEVT_STATUS_B3_MASK) {
	case UDC_DWC3_DEPEVT_STATUS_B3_CONTROL_SETUP:
		LOG_DBG("--- UDC_DWC3_DEPEVT_XFERNOTREADY_CONTROL_SETUP ---");
		break;
	case UDC_DWC3_DEPEVT_STATUS_B3_CONTROL_DATA:
		LOG_DBG("--- UDC_DWC3_DEPEVT_XFERNOTREADY_CONTROL_DATA ---");
		break;
	case UDC_DWC3_DEPEVT_STATUS_B3_CONTROL_STATUS:
		LOG_DBG("--- UDC_DWC3_DEPEVT_XFERNOTREADY_CONTROL_STATUS ---");
		break;
	}
}

static void udc_dwc3_on_xfer_done(const struct device *const dev,
				  struct udc_dwc3_ep_data *const ep_data)
{
	struct udc_dwc3_trb *trb = &ep_data->trb_buf[ep_data->tail];

	switch (trb->status & UDC_DWC3_TRB_STATUS_TRBSTS_MASK) {
	case UDC_DWC3_TRB_STATUS_TRBSTS_OK:
		break;
	case UDC_DWC3_TRB_STATUS_TRBSTS_MISSEDISOC:
		LOG_ERR("UDC_DWC3_TRB_STATUS_TRBSTS_MISSEDISOC");
		break;
	case UDC_DWC3_TRB_STATUS_TRBSTS_SETUPPENDING:
		LOG_ERR("UDC_DWC3_TRB_STATUS_TRBSTS_SETUPPENDING");
		break;
	case UDC_DWC3_TRB_STATUS_TRBSTS_XFERINPROGRESS:
		LOG_ERR("UDC_DWC3_TRB_STATUS_TRBSTS_XFERINPROGRESS");
		break;
	case UDC_DWC3_TRB_STATUS_TRBSTS_ZLPPENDING:
		LOG_ERR("UDC_DWC3_TRB_STATUS_TRBSTS_ZLPPENDING");
		break;
	default:
		CODE_UNREACHABLE;
	}
}

static void udc_dwc3_on_xfer_done_norm(const struct device *const dev, uint32_t evt)
{
	const struct udc_dwc3_config *cfg = dev->config;
	int epn = FIELD_GET(UDC_DWC3_DEPEVT_EPN_MASK, evt);
	struct udc_dwc3_ep_data *const ep_data =
		(epn & 1) ? &cfg->ep_data_in[epn >> 1] : &cfg->ep_data_out[epn >> 1];
	struct udc_dwc3_trb *trb = &ep_data->trb_buf[ep_data->tail];
	struct net_buf *buf;
	int ret;

	/* Clear the TRB that triggered the event */
	buf = udc_dwc3_pop_trb(dev, ep_data);
	LOG_DBG("evt: XFER_DONE_NORM: EP 0x%02x, data %p", ep_data->cfg.addr, (void *)buf->data);
	__ASSERT_NO_MSG(buf != NULL);
	udc_dwc3_on_xfer_done(dev, ep_data);

	/* For buffers coming from the host, update the size actually received */
	if (ep_data->cfg.caps.out) {
		buf->len = buf->size - FIELD_GET(UDC_DWC3_TRB_STATUS_BUFSIZ_MASK, trb->status);
	}

	ret = udc_submit_ep_event(dev, buf, 0);
	__ASSERT_NO_MSG(ret == 0);

	/* We just made some room for a new buffer, check if something more to enqueue */
	k_work_submit(&ep_data->work);
}

void udc_dwc3_irq_handler(void *ptr)
{
	const struct device *const dev = ptr;
	struct udc_dwc3_data *priv = udc_get_private(dev);
	mm_reg_t base = DEVICE_MMIO_NAMED_GET(dev, base);

	/* disable further interrupts until all events are processed */
	sys_set_bits(base + UDC_DWC3_GEVNTSIZ(0), UDC_DWC3_GEVNTSIZ_EVNTINTRPTMASK);

	/* TODO: IRQs convert work handler to non-delayed */
	k_work_reschedule(&priv->dwork, K_NO_WAIT);
}

/*
 * UDC API
 *
 * Interface called by Zehpyr from the upper levels of abstractions.
 */

int udc_dwc3_ep_enqueue(const struct device *const dev, struct udc_ep_config *const ep_cfg,
			 struct net_buf *buf)
{
	struct udc_dwc3_ep_data *const ep_data = (void *)ep_cfg;
	struct udc_buf_info *bi = udc_get_buf_info(buf);
	mm_reg_t base = DEVICE_MMIO_NAMED_GET(dev, base);

	LOG_DBG("Enqueued buffer to EP 0x%02x", ep_data->cfg.addr);

	switch (ep_data->cfg.addr) {
	case USB_CONTROL_EP_IN:
		/* Save the buffer to fetch it back later */
		__ASSERT(ep_data->net_buf[0] == NULL, "concurrenn requests not allowed for EP0");
		ep_data->net_buf[0] = buf;

		/* Control buffers are managed directly without a queue */
		if (bi->data) {
			udc_dwc3_trb_ctrl_data_in(dev);
		} else if (bi->status && udc_ctrl_stage_is_no_data(dev)) {
			udc_dwc3_trb_ctrl_status_2_in(dev);
		} else if (bi->status) {
			udc_dwc3_trb_ctrl_status_3_in(dev);
		} else {
			CODE_UNREACHABLE;
		}
		break;
	case USB_CONTROL_EP_OUT:
		/* expected to be handled by the driver directly */
		CODE_UNREACHABLE;
		break;
	default:
		/* Submit the buffer to the queue */
		udc_buf_put(ep_cfg, buf);

		/* Process this buffer along with other waiting */
		if (sys_read32(base + UDC_DWC3_DCTL) & UDC_DWC3_DCTL_RUNSTOP) {
			LOG_DBG("submitting to EP 0x%02x", ep_data->cfg.addr);
			k_work_submit(&ep_data->work);
		}
	}

	return 0;
}

int udc_dwc3_ep_dequeue(const struct device *const dev, struct udc_ep_config *const ep_cfg)
{
	unsigned int lock_key;
	struct net_buf *buf;

	lock_key = irq_lock();
	buf = udc_buf_get_all(ep_cfg);
	if (buf) {
		udc_submit_ep_event(dev, buf, -ECONNABORTED);
	}
	irq_unlock(lock_key);
	return 0;
}

int udc_dwc3_ep_disable(const struct device *const dev, struct udc_ep_config *const ep_cfg)
{
	struct udc_dwc3_ep_data *const ep_data = (void *)ep_cfg;
	mm_reg_t base = DEVICE_MMIO_NAMED_GET(dev, base);

	sys_clear_bit(base + UDC_DWC3_DALEPENA, ep_data->epn);
	return 0;
}

/*
 * Halt endpoint. Halted endpoint should respond with a STALL handshake.
 */
int udc_dwc3_ep_set_halt(const struct device *const dev, struct udc_ep_config *const ep_cfg)
{
	const struct udc_dwc3_config *cfg = dev->config;
	struct udc_dwc3_ep_data *ep_data = (void *)ep_cfg;

	LOG_WRN("api: stall ep=0x%02x", ep_data->cfg.addr);

	/* TODO: empty the buffers from the queue */

	switch (ep_data->cfg.addr) {
	case USB_CONTROL_EP_IN:
		/* Remove the TRBs transfer for the cancelled sequence */
		udc_dwc3_depcmd_end_xfer(dev, ep_data, UDC_DWC3_DEPCMD_HIPRI_FORCERM);

		/* The datasheet says to only set stall the OUT direction */
		ep_data = &cfg->ep_data_out[0];

		__fallthrough;
	case USB_CONTROL_EP_OUT:
		udc_dwc3_depcmd_end_xfer(dev, ep_data, 0);
		udc_dwc3_depcmd_set_stall(dev, ep_data);

		/* The hardware will automatically clear the halt state upon
		 * the next setup packet received.
		 */
		udc_dwc3_trb_ctrl_setup_out(dev);
		break;
	default:
		udc_dwc3_depcmd_set_stall(dev, ep_data);
		ep_data->cfg.stat.halted = true;
	}

	return 0;
}

int udc_dwc3_ep_clear_halt(const struct device *const dev, struct udc_ep_config *const ep_cfg)
{
	struct udc_dwc3_ep_data *const ep_data = (void *)ep_cfg;

	LOG_DBG("api: unstall ep=0x%02x", ep_data->cfg.addr);
	__ASSERT_NO_MSG(ep_data->cfg.addr != USB_CONTROL_EP_OUT);
	__ASSERT_NO_MSG(ep_data->cfg.addr != USB_CONTROL_EP_IN);

	udc_dwc3_depcmd_clear_stall(dev, ep_data);
	ep_data->cfg.stat.halted = false;

	return 0;
}

int udc_dwc3_set_address(const struct device *const dev, const uint8_t addr)
{
	mm_reg_t base = DEVICE_MMIO_NAMED_GET(dev, base);
	uint32_t reg;

	/* The address is set in the code earlier to improve latency, only
	 * checking that it is still the value done for consistency.
	 */
	reg = sys_read32(base + UDC_DWC3_DCFG);
	if (FIELD_GET(UDC_DWC3_DCFG_DEVADDR_MASK, reg) != addr) {
		return -EPROTO;
	}

	return 0;
}

enum udc_bus_speed udc_dwc3_device_speed(const struct device *const dev)
{
	mm_reg_t base = DEVICE_MMIO_NAMED_GET(dev, base);

	switch (sys_read32(base + UDC_DWC3_DSTS) & UDC_DWC3_DSTS_CONNECTSPD_MASK) {
	case UDC_DWC3_DSTS_CONNECTSPD_FS:
		return UDC_BUS_SPEED_FS;
	case UDC_DWC3_DSTS_CONNECTSPD_HS:
		return UDC_BUS_SPEED_HS;
	case UDC_DWC3_DSTS_CONNECTSPD_SS:
		return UDC_BUS_SPEED_SS;
	}

	__ASSERT(0, "unknown device speed");
	return 0;
}

int udc_dwc3_enable(const struct device *const dev)
{
	const struct udc_dwc3_config *cfg = dev->config;
	struct udc_dwc3_data *priv = udc_get_private(dev);
	mm_reg_t base = DEVICE_MMIO_NAMED_GET(dev, base);
	int ret;

	LOG_DBG("Enabling DWC3 driver");

	ret = udc_dwc3_quirk_enable(dev);
	if (ret != 0) {
		return ret;
	}

	__ASSERT_NO_MSG(atomic_test_bit(&priv->flags, UDC_DWC3_FLAG_INITIALIZED));

	/* Bootstrap: prepare reception of the initial Setup packet */
	udc_dwc3_trb_ctrl_setup_out(dev);

	/* Enable the DWC3 events */
	sys_set_bits(base + UDC_DWC3_DCTL, UDC_DWC3_DCTL_RUNSTOP);

	/* Enable the IRQ (for now, just schedule a first work queue job) */
	cfg->irq_enable_func();
	k_work_schedule(&priv->dwork, K_NO_WAIT);

	return 0;
}

int udc_dwc3_disable(const struct device *const dev)
{
	mm_reg_t base = DEVICE_MMIO_NAMED_GET(dev, base);

	LOG_DBG("Disabling DWC3 driver");

	sys_clear_bits(base + UDC_DWC3_DCTL, UDC_DWC3_DCTL_RUNSTOP);

	return 0;
}

/*
 * Hardware Init
 *
 * Prepare the driver and the hardware to being used.
 * This goes through register configuration and register commands.
 */

int udc_dwc3_ep_enable(const struct device *const dev, struct udc_ep_config *const ep_cfg)
{
	struct udc_dwc3_ep_data *const ep_data = (struct udc_dwc3_ep_data *)ep_cfg;
	mm_reg_t base = DEVICE_MMIO_NAMED_GET(dev, base);

	LOG_DBG("%s 0x%02x", __func__, ep_data->cfg.addr);

	memset(ep_data->trb_buf, 0, sizeof(*ep_data->trb_buf) * CONFIG_UDC_DWC3_TRB_NUM);
	udc_dwc3_depcmd_ep_config(dev, ep_data);
	udc_dwc3_depcmd_ep_xfer_config(dev, ep_data);

	if (!ep_data->cfg.caps.control) {
		udc_dwc3_trb_norm_init(dev, ep_data);
	}

	/* Starting from here, the endpoint can be used */
	sys_set_bits(base + UDC_DWC3_DALEPENA, UDC_DWC3_DALEPENA_USBACTEP(ep_data->epn));

	/* Walk through the list of buffer to enqueue we might have blocked */
	k_work_submit(&ep_data->work);

	return 0;
}

/*
 * Prepare and configure most of the parts, if the controller has a way
 * of detecting VBUS activity it should be enabled here.
 * Only udc_dwc3_enable() makes device visible to the host.
 */
int udc_dwc3_init(const struct device *const dev)
{
	struct udc_dwc3_data *priv = udc_get_private(dev);
	mm_reg_t base = DEVICE_MMIO_NAMED_GET(dev, base);
	int ret;

	LOG_DBG("Initializing the DWC3 core");

	ret = udc_dwc3_quirk_init(dev);
	if (ret != 0) {
		return ret;
	}

	/* Issue a soft reset to the core and USB2 and USB3 PHY */
	sys_set_bits(base + UDC_DWC3_GCTL, UDC_DWC3_GCTL_CORESOFTRESET);
	sys_set_bits(base + UDC_DWC3_GUSB3PIPECTL, UDC_DWC3_GUSB3PIPECTL_PHYSOFTRST);
	sys_set_bits(base + UDC_DWC3_GUSB2PHYCFG, UDC_DWC3_GUSB2PHYCFG_PHYSOFTRST);
	k_sleep(K_USEC(1000)); /* TODO: reduce amount of wait time */

	/* Teriminate the reset of the USB2 and USB3 PHY first */
	sys_clear_bits(base + UDC_DWC3_GUSB3PIPECTL, UDC_DWC3_GUSB3PIPECTL_PHYSOFTRST);
	sys_clear_bits(base + UDC_DWC3_GUSB2PHYCFG, UDC_DWC3_GUSB2PHYCFG_PHYSOFTRST);

	/* Teriminate the reset of the DWC3 core after it */
	sys_clear_bits(base + UDC_DWC3_GCTL, UDC_DWC3_GCTL_CORESOFTRESET);

	/* Initialize USB2 PHY vendor-specific wrappers */
	sys_set_bits(base + UDC_DWC3_U2PHYCTRL1, UDC_DWC3_U2PHYCTRL1_SEL_INTERNALCLK);
	sys_set_bits(base + UDC_DWC3_U2PHYCTRL2, UDC_DWC3_U2PHYCTRL2_REFCLK_SEL);

	/* The USB core was reset, configure it as documented */
	udc_dwc3_on_soft_reset(dev);

	/* Configure the control OUT endpoint */
	ret = udc_ep_enable_internal(dev, USB_CONTROL_EP_OUT, USB_EP_TYPE_CONTROL, 512, 0);
	if (ret != 0) {
		LOG_ERR("init: could not enable control OUT ep");
		return ret;
	}

	/* Configure the control IN endpoint */
	ret = udc_ep_enable_internal(dev, USB_CONTROL_EP_IN, USB_EP_TYPE_CONTROL, 512, 0);
	if (ret != 0) {
		LOG_ERR("init: could not enable control IN ep");
		return ret;
	}

	LOG_INF("Event buffer size is %u bytes", sys_read32(base + UDC_DWC3_GEVNTSIZ(0)));

	atomic_set_bit(&priv->flags, UDC_DWC3_FLAG_INITIALIZED);
	return 0;
}

static const struct udc_api udc_dwc3_api = {
	.lock = udc_dwc3_lock,
	.unlock = udc_dwc3_unlock,
	.device_speed = udc_dwc3_device_speed,
	.init = udc_dwc3_init,
	.enable = udc_dwc3_enable,
	.disable = udc_dwc3_disable,
	.shutdown = udc_dwc3_shutdown,
	.set_address = udc_dwc3_set_address,
	.ep_enable = udc_dwc3_ep_enable,
	.ep_disable = udc_dwc3_ep_disable,
	.ep_set_halt = udc_dwc3_ep_set_halt,
	.ep_clear_halt = udc_dwc3_ep_clear_halt,
	.ep_enqueue = udc_dwc3_ep_enqueue,
	.ep_dequeue = udc_dwc3_ep_dequeue,
};

static void udc_dwc3_ep_worker(struct k_work *const work)
{
	struct udc_dwc3_ep_data *const ep_data =
		CONTAINER_OF(work, struct udc_dwc3_ep_data, work);
	const struct device *const dev = ep_data->dev;
	struct net_buf *buf;
	int ret;

	LOG_DBG("queue: checking for pending transfers for EP 0x%02x", ep_data->cfg.addr);

	if (ep_data->cfg.stat.halted) {
		LOG_DBG("queue: endpoint is halted, not processing buffers");
		return;
	}

	while ((buf = udc_buf_peek(&ep_data->cfg)) != NULL) {
		LOG_INF("Processing buffer %p from queue", (void *)buf);

		ret = udc_dwc3_trb_bulk(dev, ep_data, buf);
		if (ret != 0) {
			LOG_DBG("queue: abort: No more room for buffer");
			break;
		}

		LOG_DBG("queue: success: Buffer enqueued");
		udc_buf_get(&ep_data->cfg);
	}
	LOG_DBG("queue: Done");
}

#define NORMAL_EP(n, fn) fn(n + 2)

static void udc_dwc3_event_worker(struct k_work *const work)
{
	struct k_work_delayable *dwork = k_work_delayable_from_work(work);
	struct udc_dwc3_data *priv = CONTAINER_OF(dwork, struct udc_dwc3_data, dwork);
	const struct device *const dev = priv->dev;
	const struct udc_dwc3_config *cfg = dev->config;
	mm_reg_t base = DEVICE_MMIO_NAMED_GET(dev, base);
	uint32_t evt;

	if (sys_read32(base + UDC_DWC3_GEVNTCOUNT(0)) == 0) {
		/* allow further interrupts */
		sys_clear_bits(base + UDC_DWC3_GEVNTSIZ(0), UDC_DWC3_GEVNTSIZ_EVNTINTRPTMASK);

		/* no more event to process */
		return;
	}

	/* Cache the current event and release the resource */
	evt = cfg->evt_buf[priv->evt_next];

	switch (evt & UDC_DWC3_EVT_MASK) {
	case UDC_DWC3_DEPEVT_XFERCOMPLETE(0):
		LOG_DBG("--- DEPEVT_XFERCOMPLETE(0) ---");
		udc_dwc3_on_ctrl_out(dev);
		break;
	case UDC_DWC3_DEPEVT_XFERCOMPLETE(1):
		LOG_DBG("--- DEPEVT_XFERCOMPLETE(1) ---");
		udc_dwc3_on_ctrl_in(dev);
		break;
	case LISTIFY(30, NORMAL_EP, (: case), UDC_DWC3_DEPEVT_XFERCOMPLETE):
	case LISTIFY(30, NORMAL_EP, (: case), UDC_DWC3_DEPEVT_XFERINPROGRESS):
		LOG_DBG("--- DEPEVT_XFERINPROGRESS ---");
		udc_dwc3_on_xfer_done_norm(dev, evt);
		break;
	case UDC_DWC3_DEPEVT_XFERNOTREADY(0):
	case UDC_DWC3_DEPEVT_XFERNOTREADY(1):
		udc_dwc3_on_xfer_not_ready(dev, evt);
		break;
	case UDC_DWC3_DEVT_DISCONNEVT:
		LOG_DBG("--- DEVT_DISCONNEVT ---");
		break;
	case UDC_DWC3_DEVT_USBRST:
		LOG_DBG("--- DEVT_USBRST ---");
		udc_dwc3_on_usb_reset(dev);
		break;
	case UDC_DWC3_DEVT_CONNECTDONE:
		LOG_DBG("--- DEVT_CONNECTDONE ---");
		udc_dwc3_on_connect_done(dev);
		break;
	case UDC_DWC3_DEVT_ULSTCHNG:
		LOG_DBG("--- DEVT_ULSTCHNG ---");
		udc_dwc3_on_link_state_event(dev);
		break;
	case UDC_DWC3_DEVT_WKUPEVT:
		LOG_DBG("--- DEVT_WKUPEVT ---");
		break;
	case UDC_DWC3_DEVT_SUSPEND:
		LOG_DBG("--- DEVT_SUSPEND ---");
		break;
	case UDC_DWC3_DEVT_SOF:
		LOG_DBG("--- DEVT_SOF ---");
		break;
	case UDC_DWC3_DEVT_CMDCMPLT:
		LOG_DBG("--- DEVT_CMDCMPLT ---");
		break;
	case UDC_DWC3_DEVT_VNDRDEVTSTRCVED:
		LOG_DBG("--- DEVT_VNDRDEVTSTRCVED ---");
		break;
	case UDC_DWC3_DEVT_ERRTICERR:
		LOG_ERR("--- DEVT_ERRTICERR ---");
		CODE_UNREACHABLE;
		break;
	case UDC_DWC3_DEVT_EVNTOVERFLOW:
		LOG_ERR("--- DEVT_EVNTOVERFLOW ---");
		CODE_UNREACHABLE;
		break;
	default:
		LOG_ERR("unhandled event: 0x%x", evt);
		CODE_UNREACHABLE;
	}

	sys_write32(sizeof(evt), base + UDC_DWC3_GEVNTCOUNT(0));
	udc_dwc3_ring_inc(&priv->evt_next, CONFIG_UDC_DWC3_EVENTS_NUM);

	LOG_DBG("--- * ---");
	/* TODO: switch to direct work item */
	k_work_reschedule(&priv->dwork, K_NO_WAIT);
}

/*
 * Initialize the controller and endpoints capabilities,
 * register endpoint structures, no hardware I/O yet.
 */
int udc_dwc3_driver_preinit(const struct device *const dev)
{
	const struct udc_dwc3_config *cfg = dev->config;
	struct udc_dwc3_data *priv = udc_get_private(dev);
	struct udc_data *data = dev->data;
	struct udc_dwc3_ep_data *ep_data;
	uint16_t mps = 0;
	int ret;

	ret = udc_dwc3_quirk_preinit(dev);
	if (ret != 0) {
		return ret;
	}

	DEVICE_MMIO_NAMED_MAP(dev, base, K_MEM_CACHE_NONE);

	k_mutex_init(&data->mutex);
	k_work_init_delayable(&priv->dwork, &udc_dwc3_event_worker);

	data->caps.rwup = true;
	switch (cfg->maximum_speed_idx) {
	case UDC_DWC3_SPEED_IDX_HIGH_SPEED:
		LOG_DBG("UDC_DWC3_SPEED_IDX_HIGH_SPEED");
		data->caps.mps0 = UDC_MPS0_64;
		data->caps.hs = true;
		mps = 1024;
		break;
	case UDC_DWC3_SPEED_IDX_FULL_SPEED:
		LOG_DBG("UDC_DWC3_SPEED_IDX_FULL_SPEED");
		data->caps.mps0 = UDC_MPS0_64;
		mps = 64;
		break;
	default:
		LOG_ERR("Not implemented");
	}

	/* Control IN endpoint */
	ep_data = &cfg->ep_data_in[0];
	k_work_init(&ep_data->work, udc_dwc3_ep_worker);
	ep_data->dev = dev;
	ep_data->cfg.addr = USB_CONTROL_EP_IN;
	ep_data->cfg.caps.in = 1;
	ep_data->cfg.caps.control = 1;
	ep_data->cfg.caps.mps = mps;
	ep_data->trb_buf = cfg->trb_buf_in[0];
	ep_data->epn = 1;
	ret = udc_register_ep(dev, &ep_data->cfg);
	if (ret != 0) {
		LOG_ERR("Failed to register endpoint");
		return ret;
	}

	/* Control OUT endpoint */
	ep_data = &cfg->ep_data_out[0];
	k_work_init(&ep_data->work, udc_dwc3_ep_worker);
	ep_data->dev = dev;
	ep_data->cfg.addr = USB_CONTROL_EP_OUT;
	ep_data->cfg.caps.out = 1;
	ep_data->cfg.caps.control = 1;
	ep_data->cfg.caps.mps = mps;
	ep_data->trb_buf = cfg->trb_buf_out[0];
	ep_data->epn = 0;
	ret = udc_register_ep(dev, &ep_data->cfg);
	if (ret != 0) {
		LOG_ERR("Failed to register endpoint");
		return ret;
	}

	/* Normal IN endpoints */
	for (int i = 1; i < cfg->num_in_eps; i++) {
		LOG_DBG("Preinit endpoint 0x%02x", USB_EP_DIR_IN | i);
		ep_data = &cfg->ep_data_in[i];
		k_work_init(&ep_data->work, udc_dwc3_ep_worker);
		ep_data->dev = dev;
		ep_data->cfg.addr = USB_EP_DIR_IN | i;
		ep_data->cfg.caps.in = true;
		ep_data->cfg.caps.bulk = true;
		ep_data->cfg.caps.interrupt = true;
		ep_data->cfg.caps.iso = true;
		ep_data->cfg.caps.mps = mps;
		ep_data->trb_buf = cfg->trb_buf_in[i];
		ep_data->epn = (i << 1) | 1;
		ret = udc_register_ep(dev, &ep_data->cfg);
		if (ret != 0) {
			LOG_ERR("Failed to register endpoint");
			return ret;
		}
	}

	/* Normal OUT endpoints */
	for (int i = 1; i < cfg->num_out_eps; i++) {
		LOG_DBG("Preinit endpoint 0x%02x", USB_EP_DIR_OUT | i);
		ep_data = &cfg->ep_data_out[i];
		k_work_init(&ep_data->work, udc_dwc3_ep_worker);
		ep_data->dev = dev;
		ep_data->cfg.addr = USB_EP_DIR_OUT | i;
		ep_data->cfg.caps.out = true;
		ep_data->cfg.caps.bulk = true;
		ep_data->cfg.caps.interrupt = true;
		ep_data->cfg.caps.iso = true;
		ep_data->cfg.caps.mps = mps;
		ep_data->trb_buf = cfg->trb_buf_out[i];
		ep_data->epn = (i << 1) | 0;
		ret = udc_register_ep(dev, &ep_data->cfg);
		if (ret != 0) {
			LOG_ERR("Failed to register endpoint");
			return ret;
		}
	}

	LOG_DBG("done");

	return 0;
}

#define UDC_DWC3_DEVICE_DEFINE(n)						\
	UDC_DWC3_QUIRK_DEFINE(n);						\
										\
	static void udc_dwc3_irq_enable_func_##n(void)				\
	{									\
		IRQ_CONNECT(DT_INST_IRQN(n), DT_INST_IRQ(n, priority),		\
			    udc_dwc3_irq_handler, DEVICE_DT_INST_GET(n), 0);	\
		irq_enable(DT_INST_IRQN(n));					\
	}									\
										\
	static void udc_dwc3_irq_disable_func_##n(void)				\
	{									\
		irq_disable(DT_INST_IRQN(n));					\
	}									\
										\
	static __nocache uint32_t udc_dwc3_dma_evt_buf_##n			\
		[CONFIG_UDC_DWC3_EVENTS_NUM]					\
		__aligned(CONFIG_UDC_DWC3_EVENTS_NUM);				\
										\
	static __nocache struct udc_dwc3_trb udc_dwc3_dma_trb_i##n		\
		[DT_INST_PROP(n, num_in_endpoints)][CONFIG_UDC_DWC3_TRB_NUM];	\
										\
	static __nocache struct udc_dwc3_trb udc_dwc3_dma_trb_o##n		\
		[DT_INST_PROP(n, num_out_endpoints)][CONFIG_UDC_DWC3_TRB_NUM];	\
										\
	static struct udc_dwc3_ep_data udc_dwc3_ep_data_i##n			\
		[DT_INST_PROP(n, num_in_endpoints)];				\
										\
	static struct udc_dwc3_ep_data udc_dwc3_ep_data_o##n			\
		[DT_INST_PROP(n, num_out_endpoints)];				\
										\
	static const struct udc_dwc3_config udc_dwc3_config_##n = {		\
		DEVICE_MMIO_NAMED_ROM_INIT_BY_NAME(base, DT_DRV_INST(n)),	\
		.quirk_data = &udc_dwc3_quirk_data_##n,				\
		.quirk_config = &udc_dwc3_quirk_config_##n,			\
		.num_in_eps = DT_INST_PROP(n, num_in_endpoints),		\
		.num_out_eps = DT_INST_PROP(n, num_out_endpoints),		\
		.ep_data_in  = udc_dwc3_ep_data_i##n,				\
		.ep_data_out = udc_dwc3_ep_data_o##n,				\
		.trb_buf_in = udc_dwc3_dma_trb_i##n,				\
		.trb_buf_out = udc_dwc3_dma_trb_o##n,				\
		.evt_buf = udc_dwc3_dma_evt_buf_##n,				\
		.maximum_speed_idx = DT_ENUM_IDX(DT_DRV_INST(n), maximum_speed),\
		.irq_enable_func = udc_dwc3_irq_enable_func_##n,		\
		.irq_disable_func = udc_dwc3_irq_disable_func_##n,		\
	};									\
										\
	static struct udc_dwc3_data udc_dwc3_priv_##n = {			\
		.dev = DEVICE_DT_INST_GET(n),					\
	};									\
										\
	static struct udc_data udc_data_##n = {					\
		.mutex = Z_MUTEX_INITIALIZER(udc_data_##n.mutex),		\
		.priv = &udc_dwc3_priv_##n,					\
	};									\
										\
	DEVICE_DT_INST_DEFINE(n, udc_dwc3_driver_preinit, NULL, &udc_data_##n,	\
			      &udc_dwc3_config_##n, POST_KERNEL,		\
			      CONFIG_KERNEL_INIT_PRIORITY_DEVICE,		\
			      &udc_dwc3_api);

DT_INST_FOREACH_STATUS_OKAY(UDC_DWC3_DEVICE_DEFINE)
