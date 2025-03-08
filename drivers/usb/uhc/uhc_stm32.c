/*
 * Copyright (c) 2024 Syslinbit SCOP SAS
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * USB host driver for STM32
 *
 * NOTE: For now only STM32H723ZG is supported. Adjustments are necessary to support other parts
 * with st_usb_otghs and st_usb_otgfs cores (based Synopsys DWC2). Parts with st_stm32_usb cores
 * are not supported yet. Only skeleton is written for such core.
 */

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/irq.h>
#include <zephyr/net/buf.h>
#include <zephyr/sys/util.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/drivers/clock_control/stm32_clock_control.h>
#include <zephyr/drivers/usb/uhc.h>

#include <soc.h>
#include <stm32_ll_bus.h>
#include <stm32_ll_pwr.h>

#include <string.h>
#include <stdint.h>
#include <stddef.h>

#include "uhc_common.h"

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(uhc_stm32, CONFIG_UHC_DRIVER_LOG_LEVEL);


#define DT_COMPAT_STM32_USB   st_stm32_usb
#define DT_COMPAT_STM32_OTGFS st_stm32_otgfs
#define DT_COMPAT_STM32_OTGHS st_stm32_otghs

#define DT_PHY_COMPAT_EMBEDDED_FS usb_nop_xceiv
#define DT_PHY_COMPAT_EMBEDDED_HS st_stm32_usbphyc
#define DT_PHY_COMPAT_ULPI        usb_ulpi_phy

#define DT_CLOCKS_PROP_MAX_LEN (2U)

#define USB_SETUP_PACKET_SIZE    (8U)
#define USB_DEFAULT_DEVICE_ADDR  (0U)
#define USB_CONTROL_ENDPOINT     (0U)
#define USB_NB_MAX_XFER_ATTEMPTS (3U)

#define UHC_STM32_MAX_FRAME_NUMBER (0x3FFFU)
#define UHC_STM32_NB_MAX_PIPE      (16U)
BUILD_ASSERT(UHC_STM32_NB_MAX_PIPE <= UINT8_MAX);

#define _IS_DRIVER_OF_INSTANCE(dev, instance) \
	(((struct uhc_stm32_data *) uhc_get_private(dev))->hcd_ptr->Instance == instance)

#if defined(USB_OTG_FS)
#define IS_USB_OTG_FS_DEVICE(dev) _IS_DRIVER_OF_INSTANCE(dev, USB_OTG_FS)
#else
#define IS_USB_OTG_FS_DEVICE(dev) (0)
#endif

#if defined(USB_OTG_HS)
#define IS_USB_OTG_HS_DEVICE(dev) _IS_DRIVER_OF_INSTANCE(dev, USB_OTG_HS)
#else
#define IS_USB_OTG_HS_DEVICE(dev) (0)
#endif

#if defined(USB_DRD_FS)
#define IS_USB_DRD_FS_DEVICE(dev) _IS_DRIVER_OF_INSTANCE(dev, USB_DRD_FS)
#else
#define IS_USB_DRD_FS_DEVICE(dev) (0)
#endif

enum phy_interface {
	PHY_INVALID       = -1,
	PHY_EXTERNAL_ULPI = HCD_PHY_ULPI,
	PHY_EMBEDDED_FS   = HCD_PHY_EMBEDDED,
#if defined(USB_OTG_HS_EMBEDDED_PHY)
	PHY_EMBEDDED_HS   = USB_OTG_HS_EMBEDDED_PHY,
#endif
};

enum usb_speed {
	USB_SPEED_INVALID = -1,
	/* Values are fixed by maximum-speed enum values order in
	 * usb-controller.yaml dts binding file.
	 */
	USB_SPEED_LOW     =  0,
	USB_SPEED_FULL    =  1,
	USB_SPEED_HIGH    =  2,
	USB_SPEED_SUPER   =  3,
};

#define DT_PHY(node_id) DT_PHANDLE(node_id, phys)

#if defined(USB_OTG_HS_EMBEDDED_PHY)
#define DT_PHY_INTERFACE_TYPE(node_id) ( \
	DT_NODE_HAS_COMPAT(DT_PHY(node_id), DT_PHY_COMPAT_EMBEDDED_FS) ? PHY_EMBEDDED_FS : ( \
	DT_NODE_HAS_COMPAT(DT_PHY(node_id), DT_PHY_COMPAT_EMBEDDED_HS) ? PHY_EMBEDDED_HS : ( \
	DT_NODE_HAS_COMPAT(DT_PHY(node_id), DT_PHY_COMPAT_ULPI) ? PHY_EXTERNAL_ULPI : ( \
	PHY_INVALID))) \
)
#define GET_PHY_INTERFACE_MAX_SPEED(phy_interface) ( \
	(phy_interface == PHY_EMBEDDED_FS) ? USB_SPEED_FULL : ( \
	(phy_interface == PHY_EMBEDDED_HS) ? USB_SPEED_HIGH : ( \
	(phy_interface == PHY_EXTERNAL_ULPI) ? USB_SPEED_HIGH : ( \
	USB_SPEED_INVALID))) \
)
#else
#define DT_PHY_INTERFACE_TYPE(node_id) ( \
	DT_NODE_HAS_COMPAT(DT_PHY(node_id), DT_PHY_COMPAT_EMBEDDED_FS) ? PHY_EMBEDDED_FS : ( \
	DT_NODE_HAS_COMPAT(DT_PHY(node_id), DT_PHY_COMPAT_ULPI) ? PHY_EXTERNAL_ULPI : ( \
	PHY_INVALID)) \
)
#define GET_PHY_INTERFACE_MAX_SPEED(phy_interface) ( \
	(phy_interface == PHY_EMBEDDED_FS) ? USB_SPEED_FULL :  ( \
	(phy_interface == PHY_EXTERNAL_ULPI) ? USB_SPEED_HIGH : ( \
	USB_SPEED_INVALID)) \
)
#endif

#define DT_PHY_MAX_SPEED(node_id) GET_PHY_INTERFACE_MAX_SPEED(DT_PHY_INTERFACE_TYPE(node_id))

#define DT_CORE_MAX_SPEED(node_id) ( \
	DT_NODE_HAS_COMPAT(node_id, DT_COMPAT_STM32_USB) ? USB_SPEED_FULL : ( \
	DT_NODE_HAS_COMPAT(node_id, DT_COMPAT_STM32_OTGFS) ? USB_SPEED_FULL : ( \
	DT_NODE_HAS_COMPAT(node_id, DT_COMPAT_STM32_OTGHS) ? USB_SPEED_HIGH : ( \
	USB_SPEED_INVALID))) \
)

#define DT_MAX_SPEED_PROP_OR(node_id, default_speed) \
	DT_ENUM_IDX_OR(node_id, maximum_speed, default_speed)

#define DT_MAX_SPEED(node_id) \
	MIN( \
		MIN( \
			DT_CORE_MAX_SPEED(node_id), \
			DT_PHY_MAX_SPEED(node_id) \
		), \
		DT_MAX_SPEED_PROP_OR(node_id, USB_SPEED_SUPER) \
	)

#if defined(USB_DRD_FS)
#define SPEED_FROM_HAL_SPEED(drd_core_speed) (     \
	(drd_core_speed == USB_DRD_SPEED_LS) ? USB_SPEED_FULL : (  \
	(drd_core_speed == USB_DRD_SPEED_FS) ? USB_SPEED_HIGH : (  \
	USB_SPEED_INVALID))                                        \
)
#endif

#if defined(USB_OTG_FS) || defined(USB_OTG_HS)
#define SPEED_FROM_HAL_SPEED(otg_core_speed) (            \
	(otg_core_speed == HPRT0_PRTSPD_LOW_SPEED) ? USB_SPEED_FULL : (   \
	(otg_core_speed == HPRT0_PRTSPD_FULL_SPEED) ? USB_SPEED_HIGH : (  \
	(otg_core_speed == HPRT0_PRTSPD_HIGH_SPEED) ? USB_SPEED_HIGH : (  \
	USB_SPEED_INVALID)))                                              \
)
#endif

#define SPEED_TO_HCD_INIT_SPEED(usb_speed) (           \
	(usb_speed >= USB_SPEED_HIGH) ? HCD_SPEED_HIGH : ( \
	(usb_speed == USB_SPEED_FULL) ? HCD_SPEED_FULL : ( \
	(usb_speed == USB_SPEED_LOW) ? HCD_SPEED_LOW : (   \
	HCD_SPEED_LOW)))                                   \
)

enum uhc_state {
	STM32_UHC_STATE_SPEED_ENUM,
	STM32_UHC_STATE_DISCONNECTED,
	STM32_UHC_STATE_READY,
};

struct uhc_stm32_data {
	enum uhc_state state;
	const struct device *dev;
	size_t num_bidir_pipes;
	bool busy_pipe[UHC_STM32_NB_MAX_PIPE];
	HCD_HandleTypeDef *hcd_ptr;
	uint16_t frame_number;
	struct uhc_transfer *ongoing_xfer;
	struct net_buf_simple_state ongoing_xfer_buf_save;
	size_t ongoing_xfer_attempts;
	uint8_t ongoing_xfer_pipe_id;
	struct k_work_q work_queue;
	struct k_work on_connect_disconnect_work;
	struct k_work on_reset_work;
	struct k_work on_sof_work;
	struct k_work on_xfer_update_work;
	struct k_work on_schedule_new_xfer_work;
	struct k_work_delayable delayed_enum_reset_work;
};

struct uhc_stm32_config {
	uint32_t irq;
	enum phy_interface phy;
	struct stm32_pclken clocks[DT_CLOCKS_PROP_MAX_LEN];
	size_t num_clock;
	const struct pinctrl_dev_config *pcfg;
	struct gpio_dt_spec ulpi_reset_gpio;
	struct gpio_dt_spec vbus_enable_gpio;
};


static void uhc_stm32_irq(const struct device *dev)
{
	struct uhc_stm32_data *priv = uhc_get_private(dev);

	/* Call the ST IRQ handler which will, in turn,
	 * will call the corresponding HAL_HCD_* callbacks
	 */
	HAL_HCD_IRQHandler(priv->hcd_ptr);
}

void HAL_HCD_Connect_Callback(HCD_HandleTypeDef *hhcd)
{
	const struct device *dev = (const struct device *)hhcd->pData;
	struct uhc_stm32_data *priv = uhc_get_private(dev);

	k_work_submit_to_queue(&priv->work_queue, &priv->on_connect_disconnect_work);
}

void HAL_HCD_Disconnect_Callback(HCD_HandleTypeDef *hhcd)
{
	const struct device *dev = (const struct device *)hhcd->pData;
	struct uhc_stm32_data *priv = uhc_get_private(dev);

	k_work_submit_to_queue(&priv->work_queue, &priv->on_connect_disconnect_work);
}

void HAL_HCD_PortEnabled_Callback(HCD_HandleTypeDef *hhcd)
{
	const struct device *dev = (const struct device *)hhcd->pData;
	struct uhc_stm32_data *priv = uhc_get_private(dev);

	k_work_submit_to_queue(&priv->work_queue, &priv->on_reset_work);
}

void HAL_HCD_PortDisabled_Callback(HCD_HandleTypeDef *hhcd)
{
	ARG_UNUSED(hhcd);
}

void HAL_HCD_SOF_Callback(HCD_HandleTypeDef *hhcd)
{
	const struct device *dev = (const struct device *)hhcd->pData;
	struct uhc_stm32_data *priv = uhc_get_private(dev);

	k_work_submit_to_queue(&priv->work_queue, &priv->on_sof_work);
}

void HAL_HCD_HC_NotifyURBChange_Callback(HCD_HandleTypeDef *hhcd, uint8_t chnum,
					 HCD_URBStateTypeDef urb_state)
{
	const struct device *dev = (const struct device *)hhcd->pData;
	struct uhc_stm32_data *priv = uhc_get_private(dev);

	k_work_submit_to_queue(&priv->work_queue, &priv->on_xfer_update_work);
}


static int priv_clock_enable(struct uhc_stm32_data *const priv)
{
	const struct uhc_stm32_config *config = priv->dev->config;
	const struct device *const clk = DEVICE_DT_GET(STM32_CLOCK_CONTROL_NODE);

	int err = 0;

	if (!device_is_ready(clk)) {
		LOG_ERR("clock control device not ready");
		return -ENODEV;
	}

	if (config->num_clock > 1) {
		err = clock_control_configure(clk,
			(clock_control_subsys_t *)&config->clocks[1], NULL
		);
		if (err) {
			LOG_ERR("Could not select USB domain clock");
			return -EIO;
		}
	}

	err = clock_control_on(clk, (clock_control_subsys_t *)&config->clocks[0]);
	if (err) {
		LOG_ERR("Unable to enable USB clock");
		return -EIO;
	}

	uint32_t clock_freq;

	err = clock_control_get_rate(clk,
		(clock_control_subsys_t *)&config->clocks[1], &clock_freq
	);
	if (err) {
		LOG_ERR("Failed to get USB domain clock frequency");
		return -EIO;
	}

	if (clock_freq != MHZ(48)) {
		LOG_ERR("USB Clock is not 48MHz (%d)", clock_freq);
		return -ENOTSUP;
	}

#if defined(CONFIG_SOC_SERIES_STM32H7X)
	if (IS_USB_OTG_HS_DEVICE(priv->dev)) {
		LL_AHB1_GRP1_EnableClockSleep(LL_AHB1_GRP1_PERIPH_USB1OTGHS);
		if (config->phy == PHY_EXTERNAL_ULPI) {
			LL_AHB1_GRP1_EnableClockSleep(LL_AHB1_GRP1_PERIPH_USB1OTGHSULPI);
		} else {
			/* ULPI clock is activated by default in sleep mode. If we use an
			 * other PHY this will prevent the USB_OTG_HS controller to work
			 * properly when the MCU enters sleep mode so we must disable it.
			 */
			LL_AHB1_GRP1_DisableClockSleep(LL_AHB1_GRP1_PERIPH_USB1OTGHSULPI);
		}
	}
#if defined(USB_OTG_FS)
	if (IS_USB_OTG_FS_DEVICE(priv->dev)) {
		LL_AHB1_GRP1_EnableClockSleep(LL_AHB1_GRP1_PERIPH_USB2OTGHS);
		/* USB_OTG_FS cannot be connected to an external ULPI PHY but
		 * ULPI clock is still activated by default in sleep mode
		 * (in run mode it is already disabled by default). This prevents
		 * the USB_OTG_FS controller to work properly when the MCU enters
		 * sleep mode so we must disable it.
		 */
		LL_AHB1_GRP1_DisableClockSleep(LL_AHB1_GRP1_PERIPH_USB2OTGHSULPI);
	}
#endif
#endif

	return 0;
}

static int priv_clock_disable(struct uhc_stm32_data *const priv)
{
	const struct uhc_stm32_config *config = priv->dev->config;
	const struct device *clk = DEVICE_DT_GET(STM32_CLOCK_CONTROL_NODE);

	int err = clock_control_off(clk, (clock_control_subsys_t *)&config->clocks[0]);

	if (err) {
		LOG_ERR("Unable to disable USB clock");
		return -EIO;
	}

	return 0;
}

static enum usb_speed priv_get_current_speed(struct uhc_stm32_data *const priv)
{
	enum usb_speed speed = SPEED_FROM_HAL_SPEED(
		HAL_HCD_GetCurrentSpeed(priv->hcd_ptr)
	);

	if (speed == USB_SPEED_INVALID) {
		LOG_ERR("Invalid USB speed returned by \"HAL_HCD_GetCurrentSpeed\"");
		__ASSERT_NO_MSG(0);

		/* falling back to low speed */
		speed = USB_SPEED_LOW;
	}

	return speed;
}

static int priv_pipe_open(struct uhc_stm32_data *const priv, uint8_t *pipe_id, uint8_t ep,
			  uint8_t dev_addr, enum usb_speed speed, uint8_t ep_type, uint16_t mps)
{
	uint8_t pipe_speed = HCD_DEVICE_SPEED_LOW;

	if (speed == USB_SPEED_LOW) {
		pipe_speed = HCD_DEVICE_SPEED_LOW;
	} else if (speed == USB_SPEED_FULL) {
		pipe_speed = HCD_DEVICE_SPEED_FULL;
	} else if (speed == USB_SPEED_HIGH) {
		pipe_speed = HCD_DEVICE_SPEED_HIGH;
	} else {
		__ASSERT_NO_MSG(0);
	}

	size_t i;
	bool found = false;

	for (i = 0; i < priv->num_bidir_pipes; i++) {
		if (priv->busy_pipe[i] == false) {
			*pipe_id = i;
			found = true;
			break;
		}
	}

	if (found == false) {
		return -ENODEV;
	}

	HAL_StatusTypeDef status = HAL_HCD_HC_Init(priv->hcd_ptr,
		*pipe_id, USB_EP_GET_IDX(ep), dev_addr, pipe_speed, ep_type, mps);

	if (status != HAL_OK) {
		return -EIO;
	}

	priv->busy_pipe[i] = true;
	return 0;
}

static int priv_pipe_close(struct uhc_stm32_data *const priv, uint8_t pipe_id)
{
	if (pipe_id > priv->num_bidir_pipes) {
		return -EINVAL;
	}

	if (priv->busy_pipe[pipe_id] != true) {
		return -EALREADY;
	}

	HAL_HCD_HC_Halt(priv->hcd_ptr, pipe_id);

	priv->busy_pipe[pipe_id] = false;

	return 0;
}

static int priv_pipe_retrieve_id(struct uhc_stm32_data *const priv, const uint8_t ep,
				 const uint8_t addr, uint8_t * const pipe_id)
{
	size_t i;
	bool found = false;

	for (i = 0; i < priv->num_bidir_pipes; i++) {
		if (priv->busy_pipe[i] == true) {
			if ((priv->hcd_ptr->hc[i].ep_num == USB_EP_GET_IDX(ep)) &&
				(priv->hcd_ptr->hc[i].dev_addr == addr)) {
				*pipe_id = i;
				found = true;
				break;
			}
		}
	}

	if (!found) {
		return -ENODEV;
	}

	return 0;
}

static void priv_pipe_close_all(struct uhc_stm32_data *const priv)
{
	size_t i;

	for (i = 0; i < priv->num_bidir_pipes; i++) {
		priv_pipe_close(priv, i);
	}
}

static void priv_pipe_init_all(struct uhc_stm32_data *const priv)
{
	size_t i;

	for (i = 0; i < priv->num_bidir_pipes; i++) {
		priv->busy_pipe[i] = false;
	}
}

static int priv_request_submit(struct uhc_stm32_data *const priv, uint8_t chan_num,
			       uint8_t direction, uint8_t ep_type, uint8_t token,
			       uint8_t *buf, uint16_t length)
{
	HAL_StatusTypeDef status = HAL_HCD_HC_SubmitRequest(priv->hcd_ptr,
		chan_num, direction, ep_type, token, buf, length, 0
	);

	if (status == HAL_OK) {
		return 0;
	} else {
		return -EIO;
	}
}

static int priv_control_setup_send(struct uhc_stm32_data *const priv, const uint8_t chan_num,
				   uint8_t *buf, uint16_t length)
{
	if (length != USB_SETUP_PACKET_SIZE) {
		return -EINVAL;
	}

	return priv_request_submit(priv, chan_num, 0, EP_TYPE_CTRL, 0, buf, length);
}

static int priv_control_status_send(struct uhc_stm32_data *const priv, const uint8_t chan_num)
{
	return priv_request_submit(priv, chan_num, 0, EP_TYPE_CTRL, 1, NULL, 0);
}

static int priv_control_status_receive(struct uhc_stm32_data *const priv, const uint8_t chan_num)
{
	return priv_request_submit(priv, chan_num, 1, EP_TYPE_CTRL, 1, NULL, 0);
}

static int priv_data_send(struct uhc_stm32_data *const priv, const uint8_t chan_num,
			  struct net_buf *const buf, const uint8_t ep_type,
			  const uint16_t maximum_packet_size)
{
	size_t tx_size = MIN(buf->len, maximum_packet_size);

	int err = priv_request_submit(priv, chan_num, 0, ep_type, 1, buf->data, tx_size);

	if (err) {
		return err;
	}

	net_buf_pull(buf, tx_size);

	return 0;
}

static int priv_data_receive(struct uhc_stm32_data *const priv, const uint8_t chan_num,
			     struct net_buf *const buf, const uint8_t ep_type)
{
	size_t len = net_buf_tailroom(buf);
	void *buffer_tail = net_buf_add(buf, len);

	int err = priv_request_submit(priv, chan_num, 1, ep_type, 1, buffer_tail, len);

	if (err) {
		net_buf_remove_mem(buf, len);
		return err;
	}

	return 0;
}

static int priv_ongoing_xfer_control_run(struct uhc_stm32_data *const priv,
					 struct uhc_transfer *const xfer)
{
	if (xfer->stage == UHC_CONTROL_STAGE_SETUP) {
		return priv_control_setup_send(priv,
			priv->ongoing_xfer_pipe_id, xfer->setup_pkt, sizeof(xfer->setup_pkt)
		);
	}

	if (xfer->buf != NULL && xfer->stage == UHC_CONTROL_STAGE_DATA) {
		if (USB_EP_DIR_IS_IN(xfer->ep)) {
			return priv_data_receive(priv,
				priv->ongoing_xfer_pipe_id, xfer->buf, USB_EP_TYPE_CONTROL
			);
		} else {
			return priv_data_send(priv,
				priv->ongoing_xfer_pipe_id, xfer->buf,
				USB_EP_TYPE_CONTROL, xfer->mps
			);
		}
	}

	if (xfer->stage == UHC_CONTROL_STAGE_STATUS) {
		if (USB_EP_DIR_IS_IN(xfer->ep)) {
			return priv_control_status_send(priv, priv->ongoing_xfer_pipe_id);
		} else {
			return priv_control_status_receive(priv, priv->ongoing_xfer_pipe_id);
		}
	}

	return -EINVAL;
}

static int priv_ongoing_xfer_bulk_run(struct uhc_stm32_data *const priv,
				      struct uhc_transfer *const xfer)
{
	if (USB_EP_DIR_IS_IN(xfer->ep)) {
		return priv_data_receive(priv,
			priv->ongoing_xfer_pipe_id, xfer->buf, USB_EP_TYPE_BULK
		);
	} else {
		return priv_data_send(priv,
			priv->ongoing_xfer_pipe_id, xfer->buf, USB_EP_TYPE_BULK, xfer->mps
		);
	}
}

static int priv_ongoing_xfer_run(struct uhc_stm32_data *const priv)
{
	if (USB_EP_GET_IDX(priv->ongoing_xfer->ep) == USB_CONTROL_ENDPOINT) {
		return priv_ongoing_xfer_control_run(priv, priv->ongoing_xfer);
	}

	/* TODO: For now all other xfer are considered as bulk transfers,
	 * add support for all type of transfer.
	 */
	return priv_ongoing_xfer_bulk_run(priv, priv->ongoing_xfer);
}

static int priv_ongoing_xfer_start_next(struct uhc_stm32_data *const priv)
{
	if (priv->ongoing_xfer != NULL) {
		/* a transfer is already ongoing */
		return 0;
	}

	priv->ongoing_xfer = uhc_xfer_get_next(priv->dev);

	if (priv->ongoing_xfer == NULL) {
		/* there is no xfer enqueued */
		return 0;
	}

	priv->ongoing_xfer_attempts = 0;

	/* Note: net_buf API does not offer a proper way to directly save the net_buf state
	 * so it's internal normally hidden net_buf_simple is saved instead
	 */
	net_buf_simple_save(&(priv->ongoing_xfer->buf->b), &(priv->ongoing_xfer_buf_save));

	/* Retrieve corresponding pipe or open one if there is none.
	 * Note: This is temporary as the upper code does not manage pipes yet.
	 */
	int err = priv_pipe_retrieve_id(priv,
		priv->ongoing_xfer->ep,
		priv->ongoing_xfer->addr,
		&(priv->ongoing_xfer_pipe_id)
	);

	if (err) {
		__ASSERT_NO_MSG(err == -ENODEV);

		uint8_t ep_type = EP_TYPE_BULK;

		if (USB_EP_GET_IDX(priv->ongoing_xfer->ep) == USB_CONTROL_ENDPOINT) {
			/* TODO : handle other type of transfers */
			ep_type = EP_TYPE_CTRL;
		}

		enum usb_speed speed = priv_get_current_speed(priv);

		err = priv_pipe_open(priv,
			&(priv->ongoing_xfer_pipe_id),
			priv->ongoing_xfer->ep,
			priv->ongoing_xfer->addr,
			speed, ep_type,
			priv->ongoing_xfer->mps
		);

		if (err) {
			return err;
		}
	}

	return priv_ongoing_xfer_run(priv);
}

static void priv_ongoing_xfer_end(struct uhc_stm32_data *const priv, const int err)
{
	if (priv->ongoing_xfer == NULL) {
		return;
	}

	/* Close pipe if it is not the control pipe as it is supposed to stay oppened.
	 * Note: This is temporary as the upper code does not manage pipes yet.
	 */
	if (USB_EP_GET_IDX(priv->ongoing_xfer->ep) != USB_CONTROL_ENDPOINT) {
		int ret = priv_pipe_close(priv, priv->ongoing_xfer_pipe_id);
		(void) ret;
		__ASSERT_NO_MSG(ret == 0);
	}

	uhc_xfer_return(priv->dev, priv->ongoing_xfer, err);

	priv->ongoing_xfer = NULL;
}

static void priv_ongoing_xfer_handle_timeout(struct uhc_stm32_data *const priv, uint16_t frame_cpt)
{
	if (priv->ongoing_xfer == NULL) {
		return;
	}

	if (priv->ongoing_xfer->timeout > frame_cpt) {
		priv->ongoing_xfer->timeout -= frame_cpt;
	} else {
		priv_ongoing_xfer_end(priv, -ETIMEDOUT);

		/* there may be more xfer to handle, submit a work to handle the next one */
		k_work_submit_to_queue(&priv->work_queue, &priv->on_schedule_new_xfer_work);
	}
}

static bool priv_ongoing_xfer_handle_err(struct uhc_stm32_data *const priv)
{
	/* increase attempt count */
	priv->ongoing_xfer_attempts++;

	if (priv->ongoing_xfer_attempts >= USB_NB_MAX_XFER_ATTEMPTS) {
		/* transmission failed too many times, cancel it. */
		priv_ongoing_xfer_end(priv, -EIO);
		return true;
	}

	return false;
}

static void priv_ongoing_xfer_control_stage_update(struct uhc_stm32_data *const priv)
{
	/* the last transfer block succeeded, reset the failed attempts counter */
	priv->ongoing_xfer_attempts = 0;

	if (priv->ongoing_xfer->stage == UHC_CONTROL_STAGE_SETUP) {
		if (priv->ongoing_xfer->buf != NULL) {
			/* next state is data stage */
			priv->ongoing_xfer->stage = UHC_CONTROL_STAGE_DATA;
		} else {
			/* there is no data so jump directly to the status stage */
			priv->ongoing_xfer->stage = UHC_CONTROL_STAGE_STATUS;
		}
	} else if (priv->ongoing_xfer->stage == UHC_CONTROL_STAGE_DATA) {
		if (USB_EP_DIR_IS_IN(priv->ongoing_xfer->ep)) {
			/* no more data left to receive, go to the status stage */
			priv->ongoing_xfer->stage = UHC_CONTROL_STAGE_STATUS;
		} else { /* OUT direction */
			if (priv->ongoing_xfer->buf->len == 0) {
				/* no more data left to send, go to the status stage */
				priv->ongoing_xfer->stage = UHC_CONTROL_STAGE_STATUS;
			} else {
				/* The transmission succeeded so far. Save the actual net_buf state
				 * in case the following partial data transmission fails so we can
				 * try to send it again.
				 */
				net_buf_simple_save(
					&(priv->ongoing_xfer->buf->b),
					&(priv->ongoing_xfer_buf_save)
				);
			}
		}
	} else if (priv->ongoing_xfer->stage == UHC_CONTROL_STAGE_STATUS) {
		/* transfer is completed */
		priv_ongoing_xfer_end(priv, 0);
	} else {
		/* this is not supposed to happen */
		__ASSERT_NO_MSG(0);
	}
}

static int priv_ongoing_xfer_control_update(struct uhc_stm32_data *const priv)
{
	HCD_URBStateTypeDef urb_state =
		HAL_HCD_HC_GetURBState(priv->hcd_ptr, priv->ongoing_xfer_pipe_id);

	if (urb_state == URB_DONE) {
		priv_ongoing_xfer_control_stage_update(priv);
	} else if (urb_state == URB_ERROR) {
		priv_ongoing_xfer_end(priv, -EIO);
	} else if (urb_state == URB_STALL) {
		if (priv->ongoing_xfer->stage == UHC_CONTROL_STAGE_SETUP) {
			/* something strange occurred, this is out of usb specs */
			priv_ongoing_xfer_end(priv, -EILSEQ);
		} else {
			priv_ongoing_xfer_end(priv, -EPIPE);
		}
	} else if (urb_state == URB_NYET) {
		/* something strange occurred, this is not supposed to happen */
		priv_ongoing_xfer_end(priv, -EILSEQ);
	} else if (urb_state == URB_NOTREADY) {
		if (priv->ongoing_xfer->stage == UHC_CONTROL_STAGE_SETUP) {
			bool xfer_is_aborted = priv_ongoing_xfer_handle_err(priv);

			if (!xfer_is_aborted) {
				/* restart from the SETUP stage */
				priv->ongoing_xfer->stage = UHC_CONTROL_STAGE_SETUP;
			}
		} else if (priv->ongoing_xfer->stage == UHC_CONTROL_STAGE_DATA) {
			if (USB_EP_DIR_IS_OUT(priv->ongoing_xfer->ep)) {
				/* Restore net buf to the last state to be able
				 * to re-run the transmission.
				 */
				net_buf_simple_restore(
					&(priv->ongoing_xfer->buf->b),
					&(priv->ongoing_xfer_buf_save)
				);
			} else {
				/* nothing to do, retry is done automatically */
				return 0;
			}
		} else if (priv->ongoing_xfer->stage == UHC_CONTROL_STAGE_STATUS) {
			if (USB_EP_DIR_IS_OUT(priv->ongoing_xfer->ep)) {
				/* nothing to do, retry is done automatically */
				return 0;
			}
		} else {
			__ASSERT_NO_MSG(0);
		}
	} else { /* URB_IDLE */
		/* just wait for the next update */
		return 0;
	}

	if (priv->ongoing_xfer != NULL) {
		return priv_ongoing_xfer_run(priv);
	}

	return 0;
}

static int priv_ongoing_xfer_bulk_update(struct uhc_stm32_data *const priv)
{
	HCD_URBStateTypeDef urb_state =
		HAL_HCD_HC_GetURBState(priv->hcd_ptr, priv->ongoing_xfer_pipe_id);

	if (urb_state == URB_DONE) {
		if (USB_EP_DIR_IS_IN(priv->ongoing_xfer->ep) &&
		    net_buf_tailroom(priv->ongoing_xfer->buf) == 0) {
			/* no more data left to receive, transmission succeeded */
			priv_ongoing_xfer_end(priv, 0);
		} else if (USB_EP_DIR_IS_OUT(priv->ongoing_xfer->ep) &&
			   priv->ongoing_xfer->buf->len == 0) {
			/* no more data left to send, transmission succeeded */
			priv_ongoing_xfer_end(priv, 0);
		}
	} else if (urb_state == URB_NOTREADY) {
		if (USB_EP_DIR_IS_OUT(priv->ongoing_xfer->ep)) {
			/* Restore net buf to the last state, to be able to re-run
			 * this part of the transmission.
			 */
			net_buf_simple_restore(
				&(priv->ongoing_xfer->buf->b), &(priv->ongoing_xfer_buf_save)
			);
		} else {
			/* nothing to do, retry is done automatically */
			return 0;
		}
	} else if (urb_state == URB_NYET) {
		/* FIXME: do something with this if needed */
		return 0;
	} else if (urb_state == URB_ERROR) {
		priv_ongoing_xfer_end(priv, -EIO);
	} else if (urb_state == URB_STALL) {
		priv_ongoing_xfer_end(priv, -EPIPE);
	} else { /* URB_IDLE */
		/* just wait for the next update */
		return 0;
	}

	if (priv->ongoing_xfer != NULL) {
		/* transfer is not completed yet, continue the transmission */
		return priv_ongoing_xfer_run(priv);
	}

	return 0;
}

static int priv_ongoing_xfer_update(struct uhc_stm32_data *const priv)
{
	if (priv->ongoing_xfer == NULL) {
		/* there is no ongoing transfer */
		return 0;
	}

	int err;

	if (USB_EP_GET_IDX(priv->ongoing_xfer->ep) == USB_CONTROL_ENDPOINT) {
		err = priv_ongoing_xfer_control_update(priv);
	} else {
		/* TODO: For now all other xfer are considered as bulk transfers,
		 * add support for all type of transfer.
		 */
		err = priv_ongoing_xfer_bulk_update(priv);
	}

	if (err) {
		return err;
	}

	if (priv->ongoing_xfer == NULL) {
		/* there may be more xfer to handle, submit a work to handle the next one */
		k_work_submit_to_queue(&priv->work_queue, &priv->on_schedule_new_xfer_work);
	}

	return 0;
}

static void priv_bus_reset(struct uhc_stm32_data *const priv)
{
#if defined(USB_OTG_FS) || defined(USB_OTG_HS)
	if (IS_USB_OTG_FS_DEVICE(priv->dev) || IS_USB_OTG_HS_DEVICE(priv->dev)) {
		/* Define and set USBx_BASE to be equal to the first USB register address,
		 * as USBx_HPRT0 is an ST defined macro which depends on it.
		 */
		uint32_t USBx_BASE = (uint32_t)priv->hcd_ptr->Instance;

		__IO uint32_t hprt0_value = 0U;

		hprt0_value = USBx_HPRT0;

		/* avoid interfering with some special bits */
		hprt0_value &= ~(
			USB_OTG_HPRT_PENA |
			USB_OTG_HPRT_PCDET |
		    USB_OTG_HPRT_PENCHNG |
			USB_OTG_HPRT_POCCHNG |
			USB_OTG_HPRT_PSUSP
		);

		/* set PRST bit */
		USBx_HPRT0 = (USB_OTG_HPRT_PRST | hprt0_value);
		k_msleep(100);

		/* clear PRST bit */
		USBx_HPRT0 = ((~USB_OTG_HPRT_PRST) & hprt0_value);
		k_msleep(10);
	}
#endif

#if defined(USB_DRD_FS)
	if (IS_USB_DRD_FS_DEVICE(priv->dev)) {
		((USB_DRD_TypeDef *) priv->hcd_ptr->Instance)->CNTR |= USB_CNTR_USBRST;
		k_msleep(100);
		((USB_DRD_TypeDef *) priv->hcd_ptr->Instance)->CNTR &= ~USB_CNTR_USBRST;
		k_msleep(30);
	}
#endif
}

static void priv_clear(struct uhc_stm32_data *const priv)
{
	/* abort a potentially ongoing transfer */
	priv_ongoing_xfer_end(priv, -ECANCELED);

	/* cancel any pontential pending work */
	k_work_cancel_delayable(&priv->delayed_enum_reset_work);
	k_work_cancel(&priv->on_reset_work);
	k_work_cancel(&priv->on_sof_work);
	k_work_cancel(&priv->on_xfer_update_work);
	k_work_cancel(&priv->on_schedule_new_xfer_work);

	priv_pipe_close_all(priv);
}

static int uhc_stm32_lock(const struct device *dev)
{
	return uhc_lock_internal(dev, K_FOREVER);
}

static int uhc_stm32_unlock(const struct device *dev)
{
	return uhc_unlock_internal(dev);
}

static int uhc_stm32_init(const struct device *dev)
{
	struct uhc_stm32_data *priv = uhc_get_private(dev);
	const struct uhc_stm32_config *config = dev->config;

	int err = 0;

	err = pinctrl_apply_state(config->pcfg, PINCTRL_STATE_DEFAULT);
	if (err < 0) {
		LOG_ERR("USB pinctrl setup failed (%d)", err);
		return err;
	}

	if (config->vbus_enable_gpio.port) {
		if (!gpio_is_ready_dt(&config->vbus_enable_gpio)) {
			LOG_ERR("vbus enable gpio not ready");
			return -ENODEV;
		}
		err = gpio_pin_configure_dt(&config->vbus_enable_gpio, GPIO_OUTPUT_INACTIVE);
		if (err) {
			LOG_ERR("Failed to configure vbus enable gpio (%d)", err);
			return err;
		}
	}

	if (config->phy == PHY_EXTERNAL_ULPI && config->ulpi_reset_gpio.port) {
		if (!gpio_is_ready_dt(&config->ulpi_reset_gpio)) {
			LOG_ERR("ULPI reset gpio is not ready");
			return -ENODEV;
		}

		/* configure the reset pin and activate it */
		err = gpio_pin_configure_dt(&config->ulpi_reset_gpio, GPIO_OUTPUT_ACTIVE);
		if (err) {
			LOG_ERR("Failed to configure ULPI reset gpio (%d)", err);
			return err;
		}

		k_usleep(10);

		/* release the reset pin */
		err = gpio_pin_set_dt(&config->ulpi_reset_gpio, 0);
		if (err) {
			LOG_ERR("Failed to release ULPI reset pin (%d)", err);
			return err;
		}
	}

#if defined(CONFIG_SOC_SERIES_STM32H7X)
	if (config->phy != PHY_EXTERNAL_ULPI) {
		LL_PWR_EnableUSBVoltageDetector();
		WAIT_FOR(
			(LL_PWR_IsActiveFlag_USB() == 0),
			1000,
			k_yield()
		);
	}
#endif

	err = priv_clock_enable(priv);
	if (err != 0) {
		LOG_ERR("Error enabling clock(s)");
		return -EIO;
	}

	HAL_StatusTypeDef status = HAL_HCD_Init(priv->hcd_ptr);

	if (status != HAL_OK) {
		LOG_ERR("HCD_Init failed, %d", (int)status);
		return -EIO;
	}

	priv_pipe_init_all(priv);

	return 0;
}

static int uhc_stm32_enable(const struct device *dev)
{
	struct uhc_stm32_data *priv = uhc_get_private(dev);
	const struct uhc_stm32_config *config = dev->config;

	priv->frame_number = USB_GetCurrentFrame(priv->hcd_ptr->Instance);

	irq_enable(config->irq);

	HAL_StatusTypeDef status = HAL_HCD_Start(priv->hcd_ptr);

	if (status != HAL_OK) {
		LOG_ERR("HCD_Start failed (%d)", (int)status);
		return -EIO;
	}

	if (config->vbus_enable_gpio.port) {
		int err = gpio_pin_set_dt(&config->vbus_enable_gpio, 1);

		if (err) {
			LOG_ERR("Failed to enable vbus power (%d)", err);
			return err;
		}
	}

	return 0;
}

static int uhc_stm32_disable(const struct device *dev)
{
	struct uhc_stm32_data *priv = uhc_get_private(dev);
	const struct uhc_stm32_config *config = dev->config;

	if (config->vbus_enable_gpio.port) {
		int err = gpio_pin_set_dt(&config->vbus_enable_gpio, 0);

		if (err) {
			LOG_ERR("Failed to disable vbus power (%d)", err);
			return err;
		}
	}

	USB_DriveVbus(priv->hcd_ptr->Instance, 0);

	HAL_StatusTypeDef status = HAL_HCD_Stop(priv->hcd_ptr);

	if (status != HAL_OK) {
		LOG_ERR("HCD_Stop failed (%d)", (int)status);
		return -EIO;
	}

	irq_disable(config->irq);

	/* clear out everything */
	priv_clear(priv);

	if (priv->state == STM32_UHC_STATE_READY) {
		/* let higher level code know that the device is not reachable anymore */
		uhc_submit_event(priv->dev, UHC_EVT_DEV_REMOVED, 0);
	}

	priv->state = STM32_UHC_STATE_DISCONNECTED;

	return 0;
}

static int uhc_stm32_shutdown(const struct device *dev)
{
	struct uhc_stm32_data *priv = uhc_get_private(dev);

	HAL_StatusTypeDef status = HAL_HCD_DeInit(priv->hcd_ptr);

	if (status != HAL_OK) {
		LOG_ERR("HAL_HCD_DeInit failed (%d)", (int)status);
		return -EIO;
	}

	int err = priv_clock_disable(priv);

	if (err) {
		LOG_ERR("Failed to disable USB clock (%d)", err);
		return err;
	}

	return 0;
}

static int uhc_stm32_bus_reset(const struct device *dev)
{
	struct uhc_stm32_data *priv = uhc_get_private(dev);

	if (priv->state == STM32_UHC_STATE_SPEED_ENUM) {
		/* a reset is already planned */
		return -EBUSY;
	}

	/* clear everything */
	priv_clear(priv);

	/* perform a reset */
	priv_bus_reset(priv);

	return 0;
}

static int uhc_stm32_sof_enable(const struct device *dev)
{
	/* nothing to do */
	return 0;
}

static int uhc_stm32_bus_suspend(const struct device *dev)
{
	struct uhc_stm32_data *priv = uhc_get_private(dev);

#if defined(USB_OTG_FS) || defined(USB_OTG_HS)
	if (IS_USB_OTG_FS_DEVICE(dev) || IS_USB_OTG_HS_DEVICE(dev)) {
		const struct uhc_stm32_config *config = priv->dev->config;

		if (config->phy == PHY_EXTERNAL_ULPI) {
			/* TODO */
			return -ENOSYS;
		}

		/* Define and set USBx_BASE to be equal to the first USB register address,
		 * as USBx_HPRT0 and USBx_PCGCCTL are ST defined macros which depend on it.
		 */
		uint32_t USBx_BASE = (uint32_t)priv->hcd_ptr->Instance;

		__IO uint32_t hprt0_value = USBx_HPRT0;

		if (hprt0_value & USB_OTG_HPRT_PSUSP) {
			return -EALREADY;
		}

		/* avoid interfering with some special bits */
		hprt0_value &= ~(
			USB_OTG_HPRT_PENA |
			USB_OTG_HPRT_PCDET |
			USB_OTG_HPRT_PENCHNG |
			USB_OTG_HPRT_POCCHNG
		);

		/* set suspend bit bit */
		USBx_HPRT0 = (USB_OTG_HPRT_PSUSP | hprt0_value);

		/* stop PHY clock */
		USBx_PCGCCTL |= USB_OTG_PCGCR_STPPCLK;
	}
#endif

#if defined(USB_DRD_FS)
	if (IS_USB_DRD_FS_DEVICE(dev)) {
		/* TODO */
		return -ENOSYS;
	}
#endif

	uhc_submit_event(dev, UHC_EVT_SUSPENDED, 0);

	return 0;
}

static int uhc_stm32_bus_resume(const struct device *dev)
{
	struct uhc_stm32_data *priv = uhc_get_private(dev);

#if defined(USB_OTG_FS) || defined(USB_OTG_HS)
	if (IS_USB_OTG_FS_DEVICE(dev) || IS_USB_OTG_HS_DEVICE(dev)) {
		const struct uhc_stm32_config *config = priv->dev->config;

		if (config->phy == PHY_EXTERNAL_ULPI) {
			/* TODO */
			return -ENOSYS;
		}

		/* Define and set USBx_BASE to be equal to the first USB register address,
		 * as USBx_HPRT0 and USBx_PCGCCTL are ST defined macros which depend on it.
		 */
		uint32_t USBx_BASE = (uint32_t)priv->hcd_ptr->Instance;

		__IO uint32_t hprt0_value = USBx_HPRT0;

		if (hprt0_value & USB_OTG_HPRT_PRES) {
			return -EBUSY;
		}

		/* restart PHY clock */
		USBx_PCGCCTL &= ~USB_OTG_PCGCR_STPPCLK;

		/* avoid interfering with some special HPRT bits and clear the suspend bit */
		hprt0_value &= ~(
			USB_OTG_HPRT_PENA |
			USB_OTG_HPRT_PCDET |
			USB_OTG_HPRT_PENCHNG |
			USB_OTG_HPRT_POCCHNG |
			USB_OTG_HPRT_PSUSP
		);

		/* set resume bit */
		USBx_HPRT0 = (USB_OTG_HPRT_PRES | hprt0_value);

		/* USB specifications says resume must be driven at least 20ms */
		k_msleep(20 + 1);

		/* clear resume bit */
		USBx_HPRT0 = ((~USB_OTG_HPRT_PRES) & hprt0_value);
		k_msleep(10);
	}
#endif

#if defined(USB_DRD_FS)
	if (IS_USB_DRD_FS_DEVICE(dev)) {
		/* TODO */
		return -ENOSYS;
	}
#endif

	uhc_submit_event(dev, UHC_EVT_RESUMED, 0);

	return 0;
}

static int uhc_stm32_ep_enqueue(const struct device *dev, struct uhc_transfer *const xfer)
{
	struct uhc_stm32_data *priv = uhc_get_private(dev);

	xfer->stage = UHC_CONTROL_STAGE_SETUP;

	int err = uhc_xfer_append(dev, xfer);

	if (err) {
		return err;
	}

	k_work_submit_to_queue(&priv->work_queue, &priv->on_schedule_new_xfer_work);

	return 0;
}

static int uhc_stm32_ep_dequeue(const struct device *dev, struct uhc_transfer *const xfer)
{
	/* TODO */
	return 0;
}

static const struct uhc_api uhc_stm32_api = {
	.lock = uhc_stm32_lock,
	.unlock = uhc_stm32_unlock,

	.init = uhc_stm32_init,
	.enable = uhc_stm32_enable,
	.disable = uhc_stm32_disable,
	.shutdown = uhc_stm32_shutdown,

	.bus_reset = uhc_stm32_bus_reset,
	.sof_enable = uhc_stm32_sof_enable,
	.bus_suspend = uhc_stm32_bus_suspend,
	.bus_resume = uhc_stm32_bus_resume,

	.ep_enqueue = uhc_stm32_ep_enqueue,
	.ep_dequeue = uhc_stm32_ep_dequeue,
};

void priv_on_port_connect_disconnect(struct k_work *work)
{
	struct uhc_stm32_data *priv =
		CONTAINER_OF(work, struct uhc_stm32_data, on_connect_disconnect_work);

	uhc_stm32_lock(priv->dev);

	bool connected = false;

#if defined(USB_OTG_FS) || defined(USB_OTG_HS)
	if (IS_USB_OTG_FS_DEVICE(priv->dev) || IS_USB_OTG_HS_DEVICE(priv->dev)) {
		/* Define and set USBx_BASE to be equal to the first USB register address,
		 * as USBx_HPRT0 is an ST defined macro which depends on it.
		 */
		uint32_t USBx_BASE = (uint32_t)priv->hcd_ptr->Instance;

		if (READ_BIT(USBx_HPRT0, USB_OTG_HPRT_PCSTS)) {
			connected = true;
		} else {
			connected = false;
		}
	}
#endif

#if defined(USB_DRD_FS)
	if (IS_USB_DRD_FS_DEVICE(priv->dev)) {
		/* TODO */
		return;
	}
#endif

	if (connected) {
		if (priv->state == STM32_UHC_STATE_READY) {
			/* A spurious disconnection occurred, cancel ongoing transfer,
			 * clear pending works and let higher level code know that a
			 * disconnection occurred.
			 */
			priv_clear(priv);
			uhc_submit_event(priv->dev, UHC_EVT_DEV_REMOVED, 0);
		}

		/* launch a speed enumeration. */
		priv->state = STM32_UHC_STATE_SPEED_ENUM;
		k_work_reschedule_for_queue(&priv->work_queue,
			&priv->delayed_enum_reset_work, K_MSEC(200)
		);
	} else {
		if (priv->state == STM32_UHC_STATE_READY) {
			/* cancel ongoing transfer and clear pending works */
			priv_clear(priv);

			/* let higher level code know that a disconnection occurred */
			uhc_submit_event(priv->dev, UHC_EVT_DEV_REMOVED, 0);
		} else if (priv->state == STM32_UHC_STATE_SPEED_ENUM) {
			/* cancel the pending enumeration reset */
			k_work_cancel_delayable(&priv->delayed_enum_reset_work);
		} else {
			/* a spurious connection occurred, nothing to do */
		}

		priv->state = STM32_UHC_STATE_DISCONNECTED;
	}

	uhc_stm32_unlock(priv->dev);
}

void priv_on_reset(struct k_work *work)
{
	struct uhc_stm32_data *priv = CONTAINER_OF(work, struct uhc_stm32_data, on_reset_work);

	uhc_stm32_lock(priv->dev);

	if (priv->state == STM32_UHC_STATE_SPEED_ENUM) {
		priv->state = STM32_UHC_STATE_READY;

		enum usb_speed current_speed = priv_get_current_speed(priv);

		if (current_speed == USB_SPEED_LOW) {
			uhc_submit_event(priv->dev, UHC_EVT_DEV_CONNECTED_LS, 0);
		} else if (current_speed == USB_SPEED_FULL) {
			uhc_submit_event(priv->dev, UHC_EVT_DEV_CONNECTED_FS, 0);
		} else {
			uhc_submit_event(priv->dev, UHC_EVT_DEV_CONNECTED_HS, 0);
		}
	} else {
		/* let higher level code know that a reset occurred */
		uhc_submit_event(priv->dev, UHC_EVT_RESETED, 0);
	}

	uhc_stm32_unlock(priv->dev);
}

void priv_on_sof(struct k_work *work)
{
	struct uhc_stm32_data *priv = CONTAINER_OF(work, struct uhc_stm32_data, on_sof_work);

	uhc_stm32_lock(priv->dev);

	uint16_t current_frame_number = USB_GetCurrentFrame(priv->hcd_ptr->Instance);

	uint16_t frame_cpt = 0;

	if (current_frame_number >= priv->frame_number) {
		frame_cpt = current_frame_number - priv->frame_number;
	} else {
		/* number of frame ticks before the overflow */
		frame_cpt = (UHC_STM32_MAX_FRAME_NUMBER - priv->frame_number);
		/* the tick responsible for the overflow */
		frame_cpt += 1;
		/* number of frame ticks since the overflow */
		frame_cpt += current_frame_number;
	}

	priv->frame_number = current_frame_number;

	if (priv->state == STM32_UHC_STATE_READY) {
		priv_ongoing_xfer_handle_timeout(priv, frame_cpt);
	}

	uhc_stm32_unlock(priv->dev);
}

void priv_on_xfer_update(struct k_work *work)
{
	struct uhc_stm32_data *priv =
		CONTAINER_OF(work, struct uhc_stm32_data, on_xfer_update_work);

	uhc_stm32_lock(priv->dev);

	if (priv->state != STM32_UHC_STATE_READY) {
		uhc_stm32_unlock(priv->dev);
		return;
	}

	int err = priv_ongoing_xfer_update(priv);

	if (err) {
		/* something went wrong, cancel the ongoing transfer and notify the upper layer */
		priv_ongoing_xfer_end(priv, -ECANCELED);
		uhc_submit_event(priv->dev, UHC_EVT_ERROR, err);
	}

	uhc_stm32_unlock(priv->dev);
}

void priv_on_schedule_new_xfer(struct k_work *work)
{
	struct uhc_stm32_data *priv =
		CONTAINER_OF(work, struct uhc_stm32_data, on_schedule_new_xfer_work);

	uhc_stm32_lock(priv->dev);

	if (priv->state != STM32_UHC_STATE_READY) {
		uhc_stm32_unlock(priv->dev);
		return;
	}

	int err = priv_ongoing_xfer_start_next(priv);

	if (err) {
		/* something went wrong, cancel the ongoing transfer and notify the upper layer */
		priv_ongoing_xfer_end(priv, -ECANCELED);
		uhc_submit_event(priv->dev, UHC_EVT_ERROR, err);
	}

	uhc_stm32_unlock(priv->dev);
}

void priv_delayed_enumeration_reset(struct k_work *work)
{
	struct k_work_delayable *delayable_work = k_work_delayable_from_work(work);
	struct uhc_stm32_data *priv =
		CONTAINER_OF(delayable_work, struct uhc_stm32_data, delayed_enum_reset_work);

	uhc_stm32_lock(priv->dev);

	if (priv->state != STM32_UHC_STATE_SPEED_ENUM) {
		uhc_stm32_unlock(priv->dev);
		return;
	}

	/* Note: This function takes time and will delay other submitted work */
	priv_bus_reset(priv);

	uhc_stm32_unlock(priv->dev);
}

static void uhc_stm32_driver_init_common(const struct device *dev)
{
	struct uhc_stm32_data *priv = uhc_get_private(dev);

	priv->state = STM32_UHC_STATE_DISCONNECTED;

	k_work_queue_init(&priv->work_queue);
	k_work_init(&priv->on_connect_disconnect_work, priv_on_port_connect_disconnect);
	k_work_init(&priv->on_reset_work, priv_on_reset);
	k_work_init(&priv->on_sof_work, priv_on_sof);
	k_work_init(&priv->on_xfer_update_work, priv_on_xfer_update);
	k_work_init(&priv->on_schedule_new_xfer_work, priv_on_schedule_new_xfer);
	k_work_init_delayable(&priv->delayed_enum_reset_work, priv_delayed_enumeration_reset);
}


#define UHC_STM32_INIT_FUNC_NAME(node_id) uhc_stm32_driver_init_##node_id

#define UHC_STM32_INIT_FUNC(node_id)                                                               \
	static int UHC_STM32_INIT_FUNC_NAME(node_id)(const struct device *dev)                     \
	{                                                                                          \
		struct uhc_stm32_data *priv = uhc_get_private(dev);                                \
		                                                                                   \
		uhc_stm32_driver_init_common(dev);                                                 \
		                                                                                   \
		k_work_queue_start(&priv->work_queue,                                              \
		   uhc_work_thread_stack_##node_id,                                                \
		   K_THREAD_STACK_SIZEOF(uhc_work_thread_stack_##node_id),                         \
		   K_PRIO_COOP(2),                                                                 \
		   NULL                                                                            \
		);                                                                                 \
	                                                                                           \
		IRQ_CONNECT(DT_IRQN(node_id),                                                      \
			DT_IRQ(node_id, priority), uhc_stm32_irq,                                  \
			DEVICE_DT_GET(node_id), 0                                                  \
		);                                                                                 \
	                                                                                           \
		return 0;                                                                          \
	}

#define STM32_INIT_COMMON(node_id)                                                                 \
	PINCTRL_DT_DEFINE(node_id);                                                                \
	                                                                                           \
	BUILD_ASSERT(DT_NUM_CLOCKS(node_id) <= DT_CLOCKS_PROP_MAX_LEN,                             \
		"Invalid number of element in \"clock\" property in device tree");                 \
	BUILD_ASSERT(DT_PROP(node_id, num_host_channels) <= UHC_STM32_NB_MAX_PIPE,                 \
		"Invalid number of host channels");                                                \
	BUILD_ASSERT(DT_PHY_INTERFACE_TYPE(node_id) != PHY_INVALID,                                \
		"Unsupported or incompatible USB PHY defined in device tree");                     \
	BUILD_ASSERT(DT_MAX_SPEED(node_id) != USB_SPEED_INVALID,                                   \
		"Invalid usb speed");                                                              \
	                                                                                           \
	static const struct uhc_stm32_config uhc_config_##node_id = {                              \
		.irq = DT_IRQN(node_id),                                                           \
		.pcfg = PINCTRL_DT_DEV_CONFIG_GET(node_id),                                        \
		.vbus_enable_gpio = GPIO_DT_SPEC_GET_OR(node_id, vbus_gpios, {0}),                 \
		.clocks = STM32_DT_CLOCKS(node_id),                                                \
		.num_clock = DT_NUM_CLOCKS(node_id),                                               \
		.phy = DT_PHY_INTERFACE_TYPE(node_id),                                             \
		.ulpi_reset_gpio = GPIO_DT_SPEC_GET_OR(DT_PHY(node_id), reset_gpios, {0}),         \
	};                                                                                         \
	                                                                                           \
	static HCD_HandleTypeDef uhc_stm32_hcd_##node_id = {                                       \
		.pData = (void *) DEVICE_DT_GET(node_id),                                          \
		.Instance = (HCD_TypeDef *) DT_REG_ADDR(node_id),                                  \
		.Init = {                                                                          \
			.Host_channels = DT_PROP(node_id, num_host_channels),                      \
			.dma_enable = 0,                                                           \
			.battery_charging_enable = DISABLE,                                        \
			.use_external_vbus = DISABLE,                                              \
			.vbus_sensing_enable = (                                                   \
				(DT_PHY_INTERFACE_TYPE(node_id) == PHY_EXTERNAL_ULPI) ?            \
					ENABLE : DISABLE                                           \
			),                                                                         \
			.speed = SPEED_TO_HCD_INIT_SPEED(DT_MAX_SPEED(node_id)),                   \
			.phy_itface = uhc_config_##node_id.phy,                                    \
		},                                                                                 \
	};                                                                                         \
	                                                                                           \
	static struct uhc_stm32_data uhc_priv_data_##node_id = {                                   \
		.hcd_ptr = &(uhc_stm32_hcd_##node_id),                                             \
		.num_bidir_pipes = DT_PROP(node_id, num_host_channels),                            \
		.dev = DEVICE_DT_GET(node_id),                                                     \
	};                                                                                         \
	                                                                                           \
	static struct uhc_data uhc_data_##node_id = {                                              \
		.mutex = Z_MUTEX_INITIALIZER(uhc_data_##node_id.mutex),                            \
		.caps = {                                                                          \
			.hs = ((DT_MAX_SPEED(node_id) >= USB_SPEED_HIGH) ? 1 : 0),                 \
		},                                                                                 \
		.priv = &uhc_priv_data_##node_id,                                                  \
	};                                                                                         \
	                                                                                           \
	static K_THREAD_STACK_DEFINE(                                                              \
		uhc_work_thread_stack_##node_id,                                                   \
		CONFIG_UHC_STM32_DRV_THREAD_STACK_SIZE                                             \
	);                                                                                         \
	                                                                                           \
	UHC_STM32_INIT_FUNC(node_id);                                                              \
	                                                                                           \
	DEVICE_DT_DEFINE(node_id, UHC_STM32_INIT_FUNC_NAME(node_id), NULL,                         \
			 &uhc_data_##node_id, &uhc_config_##node_id,                               \
			 POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_DEVICE,                          \
			 &uhc_stm32_api);


#define STM32_USB_INIT(node_id)                                                                    \
	BUILD_ASSERT(0, "Not implemented");                                                        \
	BUILD_ASSERT(DT_NODE_HAS_COMPAT(DT_PHY(node_id), DT_PHY_COMPAT_EMBEDDED_FS),               \
		"Invalid PHY defined in device tree");                                             \
	                                                                                           \
	STM32_INIT_COMMON(node_id)

#define STM32_OTGFS_INIT(node_id)                                                                  \
	BUILD_ASSERT(DT_NODE_HAS_COMPAT(DT_PHY(node_id), DT_PHY_COMPAT_EMBEDDED_FS),               \
		"Invalid PHY defined in device tree");                                             \
	                                                                                           \
	STM32_INIT_COMMON(node_id)

#define STM32_OTGHS_INIT(node_id) STM32_INIT_COMMON(node_id)


DT_FOREACH_STATUS_OKAY(DT_COMPAT_STM32_USB, STM32_USB_INIT)
DT_FOREACH_STATUS_OKAY(DT_COMPAT_STM32_OTGFS, STM32_OTGFS_INIT)
DT_FOREACH_STATUS_OKAY(DT_COMPAT_STM32_OTGHS, STM32_OTGHS_INIT)
