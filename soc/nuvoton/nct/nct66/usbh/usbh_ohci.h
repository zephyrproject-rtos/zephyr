/*
 * URB OHCI HCD (Host Controller Driver) for USB.
 *
 * (C) Copyright 1999 Roman Weissgaerber <weissg@vienna.at>
 * (C) Copyright 2000 David Brownell <david-b@pacbell.net>
 *
 * usb-ohci.h
 */

#ifndef _USB_OHCI_H_
#define _USB_OHCI_H_

/*------------------------------------------------------------------------------------
 *
 *  USB Host Controller
 *
 *------------------------------------------------------------------------------------*/
#define OHCI_BASE           OHCI_BASE_ADDR
#define HC_REVISION         (OHCI_BASE+0x00)    // HcRevision - Revision Register
#define HC_CONTROL          (OHCI_BASE+0x04)    // CTL  - Control Register
#define HC_CMD_STATUS       (OHCI_BASE+0x08)    // HcCommandStatus - Command Status Register
#define HC_INT_STATUS       (OHCI_BASE+0x0C)    // HcInterruptStatus  - Interrupt Status Register
#define HC_INT_ENABLE       (OHCI_BASE+0x10)    // HcInterruptEnable - Interrupt Enable Register
#define HC_INT_DISABLE      (OHCI_BASE+0x14)    // HcInterruptDisable - Interrupt Disable Register
#define HC_HCCA             (OHCI_BASE+0x18)    // HcHCCA - Communication Area Register
#define HC_PERIOD_CURED     (OHCI_BASE+0x1C)    // HcPeriodCurrentED - Period Current ED Register
#define HC_CTRL_HEADED      (OHCI_BASE+0x20)    // HcControlHeadED - Control Head ED Register
#define HC_CTRL_CURED       (OHCI_BASE+0x24)    // HcControlCurrentED - Control Current ED Register
#define HC_BULK_HEADED      (OHCI_BASE+0x28)    // HcBulkHeadED - Bulk Head ED Register
#define HC_BULK_CURED       (OHCI_BASE+0x2C)    // HcBulkCurrentED - Bulk Current ED Register
#define HC_DONE_HEAD        (OHCI_BASE+0x30)    // HcBulkCurrentED - Done Head Register
#define HC_FM_INTERVAL      (OHCI_BASE+0x34)    // HcFmInterval - Frame Interval Register
#define HC_FM_REMAINING     (OHCI_BASE+0x38)    // HcFrameRemaining - Frame Remaining Register
#define HC_FM_NUMBER        (OHCI_BASE+0x3C)    // HcFmNumber - Frame Number Register
#define HC_PERIOD_START     (OHCI_BASE+0x40)    // HcPeriodicStart - Periodic Start Register
#define HC_LS_THRESHOLD     (OHCI_BASE+0x44)    // HcLSThreshold - Low Speed Threshold Register
#define HC_RH_DESCRIPTORA   (OHCI_BASE+0x48)    // HcRhDescriptorA - Root Hub Descriptor A Register
#define HC_RH_DESCRIPTORB   (OHCI_BASE+0x4C)    // HcRevision - Root Hub Descriptor B Register
#define HC_RH_STATUS        (OHCI_BASE+0x50)    // HcRhStatus - Root Hub Status Register
#define HC_RH_PORT_STATUS1  (OHCI_BASE+0x54)    // HcRevision - Root Hub Port Status [1]
#define HC_RH_PORT_STATUS2  (OHCI_BASE+0x58)    // HcRevision - Root Hub Port Status [2]
#define HC_RH_OP_MODE       (OHCI_BASE+0x204)


/* ED States */

#define ED_NEW          0x00
#define ED_UNLINK       0x01
#define ED_OPER         0x02
#define ED_DEL          0x04
#define ED_URB_DEL      0x08

/* usb_ohci_ed */
typedef struct ohci_ed_t {
    uint32_t  hwINFO;
    uint32_t  hwTailP;
    uint32_t  hwHeadP;
    uint32_t  hwNextED;

    struct  ohci_ed_t *ed_prev;
    uint8_t   int_period;
    uint8_t   int_branch;
    uint8_t   int_load;
    uint8_t   int_interval;
    uint8_t   state;
    uint8_t   type;
    uint16_t  last_iso;
    struct  ohci_ed_t *ed_rm_list;
} ED_T, *EDP_T;



/* TD info field */
#define TD_CC       0xF0000000
#define TD_CC_GET(td_p) ((td_p >>28) & 0x0F)
#define TD_CC_SET(td_p, cc) (td_p) = ((td_p) & 0x0FFFFFFF) | (((cc) & 0x0F) << 28)
#define TD_EC       0x0C000000
//#define TD_T        0x03000000
#define TD_T_DATA0  0x02000000
#define TD_T_DATA1  0x03000000
#define TD_T_TOGGLE 0x00000000
#define TD_R        0x00040000
#define TD_DI       0x00E00000
#define TD_DI_SET(X) (((X) & 0x07)<< 21)
#define TD_DP       0x00180000
#define TD_DP_SETUP 0x00000000
#define TD_DP_IN    0x00100000
#define TD_DP_OUT   0x00080000

#define TD_ISO      0x00010000
#define TD_DEL      0x00020000

/* CC Codes */
#define TD_CC_NOERROR      0x00
#define TD_CC_CRC          0x01
#define TD_CC_BITSTUFFING  0x02
#define TD_CC_DATATOGGLEM  0x03
#define TD_CC_STALL        0x04
#define TD_DEVNOTRESP      0x05
#define TD_PIDCHECKFAIL    0x06
#define TD_UNEXPECTEDPID   0x07
#define TD_DATAOVERRUN     0x08
#define TD_DATAUNDERRUN    0x09
#define TD_BUFFEROVERRUN   0x0C
#define TD_BUFFERUNDERRUN  0x0D
#define TD_NOTACCESSED     0x0E


#define MAXPSW             8

typedef struct ohci_td_t {
    uint32_t  hwINFO;
    uint32_t  hwCBP;            /* Current Buffer Pointer */
    uint32_t  hwNextTD;         /* Next TD Pointer */
    uint32_t  hwBE;             /* Memory Buffer End Pointer */
    uint32_t  hwPSW[4];
    int       is_iso_packet;
    int       index;
    ED_T      *ed;
    struct    ohci_td_t *next_dl_td;
    URB_T     *urb;
    uint32_t  dummy[3];         /* let TD_T to be 64 bytes for 32 bytes memory alignment */
} TD_T;


#define OHCI_ED_SKIP    (1 << 14)

/*
 * The HCCA (Host Controller Communications Area) is a 256 byte
 * structure defined in the OHCI spec. that the host controller is
 * told the base address of.  It must be 256-byte aligned.
 */

#define NUM_INTS 32     /* part of the OHCI standard */

typedef struct ohci_hcca {
    uint32_t   int_table[NUM_INTS];    /* Interrupt ED table */
    uint16_t   frame_no;               /* current frame number */
    uint16_t   pad1;                   /* set to 0 on each frame_no change */
    uint32_t   done_head;              /* info returned for an interrupt */
    //uint8_t    reserved_for_hc[116];
} OHCI_HCCA_T;


/*
 * Maximum number of root hub ports.
 */
#define MAX_ROOT_PORTS  2

/*
 * This is the structure of the OHCI controller's memory mapped I/O
 * region.  This is Memory Mapped I/O.  You must use the readl() and
 * writel() macros defined in asm/io.h to access these!!
 */
struct ohci_regs {
    /* control and status registers */
    uint32_t   HcRevision;
    uint32_t   CTL;
    uint32_t   HcCommandStatus;
    uint32_t   HcInterruptStatus;
    uint32_t   HcInterruptEnable;
    uint32_t   HcInterruptDisable;
    /* memory pointers */
    uint32_t   HcHCCA;
    uint32_t   HcPeriodCurrentED;
    uint32_t   HcControlHeadED;
    uint32_t   HcControlCurrentED;
    uint32_t   HcBulkHeadED;
    uint32_t   HcBulkCurrentED;
    uint32_t   HcDoneHead;
    /* frame counters */
    uint32_t   HcFmInterval;
    uint32_t   HcFrameRemaining;
    uint32_t   HcFmNumber;
    uint32_t   HcPeriodicStart;
    uint32_t   HcLSThreshold;
    /* Root hub ports */
    struct  ohci_roothub_regs {
        uint32_t   a;
        uint32_t   b;
        uint32_t   status;
        uint32_t   portstatus[MAX_ROOT_PORTS];
    }  roothub;
};
typedef struct ohci_regs OHCI_REGS_T;


/* OHCI CONTROL AND STATUS REGISTER MASKS */

/*
 * CTL (control) register masks
 */
#define OHCI_CTRL_CBSR  (3 << 0)        /* control/bulk service ratio */
#define OHCI_CTRL_PLE   (1 << 2)        /* periodic list enable */
#define OHCI_CTRL_IE    (1 << 3)        /* isochronous enable */
#define OHCI_CTRL_CLE   (1 << 4)        /* control list enable */
#define OHCI_CTRL_BLE   (1 << 5)        /* bulk list enable */
#define OHCI_CTRL_HCFS  (3 << 6)        /* host controller functional state */
#define OHCI_CTRL_IR    (1 << 8)        /* interrupt routing */
#define OHCI_CTRL_RWC   (1 << 9)        /* remote wakeup connected */
#define OHCI_CTRL_RWE   (1 << 10)       /* remote wakeup enable */

/* pre-shifted values for HCFS */
#define OHCI_USB_RESET   (0 << 6)
#define OHCI_USB_RESUME  (1 << 6)
#define OHCI_USB_OPER    (2 << 6)
#define OHCI_USB_SUSPEND (3 << 6)

/*
 * HcCommandStatus (cmdstatus) register masks
 */
#define OHCI_HCR        (1 << 0)        /* host controller reset */
#define OHCI_CLF        (1 << 1)        /* control list filled */
#define OHCI_BLF        (1 << 2)        /* bulk list filled */
#define OHCI_OCR        (1 << 3)        /* ownership change request */
#define OHCI_SOC        (3 << 16)       /* scheduling overrun count */

/*
 * masks used with interrupt registers:
 * HcInterruptStatus (intrstatus)
 * HcInterruptEnable (intrenable)
 * HcInterruptDisable (intrdisable)
 */
#define OHCI_INTR_SO    (1 << 0)        /* scheduling overrun */
#define OHCI_INTR_WDH   (1 << 1)        /* writeback of done_head */
#define OHCI_INTR_SF    (1 << 2)        /* start frame */
#define OHCI_INTR_RD    (1 << 3)        /* resume detect */
#define OHCI_INTR_UE    (1 << 4)        /* unrecoverable error */
#define OHCI_INTR_FNO   (1 << 5)        /* frame number overflow */
#define OHCI_INTR_RHSC  (1 << 6)        /* root hub status change */
#define OHCI_INTR_OC    (1 << 30)       /* ownership change */
#define OHCI_INTR_MIE   0x80000000      /* master interrupt enable */


/* Virtual Root HUB */
typedef struct virt_root_hub {
    int   devnum; /* Address of Root Hub endpoint */
    void  *urb;
    void  *int_addr;
    int   send;
    int   interval;
} VIRT_ROOT_HUB_T;



/* USB HUB CONSTANTS (not OHCI-specific; see hub.h) */

/* destination of request */
#define RH_INTERFACE                0x0100
#define RH_ENDPOINT                 0x0200
#define RH_OTHER                    0x0300
#define RH_CLASS                    0x2000
#define RH_VENDOR                   0x4000

/* Requests: bRequest << 8 | bmRequestType */
#define RH_GET_STATUS               0x8000
#define RH_CLEAR_FEATURE            0x0001
#define RH_SET_FEATURE              0x0003
#define RH_SET_ADDRESS              0x0005
#define RH_GET_DESCRIPTOR           0x8006
#define RH_SET_DESCRIPTOR           0x0007
#define RH_GET_CONFIGURATION        0x8008
#define RH_SET_CONFIGURATION        0x0009
#define RH_GET_STATE                0x8002
#define RH_GET_INTERFACE            0x800A
#define RH_SET_INTERFACE            0x000B
#define RH_SYNC_FRAME               0x800C
/* Our Vendor Specific Request */
#define RH_SET_EP                   0x0020


/* Hub port features */
#define RH_PORT_CONNECTION          0x00
#define RH_PORT_ENABLE              0x01
#define RH_PORT_SUSPEND             0x02
#define RH_PORT_OVER_CURRENT        0x03
#define RH_PORT_RESET               0x04
#define RH_PORT_POWER               0x08
#define RH_PORT_LOW_SPEED           0x09

#define RH_C_PORT_CONNECTION        0x10
#define RH_C_PORT_ENABLE            0x11
#define RH_C_PORT_SUSPEND           0x12
#define RH_C_PORT_OVER_CURRENT      0x13
#define RH_C_PORT_RESET             0x14

/* Hub features */
#define RH_C_HUB_LOCAL_POWER        0x00
#define RH_C_HUB_OVER_CURRENT       0x01

#define RH_DEVICE_REMOTE_WAKEUP     0x00
#define RH_ENDPOINT_STALL           0x01

#define RH_ACK                      0x01
#define RH_REQ_ERR                  -1
#define RH_NACK                     0x00


/* OHCI ROOT HUB REGISTER MASKS */

/* roothub.portstatus [i] bits */
#define RH_PS_CCS           0x00000001          /* current connect status */
#define RH_PS_PES           0x00000002          /* port enable status*/
#define RH_PS_PSS           0x00000004          /* port suspend status */
#define RH_PS_POCI          0x00000008          /* port over current indicator */
#define RH_PS_PRS           0x00000010          /* port reset status */
#define RH_PS_PPS           0x00000100          /* port power status */
#define RH_PS_LSDA          0x00000200          /* low speed device attached */
#define RH_PS_CSC           0x00010000          /* connect status change */
#define RH_PS_PESC          0x00020000          /* port enable status change */
#define RH_PS_PSSC          0x00040000          /* port suspend status change */
#define RH_PS_OCIC          0x00080000          /* over current indicator change */
#define RH_PS_PRSC          0x00100000          /* port reset status change */

/* roothub.status bits */
#define RH_HS_LPS           0x00000001          /* local power status */
#define RH_HS_OCI           0x00000002          /* over current indicator */
#define RH_HS_DRWE          0x00008000          /* device remote wakeup enable */
#define RH_HS_LPSC          0x00010000          /* local power status change */
#define RH_HS_OCIC          0x00020000          /* over current indicator change */
#define RH_HS_CRWE          0x80000000          /* clear remote wakeup enable */

/* roothub.b masks */
#define RH_B_DR             0x0000FFFF          /* device removable flags */
#define RH_B_PPCM           0xFFFF0000          /* port power control mask */

/* roothub.a masks */
#define RH_A_NDP            (0xFF)              /* number of downstream ports */
#define RH_A_PSM            (1 << 8)            /* power switching mode */
#define RH_A_NPS            (1 << 9)            /* no power switching */
#define RH_A_DT             (1 << 10)           /* device type (mbz) */
#define RH_A_OCPM           (1 << 11)           /* over current protection mode */
#define RH_A_NOCP           (1 << 12)           /* no over current protection */
#define RH_A_POTPGT         (0xFF000000)        /* power on to power good time */

#undef min
#define min(a,b) (((a)<(b))?(a):(b))


/* urb */

#define URB_DEL 1

/*
 * This is the full ohci controller description
 *
 * Note how the "proper" USB information is just
 * a subset of what the full implementation needs. (Linus)
 */

typedef struct ohci {
    OHCI_HCCA_T  *hcca;                /* hcca */
    /* Modified by YCHuang, from hcca to *hcca */
    /* We must guarantee the HCCA is located on 256 */
    /* bytes boundary. */
    int     disabled;                  /* e.g. got a UE, we're hung */
    int     resume_count;              /* atomic_t, defending against multiple resumes */

    //OHCI_REGS_T  *regs;                /* OHCI controller's memory */

    int     ohci_int_load[32];         /* load of the 32 Interrupt Chains (for load balancing)*/
    ED_T    *ed_rm_list[2];            /* lists of all endpoints to be removed */
    ED_T    *ed_bulktail;              /* last endpoint of bulk list */
    ED_T    *ed_controltail;           /* last endpoint of control list */
    ED_T    *ed_isotail;               /* last endpoint of iso list */
    int     intrstatus;
    uint32_t  hc_control;                /* copy of the hc control reg */
    USB_BUS_T  *bus;
    USB_DEV_T  *dev[DEV_MAX_NUM];
    VIRT_ROOT_HUB_T  rh;
} OHCI_T;


#define NUM_TDS 0               /* num of preallocated transfer descriptors */
#define NUM_EDS 32              /* num of preallocated endpoint descriptors */

typedef struct ohci_device {
    //ED_T    ed[NUM_EDS];
    EDP_T   edp[NUM_EDS];
    int     ed_cnt;
} OHCI_DEVICE_T;


/* #define ohci_to_usb(ohci)    ((ohci)->usb) */
#define usb_to_ohci(usb)        ((OHCI_DEVICE_T *)(usb)->hcpriv)

#endif  /* _USB_OHCI_H_ */

