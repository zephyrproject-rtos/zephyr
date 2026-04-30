/*
 * Cadence USB3 Device Controller driver for Zephyr
 *
 * Copyright (C) 2026 Texas Instruments
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_USB_UDC_CDNS3_H_
#define ZEPHYR_DRIVERS_USB_UDC_CDNS3_H_

#include <zephyr/sys/device_mmio.h>
#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/usb/udc.h>
#include <zephyr/sys/sys_heap.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/drivers/pinctrl.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Indices matching the "device-speed" devicetree property values. */
enum {
	UDC_CDNS3_SPEED_IDX_FULL_SPEED = 1,
	UDC_CDNS3_SPEED_IDX_HIGH_SPEED = 2,
	UDC_CDNS3_SPEED_IDX_SUPER_SPEED = 3,
};

#define CDNS3_DEV_VER_V3 0x2450d

/* CDNS3 OTG/DRD Controller Registers */
struct udc_cdns3_otg_regs {
	uint8_t RESERVED[20];     /**< Reserved, offset: 0x0 - 0x13 */
	volatile uint32_t OTGSTS; /**< OTG Status, offset: 0x14 */
};

/* CDNS3 USB Device Controller Registers */
struct udc_cdns3_device_regs {
	volatile uint32_t USB_CONF;        /**< Global Configuration, offset: 0x0 */
	volatile uint32_t USB_STS;         /**< Global Status, offset: 0x4 */
	volatile uint32_t USB_CMD;         /**< Global Command, offset: 0x8 */
	volatile uint32_t USB_ITPN;        /**< ITP/SOF number, offset: 0xC */
	volatile uint32_t USB_LPM;         /**< LPM configuration, offset: 0x10 */
	volatile uint32_t USB_IEN;         /**< USB Interrupt Enable, offset: 0x14 */
	volatile uint32_t USB_ISTS;        /**< USB Interrupt Status, offset: 0x18 */
	volatile uint32_t EP_SEL;          /**< Endpoint Select, offset: 0x1C */
	volatile uint32_t EP_TRADDR;       /**< EP Transfer Ring Address, offset: 0x20 */
	volatile uint32_t EP_CFG;          /**< EP Configuration, offset: 0x24 */
	volatile uint32_t EP_CMD;          /**< EP Command, offset: 0x28 */
	volatile uint32_t EP_STS;          /**< EP Status, offset: 0x2C */
	volatile uint32_t EP_STS_SID;      /**< EP Status Stream ID, offset: 0x30 */
	volatile uint32_t EP_STS_EN;       /**< EP Status Enable, offset: 0x34 */
	volatile uint32_t DRBL;            /**< Doorbell, offset: 0x38 */
	volatile uint32_t EP_IEN;          /**< EP Interrupt Enable, offset: 0x3C */
	volatile uint32_t EP_ISTS;         /**< EP Interrupt Status, offset: 0x40 */
	volatile uint32_t USB_PWR;         /**< USB Power Management, offset: 0x44 */
	volatile uint32_t USB_CONF2;       /**< USB Configuration 2, offset: 0x48 */
	volatile uint32_t USB_CAP1;        /**< USB Capability 1, offset: 0x4C */
	volatile uint32_t USB_CAP2;        /**< USB Capability 2, offset: 0x50 */
	volatile uint32_t USB_CAP3;        /**< USB Capability 3, offset: 0x54 */
	volatile uint32_t USB_CAP4;        /**< USB Capability 4, offset: 0x58 */
	volatile uint32_t USB_CAP5;        /**< USB Capability 5, offset: 0x5C */
	volatile uint32_t USB_CAP6;        /**< USB Capability 6 (Version), offset: 0x60 */
	volatile uint32_t USB_CPKT1;       /**< Custom Packet 1, offset: 0x64 */
	volatile uint32_t USB_CPKT2;       /**< Custom Packet 2, offset: 0x68 */
	volatile uint32_t USB_CPKT3;       /**< Custom Packet 3, offset: 0x6C */
	volatile uint32_t EP_DMA_EXT_ADDR; /**< EP DMA Extended Address, offset: 0x70 */
	volatile uint32_t BUF_ADDR;        /**< Buffer Address, offset: 0x74 */
	volatile uint32_t BUF_DATA;        /**< Buffer Data, offset: 0x78 */
	volatile uint32_t BUF_CTRL;        /**< Buffer Control, offset: 0x7C */
	volatile uint32_t DTRANS;          /**< DMA Transfer, offset: 0x80 */
	volatile uint32_t TDL_FROM_TRB;    /**< TDL from TRB offset: 0x84 */
	volatile uint32_t TDL_BEH;         /**< TDL Behavior, offset: 0x88 */
	volatile uint32_t EP_TDL;          /**< EP TDL, offset: 0x8C */
	volatile uint32_t TDL_BEH2;        /**< TDL Behavior 2, offset: 0x90 */
	volatile uint32_t DMA_ADV_TD;      /**< DMA Advance TD configuration, offset: 0x94 */
};

/* CDNS3 Transfer Request Block (TRB) */
struct cdns3_trb {
	uint32_t buffer;  /* Data buffer pointer */
	uint32_t length;  /* Transfer length and status */
	uint32_t control; /* Control and type information */
} __packed;

/* OTGSTS - OTG Status Register bits */
#define CDNS3_OTGSTS_OTG_NRDY  BIT(11) /* OTG Not Ready */
#define CDNS3_OTGSTS_XHC_READY BIT(26) /* Host Controller Ready */
#define CDNS3_OTGSTS_DEV_READY BIT(27) /* Device Controller Ready */

/* Timing values */
#define CDNS3_OTG_READY_TIMEOUT_US (1000 * 1000) /* 1000ms */
#define CDNS3_DEV_READY_TIMEOUT_US (1000 * 1000) /* 1000ms */

/* USB_CONF - Global Configuration Register */
#define USB_CONF_DEVDS  BIT(15) /* Device Disable */
#define USB_CONF_DEVEN  BIT(14) /* Device Enable */
#define USB_CONF_DMULT  BIT(9)  /* DMA Multiple Mode */
#define USB_CONF_SWRST  BIT(5)  /* Software Reset */
#define USB_CONF_CFGSET BIT(0)  /* Configuration Set */
#define USB_CONF_CFGRST BIT(0)  /* Configuration Reset */

/* USB_STS - Global Status Register */
#define USB_STS_USBSSPD(p) FIELD_GET(GENMASK(6, 4), p)

/* Speed definitions for USB_STS_USBSSPD */
#define USB_STS_SPD_FS 2
#define USB_STS_SPD_HS 3
#define USB_STS_SPD_SS 4

/* USB_CMD - Global Command Register */
#define USB_CMD_FADDR(p) FIELD_PREP(GENMASK(7, 1), p)
#define USB_CMD_SET_ADDR BIT(0) /* Set Address */

/* USB_IEN/USB_ISTS - Interrupt Enable/Status Registers */
#define USB_ISTS_CFGRESI BIT(26) /* Configuration Reset */
#define USB_ISTS_L2EXTI  BIT(22) /* L2 Exit */
#define USB_ISTS_L1EXTI  BIT(21) /* L1 Exit */
#define USB_ISTS_L2ENTI  BIT(20) /* L2 Entry */
#define USB_ISTS_L1ENTI  BIT(19) /* L1 Entry */
#define USB_ISTS_U2RESI  BIT(18) /* USB Reset (HS/FS) */
#define USB_ISTS_U3EXTI  BIT(5)  /* U3 Exit */
#define USB_ISTS_U3ENTI  BIT(4)  /* U3 Entry */
#define USB_ISTS_UHRESI  BIT(3)  /* Hot Reset */
#define USB_ISTS_UWRESI  BIT(2)  /* Warm Reset */
#define USB_ISTS_DISI    BIT(1)  /* SS Disconnection */
#define USB_ISTS_CONI    BIT(0)  /* SS Connection */
#define USB_ISTS_INIT                                                                              \
	(USB_ISTS_U2RESI | USB_ISTS_CONI | USB_ISTS_DISI | USB_ISTS_UWRESI | USB_ISTS_UHRESI |     \
	 USB_ISTS_U3ENTI | USB_ISTS_U3EXTI | USB_ISTS_L1ENTI | USB_ISTS_L1EXTI | USB_ISTS_CFGRESI)

/* EP_CFG - Endpoint Configuration Register */
#define EP_CFG_MAXPKTSIZE(p) FIELD_PREP(GENMASK(26, 16), p)
#define EP_CFG_MAXBURST(p)   FIELD_PREP(GENMASK(11, 8), p)
#define EP_CFG_EPTYPE(p)     FIELD_PREP(GENMASK(2, 1), p)
#define EP_CFG_EPTYPE_CTRL   0
#define EP_CFG_EPTYPE_ISOC   1
#define EP_CFG_EPTYPE_BULK   2
#define EP_CFG_EPTYPE_INT    3
#define EP_CFG_ENABLE        BIT(0) /* Enable Endpoint */

/* EP_CMD - Endpoint Command Register */
#define EP_CMD_DRDY     BIT(6) /* Data Ready */
#define EP_CMD_REQ_CMPL BIT(5) /* Request Complete */
#define EP_CMD_ERDY     BIT(3) /* Send ERDY packet */
#define EP_CMD_SSTALL   BIT(1) /* Set Stall */
#define EP_CMD_CSTALL   BIT(0) /* Clear Stall */

/* EP_STS - Endpoint Status Register */
#define EP_STS_IOT     BIT(19) /* Interrupt on Transfer Complete */
#define EP_STS_TRBERR  BIT(7)  /* TRB Error */
#define EP_STS_DESCMIS BIT(4)  /* Descriptor Missing */
#define EP_STS_ISP     BIT(3)  /* Interrupt on Short Packet */
#define EP_STS_IOC     BIT(2)  /* Interrupt on Completion */
#define EP_STS_STALL   BIT(1)  /* Stall */
#define EP_STS_SETUP   BIT(0)  /* Setup Packet */

/* EP_STS_EN - Endpoint Status Enable Register */
#define EP_STS_EN_TRBERREN  BIT(7) /* TRB Error Enable */
#define EP_STS_EN_DESCMISEN BIT(4) /* Descriptor Missing Enable */
#define EP_STS_EN_SETUPEN   BIT(0) /* Setup Enable */

/* TRB Control field bits */
#define TRB_TYPE(p) FIELD_PREP(GENMASK(15, 10), p)
#define TRB_IOC     BIT(5) /* Interrupt on Completion */
#define TRB_CHAIN   BIT(4) /* Chain bit */
#define TRB_ISP     BIT(2) /* Interrupt on Short Packet */
#define TRB_TOGGLE  BIT(1) /* Toggle (Link TRB only) */
#define TRB_CYCLE   BIT(0) /* Cycle bit */

/* TRB Length field */
#define TRB_LEN_MASK GENMASK(16, 0) /* Length field */

/* TRB Types */
#define TRB_LINK   6
#define TRB_NORMAL 1

/* Utilities */
#define CDNS3_TO_IRQ_IDX(addr) (USB_EP_GET_IDX(addr) + ((addr & USB_EP_DIR_IN) ? 16 : 0))
#define CDNS3_FROM_IRQ_IDX(idx) ((idx < 16 ? USB_EP_DIR_OUT : USB_EP_DIR_IN) | (idx % 16))

/* Quirks-specific function pointers*/
struct udc_cdns3_quirks_ops {
	int (*preinit)(const struct device *const dev);
	int (*init)(const struct device *const dev);
	int (*enable)(const struct device *const dev);
	int (*disable)(const struct device *const dev);
	int (*shutdown)(const struct device *const dev);
};

/* Quirks wrapper - combines ops, config, and data */
struct udc_cdns3_quirks {
	const struct udc_cdns3_quirks_ops *ops; /* Function operations */
	const void *config;                     /* Quirk specific configuration */
	void *data;                             /* Quirk specific data */
};

/* Per-endpoint data structure */
struct udc_cdns3_ep_data {
	struct udc_ep_config cfg;
	const struct device *dev;
	struct cdns3_trb *trb_pool;
	struct net_buf **trb_buffers;
	uint16_t enqueue; /* Producer index */
	uint16_t dequeue; /* Consumer index */
	bool pcs;         /* Producer cycle state */
	bool ccs;         /* Consumer cycle state */
	uint16_t free_trbs;
	struct k_work work;
};

/* Driver configuration */
struct udc_cdns3_config {
	DEVICE_MMIO_NAMED_ROM(otg);
	DEVICE_MMIO_NAMED_ROM(device);

	void (*irq_enable_func)(const struct device *dev);
	void (*irq_disable_func)(const struct device *dev);
	uint32_t max_speed;
	const struct pinctrl_dev_config *pinctrl;
	uint8_t num_in_endpoints;
	uint8_t num_out_endpoints;
};

/* Driver private data */
struct udc_cdns3_data {
	DEVICE_MMIO_NAMED_RAM(otg);
	DEVICE_MMIO_NAMED_RAM(device);

	const struct device *dev;
	struct udc_cdns3_ep_data *in_ep_data;
	struct udc_cdns3_ep_data *out_ep_data;
	struct udc_ep_config in_ep0_cfg;
	struct udc_ep_config out_ep0_cfg;
	struct cdns3_trb ep0_trb;
	struct k_work_delayable irq_work;
	struct udc_cdns3_quirks *quirks;
	bool wait_for_setup;
};

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_DRIVERS_USB_UDC_CDNS3_H_ */
