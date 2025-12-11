/*
 * Copyright (c) 2023 Linaro Limited
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file  udc_stm32.c
 * @brief STM32 USB device controller (UDC) driver
 */

#include <soc.h>
#include <stm32_ll_bus.h>
#include <stm32_ll_pwr.h>
#include <stm32_ll_rcc.h>
#include <stm32_ll_system.h>
#include <string.h>
#include <zephyr/irq.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/drivers/clock_control/stm32_clock_control.h>
#include <zephyr/sys/util.h>

#include "udc_common.h"

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(udc_stm32, CONFIG_UDC_DRIVER_LOG_LEVEL);

/*
 * The STM32 HAL does not provide PCD_SPEED_HIGH and PCD_SPEED_HIGH_IN_FULL
 * on series which lack HS-capable hardware. Provide dummy definitions for
 * these series to remove checks elsewhere in the driver. The exact value
 * of the dummy definitions in insignificant, as long as they are not equal
 * to PCD_SPEED_FULL (which is always provided).
 */
#if !defined(PCD_SPEED_HIGH)
#define PCD_SPEED_HIGH		(PCD_SPEED_FULL + 1)
#define PCD_SPEED_HIGH_IN_FULL	(PCD_SPEED_HIGH + 1)
#endif

#if DT_HAS_COMPAT_STATUS_OKAY(st_stm32_otghs)
#define DT_DRV_COMPAT st_stm32_otghs
#define UDC_STM32_IRQ_NAME     otghs
#elif DT_HAS_COMPAT_STATUS_OKAY(st_stm32_otgfs)
#define DT_DRV_COMPAT st_stm32_otgfs
#define UDC_STM32_IRQ_NAME     otgfs
#elif DT_HAS_COMPAT_STATUS_OKAY(st_stm32_usb)
#define DT_DRV_COMPAT st_stm32_usb
#define UDC_STM32_IRQ_NAME     usb
#endif

/* Shorthand to obtain PHY node for an instance */
#define UDC_STM32_PHY(usb_node)			DT_PROP_BY_IDX(usb_node, phys, 0)

/* Evaluates to 1 if PHY of 'usb_node' is an embedded HS PHY, 0 otherwise */
#define UDC_STM32_PHY_HAS_EMBEDDED_HS_COMPAT(usb_node)					\
	UTIL_OR(DT_NODE_HAS_COMPAT(UDC_STM32_PHY(usb_node), st_stm32_usbphyc),		\
		DT_NODE_HAS_COMPAT(UDC_STM32_PHY(usb_node), st_stm32u5_otghs_phy))

/* Evaluates to 1 if 'usb_node' is HS-capable, 0 otherwise. */
#define UDC_STM32_NODE_IS_HS_CAPABLE(usb_node)	DT_NODE_HAS_COMPAT(usb_node, st_stm32_otghs)

/*
 * Returns the 'PCD_PHY_Module' value for 'usb_node', which
 * corresponds to the PHY interface that should be used by
 * the USB controller.
 *
 * This value may be one of:
 *    - PCD_PHY_EMBEDDED: embedded Full-Speed PHY
 *    - PCD_PHY_UTMI: embedded High-Speed PHY over UTMI+
 *    - PCD_PHY_ULPI: external High-Speed PHY over ULPI
 *
 * The correct value is always PCD_PHY_EMBEDDED for nodes
 * that are not HS-capable: these instances are always
 * hardwired to an embedded FS PHY.
 *
 * The correct value for HS-capable nodes is determined from
 * the 'compatible' list on the PHY DT node, which is referenced
 * by the USB controller's 'phys' property:
 *  - External HS PHYs must have 'usb-ulpi-phy' compatible
 *  - Embedded HS PHYs must have one of the ST-specific compatibles
 *  - Others ('usb-nop-xceiv') are assumed to be embedded FS PHYs
 */
#define UDC_STM32_NODE_PHY_ITFACE(usb_node)					\
	COND_CODE_0(UDC_STM32_NODE_IS_HS_CAPABLE(usb_node),			\
		(PCD_PHY_EMBEDDED),						\
	(COND_CODE_1(DT_NODE_HAS_COMPAT(UDC_STM32_PHY(usb_node), usb_ulpi_phy),	\
		(PCD_PHY_ULPI),							\
	(COND_CODE_1(UDC_STM32_PHY_HAS_EMBEDDED_HS_COMPAT(usb_node),		\
		(PCD_PHY_UTMI),							\
		(PCD_PHY_EMBEDDED))						\
	))))

/*
 * Evaluates to 1 if 'usb_node' uses an embedded FS PHY or has
 * the 'maximum-speed' property set to 'full-speed', 0 otherwise.
 *
 * N.B.: enum index 1 corresponds to 'full-speed'
 */
#define UDC_STM32_NODE_LIMITED_TO_FS(usb_node)					\
	UTIL_OR(IS_EQ(UDC_STM32_NODE_PHY_ITFACE(usb_node), PHY_PCD_EMBEDDED),	\
		UTIL_AND(DT_NODE_HAS_PROP(usb_node, maximum_speed),		\
			IS_EQ(DT_ENUM_IDX(usb_node, maximum_speed), 1)))

/*
 * Returns the 'PCD_Speed' value for 'usb_node', which indicates
 * the operation mode in which controller should be configured.
 *
 * The 'maximum-speed' property is taken into account but only when
 * present on an HS-capable instance to force 'full-speed' mode; all
 * other uses are invalid and silently ignored.
 */
#define UDC_STM32_NODE_SPEED(usb_node)					\
	COND_CODE_0(UDC_STM32_NODE_IS_HS_CAPABLE(usb_node),		\
		(PCD_SPEED_FULL),					\
	(COND_CODE_1(UDC_STM32_NODE_LIMITED_TO_FS(usb_node),		\
		(PCD_SPEED_HIGH_IN_FULL),				\
		(PCD_SPEED_HIGH))))

/*
 * Returns max packet size allowed for endpoints of 'usb_node'
 *
 * Hardware always supports the maximal value allowed
 * by the USB Specification at a given operating speed:
 * 1024 bytes in High-Speed, 1023 bytes in Full-Speed
 */
#define UDC_STM32_NODE_EP_MPS(node_id)					\
	((UDC_STM32_NODE_SPEED(node_id) == PCD_SPEED_HIGH) ? 1024U : 1023U)

#if DT_HAS_COMPAT_STATUS_OKAY(st_stm32n6_otghs)
#define USB_USBPHYC_CR_FSEL_24MHZ        USB_USBPHYC_CR_FSEL_1
#endif

#if DT_HAS_COMPAT_STATUS_OKAY(st_stm32u5_otghs_phy)
static const int syscfg_otg_hs_phy_clk[] = {
	SYSCFG_OTG_HS_PHY_CLK_SELECT_1,	/* 16Mhz   */
	SYSCFG_OTG_HS_PHY_CLK_SELECT_2,	/* 19.2Mhz */
	SYSCFG_OTG_HS_PHY_CLK_SELECT_3,	/* 20Mhz   */
	SYSCFG_OTG_HS_PHY_CLK_SELECT_4,	/* 24Mhz   */
	SYSCFG_OTG_HS_PHY_CLK_SELECT_5,	/* 26Mhz   */
	SYSCFG_OTG_HS_PHY_CLK_SELECT_6,	/* 32Mhz   */
};
#endif

/*
 * Hardcode EP0 max packet size (bMaxPacketSize0) to 64,
 * which is the maximum allowed by the USB Specification
 * and supported by all STM32 USB controllers.
 */
#define UDC_STM32_EP0_MAX_PACKET_SIZE	64U

enum udc_stm32_msg_type {
	UDC_STM32_MSG_SETUP,
	UDC_STM32_MSG_DATA_OUT,
	UDC_STM32_MSG_DATA_IN,
};

struct udc_stm32_msg {
	uint8_t type;
	uint8_t ep;
	uint16_t rx_count;
};

struct udc_stm32_data  {
	PCD_HandleTypeDef pcd;
	const struct device *dev;
	uint32_t occupied_mem;
	/* wLength of SETUP packet for s-out-status */
	uint32_t ep0_out_wlength;
	struct k_thread thread_data;
	struct k_msgq msgq_data;
	char msgq_buf[CONFIG_UDC_STM32_MAX_QMESSAGES * sizeof(struct udc_stm32_msg)];
};

struct udc_stm32_config {
	/* Controller MMIO base address */
	void *base;
	/* # of bidirectional endpoints supported */
	uint32_t num_endpoints;
	/* USB SRAM size (in bytes) */
	uint32_t dram_size;
	/* IRQ_CONNECT() per-instance wrapper */
	void (*irq_connect)(void);
	/* Global USB interrupt IRQn */
	uint32_t irqn;
	/*
	 * Clock configuration from DTS
	 *
	 * Note that this actually points to a const
	 * struct stm32_pclken but dropping the const
	 * qualifier here allows calling Clock Control
	 * without cast to clock_control_subsys_t.
	 */
	struct stm32_pclken *pclken;
	/* Pinctrl configuration from DTS */
	const struct pinctrl_dev_config *pinctrl;
	/* Disconnect GPIO information (if applicable) */
	const struct gpio_dt_spec disconnect_gpio;
	/* ULPI reset GPIO information (if applicable) */
	const struct gpio_dt_spec ulpi_reset_gpio;
	/* PHY selected for use by instance */
	uint32_t selected_phy;
	/* Speed selected for use by instance */
	uint32_t selected_speed;
	/* EP configurations */
	struct udc_ep_config *in_eps;
	struct udc_ep_config *out_eps;
	/* Worker thread stack info */
	k_thread_stack_t *thread_stack;
	size_t thread_stack_size;
	/* Maximal packet size allowed for endpoints */
	uint16_t ep_mps;
	/* Number of entries in `pclken` */
	uint8_t num_clocks;
};

static int udc_stm32_clock_enable(const struct device *);
static int udc_stm32_clock_disable(const struct device *);

static void udc_stm32_lock(const struct device *dev)
{
	udc_lock_internal(dev, K_FOREVER);
}

static void udc_stm32_unlock(const struct device *dev)
{
	udc_unlock_internal(dev);
}

#define hpcd2data(hpcd) CONTAINER_OF(hpcd, struct udc_stm32_data, pcd);

void HAL_PCD_ResetCallback(PCD_HandleTypeDef *hpcd)
{
	struct udc_stm32_data *priv = hpcd2data(hpcd);
	const struct device *dev = priv->dev;
	struct udc_ep_config *ep_cfg;
	HAL_StatusTypeDef __maybe_unused status;

	/* Re-Enable control endpoints */
	ep_cfg = udc_get_ep_cfg(dev, USB_CONTROL_EP_OUT);
	if (ep_cfg != NULL && ep_cfg->stat.enabled) {
		status = HAL_PCD_EP_Open(&priv->pcd, USB_CONTROL_EP_OUT,
					 UDC_STM32_EP0_MAX_PACKET_SIZE,
					 EP_TYPE_CTRL);
		__ASSERT_NO_MSG(status == HAL_OK);
	}

	ep_cfg = udc_get_ep_cfg(dev, USB_CONTROL_EP_IN);
	if (ep_cfg != NULL && ep_cfg->stat.enabled) {
		status = HAL_PCD_EP_Open(&priv->pcd, USB_CONTROL_EP_IN,
					 UDC_STM32_EP0_MAX_PACKET_SIZE,
					 EP_TYPE_CTRL);
		__ASSERT_NO_MSG(status == HAL_OK);
	}

	udc_set_suspended(dev, false);
	udc_submit_event(priv->dev, UDC_EVT_RESET, 0);
}

void HAL_PCD_ConnectCallback(PCD_HandleTypeDef *hpcd)
{
	struct udc_stm32_data *priv = hpcd2data(hpcd);

	udc_submit_event(priv->dev, UDC_EVT_VBUS_READY, 0);
}

void HAL_PCD_DisconnectCallback(PCD_HandleTypeDef *hpcd)
{
	struct udc_stm32_data *priv = hpcd2data(hpcd);

	udc_submit_event(priv->dev, UDC_EVT_VBUS_REMOVED, 0);
}

void HAL_PCD_SuspendCallback(PCD_HandleTypeDef *hpcd)
{
	struct udc_stm32_data *priv = hpcd2data(hpcd);

	udc_set_suspended(priv->dev, true);
	udc_submit_event(priv->dev, UDC_EVT_SUSPEND, 0);
}

void HAL_PCD_ResumeCallback(PCD_HandleTypeDef *hpcd)
{
	struct udc_stm32_data *priv = hpcd2data(hpcd);

	udc_set_suspended(priv->dev, false);
	udc_submit_event(priv->dev, UDC_EVT_RESUME, 0);
}

void HAL_PCD_SetupStageCallback(PCD_HandleTypeDef *hpcd)
{
	struct udc_stm32_data *priv = hpcd2data(hpcd);
	struct udc_stm32_msg msg = {.type = UDC_STM32_MSG_SETUP};
	int err;

	err = k_msgq_put(&priv->msgq_data, &msg, K_NO_WAIT);

	if (err < 0) {
		LOG_ERR("UDC Message queue overrun");
	}
}

void HAL_PCD_SOFCallback(PCD_HandleTypeDef *hpcd)
{
	struct udc_stm32_data *priv = hpcd2data(hpcd);

	udc_submit_sof_event(priv->dev);
}

/*
 * Prepare OUT EP0 for reception.
 *
 * @param dev		USB controller
 * @param length	wLength from SETUP packet for s-out-status
 *                      0 for s-in-status ZLP
 */
static int udc_stm32_prep_out_ep0_rx(const struct device *dev, const size_t length)
{
	struct udc_stm32_data *priv = udc_get_private(dev);
	struct udc_ep_config *ep_cfg = udc_get_ep_cfg(dev, USB_CONTROL_EP_OUT);
	struct net_buf *buf;
	uint32_t buf_size;

	udc_ep_set_busy(ep_cfg, true);

	/*
	 * Make sure OUT EP0 can receive bMaxPacketSize0 bytes
	 * from each Data packet by rounding up allocation size
	 * even if "device behaviour is undefined if the host
	 * should send more data than specified in wLength"
	 * according to the USB Specification.
	 *
	 * Note that ROUND_UP() will return 0 for ZLP.
	 */
	buf_size = ROUND_UP(length, UDC_STM32_EP0_MAX_PACKET_SIZE);

	buf = udc_ctrl_alloc(dev, USB_CONTROL_EP_OUT, buf_size);
	if (buf == NULL) {
		return -ENOMEM;
	}

	k_fifo_put(&ep_cfg->fifo, buf);

	/*
	 * Keep track of how much data we're expecting from
	 * host so we know when the transfer is complete.
	 * Unlike other endpoints, this bookkeeping isn't
	 * done by the HAL for OUT EP0.
	 */
	priv->ep0_out_wlength = length;

	/* Don't try to receive more than bMaxPacketSize0 */
	if (HAL_PCD_EP_Receive(&priv->pcd, ep_cfg->addr, net_buf_tail(buf),
			       UDC_STM32_EP0_MAX_PACKET_SIZE) != HAL_OK) {
		return -EIO;
	}

	return 0;
}

static void udc_stm32_flush_tx_fifo(const struct device *dev)
{
	struct udc_stm32_data *priv = udc_get_private(dev);
	struct udc_ep_config *ep_cfg = udc_get_ep_cfg(dev, USB_CONTROL_EP_OUT);
	HAL_StatusTypeDef __maybe_unused status;

	status = HAL_PCD_EP_Receive(&priv->pcd, ep_cfg->addr, NULL, 0);
	__ASSERT_NO_MSG(status == HAL_OK);
}

static int udc_stm32_tx(const struct device *dev, struct udc_ep_config *ep_cfg,
			struct net_buf *buf)
{
	struct udc_stm32_data *priv = udc_get_private(dev);
	HAL_StatusTypeDef status;
	uint8_t *data;
	uint32_t len;

	LOG_DBG("TX ep 0x%02x len %u", ep_cfg->addr, buf->len);

	if (udc_ep_is_busy(ep_cfg)) {
		return 0;
	}

	data = buf->data;
	len = buf->len;

	if (ep_cfg->addr == USB_CONTROL_EP_IN) {
		len = MIN(UDC_STM32_EP0_MAX_PACKET_SIZE, buf->len);
	}

	buf->data += len;
	buf->len -= len;

	status = HAL_PCD_EP_Transmit(&priv->pcd, ep_cfg->addr, data, len);
	if (status != HAL_OK) {
		LOG_ERR("HAL_PCD_EP_Transmit failed(0x%02x), %d", ep_cfg->addr, (int)status);
		return -EIO;
	}

	udc_ep_set_busy(ep_cfg, true);

	if (ep_cfg->addr == USB_CONTROL_EP_IN && len > 0U) {
		/* Wait for an empty package from the host.
		 * This also flushes the TX FIFO to the host.
		 */
		if (DT_HAS_COMPAT_STATUS_OKAY(st_stm32_usb)) {
			udc_stm32_flush_tx_fifo(dev);
		} else {
			udc_stm32_prep_out_ep0_rx(dev, 0);
		}
	}

	return 0;
}

static int udc_stm32_rx(const struct device *dev, struct udc_ep_config *ep_cfg,
			struct net_buf *buf)
{
	struct udc_stm32_data *priv = udc_get_private(dev);
	HAL_StatusTypeDef status;

	/* OUT EP0 requires special logic! */
	__ASSERT_NO_MSG(ep_cfg->addr != USB_CONTROL_EP_OUT);

	LOG_DBG("RX ep 0x%02x len %u", ep_cfg->addr, buf->size);

	if (udc_ep_is_busy(ep_cfg)) {
		return 0;
	}

	status = HAL_PCD_EP_Receive(&priv->pcd, ep_cfg->addr, buf->data, buf->size);
	if (status != HAL_OK) {
		LOG_ERR("HAL_PCD_EP_Receive failed(0x%02x), %d", ep_cfg->addr, (int)status);
		return -EIO;
	}

	udc_ep_set_busy(ep_cfg, true);

	return 0;
}

void HAL_PCD_DataOutStageCallback(PCD_HandleTypeDef *hpcd, uint8_t epnum)
{
	uint32_t rx_count = HAL_PCD_EP_GetRxCount(hpcd, epnum);
	struct udc_stm32_data *priv = hpcd2data(hpcd);
	struct udc_stm32_msg msg = {
		.type = UDC_STM32_MSG_DATA_OUT,
		.ep = epnum,
		.rx_count = rx_count,
	};
	int err;

	err = k_msgq_put(&priv->msgq_data, &msg, K_NO_WAIT);
	if (err != 0) {
		LOG_ERR("UDC Message queue overrun");
	}
}

void HAL_PCD_DataInStageCallback(PCD_HandleTypeDef *hpcd, uint8_t epnum)
{
	struct udc_stm32_data *priv = hpcd2data(hpcd);
	struct udc_stm32_msg msg = {
		.type = UDC_STM32_MSG_DATA_IN,
		.ep = epnum,
	};
	int err;

	err = k_msgq_put(&priv->msgq_data, &msg, K_NO_WAIT);
	if (err != 0) {
		LOG_ERR("UDC Message queue overrun");
	}
}

static void handle_msg_data_out(struct udc_stm32_data *priv, uint8_t epnum, uint16_t rx_count)
{
	const struct device *dev = priv->dev;
	struct udc_ep_config *ep_cfg;
	uint8_t ep = epnum | USB_EP_DIR_OUT;
	struct net_buf *buf;

	LOG_DBG("DataOut ep 0x%02x",  ep);

	ep_cfg = udc_get_ep_cfg(dev, ep);

	buf = udc_buf_peek(ep_cfg);
	if (unlikely(buf == NULL)) {
		LOG_ERR("ep 0x%02x queue is empty", ep);
		udc_ep_set_busy(ep_cfg, false);
		return;
	}

	/* HAL copies data - we just need to update bookkeeping */
	net_buf_add(buf, rx_count);

	if (ep == USB_CONTROL_EP_OUT) {
		/*
		 * OUT EP0 is used for two purposes:
		 *  - receive 'out' Data packets during s-(out)-status
		 *  - receive Status OUT ZLP during s-in-(status)
		 */
		if (udc_ctrl_stage_is_status_out(dev)) {
			/* s-in-status completed */
			__ASSERT_NO_MSG(rx_count == 0);
			udc_ctrl_update_stage(dev, buf);
			udc_ctrl_submit_status(dev, buf);
		} else {
			/* Verify that host did not send more data than it promised */
			__ASSERT(buf->len <= priv->ep0_out_wlength,
				 "Received more data from Host than expected!");

			/* Check if the data stage is complete */
			if (buf->len < priv->ep0_out_wlength) {
				HAL_StatusTypeDef __maybe_unused status;

				/* Not yet - prepare to receive more data and wait */
				status = HAL_PCD_EP_Receive(&priv->pcd, ep_cfg->addr,
							    net_buf_tail(buf),
							    UDC_STM32_EP0_MAX_PACKET_SIZE);
				__ASSERT_NO_MSG(status == HAL_OK);
				return;
			} /* else: buf->len == priv->ep0_out_wlength */

			/*
			 * Data stage is complete: update to next step
			 * which should be Status IN, then submit the
			 * Setup+Data phase buffers to UDC stack and
			 * let it handle the next stage.
			 */
			udc_ctrl_update_stage(dev, buf);
			__ASSERT_NO_MSG(udc_ctrl_stage_is_status_in(dev));
			udc_ctrl_submit_s_out_status(dev, buf);
		}
	} else {
		udc_submit_ep_event(dev, buf, 0);
	}

	/* Buffer was filled and submitted - remove it from queue */
	(void)udc_buf_get(ep_cfg);

	/* Endpoint is no longer busy */
	udc_ep_set_busy(ep_cfg, false);

	/* Prepare next transfer for EP if its queue is not empty */
	buf = udc_buf_peek(ep_cfg);
	if (buf != NULL) {
		/*
		 * Only the driver is allowed to queue transfers on OUT EP0,
		 * and it should only be doing so once per Control transfer.
		 * If it has a queued transfer, something must be wrong.
		 */
		__ASSERT(ep_cfg->addr != USB_CONTROL_EP_OUT,
			 "OUT EP0 should never have pending transfers!");

		udc_stm32_rx(dev, ep_cfg, buf);
	}
}

static void handle_msg_data_in(struct udc_stm32_data *priv, uint8_t epnum)
{
	const struct device *dev = priv->dev;
	struct udc_ep_config *ep_cfg;
	uint8_t ep = epnum | USB_EP_DIR_IN;
	struct net_buf *buf;
	HAL_StatusTypeDef status;

	LOG_DBG("DataIn ep 0x%02x",  ep);

	ep_cfg = udc_get_ep_cfg(dev, ep);
	udc_ep_set_busy(ep_cfg, false);

	buf = udc_buf_peek(ep_cfg);
	if (unlikely(buf == NULL)) {
		return;
	}

	if (ep == USB_CONTROL_EP_IN && buf->len > 0U) {
		uint32_t len = MIN(UDC_STM32_EP0_MAX_PACKET_SIZE, buf->len);

		status = HAL_PCD_EP_Transmit(&priv->pcd, ep, buf->data, len);
		if (status != HAL_OK) {
			LOG_ERR("HAL_PCD_EP_Transmit failed: %d", status);
			__ASSERT_NO_MSG(0);
			return;
		}

		buf->len -= len;
		buf->data += len;

		return;
	}

	if (udc_ep_buf_has_zlp(buf)) {
		udc_ep_buf_clear_zlp(buf);
		status = HAL_PCD_EP_Transmit(&priv->pcd, ep, buf->data, 0);
		if (status != HAL_OK) {
			LOG_ERR("HAL_PCD_EP_Transmit failed: %d", status);
			__ASSERT_NO_MSG(0);
		}

		return;
	}

	udc_buf_get(ep_cfg);

	if (ep == USB_CONTROL_EP_IN) {
		if (udc_ctrl_stage_is_status_in(dev) ||
		    udc_ctrl_stage_is_no_data(dev)) {
			/* Status stage finished, notify upper layer */
			udc_ctrl_submit_status(dev, buf);
		}

		/* Update to next stage of control transfer */
		udc_ctrl_update_stage(dev, buf);

		if (udc_ctrl_stage_is_status_out(dev)) {
			/*
			 * IN transfer finished, release buffer,
			 * control OUT buffer should be already fed.
			 */
			net_buf_unref(buf);
		}

		return;
	}

	udc_submit_ep_event(dev, buf, 0);

	buf = udc_buf_peek(ep_cfg);
	if (buf != NULL) {
		udc_stm32_tx(dev, ep_cfg, buf);
	}
}

static void handle_msg_setup(struct udc_stm32_data *priv)
{
	struct usb_setup_packet *setup = (void *)priv->pcd.Setup;
	const struct device *dev = priv->dev;
	struct net_buf *buf;
	int err;

	/* Drop all transfers in control endpoints queue upon new SETUP */
	buf = udc_buf_get_all(udc_get_ep_cfg(dev, USB_CONTROL_EP_OUT));
	if (buf != NULL) {
		net_buf_unref(buf);
	}

	buf = udc_buf_get_all(udc_get_ep_cfg(dev, USB_CONTROL_EP_IN));
	if (buf != NULL) {
		net_buf_unref(buf);
	}

	buf = udc_ctrl_alloc(dev, USB_CONTROL_EP_OUT, sizeof(struct usb_setup_packet));
	if (buf == NULL) {
		LOG_ERR("Failed to allocate for setup");
		return;
	}

	udc_ep_buf_set_setup(buf);
	net_buf_add_mem(buf, setup, sizeof(struct usb_setup_packet));

	udc_ctrl_update_stage(dev, buf);

	if (udc_ctrl_stage_is_data_out(dev)) {
		/*  Allocate and feed buffer for data OUT stage */
		err = udc_stm32_prep_out_ep0_rx(dev, udc_data_stage_length(buf));
		if (err == -ENOMEM) {
			udc_submit_ep_event(dev, buf, err);
		}
	} else if (udc_ctrl_stage_is_data_in(dev)) {
		udc_ctrl_submit_s_in_status(dev);
	} else {
		udc_ctrl_submit_s_status(dev);
	}
}

static void udc_stm32_thread_handler(void *arg1, void *arg2, void *arg3)
{
	const struct device *dev = arg1;
	struct udc_stm32_data *priv = udc_get_private(dev);
	struct udc_stm32_msg msg;

	while (true) {
		k_msgq_get(&priv->msgq_data, &msg, K_FOREVER);
		switch (msg.type) {
		case UDC_STM32_MSG_SETUP:
			handle_msg_setup(priv);
			break;
		case UDC_STM32_MSG_DATA_IN:
			handle_msg_data_in(priv, msg.ep);
			break;
		case UDC_STM32_MSG_DATA_OUT:
			handle_msg_data_out(priv, msg.ep, msg.rx_count);
			break;
		}
	}
}

void HAL_PCDEx_SetConnectionState(PCD_HandleTypeDef *hpcd, uint8_t state)
{
	struct udc_stm32_data *priv = hpcd2data(hpcd);
	const struct udc_stm32_config *cfg = priv->dev->config;

	if (cfg->disconnect_gpio.port != NULL) {
		gpio_pin_configure_dt(&cfg->disconnect_gpio,
				      state ? GPIO_OUTPUT_ACTIVE : GPIO_OUTPUT_INACTIVE);
	}
}

/*
 * The callbacks above are invoked by HAL_PCD_IRQHandler() when appropriate.
 * HAL_PCD_IRQHandler() is registered as ISR for this driver because it just
 * so happens to match the Zephyr ISR calling convention, and we don't need
 * to do any additional processing upon interrupt: this saves a few cycles
 * when taking an interrupt and ought to consume less ROM too.
 */

int udc_stm32_init(const struct device *dev)
{
	struct udc_stm32_data *priv = udc_get_private(dev);
	const struct udc_stm32_config *cfg = dev->config;
	HAL_StatusTypeDef status;

	if (udc_stm32_clock_enable(dev) < 0) {
		LOG_ERR("Error enabling clock(s)");
		return -EIO;
	}

	/* Wipe and (re)initialize HAL context */
	memset(&priv->pcd, 0, sizeof(priv->pcd));

	priv->pcd.Instance = cfg->base;
	priv->pcd.Init.dev_endpoints = cfg->num_endpoints;
	priv->pcd.Init.ep0_mps = UDC_STM32_EP0_MAX_PACKET_SIZE;
	priv->pcd.Init.phy_itface = cfg->selected_phy;
	priv->pcd.Init.speed = cfg->selected_speed;

	status = HAL_PCD_Init(&priv->pcd);
	if (status != HAL_OK) {
		LOG_ERR("PCD_Init failed, %d", (int)status);
		return -EIO;
	}

	if (HAL_PCD_Stop(&priv->pcd) != HAL_OK) {
		return -EIO;
	}

	return 0;
}

#if defined(USB) || defined(USB_DRD_FS)
static inline void udc_stm32_mem_init(const struct device *dev)
{
	struct udc_stm32_data *priv = udc_get_private(dev);
	const struct udc_stm32_config *cfg = dev->config;

	/**
	 * Endpoint configuration table is placed at the
	 * beginning of Private Memory Area and consumes
	 * 8 bytes for each endpoint.
	 */
	priv->occupied_mem = 8 * cfg->num_endpoints;
}

static int udc_stm32_ep_mem_config(const struct device *dev,
				   struct udc_ep_config *ep_cfg,
				   bool enable)
{
	struct udc_stm32_data *priv = udc_get_private(dev);
	const struct udc_stm32_config *cfg = dev->config;
	uint32_t size;

	size = MIN(udc_mps_ep_size(ep_cfg), cfg->ep_mps);

	if (!enable) {
		priv->occupied_mem -= size;
		return 0;
	}

	if (priv->occupied_mem + size >= cfg->dram_size) {
		LOG_ERR("Unable to allocate FIFO for 0x%02x", ep_cfg->addr);
		return -ENOMEM;
	}

	/* Configure PMA offset for the endpoint */
	if (HAL_PCDEx_PMAConfig(&priv->pcd, ep_cfg->addr, PCD_SNG_BUF,
				priv->occupied_mem) != HAL_OK) {
		return -EIO;
	}

	priv->occupied_mem += size;

	return 0;
}
#else
static void udc_stm32_mem_init(const struct device *dev)
{
	struct udc_stm32_data *priv = udc_get_private(dev);
	const struct udc_stm32_config *cfg = dev->config;
	uint32_t rxfifo_size; /* in words */
	HAL_StatusTypeDef __maybe_unused status;

	LOG_DBG("DRAM size: %uB", cfg->dram_size);

	/*
	 * In addition to the user-provided baseline, RxFIFO should fit:
	 *	- Global OUT NAK (1 word)
	 *	- Received packet information (1 word)
	 *	- Transfer complete status information (2 words per OUT endpoint)
	 *
	 * Align user-provided baseline up to 32-bit word size then
	 * add this "fixed" overhead to obtain the final RxFIFO size.
	 */
	rxfifo_size = DIV_ROUND_UP(CONFIG_UDC_STM32_OTG_RXFIFO_BASELINE_SIZE, 4U);
	rxfifo_size += 2U; /* Global OUT NAK and Rx packet info */
	rxfifo_size += 2U * cfg->num_endpoints;

	LOG_DBG("RxFIFO size: %uB", rxfifo_size * 4U);

	status = HAL_PCDEx_SetRxFiFo(&priv->pcd, rxfifo_size);
	__ASSERT_NO_MSG(status == HAL_OK);

	priv->occupied_mem = rxfifo_size * 4U;

	/* For EP0 TX, reserve only one MPS */
	status = HAL_PCDEx_SetTxFiFo(&priv->pcd, 0,
				     DIV_ROUND_UP(UDC_STM32_EP0_MAX_PACKET_SIZE, 4U));
	__ASSERT_NO_MSG(status == HAL_OK);

	priv->occupied_mem += UDC_STM32_EP0_MAX_PACKET_SIZE;

	/* Reset TX allocs */
	for (unsigned int i = 1U; i < cfg->num_endpoints; i++) {
		status = HAL_PCDEx_SetTxFiFo(&priv->pcd, i, 0);
		__ASSERT_NO_MSG(status == HAL_OK);
	}
}

static int udc_stm32_ep_mem_config(const struct device *dev,
				   struct udc_ep_config *ep_cfg,
				   bool enable)
{
	struct udc_stm32_data *priv = udc_get_private(dev);
	const struct udc_stm32_config *cfg = dev->config;
	unsigned int words;

	if (!USB_EP_DIR_IS_IN(ep_cfg->addr) || USB_EP_GET_IDX(ep_cfg->addr) == 0U) {
		return 0;
	}

	words = DIV_ROUND_UP(MIN(udc_mps_ep_size(ep_cfg), cfg->ep_mps), 4U);
	words = (words <= 64) ? words * 2 : words;

	if (!enable) {
		if (priv->occupied_mem >= (words * 4)) {
			priv->occupied_mem -= (words * 4);
		}
		if (HAL_PCDEx_SetTxFiFo(&priv->pcd, USB_EP_GET_IDX(ep_cfg->addr), 0) != HAL_OK) {
			return -EIO;
		}
		return 0;
	}

	if (cfg->dram_size - priv->occupied_mem < words * 4) {
		LOG_ERR("Unable to allocate FIFO for 0x%02x", ep_cfg->addr);
		return -ENOMEM;
	}

	if (HAL_PCDEx_SetTxFiFo(&priv->pcd, USB_EP_GET_IDX(ep_cfg->addr), words) != HAL_OK) {
		return -EIO;
	}

	priv->occupied_mem += words * 4;

	return 0;
}
#endif

static int udc_stm32_enable(const struct device *dev)
{
	struct udc_stm32_data *priv = udc_get_private(dev);
	const struct udc_stm32_config *cfg = dev->config;
	HAL_StatusTypeDef status;
	int ret;

	LOG_DBG("Enable UDC");

	udc_stm32_mem_init(dev);

	status = HAL_PCD_Start(&priv->pcd);
	if (status != HAL_OK) {
		LOG_ERR("PCD_Start failed, %d", (int)status);
		return -EIO;
	}

	ret = udc_ep_enable_internal(dev, USB_CONTROL_EP_OUT,
				     USB_EP_TYPE_CONTROL,
				     UDC_STM32_EP0_MAX_PACKET_SIZE, 0);
	if (ret != 0) {
		LOG_ERR("Failed enabling ep 0x%02x", USB_CONTROL_EP_OUT);
		return ret;
	}

	ret = udc_ep_enable_internal(dev, USB_CONTROL_EP_IN,
				     USB_EP_TYPE_CONTROL,
				     UDC_STM32_EP0_MAX_PACKET_SIZE, 0);
	if (ret != 0) {
		LOG_ERR("Failed enabling ep 0x%02x", USB_CONTROL_EP_IN);
		return ret;
	}

	irq_enable(cfg->irqn);

	return 0;
}

static int udc_stm32_disable(const struct device *dev)
{
	struct udc_stm32_data *priv = udc_get_private(dev);
	const struct udc_stm32_config *cfg = dev->config;
	HAL_StatusTypeDef status;

	irq_disable(cfg->irqn);

	if (udc_ep_disable_internal(dev, USB_CONTROL_EP_OUT) != 0) {
		LOG_ERR("Failed to disable control endpoint");
		return -EIO;
	}

	if (udc_ep_disable_internal(dev, USB_CONTROL_EP_IN) != 0) {
		LOG_ERR("Failed to disable control endpoint");
		return -EIO;
	}

	status = HAL_PCD_Stop(&priv->pcd);
	if (status != HAL_OK) {
		LOG_ERR("PCD_Stop failed, %d", (int)status);
		return -EIO;
	}

	return 0;
}

static int udc_stm32_shutdown(const struct device *dev)
{
	struct udc_stm32_data *priv = udc_get_private(dev);
	const struct udc_stm32_config *cfg = dev->config;
	HAL_StatusTypeDef status;

	status = HAL_PCD_DeInit(&priv->pcd);
	if (status != HAL_OK) {
		LOG_ERR("PCD_DeInit failed, %d", (int)status);
		/* continue anyway */
	}

	if (udc_stm32_clock_disable(dev) < 0) {
		LOG_ERR("Error disabling clock(s)");
		/* continue anyway */
	}

	if (irq_is_enabled(cfg->irqn)) {
		irq_disable(cfg->irqn);
	}

	return 0;
}

static int udc_stm32_set_address(const struct device *dev, const uint8_t addr)
{
	struct udc_stm32_data *priv = udc_get_private(dev);
	HAL_StatusTypeDef status;

	LOG_DBG("Set Address %u", addr);

	status = HAL_PCD_SetAddress(&priv->pcd, addr);
	if (status != HAL_OK) {
		LOG_ERR("HAL_PCD_SetAddress failed(0x%02x), %d",
			addr, (int)status);
		return -EIO;
	}

	return 0;
}

static int udc_stm32_host_wakeup(const struct device *dev)
{
	struct udc_stm32_data *priv = udc_get_private(dev);
	HAL_StatusTypeDef status;

	status = HAL_PCD_ActivateRemoteWakeup(&priv->pcd);
	if (status != HAL_OK) {
		LOG_ERR("HAL_PCD_ActivateRemoteWakeup, %d", (int)status);
		return -EIO;
	}

	/* Must be active from 1ms to 15ms as per reference manual. */
	k_sleep(K_MSEC(2));

	status = HAL_PCD_DeActivateRemoteWakeup(&priv->pcd);
	if (status != HAL_OK) {
		return -EIO;
	}

	udc_set_suspended(dev, false);
	udc_submit_event(dev, UDC_EVT_RESUME, 0);

	return 0;
}

static int udc_stm32_ep_enable(const struct device *dev,
			       struct udc_ep_config *ep_cfg)
{
	struct udc_stm32_data *priv = udc_get_private(dev);
	HAL_StatusTypeDef status;
	uint8_t ep_type;
	int ret;

	LOG_DBG("Enable ep 0x%02x", ep_cfg->addr);

	switch (ep_cfg->attributes & USB_EP_TRANSFER_TYPE_MASK) {
	case USB_EP_TYPE_CONTROL:
		ep_type = EP_TYPE_CTRL;
		break;
	case USB_EP_TYPE_BULK:
		ep_type = EP_TYPE_BULK;
		break;
	case USB_EP_TYPE_INTERRUPT:
		ep_type = EP_TYPE_INTR;
		break;
	case USB_EP_TYPE_ISO:
		ep_type = EP_TYPE_ISOC;
		break;
	default:
		return -EINVAL;
	}

	ret = udc_stm32_ep_mem_config(dev, ep_cfg, true);
	if (ret != 0) {
		return ret;
	}

	status = HAL_PCD_EP_Open(&priv->pcd, ep_cfg->addr,
				 udc_mps_ep_size(ep_cfg), ep_type);
	if (status != HAL_OK) {
		LOG_ERR("HAL_PCD_EP_Open failed(0x%02x), %d",
			ep_cfg->addr, (int)status);
		return -EIO;
	}

	return 0;
}

static int udc_stm32_ep_disable(const struct device *dev,
			      struct udc_ep_config *ep_cfg)
{
	struct udc_stm32_data *priv = udc_get_private(dev);
	HAL_StatusTypeDef status;

	LOG_DBG("Disable ep 0x%02x", ep_cfg->addr);

	status = HAL_PCD_EP_Close(&priv->pcd, ep_cfg->addr);
	if (status != HAL_OK) {
		LOG_ERR("HAL_PCD_EP_Close failed(0x%02x), %d",
			ep_cfg->addr, (int)status);
		return -EIO;
	}

	return udc_stm32_ep_mem_config(dev, ep_cfg, false);
}

static int udc_stm32_ep_set_halt(const struct device *dev,
				 struct udc_ep_config *ep_cfg)
{
	struct udc_stm32_data *priv = udc_get_private(dev);
	HAL_StatusTypeDef status;

	LOG_DBG("Halt ep 0x%02x", ep_cfg->addr);

	status = HAL_PCD_EP_SetStall(&priv->pcd, ep_cfg->addr);
	if (status != HAL_OK) {
		LOG_ERR("HAL_PCD_EP_SetStall failed(0x%02x), %d",
			ep_cfg->addr, (int)status);
		return -EIO;
	}

	/* Mark endpoint as halted if not control EP */
	if (USB_EP_GET_IDX(ep_cfg->addr) != 0U) {
		ep_cfg->stat.halted = true;
	}

	return 0;
}

static int udc_stm32_ep_clear_halt(const struct device *dev,
				   struct udc_ep_config *ep_cfg)
{
	struct udc_stm32_data *priv = udc_get_private(dev);
	HAL_StatusTypeDef status;
	struct net_buf *buf;

	LOG_DBG("Clear halt for ep 0x%02x", ep_cfg->addr);

	status = HAL_PCD_EP_ClrStall(&priv->pcd, ep_cfg->addr);
	if (status != HAL_OK) {
		LOG_ERR("HAL_PCD_EP_ClrStall failed(0x%02x), %d",
			ep_cfg->addr, (int)status);
		return -EIO;
	}

	/* Clear halt bit from endpoint status */
	ep_cfg->stat.halted = false;

	/* Check if there are transfers queued for EP */
	buf = udc_buf_peek(ep_cfg);
	if (buf != NULL) {
		/*
		 * There is at least one transfer pending.
		 * IN EP transfer can be started only if not busy;
		 * OUT EP transfer should be prepared only if busy.
		 */
		const bool busy = udc_ep_is_busy(ep_cfg);

		if (USB_EP_DIR_IS_IN(ep_cfg->addr) && !busy) {
			udc_stm32_tx(dev, ep_cfg, buf);
		} else if (USB_EP_DIR_IS_OUT(ep_cfg->addr) && busy) {
			udc_stm32_rx(dev, ep_cfg, buf);
		}
	}
	return 0;
}

static int udc_stm32_ep_flush(const struct device *dev,
			      struct udc_ep_config *ep_cfg)
{
	struct udc_stm32_data *priv = udc_get_private(dev);
	HAL_StatusTypeDef status;

	LOG_DBG("Flush ep 0x%02x", ep_cfg->addr);

	status = HAL_PCD_EP_Flush(&priv->pcd, ep_cfg->addr);
	if (status != HAL_OK) {
		LOG_ERR("HAL_PCD_EP_Flush failed(0x%02x), %d",
			ep_cfg->addr, (int)status);
		return -EIO;
	}

	return 0;
}

static int udc_stm32_ep_enqueue(const struct device *dev,
				struct udc_ep_config *ep_cfg,
				struct net_buf *buf)
{
	unsigned int lock_key;
	int ret = 0;

	udc_buf_put(ep_cfg, buf);

	lock_key = irq_lock();

	if (USB_EP_DIR_IS_IN(ep_cfg->addr)) {
		if (ep_cfg->stat.halted) {
			LOG_DBG("skip enqueue for halted ep 0x%02x", ep_cfg->addr);
		} else {
			ret = udc_stm32_tx(dev, ep_cfg, buf);
		}
	} else {
		ret = udc_stm32_rx(dev, ep_cfg, buf);
	}

	irq_unlock(lock_key);

	return ret;
}

static int udc_stm32_ep_dequeue(const struct device *dev,
				struct udc_ep_config *ep_cfg)
{
	struct net_buf *buf;

	udc_stm32_ep_flush(dev, ep_cfg);

	buf = udc_buf_get_all(ep_cfg);
	if (buf != NULL) {
		udc_submit_ep_event(dev, buf, -ECONNABORTED);
	}

	udc_ep_set_busy(ep_cfg, false);

	return 0;
}

static enum udc_bus_speed udc_stm32_device_speed(const struct device *dev)
{
	struct udc_stm32_data *priv = udc_get_private(dev);

	/*
	 * N.B.: pcd.Init.speed is used here on purpose instead
	 * of udc_stm32_config::selected_speed because HAL updates
	 * this field after USB enumeration to reflect actual bus speed.
	 */

	if (priv->pcd.Init.speed == PCD_SPEED_HIGH) {
		return UDC_BUS_SPEED_HS;
	}

	if (priv->pcd.Init.speed == PCD_SPEED_HIGH_IN_FULL ||
	    priv->pcd.Init.speed == PCD_SPEED_FULL) {
		return UDC_BUS_SPEED_FS;
	}

	return UDC_BUS_UNKNOWN;
}

static const struct udc_api udc_stm32_api = {
	.lock = udc_stm32_lock,
	.unlock = udc_stm32_unlock,
	.init = udc_stm32_init,
	.enable = udc_stm32_enable,
	.disable = udc_stm32_disable,
	.shutdown = udc_stm32_shutdown,
	.set_address = udc_stm32_set_address,
	.host_wakeup = udc_stm32_host_wakeup,
	.ep_try_config = NULL,
	.ep_enable = udc_stm32_ep_enable,
	.ep_disable = udc_stm32_ep_disable,
	.ep_set_halt = udc_stm32_ep_set_halt,
	.ep_clear_halt = udc_stm32_ep_clear_halt,
	.ep_enqueue = udc_stm32_ep_enqueue,
	.ep_dequeue = udc_stm32_ep_dequeue,
	.device_speed = udc_stm32_device_speed,
};

/* ----------------- Instance/Device specific data ----------------- */

/*
 * USB, USB_OTG_FS and USB_DRD_FS are defined in STM32Cube HAL and allows to
 * distinguish between two kind of USB DC. STM32 F0, F3, L0 and G4 series
 * support USB device controller. STM32 F4 and F7 series support USB_OTG_FS
 * device controller. STM32 F1 and L4 series support either USB or USB_OTG_FS
 * device controller.STM32 G0 series supports USB_DRD_FS device controller.
 *
 * WARNING: Don't mix USB defined in STM32Cube HAL and CONFIG_USB_* from Zephyr
 * Kconfig system.
 */
K_THREAD_STACK_DEFINE(udc0_thr_stk, CONFIG_UDC_STM32_STACK_SIZE);

static struct udc_ep_config udc0_in_ep_cfg[DT_INST_PROP(0, num_bidir_endpoints)];
static struct udc_ep_config udc0_out_ep_cfg[DT_INST_PROP(0, num_bidir_endpoints)];

static struct udc_stm32_data udc0_priv;

static struct udc_data udc0_data = {
	.mutex = Z_MUTEX_INITIALIZER(udc0_data.mutex),
	.priv = &udc0_priv,
};

static void udc0_irq_connect(void)
{
	IRQ_CONNECT(DT_INST_IRQ_BY_NAME(0, UDC_STM32_IRQ_NAME, irq),
		    DT_INST_IRQ_BY_NAME(0, UDC_STM32_IRQ_NAME, priority),
		    HAL_PCD_IRQHandler, &udc0_priv.pcd, 0);
}

PINCTRL_DT_INST_DEFINE(0);

static const struct stm32_pclken udc0_pclken[] = STM32_DT_INST_CLOCKS(0);

static const struct udc_stm32_config udc0_cfg  = {
	.base = (void *)DT_INST_REG_ADDR(0),
	.num_endpoints = DT_INST_PROP(0, num_bidir_endpoints),
	.dram_size = DT_INST_PROP(0, ram_size),
	.irq_connect = udc0_irq_connect,
	.irqn = DT_INST_IRQ_BY_NAME(0, UDC_STM32_IRQ_NAME, irq),
	.pclken = (struct stm32_pclken *)udc0_pclken,
	.num_clocks = DT_INST_NUM_CLOCKS(0),
	.pinctrl = PINCTRL_DT_INST_DEV_CONFIG_GET(0),
	.in_eps = udc0_in_ep_cfg,
	.out_eps = udc0_out_ep_cfg,
	.ep_mps = UDC_STM32_NODE_EP_MPS(DT_DRV_INST(0)),
	.selected_phy = UDC_STM32_NODE_PHY_ITFACE(DT_DRV_INST(0)),
	.selected_speed = UDC_STM32_NODE_SPEED(DT_DRV_INST(0)),
	.thread_stack = udc0_thr_stk,
	.thread_stack_size = K_THREAD_STACK_SIZEOF(udc0_thr_stk),
	.disconnect_gpio = GPIO_DT_SPEC_INST_GET_OR(0, disconnect_gpios, {0}),
	.ulpi_reset_gpio = GPIO_DT_SPEC_GET_OR(UDC_STM32_PHY(DT_DRV_INST(0)), reset_gpios, {0}),
};

static int udc_stm32_clock_enable(const struct device *dev)
{
	const struct device *const clk = DEVICE_DT_GET(STM32_CLOCK_CONTROL_NODE);
	const struct udc_stm32_config *cfg = dev->config;

	if (!device_is_ready(clk)) {
		LOG_ERR("clock control device not ready");
		return -ENODEV;
	}

	/* Power configuration */
#if defined(CONFIG_SOC_SERIES_STM32H7X)
	LL_PWR_EnableUSBVoltageDetector();

	/* Per AN2606: USBREGEN not supported when running in FS mode. */
	LL_PWR_DisableUSBReg();
	while (!LL_PWR_IsActiveFlag_USB()) {
		LOG_INF("PWR not active yet");
		k_msleep(100);
	}
#elif defined(CONFIG_SOC_SERIES_STM32U5X)
	/* Sequence to enable the power of the OTG HS on a stm32U5 serie : Enable VDDUSB */
	__ASSERT_NO_MSG(LL_AHB3_GRP1_IsEnabledClock(LL_AHB3_GRP1_PERIPH_PWR));

	/* Check that power range is 1 or 2 */
	if (LL_PWR_GetRegulVoltageScaling() < LL_PWR_REGU_VOLTAGE_SCALE2) {
		LOG_ERR("Wrong Power range to use USB OTG HS");
		return -EIO;
	}

	LL_PWR_EnableVddUSB();

	#if DT_HAS_COMPAT_STATUS_OKAY(st_stm32_otghs)
		/* Configure VOSR register of USB HSTransceiverSupply(); */
		LL_PWR_EnableUSBPowerSupply();
		LL_PWR_EnableUSBEPODBooster();
		while (LL_PWR_IsActiveFlag_USBBOOST() != 1) {
			/* Wait for USB EPOD BOOST ready */
		}
	#endif /* DT_HAS_COMPAT_STATUS_OKAY(st_stm32_otghs) */
#elif defined(CONFIG_SOC_SERIES_STM32N6X)
	/* Enable Vdd33USB voltage monitoring */
	LL_PWR_EnableVddUSBMonitoring();
	while (!LL_PWR_IsActiveFlag_USB33RDY()) {
		/* Wait for Vdd33USB ready */
	}

	/* Enable VDDUSB */
	LL_PWR_EnableVddUSB();
#elif defined(CONFIG_SOC_SERIES_STM32WBAX)
	/* Remove VDDUSB power isolation */
	LL_PWR_EnableVddUSB();

	/* Make sure that voltage scaling is Range 1 */
	__ASSERT_NO_MSG(LL_PWR_GetRegulCurrentVOS() == LL_PWR_REGU_VOLTAGE_SCALE1);

	/* Enable VDD11USB */
	LL_PWR_EnableVdd11USB();

	/* Enable USB OTG internal power */
	LL_PWR_EnableUSBPWR();

	while (!LL_PWR_IsActiveFlag_VDD11USBRDY()) {
		/* Wait for VDD11USB supply to be ready */
	}

	/* Enable USB OTG booster */
	LL_PWR_EnableUSBBooster();

	while (!LL_PWR_IsActiveFlag_USBBOOSTRDY()) {
		/* Wait for USB OTG booster to be ready */
	}
#elif defined(PWR_USBSCR_USB33SV) || defined(PWR_SVMCR_USV)
	/*
	 * VDDUSB independent USB supply (PWR clock is on)
	 * with LL_PWR_EnableVDDUSB function (higher case)
	 */
	LL_PWR_EnableVDDUSB();
#endif

	if (cfg->num_clocks > 1) {
		if (clock_control_configure(clk, &cfg->pclken[1], NULL) != 0) {
			LOG_ERR("Could not select USB domain clock");
			return -EIO;
		}
	}

	if (clock_control_on(clk, &cfg->pclken[0]) != 0) {
		LOG_ERR("Unable to enable USB clock");
		return -EIO;
	}

	if (IS_ENABLED(CONFIG_UDC_STM32_CLOCK_CHECK) && cfg->num_clocks > 1) {
		uint32_t usb_clock_rate;

		if (clock_control_get_rate(clk, &cfg->pclken[1], &usb_clock_rate) != 0) {
			LOG_ERR("Failed to get USB domain clock rate");
			return -EIO;
		}

		if (usb_clock_rate != MHZ(48)) {
			LOG_ERR("USB Clock is not 48MHz (%d)", usb_clock_rate);
			return -ENOTSUP;
		}
	}

	/* Previous check won't work in case of F1/F3. Add build time check */
#if defined(RCC_CFGR_OTGFSPRE) || defined(RCC_CFGR_USBPRE)

#if (MHZ(48) == CONFIG_SYS_CLOCK_HW_CYCLES_PER_SEC) && !defined(STM32_PLL_USBPRE)
	/* PLL output clock is set to 48MHz, it should not be divided */
#warning USBPRE/OTGFSPRE should be set in rcc node
#endif

#endif /* RCC_CFGR_OTGFSPRE / RCC_CFGR_USBPRE */

	/* PHY configuration */
#if DT_HAS_COMPAT_STATUS_OKAY(st_stm32_otghs)
#if defined(CONFIG_SOC_SERIES_STM32N6X)
	/*
	 * Note that the USBPHYC is clocked only when
	 * the OTG_HS instance is also clocked, so this
	 * must come after clock_control_on() or the
	 * SoC will deadlock.
	 */

	/* Reset specific configuration bits before setting new values */
	USB1_HS_PHYC->USBPHYC_CR &= ~USB_USBPHYC_CR_FSEL_Msk;

	/* Configure the USB PHY Control Register to operate in the High frequency "24 MHz"
	 * by setting the Frequency Selection (FSEL) bits 4 and 5 to 10,
	 * which ensures proper communication.
	 */
	USB1_HS_PHYC->USBPHYC_CR |= USB_USBPHYC_CR_FSEL_24MHZ;

	/* Peripheral OTGPHY clock enable */
	LL_AHB5_GRP1_EnableClock(LL_AHB5_GRP1_PERIPH_OTGPHY1);
#elif DT_HAS_COMPAT_STATUS_OKAY(st_stm32u5_otghs_phy)
	const struct stm32_pclken hsphy_clk[] = STM32_DT_CLOCKS(DT_NODELABEL(otghs_phy));
	const uint32_t hsphy_clknum = DT_NUM_CLOCKS(DT_NODELABEL(otghs_phy));

	/* Configure OTG PHY reference clock through SYSCFG */
	__HAL_RCC_SYSCFG_CLK_ENABLE();

	HAL_SYSCFG_SetOTGPHYReferenceClockSelection(
		syscfg_otg_hs_phy_clk[DT_ENUM_IDX(DT_NODELABEL(otghs_phy), clock_reference)]
	);

	/* De-assert reset and enable clock of OTG PHY */
	HAL_SYSCFG_EnableOTGPHY(SYSCFG_OTG_HS_PHY_ENABLE);

	if (hsphy_clknum > 1) {
		if (clock_control_configure(clk, (void *)&hsphy_clk[1], NULL) != 0) {
			LOG_ERR("Failed OTGHS PHY mux configuration");
			return -EIO;
		}
	}

	if (clock_control_on(clk, (void *)&hsphy_clk[0]) != 0) {
		LOG_ERR("Failed enabling OTGHS PHY clock");
		return -EIO;
	}
#elif defined(CONFIG_SOC_SERIES_STM32H7X)
	/*
	 * If HS PHY (over ULPI) is used, enable ULPI interface clock.
	 * Otherwise, disable ULPI clock in sleep/low-power mode.
	 * (No need to disable Run mode clock, it is off by default)
	 */
	if (UDC_STM32_NODE_PHY_ITFACE(DT_DRV_INST(0)) == PCD_PHY_ULPI) {
		LL_AHB1_GRP1_EnableClock(LL_AHB1_GRP1_PERIPH_USB1OTGHSULPI);
	} else {
		LL_AHB1_GRP1_DisableClockSleep(LL_AHB1_GRP1_PERIPH_USB1OTGHSULPI);
	}
#elif defined(CONFIG_SOC_SERIES_STM32F7X)
	/*
	 * Preprocessor check is required here because
	 * the OTGPHYC defines are not provided if it
	 * doesn't exist on SoC.
	 */
	#if UDC_STM32_NODE_PHY_ITFACE(DT_DRV_INST(0)) == PCD_PHY_ULPI
		LL_AHB1_GRP1_EnableClock(LL_AHB1_GRP1_PERIPH_OTGHSULPI);
	#elif UDC_STM32_NODE_PHY_ITFACE(DT_DRV_INST(0)) == PCD_PHY_UTMI
		/*
		 * For some reason, the ULPI clock still needs to be
		 * enabled when the internal USBPHYC HS PHY is used.
		 */
		LL_AHB1_GRP1_EnableClock(LL_AHB1_GRP1_PERIPH_OTGHSULPI);
		LL_APB2_GRP1_EnableClock(LL_APB2_GRP1_PERIPH_OTGPHYC);
	#endif
#else /* CONFIG_SOC_SERIES_STM32F2X || CONFIG_SOC_SERIES_STM32F4X */
	if (UDC_STM32_NODE_PHY_ITFACE(DT_DRV_INST(0)) == PCD_PHY_ULPI) {
		LL_AHB1_GRP1_EnableClock(LL_AHB1_GRP1_PERIPH_OTGHSULPI);
	} else if (UDC_STM32_NODE_SPEED(DT_DRV_INST(0)) == PCD_SPEED_HIGH_IN_FULL) {
		/*
		 * Some parts of the STM32F4 series require the OTGHSULPILPEN to be
		 * cleared if the OTG_HS is used in FS mode. Disable it on all parts
		 * since it has no nefarious effect if performed when not required.
		 */
		LL_AHB1_GRP1_DisableClockLowPower(LL_AHB1_GRP1_PERIPH_OTGHSULPI);
	}
#endif /* CONFIG_SOC_SERIES_* */
#elif defined(CONFIG_SOC_SERIES_STM32H7X) && DT_HAS_COMPAT_STATUS_OKAY(st_stm32_otgfs)
	/* The USB2 controller only works in FS mode, but the ULPI clock needs
	 * to be disabled in sleep mode for it to work.
	 */
	LL_AHB1_GRP1_DisableClockSleep(LL_AHB1_GRP1_PERIPH_USB2OTGHSULPI);
#endif /* DT_HAS_COMPAT_STATUS_OKAY(st_stm32_otghs) */

	return 0;
}

static int udc_stm32_clock_disable(const struct device *dev)
{
	const struct device *clk = DEVICE_DT_GET(STM32_CLOCK_CONTROL_NODE);
	const struct udc_stm32_config *cfg = dev->config;

	if (clock_control_off(clk, &cfg->pclken[0]) != 0) {
		LOG_ERR("Unable to disable USB clock");
		return -EIO;
	}
#if DT_HAS_COMPAT_STATUS_OKAY(st_stm32_otghs) && defined(CONFIG_SOC_SERIES_STM32U5X)
	LL_AHB2_GRP1_DisableClock(LL_AHB2_GRP1_PERIPH_USBPHY);
#endif

	return 0;
}

static int udc_stm32_driver_init0(const struct device *dev)
{
	struct udc_stm32_data *priv = udc_get_private(dev);
	const struct udc_stm32_config *cfg = dev->config;
	struct udc_data *data = dev->data;
	int err;

	for (unsigned int i = 0; i < cfg->num_endpoints; i++) {
		cfg->out_eps[i].caps.out = 1;
		if (i == 0) {
			cfg->out_eps[i].caps.control = 1;
			cfg->out_eps[i].caps.mps = UDC_STM32_EP0_MAX_PACKET_SIZE;
		} else {
			cfg->out_eps[i].caps.bulk = 1;
			cfg->out_eps[i].caps.interrupt = 1;
			cfg->out_eps[i].caps.iso = 1;
			cfg->out_eps[i].caps.mps = cfg->ep_mps;
		}

		cfg->out_eps[i].addr = USB_EP_DIR_OUT | i;
		err = udc_register_ep(dev, cfg->out_eps + i);
		if (err != 0) {
			LOG_ERR("Failed to register endpoint");
			return err;
		}
	}

	for (unsigned int i = 0; i < cfg->num_endpoints; i++) {
		cfg->in_eps[i].caps.in = 1;
		if (i == 0) {
			cfg->in_eps[i].caps.control = 1;
			cfg->in_eps[i].caps.mps = UDC_STM32_EP0_MAX_PACKET_SIZE;
		} else {
			cfg->in_eps[i].caps.bulk = 1;
			cfg->in_eps[i].caps.interrupt = 1;
			cfg->in_eps[i].caps.iso = 1;
			cfg->in_eps[i].caps.mps = cfg->ep_mps;
		}

		cfg->in_eps[i].addr = USB_EP_DIR_IN | i;
		err = udc_register_ep(dev, cfg->in_eps + i);
		if (err != 0) {
			LOG_ERR("Failed to register endpoint");
			return err;
		}
	}

	data->caps.rwup = true;
	data->caps.out_ack = false;
	data->caps.addr_before_status = true;
	data->caps.mps0 = UDC_MPS0_64;
	if (cfg->selected_speed == PCD_SPEED_HIGH) {
		data->caps.hs = true;
	}

	priv->dev = dev;

	k_msgq_init(&priv->msgq_data, priv->msgq_buf, sizeof(struct udc_stm32_msg),
		    CONFIG_UDC_STM32_MAX_QMESSAGES);

	k_thread_create(&priv->thread_data, cfg->thread_stack,
			cfg->thread_stack_size, udc_stm32_thread_handler,
			(void *)dev, NULL, NULL, K_PRIO_COOP(CONFIG_UDC_STM32_THREAD_PRIORITY),
			K_ESSENTIAL, K_NO_WAIT);
	k_thread_name_set(&priv->thread_data, dev->name);

	/*
	 * Note that this really only configures the interrupt priority;
	 * IRQn-to-ISR mapping is done at build time by IRQ_CONNECT().
	 */
	cfg->irq_connect();

	err = pinctrl_apply_state(cfg->pinctrl, PINCTRL_STATE_DEFAULT);

	/* Ignore -ENOENT returned on series without pinctrl */
	if (err < 0 && err != -ENOENT) {
		LOG_ERR("USB pinctrl setup failed (%d)", err);
		return err;
	}

#ifdef SYSCFG_CFGR1_USB_IT_RMP
	/*
	 * STM32F302/F303: USB IRQ collides with CAN_1 IRQ (§14.1.3, RM0316)
	 * Remap IRQ by default to enable use of both IPs simultaneoulsy
	 * This should be done before calling any HAL function
	 */
	if (LL_APB2_GRP1_IsEnabledClock(LL_APB2_GRP1_PERIPH_SYSCFG)) {
		LL_SYSCFG_EnableRemapIT_USB();
	} else {
		LOG_ERR("System Configuration Controller clock is "
			"disabled. Unable to enable IRQ remapping.");
	}
#endif

	if (cfg->ulpi_reset_gpio.port != NULL) {
		if (!gpio_is_ready_dt(&cfg->ulpi_reset_gpio)) {
			LOG_ERR("Reset GPIO device not ready");
			return -EINVAL;
		}
		if (gpio_pin_configure_dt(&cfg->ulpi_reset_gpio, GPIO_OUTPUT_INACTIVE) != 0) {
			LOG_ERR("Couldn't configure reset pin");
			return -EIO;
		}
	}

	/*cd
	 * Required for at least STM32L4 devices as they electrically
	 * isolate USB features from VDDUSB. It must be enabled before
	 * USB can function. Refer to section 5.1.3 in DM00083560 or
	 * DM00310109.
	 */
#ifdef PWR_CR2_USV
#if defined(LL_APB1_GRP1_PERIPH_PWR)
	if (LL_APB1_GRP1_IsEnabledClock(LL_APB1_GRP1_PERIPH_PWR)) {
		LL_PWR_EnableVddUSB();
	} else {
		LL_APB1_GRP1_EnableClock(LL_APB1_GRP1_PERIPH_PWR);
		LL_PWR_EnableVddUSB();
		LL_APB1_GRP1_DisableClock(LL_APB1_GRP1_PERIPH_PWR);
	}
	#else
	LL_PWR_EnableVddUSB();
#endif /* defined(LL_APB1_GRP1_PERIPH_PWR) */
#endif /* PWR_CR2_USV */

	return 0;
}

DEVICE_DT_INST_DEFINE(0, udc_stm32_driver_init0, NULL, &udc0_data, &udc0_cfg,
		      POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_DEVICE,
		      &udc_stm32_api);
