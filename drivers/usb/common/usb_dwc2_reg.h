/*
 * Copyright (c) 2025 Espressif Systems (Shanghai) Co., Ltd.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_USB_COMMON_USB_DWC2_REG
#define ZEPHYR_DRIVERS_USB_COMMON_USB_DWC2_REG

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================= */
/* Register bit mask definitions for the DWC2 USB controller     */
/* ============================================================= */

typedef union {
	struct {
		uint32_t glbllntrmsk: 1;
		uint32_t hbstlen: 4;
		uint32_t dmaen: 1;
		uint32_t reserved_6: 1;
		uint32_t nptxfemplvl: 1;
		uint32_t ptxfemplvl: 1;
		uint32_t reserved_9: 12;
		uint32_t remmemsupp: 1;
		uint32_t notialldmawrit: 1;
		uint32_t ahbsingle: 1;
		uint32_t invdescendianess: 1;
		uint32_t reserved_25: 7;
	};
	uint32_t val;
} usb_dwc2_gahbcfg_reg_t;

typedef union {
	struct {
		uint32_t toutcal: 3;
		uint32_t phyif: 1;
		uint32_t ulpiutmisel: 1;
		uint32_t fsintf: 1;
		uint32_t physel: 1;
		uint32_t reserved_7: 1;
		uint32_t srpcap: 1;
		uint32_t hnpcap: 1;
		uint32_t usbtrdtim: 4;
		uint32_t reserved_14: 1;
		uint32_t phylpwrclksel: 1;
		uint32_t reserved_16: 1;
		uint32_t reserved_17: 1;
		uint32_t reserved_18: 1;
		uint32_t reserved_19: 1;
		uint32_t reserved_20: 1;
		uint32_t reserved_21: 1;
		uint32_t termseldlpulse: 1;
		uint32_t reserved_23: 1;
		uint32_t reserved_24: 1;
		uint32_t reserved_25: 1;
		uint32_t icusbcap: 1;
		uint32_t reserved_27: 1;
		uint32_t txenddelay: 1;
		uint32_t forcehstmode: 1;
		uint32_t forcedevmode: 1;
		uint32_t corrupttxpkt: 1;
	};
	uint32_t val;
} usb_dwc2_gusbcfg_reg_t;

typedef union {
	struct {
		uint32_t csftrst: 1;
		uint32_t piufssftrst: 1;
		uint32_t frmcntrrst: 1;
		uint32_t reserved_3: 1;
		uint32_t rxfflsh: 1;
		uint32_t txfflsh: 1;
		uint32_t txfnum: 5;
		uint32_t reserved_11: 19;
		uint32_t dmareq: 1;
		uint32_t ahbidle: 1;
	};
	uint32_t val;
} usb_dwc2_grstctl_reg_t;

typedef union {
    struct {
        uint32_t rxfdep: 16;
        uint32_t reserved_16: 16;
    };
    uint32_t val;
} usb_dwc2_grxfsiz_reg_t;

typedef union {
    struct {
        uint32_t nptxfstaddr: 16;
        uint32_t nptxfdep: 16;
    };
    uint32_t val;
} usb_dwc2_gnptxfsiz_reg_t;

typedef union {
    struct {
        uint32_t nptxfspcavail: 16;
        uint32_t nptxqspcavail: 8;
        uint32_t nptxqtop: 7;
        uint32_t reserved_31: 1;
    };
    uint32_t val;
} usb_dwc2_gnptxsts_reg_t;

typedef union {
    struct {
        uint32_t ptxfstaddr: 16;
        uint32_t ptxfsize: 16;
    };
    uint32_t val;
} usb_dwc2_hptxfsiz_reg_t;
typedef union {
	struct {
		uint32_t curmod: 1;
		uint32_t modemis: 1;
		uint32_t otgint: 1;
		uint32_t sof: 1;
		uint32_t rxflvl: 1;
		uint32_t nptxfemp: 1;
		uint32_t ginnakeff: 1;
		uint32_t goutnakeff: 1;
		uint32_t reserved_8: 1;
		uint32_t reserved_9: 1;
		uint32_t erlysusp: 1;
		uint32_t usbsusp: 1;
		uint32_t usbrst: 1;
		uint32_t enumdone: 1;
		uint32_t isooutdrop: 1;
		uint32_t eopf: 1;
		uint32_t reserved_16: 1;
		uint32_t epmis: 1;
		uint32_t iepint: 1;
		uint32_t oepint: 1;
		uint32_t incompisoin: 1;
		uint32_t incompip: 1;
		uint32_t fetsusp: 1;
		uint32_t resetdet: 1;
		uint32_t prtint: 1;
		uint32_t hchint: 1;
		uint32_t ptxfemp: 1;
		uint32_t reserved_27: 1;
		uint32_t conidstschng: 1;
		uint32_t disconnint: 1;
		uint32_t sessreqint: 1;
		uint32_t wkupint: 1;
	};
	uint32_t val;
} usb_dwc2_gintsts_reg_t;

typedef union {
	struct {
		uint32_t reserved_0: 1;
		uint32_t modemismsk: 1;
		uint32_t otgintmsk: 1;
		uint32_t sofmsk: 1;
		uint32_t rxflvlmsk: 1;
		uint32_t nptxfempmsk: 1;
		uint32_t ginnakeffmsk: 1;
		uint32_t goutnackeffmsk: 1;
		uint32_t reserved_8: 1;
		uint32_t reserved_9: 1;
		uint32_t erlysuspmsk: 1;
		uint32_t usbsuspmsk: 1;
		uint32_t usbrstmsk: 1;
		uint32_t enumdonemsk: 1;
		uint32_t isooutdropmsk: 1;
		uint32_t eopfmsk: 1;
		uint32_t reserved_16: 1;
		uint32_t epmismsk: 1;
		uint32_t iepintmsk: 1;
		uint32_t oepintmsk: 1;
		uint32_t incompisoinmsk: 1;
		uint32_t incompipmsk: 1;
		uint32_t fetsuspmsk: 1;
		uint32_t resetdetmsk: 1;
		uint32_t prtintmsk: 1;
		uint32_t hchintmsk: 1;
		uint32_t ptxfempmsk: 1;
		uint32_t reserved_27: 1;
		uint32_t conidstschngmsk: 1;
		uint32_t disconnintmsk: 1;
		uint32_t sessreqintmsk: 1;
		uint32_t wkupintmsk: 1;
	};
	uint32_t val;
} usb_dwc2_gintmsk_reg_t;

typedef union {
	struct {
		uint32_t epdir;
	};
	uint32_t val;
} usb_dwc2_ghwcfg1_reg_t;

typedef union {
	struct {
		uint32_t opmode               : 3;
		uint32_t arch                 : 2;
		uint32_t singlepoint          : 1;
		uint32_t hsphytype            : 2;
		uint32_t fsphytype            : 2;
		uint32_t numdevep             : 4;
		uint32_t numhostch            : 4;
		uint32_t periodchannelsupport : 1;
		uint32_t enabledynamicfifo    : 1;
		uint32_t mulprocintrpt        : 1;
		uint32_t reserved21           : 1;
		uint32_t nptxqdepth           : 2;
		uint32_t ptxqdepth            : 2;
		uint32_t tokenqdepth          : 5;
		uint32_t otgenableicusb       : 1;
	};
	uint32_t val;
} usb_dwc2_ghwcfg2_reg_t;

typedef union {
	struct {
		uint32_t xfersizewidth: 4;
		uint32_t pktsizewidth: 3;
		uint32_t otgen: 1;
		uint32_t i2cintsel: 1;
		uint32_t vndctlsupt: 1;
		uint32_t optfeature: 1;
		uint32_t rsttype: 1;
		uint32_t adpsupport: 1;
		uint32_t hsicmode: 1;
		uint32_t bcsupport: 1;
		uint32_t lpmmode: 1;
		uint32_t dfifodepth: 16;
	};
	uint32_t val;
} usb_dwc2_ghwcfg3_reg_t;

typedef union {
	struct {
		uint32_t numdev_perio_eps: 4;
		uint32_t partialpwrdn: 1;
		uint32_t ahbfreq: 1;
		uint32_t hibernation: 1;
		uint32_t extendedhibernation: 1;
		uint32_t reserved_8: 4;
		uint32_t acgsupt: 1;
		uint32_t enhancedlpmsupt: 1;
		uint32_t phydatawidth: 2;
		uint32_t numctleps: 4;
		uint32_t iddqfltr: 1;
		uint32_t vbusvalidfltr: 1;
		uint32_t avalidfltr: 1;
		uint32_t bvalidfltr: 1;
		uint32_t sessendfltr: 1;
		uint32_t dedfifomode: 1;
		uint32_t ineps: 4;
		uint32_t descdmaenabled: 1;
		uint32_t descdma: 1;
	};
	uint32_t val;
} usb_dwc2_ghwcfg4_reg_t;

typedef union {
    struct {
        uint32_t gdfifocfg: 16;
        uint32_t epinfobaseaddr: 16;
    };
    uint32_t val;
} usb_dwc2_gdfifocfg_reg_t;

typedef union {
	struct {
		uint32_t fslspclksel: 2;
		uint32_t fslssupp: 1;
		uint32_t reserved_3: 4;
		uint32_t ena32khzs: 1;
		uint32_t resvalid: 8;
		uint32_t reserved_16: 1;
		uint32_t reserved_17: 6;
		uint32_t descdma: 1;
		uint32_t frlisten: 2;
		uint32_t perschedena: 1;
		uint32_t reserved_27: 4;
		uint32_t modechtimen: 1;
	};
	uint32_t val;
} usb_dwc2_hcfg_reg_t;

typedef union {
	struct {
		uint32_t frint: 16;
		uint32_t hfirrldctrl: 1;
		uint32_t reserved_17: 15;
	};
	uint32_t val;
} usb_dwc2_hfir_reg_t;

typedef union {
	struct {
		uint32_t frnum: 16;
		uint32_t frrem: 16;
	};
	uint32_t val;
} usb_dwc2_hfnum_reg_t;

typedef union {
	struct {
		uint32_t ptxfspcavail: 16;
		uint32_t ptxqspcavail: 8;
		uint32_t ptxqtop: 8;
	};
	uint32_t val;
} usb_dwc2_hptxsts_reg_t;

typedef union {
	struct {
		uint32_t haint: 16;
		uint32_t reserved_16: 16;
	};
	uint32_t val;
} usb_dwc2_haint_reg_t;

typedef union {
	struct {
		uint32_t haintmsk: 16;
		uint32_t reserved_16: 16;
	};
	uint32_t val;
} usb_dwc2_haintmsk_reg_t;

typedef union {
	struct {
		uint32_t hflbaddr;
	};
	uint32_t val;
} usb_dwc2_hflbaddr_reg_t;

typedef union {
	struct {
		uint32_t prtconnsts: 1;
		uint32_t prtconndet: 1;
		uint32_t prtena: 1;
		uint32_t prtenchng: 1;
		uint32_t prtovrcurract: 1;
		uint32_t prtovrcurrchng: 1;
		uint32_t prtres: 1;
		uint32_t prtsusp: 1;
		uint32_t prtrst: 1;
		uint32_t reserved_9: 1;
		uint32_t prtlnsts: 2;
		uint32_t prtpwr: 1;
		uint32_t prttstctl: 4;
		uint32_t prtspd: 2;
		uint32_t reserved_19: 13;
	};
	uint32_t val;
} usb_dwc2_hprt_reg_t;

typedef union {
	struct {
		uint32_t mps: 11;
		uint32_t epnum: 4;
		uint32_t epdir: 1;
		uint32_t reserved_16: 1;
		uint32_t lspddev: 1;
		uint32_t eptype: 2;
		uint32_t ec: 2;
		uint32_t devaddr: 7;
		uint32_t oddfrm: 1;
		uint32_t chdis: 1;
		uint32_t chena: 1;
	};
	uint32_t val;
} usb_dwc2_hcchar_reg_t;

typedef union {
	struct {
		uint32_t prtaddr: 7;
		uint32_t hubaddr: 7;
		uint32_t xactpos: 2;
		uint32_t compsplt: 1;
		uint32_t reserved_17: 14;
		uint32_t spltena: 1;
	};
	uint32_t val;
} usb_dwc2_hcsplt_reg_t;

typedef union {
	struct {
		uint32_t xfercompl: 1;
		uint32_t chhltd: 1;
		uint32_t ahberr: 1;
		uint32_t stall: 1;
		uint32_t nack: 1;
		uint32_t ack: 1;
		uint32_t nyet: 1;
		uint32_t xacterr: 1;
		uint32_t bblerr: 1;
		uint32_t frmovrun: 1;
		uint32_t datatglerr: 1;
		uint32_t bnaintr: 1;
		uint32_t xcs_xact_err: 1;
		uint32_t desc_lst_rollintr: 1;
		uint32_t reserved_14: 18;
	};
	uint32_t val;
} usb_dwc2_hcint_reg_t;

typedef union {
	struct {
		uint32_t xfercomplmsk: 1;
		uint32_t chhltdmsk: 1;
		uint32_t ahberrmsk: 1;
		uint32_t reserved_3: 1;
		uint32_t reserved_4: 1;
		uint32_t reserved_5: 1;
		uint32_t reserved_6: 1;
		uint32_t reserved_7: 1;
		uint32_t reserved_8: 1;
		uint32_t reserved_9: 1;
		uint32_t reserved_10: 1;
		uint32_t bnaintrmsk: 1;
		uint32_t reserved_12: 1;
		uint32_t desc_lst_rollintrmsk: 1;
		uint32_t reserved_14: 18;
	};
	uint32_t val;
} usb_dwc2_hcintmsk_reg_t;

typedef union {
	struct {
		uint32_t xfersize: 19;
		uint32_t pktcnt: 10;
		uint32_t pid: 2;
		uint32_t dopng: 1;
	};
	uint32_t val;
} usb_dwc2_hctsiz_reg_t;

typedef union {
	struct {
		uint32_t dmaaddr;
	};
	uint32_t val;
} usb_dwc2_hcdma_reg_t;

typedef union {
	struct {
		uint32_t hcdmab;
	};
	uint32_t val;
} usb_dwc2_hcdmab_reg_t;

typedef struct {
	volatile uint32_t hcchar;
	volatile uint32_t hcsplt;
	volatile uint32_t hcint;
	volatile uint32_t hcintmsk;
	volatile uint32_t hctsiz;
	volatile uint32_t hcdma;
	volatile uint32_t reserved_0x18[1];
	volatile uint32_t hcdmab;
} usb_dwc2_host_chan_regs_t;

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_DRIVERS_USB_COMMON_USB_DWC2_REG */
