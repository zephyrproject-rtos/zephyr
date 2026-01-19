/*
 * Copyright (c) 2026 Nuvoton Technology Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_USB_UDC_NUMAKER_H
#define ZEPHYR_DRIVERS_USB_UDC_NUMAKER_H

#include <stdint.h>
#include <soc.h>

/* Dummy USBD/HSUSBD device definitions
 *
 * This is used to pass compile for targets not supporting this device.
 * Related code should be unreachable.
 */

#if defined(CONFIG_SOC_SERIES_M333X)

BUILD_ASSERT(DT_NUM_INST_STATUS_OKAY(nuvoton_numaker_usbd) == 0,
	     "The SoC series should have no USBD");

typedef struct {
	uint32_t BUFSEG;
	uint32_t MXPLD;
	uint32_t CFG;
	uint32_t CFGP;
} USBD_EP_T;

#define USBD_CFGP_SSTALL_Msk 0
#define USBD_CFGP_CLRRDY_Msk 0
#define USBD_CFG_DSQSYNC_Msk 0
#define USBD_CFG_EPNUM_Pos   0
#define USBD_CFG_EPNUM_Msk   0
#define USBD_CFG_STATE_Msk   0
#define USBD_MXPLD_MXPLD_Pos 0
#define USBD_MXPLD_MXPLD_Msk 0

#define USBD_CFG_CSTALL         0
#define USBD_CFG_EPMODE_DISABLE 0
#define USBD_CFG_EPMODE_IN      0
#define USBD_CFG_EPMODE_OUT     0
#define USBD_CFG_TYPE_ISO       0

typedef struct {
	uint32_t INTEN;
	uint32_t INTSTS;
	uint32_t FADDR;
	uint32_t EPSTS;
	uint32_t ATTR;
	uint32_t VBUSDET;
	uint32_t STBUFSEG;
	uint32_t EPSTS0;
	uint32_t EPSTS1;
	uint32_t EPSTS2;
	uint32_t EPSTS3;
	uint32_t EPINTSTS;
	uint32_t SE0;
	USBD_EP_T EP[1];
} USBD_T;

#define USBD_INTSTS_SOFIF_Msk      0
#define USBD_ATTR_BYTEM_Msk        0
#define USBD_ATTR_DPPUEN_Msk       0
#define USBD_ATTR_PHYEN_Msk        0
#define USBD_ATTR_PWRDN_Msk        0
#define USBD_ATTR_RWAKEUP_Msk      0
#define USBD_ATTR_USBEN_Msk        0
#define USBD_VBUSDET_VBUSDET_Msk   0
#define USBD_STBUFSEG_STBUFSEG_Msk 0

#define USBD_INT_BUS       0
#define USBD_INT_FLDET     0
#define USBD_INT_USB       0
#define USBD_INT_WAKEUP    0
#define USBD_INTSTS_BUS    0
#define USBD_INTSTS_FLDET  0
#define USBD_INTSTS_SETUP  0
#define USBD_INTSTS_USB    0
#define USBD_INTSTS_WAKEUP 0
#define USBD_PHY_EN        0
#define USBD_STATE_RESUME  0
#define USBD_STATE_SUSPEND 0
#define USBD_STATE_USBRST  0
#define USBD_USB_EN        0
#define USBD_DRVSE0        0

#define EP0 0
#define EP1 1

#endif

/* Dummy HSUSBD device definition
 *
 * This is used to pass compile for targets not supporting this device.
 * Related code should be unreachable.
 */
#if defined(CONFIG_SOC_SERIES_M2L31X)

BUILD_ASSERT(DT_NUM_INST_STATUS_OKAY(nuvoton_numaker_hsusbd) == 0,
	     "The SoC series should have no HSUSBD");

typedef struct {
	union {
		uint32_t EPDAT;
		uint8_t EPDAT_BYTE;
	};
	uint32_t EPINTSTS;
	uint32_t EPINTEN;
	uint32_t EPDATCNT;
	uint32_t EPRSPCTL;
	uint32_t EPMPS;
	uint32_t EPTXCNT;
	uint32_t EPCFG;
	uint32_t EPBUFSTART;
	uint32_t EPBUFEND;
} HSUSBD_EP_T;

#define HSUSBD_EPINTSTS_BUFEMPTYIF_Msk 0
#define HSUSBD_EPINTSTS_BUFFULLIF_Msk  0
#define HSUSBD_EPINTSTS_RXPKIF_Msk     0
#define HSUSBD_EPINTEN_RXPKIEN_Msk     0
#define HSUSBD_EPINTEN_TXPKIEN_Msk     0
#define HSUSBD_EPDATCNT_DATCNT_Pos     0
#define HSUSBD_EPDATCNT_DATCNT_Msk     0
#define HSUSBD_EPRSPCTL_HALT_Msk       0
#define HSUSBD_EPRSPCTL_MODE_Msk       0
#define HSUSBD_EPRSPCTL_TOGGLE_Msk     0
#define HSUSBD_EPCFG_EPDIR_Msk         0
#define HSUSBD_EPCFG_EPEN_Msk          0
#define HSUSBD_EPCFG_EPNUM_Pos         0
#define HSUSBD_EPCFG_EPNUM_Msk         0

typedef struct {
	uint32_t GINTSTS;
	uint32_t GINTEN;
	uint32_t BUSINTSTS;
	uint32_t BUSINTEN;
	uint32_t OPER;
	uint32_t FRAMECNT;
	uint32_t FADDR;
	uint32_t TEST;
	union {
		uint32_t CEPDAT;
		uint8_t CEPDAT_BYTE;
	};
	uint32_t CEPCTL;
	uint32_t CEPINTEN;
	uint32_t CEPINTSTS;
	uint32_t CEPTXCNT;
	uint32_t CEPRXCNT;
	uint32_t CEPDATCNT;
	uint32_t SETUP1_0;
	uint32_t SETUP3_2;
	uint32_t SETUP5_4;
	uint32_t SETUP7_6;
	uint32_t CEPBUFSTART;
	uint32_t CEPBUFEND;
	uint32_t DMACTL;
	uint32_t DMACNT;
	HSUSBD_EP_T EP[1];
	uint32_t DMAADDR;
	uint32_t PHYCTL;
} HSUSBD_T;

#define HSUSBD_GINTEN_CEPIEN_Msk        0
#define HSUSBD_GINTEN_EPAIEN_Pos        0
#define HSUSBD_GINTEN_USBIEN_Msk        0
#define HSUSBD_GINTSTS_EPAIF_Pos        0
#define HSUSBD_BUSINTEN_RESUMEIEN_Msk   0
#define HSUSBD_BUSINTEN_RSTIEN_Msk      0
#define HSUSBD_BUSINTEN_SUSPENDIEN_Msk  0
#define HSUSBD_BUSINTEN_VBUSDETIEN_Msk  0
#define HSUSBD_BUSINTSTS_RESUMEIF_Msk   0
#define HSUSBD_BUSINTSTS_RSTIF_Msk      0
#define HSUSBD_BUSINTSTS_SOFIF_Msk      0
#define HSUSBD_BUSINTSTS_SUSPENDIF_Msk  0
#define HSUSBD_BUSINTSTS_VBUSDETIF_Msk  0
#define HSUSBD_OPER_CURSPD_Msk          0
#define HSUSBD_OPER_HISPDEN_Msk         0
#define HSUSBD_OPER_RESUMEEN_Msk        0
#define HSUSBD_CEPCTL_NAKCLR_Msk        0
#define HSUSBD_CEPINTEN_ERRIEN_Msk      0
#define HSUSBD_CEPINTEN_RXPKIEN_Msk     0
#define HSUSBD_CEPINTEN_SETUPPKIEN_Msk  0
#define HSUSBD_CEPINTEN_SETUPTKIEN_Msk  0
#define HSUSBD_CEPINTEN_STALLIEN_Msk    0
#define HSUSBD_CEPINTEN_STSDONEIEN_Msk  0
#define HSUSBD_CEPINTEN_TXPKIEN_Msk     0
#define HSUSBD_CEPINTSTS_BUFEMPTYIF_Msk 0
#define HSUSBD_CEPINTSTS_BUFFULLIF_Msk  0
#define HSUSBD_CEPINTSTS_RXPKIF_Msk     0
#define HSUSBD_CEPINTSTS_SETUPPKIF_Msk  0
#define HSUSBD_CEPINTSTS_SETUPTKIF_Msk  0
#define HSUSBD_CEPINTSTS_STSDONEIF_Msk  0
#define HSUSBD_CEPINTSTS_TXPKIF_Msk     0
#define HSUSBD_CEPDATCNT_DATCNT_Pos     0
#define HSUSBD_CEPDATCNT_DATCNT_Msk     0
#define HSUSBD_PHYCTL_DPPUEN_Msk        0
#define HSUSBD_PHYCTL_PHYCLKSTB_Msk     0
#define HSUSBD_PHYCTL_PHYEN_Msk         0
#define HSUSBD_PHYCTL_VBUSDET_Msk       0
#define HSUSBD_PHYCTL_VBUSWKEN_Msk      0

#define HSUSBD_CEPCTL_FLUSH          0
#define HSUSBD_CEPCTL_NAKCLR         0
#define HSUSBD_CEPCTL_STALL          0
#define HSUSBD_CEPCTL_ZEROLEN        0
#define HSUSBD_EP_CFG_DIR_IN         0
#define HSUSBD_EP_CFG_DIR_OUT        0
#define HSUSBD_EP_CFG_TYPE_BULK      0
#define HSUSBD_EP_CFG_TYPE_INT       0
#define HSUSBD_EP_CFG_TYPE_ISO       0
#define HSUSBD_EP_CFG_VALID          0
#define HSUSBD_EP_RSPCTL_FLUSH       0
#define HSUSBD_EP_RSPCTL_HALT        0
#define HSUSBD_EP_RSPCTL_MODE_AUTO   0
#define HSUSBD_EP_RSPCTL_MODE_FLY    0
#define HSUSBD_EP_RSPCTL_MODE_MANUAL 0
#define HSUSBD_EP_RSPCTL_SHORTTXEN   0
#define HSUSBD_EP_RSPCTL_TOGGLE      0
#define HSUSBD_EP_RSPCTL_ZEROLEN     0

#define CEP 0
#define EPA 0
#define EPB 0

#endif

#endif /* ZEPHYR_DRIVERS_USB_UDC_NUMAKER_H */
