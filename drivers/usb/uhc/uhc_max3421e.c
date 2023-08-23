/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * MAX3421E USB Peripheral/Host Controller with SPI Interface.
 * NOTE: Driver supports only host mode yet.
 */

#define DT_DRV_COMPAT maxim_max3421e_spi

#include <string.h>

#include <zephyr/kernel.h>
#include <zephyr/init.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/spi.h>
#include <zephyr/drivers/usb/uhc.h>

#include "uhc_common.h"
#include "uhc_max3421e.h"

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(max3421e, CONFIG_UHC_DRIVER_LOG_LEVEL);

static K_KERNEL_STACK_DEFINE(drv_stack, CONFIG_MAX3421E_THREAD_STACK_SIZE);
static struct k_thread drv_stack_data;

#define MAX3421E_STATE_BUS_RESET	0
#define MAX3421E_STATE_BUS_RESUME	1

struct max3421e_data {
	struct gpio_callback gpio_cb;
	struct uhc_transfer *last_xfer;
	struct k_sem irq_sem;
	atomic_t state;
	uint16_t tog_in;
	uint16_t tog_out;
	uint8_t addr;
	uint8_t hirq;
	uint8_t hien;
	uint8_t mode;
	uint8_t hxfr;
	uint8_t hrsl;
};

struct max3421e_config {
	struct spi_dt_spec dt_spi;
	struct gpio_dt_spec dt_int;
	struct gpio_dt_spec dt_rst;
};

static int max3421e_read_hirq(const struct device *dev,
			      const uint8_t reg,
			      uint8_t *const data,
			      const uint32_t count,
			      bool update_hirq)
{
	struct max3421e_data *priv = uhc_get_private(dev);
	const struct max3421e_config *config = dev->config;
	uint8_t cmd = MAX3421E_CMD_SPI_READ(reg);
	uint8_t hirq;
	int ret;

	const struct spi_buf cmd_buf = {
		.buf = &cmd,
		.len = sizeof(cmd)
	};
	const struct spi_buf rx_buf[] = {
		{
			.buf = &hirq,
			.len = sizeof(hirq)
		},
		{
			.buf = data,
			.len = count
		}
	};

	const struct spi_buf_set tx = {
		.buffers = &cmd_buf,
		.count = 1
	};
	const struct spi_buf_set rx = {
		.buffers = rx_buf,
		.count = ARRAY_SIZE(rx_buf)
	};

	ret = spi_transceive_dt(&config->dt_spi, &tx, &rx);
	if (unlikely(update_hirq)) {
		priv->hirq = hirq;
	}

	return ret;
}

static int max3421e_read(const struct device *dev,
			 const uint8_t reg,
			 uint8_t *const data,
			 const uint32_t count)
{

	return max3421e_read_hirq(dev, reg, data, count, false);
}

static int max3421e_write_byte(const struct device *dev,
			       const uint8_t reg,
			       const uint8_t val)
{
	const struct max3421e_config *config = dev->config;
	uint8_t buf[2] = {MAX3421E_CMD_SPI_WRITE(reg), val};

	const struct spi_buf cmd_buf = {
		.buf = &buf,
		.len = sizeof(buf)
	};
	const struct spi_buf_set tx = {
		.buffers = &cmd_buf,
		.count = 1
	};

	return spi_write_dt(&config->dt_spi, &tx);
}

static int max3421e_write(const struct device *dev,
			  const uint8_t reg,
			  uint8_t *const data,
			  const size_t count)
{
	const struct max3421e_config *config = dev->config;
	uint8_t cmd = MAX3421E_CMD_SPI_WRITE(reg);

	const struct spi_buf cmd_buf[] = {
		{
			.buf = &cmd,
			.len = sizeof(cmd),
		},
		{
			.buf = data,
			.len = count,
		},
	};
	const struct spi_buf_set tx = {
		.buffers = cmd_buf,
		.count = ARRAY_SIZE(cmd_buf),
	};

	return spi_write_dt(&config->dt_spi, &tx);
}

static int max3421e_lock(const struct device *dev)
{
	struct uhc_data *data = dev->data;

	return k_mutex_lock(&data->mutex, K_FOREVER);
}

static int max3421e_unlock(const struct device *dev)
{
	struct uhc_data *data = dev->data;

	return k_mutex_unlock(&data->mutex);
}

/* Disable Host Interrupt */
static ALWAYS_INLINE int max3421e_hien_disable(const struct device *dev,
					       const uint8_t hint)
{
	struct max3421e_data *priv = uhc_get_private(dev);

	priv->hien &= ~hint;

	return max3421e_write_byte(dev, MAX3421E_REG_HIEN, priv->hien);
}

/* Set peripheral (device) address to be used in next transfer */
static ALWAYS_INLINE int max3421e_peraddr(const struct device *dev,
					  const uint8_t addr)
{
	struct max3421e_data *priv = uhc_get_private(dev);
	int ret = 0;

	if (priv->addr != addr) {
		/*
		 * TODO: Consider how to force the update of toggle values
		 * for the next transfer. Necessary if we want to support
		 * multiple peripherals.
		 */
		ret = max3421e_write_byte(dev, MAX3421E_REG_PERADDR, addr);
		if (ret == 0) {
			priv->addr = addr;
		}
	}

	return ret;
}

/* Update driver's knowledge about DATA PID */
static ALWAYS_INLINE void max3421e_tgl_update(const struct device *dev)
{
	struct max3421e_data *priv = uhc_get_private(dev);
	uint8_t ep_idx = MAX3421E_EP(priv->hxfr);

	if (priv->hxfr & MAX3421E_OUTNIN) {
		if (priv->hrsl & MAX3421E_SNDTOGRD) {
			priv->tog_out |= BIT(ep_idx);
		} else {
			priv->tog_out &= ~BIT(ep_idx);
		}
	} else {
		if (priv->hrsl & MAX3421E_RCVTOGRD) {
			priv->tog_in |= BIT(ep_idx);
		} else {
			priv->tog_in &= ~BIT(ep_idx);
		}
	}

	LOG_DBG("tog_in 0x%02x tog_out 0x%02x last-hxfr 0x%02x hrsl 0x%02x",
		priv->tog_in, priv->tog_out, priv->hxfr, priv->hrsl);
}

/* Get DATA PID to be used for the next transfer */
static ALWAYS_INLINE uint8_t max3421e_tgl_next(const struct device *dev,
					       const uint8_t hxfr)
{
	struct max3421e_data *priv = uhc_get_private(dev);
	uint8_t ep_idx = MAX3421E_EP(hxfr);
	uint8_t hctl;

	/* Force DATA1 PID for the data stage of control transfer */
	if (hxfr & MAX3421E_SETUP) {
		priv->tog_in |= BIT(0);
		priv->tog_out |= BIT(0);
	}

	if (hxfr & MAX3421E_OUTNIN) {
		hctl = (priv->tog_out & BIT(ep_idx)) ? MAX3421E_SNDTOG1 :
						       MAX3421E_SNDTOG0;
	} else {
		hctl = (priv->tog_in & BIT(ep_idx)) ? MAX3421E_RCVTOG1 :
						      MAX3421E_RCVTOG0;
	}

	return hctl;
}

static ALWAYS_INLINE int max3421e_hxfr_start(const struct device *dev,
					     const uint8_t hxfr)
{
	struct max3421e_data *priv = uhc_get_private(dev);

	if (priv->hxfr != hxfr) {
		uint8_t reg[2] = {0, hxfr};

		/* Update DATA PID if transfer parameter changes */
		max3421e_tgl_update(dev);
		reg[0] = max3421e_tgl_next(dev, hxfr);
		priv->hxfr = hxfr;
		LOG_DBG("hctl 0x%02x hxfr 0x%02x", reg[0], reg[1]);

		return max3421e_write(dev, MAX3421E_REG_HCTL,
				      reg, sizeof(reg));
	}

	return max3421e_write_byte(dev, MAX3421E_REG_HXFR, priv->hxfr);
}

static int max3421e_xfer_data(const struct device *dev,
			      struct net_buf *const buf,
			      const uint8_t ep)
{
	const uint8_t ep_idx = USB_EP_GET_IDX(ep);
	int ret;

	if (USB_EP_DIR_IS_IN(ep)) {
		LOG_DBG("bulk in %p %u", buf, net_buf_tailroom(buf));
		ret = max3421e_hxfr_start(dev, MAX3421E_HXFR_BULKIN(ep_idx));
	} else {
		size_t len;

		len = MIN(MAX3421E_MAX_EP_SIZE, buf->len);
		LOG_DBG("bulk out %p %u", buf, len);

		ret = max3421e_write(dev, MAX3421E_REG_SNDFIFO, buf->data, len);
		if (ret) {
			return ret;
		}

		ret = max3421e_write_byte(dev, MAX3421E_REG_SNDBC, len);
		if (ret) {
			return ret;
		}

		/*
		 * FIXME: Pull should happen after device ACKs the data,
		 *        move to max3421e_hrslt_success().
		 */
		net_buf_pull(buf, len);
		ret = max3421e_hxfr_start(dev, MAX3421E_HXFR_BULKOUT(ep_idx));
	}

	return ret;
}

static int max3421e_xfer_control(const struct device *dev,
				 struct uhc_transfer *const xfer,
				 const uint8_t hrsl)
{
	struct max3421e_data *priv = uhc_get_private(dev);
	struct net_buf *buf = xfer->buf;
	int ret;

	/* Just restart if device NAKed packet */
	if (HRSLT_IS_NAK(hrsl)) {
		return max3421e_hxfr_start(dev, priv->hxfr);
	}


	if (xfer->stage == UHC_CONTROL_STAGE_SETUP) {
		LOG_DBG("Handle SETUP stage");
		ret = max3421e_write(dev, MAX3421E_REG_SUDFIFO,
				     xfer->setup_pkt, sizeof(xfer->setup_pkt));
		if (ret) {
			return ret;
		}

		ret = max3421e_hxfr_start(dev, MAX3421E_HXFR_SETUP(0));
		if (ret) {
			return ret;
		}

		return 0;
	}

	if (buf != NULL && xfer->stage == UHC_CONTROL_STAGE_DATA) {
		LOG_DBG("Handle DATA stage");
		return max3421e_xfer_data(dev, buf, xfer->ep);
	}

	if (xfer->stage == UHC_CONTROL_STAGE_STATUS) {
		LOG_DBG("Handle STATUS stage");
		if (USB_EP_DIR_IS_IN(xfer->ep)) {
			ret = max3421e_hxfr_start(dev, MAX3421E_HXFR_HSOUT(0));
		} else {
			ret = max3421e_hxfr_start(dev, MAX3421E_HXFR_HSIN(0));
		}

		return ret;
	}

	return -EINVAL;
}

static int max3421e_xfer_bulk(const struct device *dev,
			      struct uhc_transfer *const xfer,
			      const uint8_t hrsl)
{
	struct max3421e_data *priv = uhc_get_private(dev);
	struct net_buf *buf = xfer->buf;

	/* Just restart if device NAKed packet */
	if (HRSLT_IS_NAK(hrsl)) {
		return max3421e_hxfr_start(dev, priv->hxfr);
	}

	if (buf == NULL) {
		LOG_ERR("No buffer to handle");
		return -ENODATA;
	}

	return max3421e_xfer_data(dev, buf, xfer->ep);
}

static int max3421e_schedule_xfer(const struct device *dev)
{
	struct max3421e_data *priv = uhc_get_private(dev);
	const uint8_t hirq = priv->hirq;
	const uint8_t hrsl = priv->hrsl;

	if (priv->last_xfer == NULL) {
		int ret;

		priv->last_xfer = uhc_xfer_get_next(dev);
		if (priv->last_xfer == NULL) {
			LOG_DBG("Nothing to transfer");
			return 0;
		}

		LOG_DBG("Next transfer %p", priv->last_xfer);
		ret = max3421e_peraddr(dev, priv->last_xfer->addr);
		if (ret) {
			return ret;
		}
	}

	if (hirq & MAX3421E_FRAME) {
		if (priv->last_xfer->timeout) {
			priv->last_xfer->timeout--;
		} else {
			LOG_INF("Transfer timeout");
		}
	}

	/*
	 * TODO: currently we only support control transfers and
	 * treat all others as bulk.
	 */
	if (USB_EP_GET_IDX(priv->last_xfer->ep) == 0) {
		return max3421e_xfer_control(dev, priv->last_xfer, hrsl);
	}

	return max3421e_xfer_bulk(dev, priv->last_xfer, hrsl);
}

static void max3421e_xfer_drop_active(const struct device *dev, int err)
{
	struct max3421e_data *priv = uhc_get_private(dev);

	if (priv->last_xfer) {
		uhc_xfer_return(dev, priv->last_xfer, err);
		priv->last_xfer = NULL;
	}
}

static int max3421e_hrslt_success(const struct device *dev)
{
	struct max3421e_data *priv = uhc_get_private(dev);
	struct uhc_transfer *const xfer = priv->last_xfer;
	struct net_buf *buf = xfer->buf;
	bool finished = false;
	int err = 0;
	size_t len;
	uint8_t bc;

	switch (MAX3421E_HXFR_TYPE(priv->hxfr)) {
	case MAX3421E_HXFR_TYPE_SETUP:
		if (xfer->buf != NULL) {
			xfer->stage = UHC_CONTROL_STAGE_DATA;
		} else {
			xfer->stage = UHC_CONTROL_STAGE_STATUS;
		}
		break;
	case MAX3421E_HXFR_TYPE_HSOUT:
		LOG_DBG("HSOUT");
		finished = true;
		break;
	case MAX3421E_HXFR_TYPE_HSIN:
		LOG_DBG("HSIN");
		finished = true;
		break;
	case MAX3421E_HXFR_TYPE_ISOOUT:
		LOG_ERR("ISO OUT is not implemented");
		k_panic();
		break;
	case MAX3421E_HXFR_TYPE_ISOIN:
		LOG_ERR("ISO IN is not implemented");
		k_panic();
		break;
	case MAX3421E_HXFR_TYPE_BULKOUT:
		if (buf->len == 0) {
			LOG_INF("hrslt bulk out %u", buf->len);
			if (xfer->ep == USB_CONTROL_EP_OUT) {
				xfer->stage = UHC_CONTROL_STAGE_STATUS;
			} else {
				finished = true;
			}
		}
		break;
	case MAX3421E_HXFR_TYPE_BULKIN:
		err = max3421e_read(dev, MAX3421E_REG_RCVBC, &bc, sizeof(bc));
		if (err) {
			break;
		}

		if (bc > net_buf_tailroom(buf)) {
			LOG_WRN("%u received bytes will be dropped",
				bc - net_buf_tailroom(buf));
		}

		len = MIN(net_buf_tailroom(buf), bc);
		err = max3421e_read(dev, MAX3421E_REG_RCVFIFO,
				    net_buf_add(buf, len), len);
		if (err) {
			break;
		}

		LOG_INF("bc %u tr %u", bc, net_buf_tailroom(buf));

		if (bc < MAX3421E_MAX_EP_SIZE || !net_buf_tailroom(buf)) {
			LOG_INF("hrslt bulk in %u, %u", bc, len);
			if (xfer->ep == USB_CONTROL_EP_IN) {
				xfer->stage = UHC_CONTROL_STAGE_STATUS;
			} else {
				finished = true;
			}
		}
		break;
	}

	if (finished) {
		LOG_DBG("Transfer finished");
		uhc_xfer_return(dev, xfer, 0);
		priv->last_xfer = NULL;
	}

	if (err) {
		max3421e_xfer_drop_active(dev, err);
	}

	return err;
}

static int max3421e_handle_hxfrdn(const struct device *dev)
{
	struct max3421e_data *priv = uhc_get_private(dev);
	struct uhc_transfer *const xfer = priv->last_xfer;
	const uint8_t hrsl = priv->hrsl;
	int ret = 0;

	if (xfer == NULL) {
		LOG_ERR("No transfers to handle");
		return -ENODATA;
	}

	switch (MAX3421E_HRSLT(hrsl)) {
	case MAX3421E_HR_NAK:
		/*
		 * The transfer did not take place within
		 * the specified number of frames.
		 *
		 * TODO: Transfer cancel request (xfer->cancel)
		 * can be handled here as well.
		 */
		if (xfer->timeout == 0) {
			max3421e_xfer_drop_active(dev, -ETIMEDOUT);
		}

		break;
	case MAX3421E_HR_STALL:
		max3421e_xfer_drop_active(dev, -EPIPE);
		break;
	case MAX3421E_HR_TOGERR:
		LOG_WRN("Toggle error");
		break;
	case MAX3421E_HR_SUCCESS:
		ret = max3421e_hrslt_success(dev);
		break;
	default:
		/* TODO: Handle all reasonalbe result codes */
		max3421e_xfer_drop_active(dev, -EINVAL);
		ret = -EINVAL;
		break;
	}

	return ret;
}

static void max3421e_handle_condet(const struct device *dev)
{
	struct max3421e_data *priv = uhc_get_private(dev);
	const uint8_t jk = priv->hrsl & MAX3421E_JKSTATUS_MASK;
	enum uhc_event_type type = UHC_EVT_ERROR;

	/*
	 * JSTATUS:KSTATUS 0:0 - SE0
	 * JSTATUS:KSTATUS 0:1 - K   (Resume)
	 * JSTATUS:KSTATUS 1:0 - J   (Idle)
	 */
	if (jk == 0) {
		/* Device disconnected */
		type = UHC_EVT_DEV_REMOVED;
	}

	if (jk == MAX3421E_JSTATUS) {
		/* Device connected */
		type = UHC_EVT_DEV_CONNECTED_FS;
	}

	if (jk == MAX3421E_KSTATUS) {
		/* Device connected */
		type = UHC_EVT_DEV_CONNECTED_LS;
	}

	uhc_submit_event(dev, type, 0);
}

static void max3421e_bus_event(const struct device *dev)
{
	struct max3421e_data *priv = uhc_get_private(dev);

	if (atomic_test_and_clear_bit(&priv->state,
				      MAX3421E_STATE_BUS_RESUME)) {
		/* Resume operation done event */
		uhc_submit_event(dev, UHC_EVT_RESUMED, 0);
	}

	if (atomic_test_and_clear_bit(&priv->state,
				      MAX3421E_STATE_BUS_RESET)) {
		/* Reset operation done event */
		uhc_submit_event(dev, UHC_EVT_RESETED, 0);
	}
}

static int max3421e_update_hrsl_hirq(const struct device *dev)
{
	struct max3421e_data *priv = uhc_get_private(dev);
	int err;

	err = max3421e_read_hirq(dev, MAX3421E_REG_HRSL, &priv->hrsl, 1, true);
	/* Consider only enabled interrupts and RCVDAV bit (see RCVBC description) */
	priv->hirq &= priv->hien | MAX3421E_RCVDAV;
	LOG_DBG("HIRQ 0x%02x HRSLT %d", priv->hirq, MAX3421E_HRSLT(priv->hrsl));

	return err;
}

static int max3421e_clear_hirq(const struct device *dev, const uint8_t hirq)
{
	return max3421e_write_byte(dev, MAX3421E_REG_HIRQ, hirq);
}

static int max3421e_handle_bus_irq(const struct device *dev)
{
	struct max3421e_data *priv = uhc_get_private(dev);
	const uint8_t hirq = priv->hirq;
	int ret = 0;

	/* Suspend operation Done Interrupt (bus suspended) */
	if (hirq & MAX3421E_SUSDN) {
		ret = max3421e_hien_disable(dev, MAX3421E_SUSDN);
		uhc_submit_event(dev, UHC_EVT_SUSPENDED, 0);
	}

	/* Peripheral Connect/Disconnect Interrupt */
	if (hirq & MAX3421E_CONDET) {
		max3421e_handle_condet(dev);
	}

	/* Remote Wakeup Interrupt */
	if (hirq & MAX3421E_RWU) {
		uhc_submit_event(dev, UHC_EVT_RWUP, 0);
	}

	/* Bus Reset or Bus Resume event */
	if (hirq & MAX3421E_BUSEVENT) {
		max3421e_bus_event(dev);
	}

	return ret;
}

static void uhc_max3421e_thread(const struct device *dev)
{
	struct max3421e_data *priv = uhc_get_private(dev);

	LOG_DBG("MAX3421E thread started");

	while (true) {
		bool schedule = false;
		int err;

		k_sem_take(&priv->irq_sem, K_FOREVER);

		max3421e_lock(dev);

		/*
		 * Get HRSL and HIRQ values, do not perform any operation
		 * that changes the state of the bus yet.
		 */
		err = max3421e_update_hrsl_hirq(dev);
		if (unlikely(err)) {
			uhc_submit_event(dev, UHC_EVT_ERROR, err);
		}

		/* Host Transfer Done Interrupt */
		if (priv->hirq & MAX3421E_HXFRDN) {
			err = max3421e_handle_hxfrdn(dev);
			schedule = true;
		}

		/* Frame Generator Interrupt */
		if (priv->hirq & MAX3421E_FRAME) {
			schedule = HRSLT_IS_BUSY(priv->hrsl) ? false : true;
		}

		/* Shorten the if path a little */
		if (priv->hirq & ~(MAX3421E_FRAME | MAX3421E_HXFRDN)) {
			err = max3421e_handle_bus_irq(dev);
			if (unlikely(err)) {
				uhc_submit_event(dev, UHC_EVT_ERROR, err);
			}
		}

		/* Clear interrupts and schedule new bus transfer */
		err = max3421e_clear_hirq(dev, priv->hirq);
		if (unlikely(err)) {
			uhc_submit_event(dev, UHC_EVT_ERROR, err);
		}

		if (schedule) {
			err = max3421e_schedule_xfer(dev);
			if (unlikely(err)) {
				uhc_submit_event(dev, UHC_EVT_ERROR, err);
			}
		}

		max3421e_unlock(dev);
	}
}

static void max3421e_gpio_cb(const struct device *dev,
			     struct gpio_callback *cb,
			     uint32_t pins)
{
	struct max3421e_data *priv =
		CONTAINER_OF(cb, struct max3421e_data, gpio_cb);

	k_sem_give(&priv->irq_sem);
}

/* Enable SOF generator */
static int max3421e_sof_enable(const struct device *dev)
{
	struct max3421e_data *priv = uhc_get_private(dev);

	if (priv->mode & MAX3421E_SOFKAENAB) {
		return -EALREADY;
	}

	priv->mode |= MAX3421E_SOFKAENAB;

	return max3421e_write_byte(dev, MAX3421E_REG_MODE, priv->mode);
}

/* Disable SOF generator and suspend bus */
static int max3421e_bus_suspend(const struct device *dev)
{
	struct max3421e_data *priv = uhc_get_private(dev);

	if (!(priv->mode & MAX3421E_SOFKAENAB)) {
		return -EALREADY;
	}

	priv->hien |= MAX3421E_SUSDN;
	priv->mode &= ~MAX3421E_SOFKAENAB;

	uint8_t tmp[3] = {MAX3421E_SUSDN, priv->hien, priv->mode};

	return max3421e_write(dev, MAX3421E_REG_HIRQ, tmp, sizeof(tmp));
}

/* Signal bus reset, 50ms SE0 signal */
static int max3421e_bus_reset(const struct device *dev)
{
	struct max3421e_data *priv = uhc_get_private(dev);
	int ret;

	if (atomic_test_bit(&priv->state, MAX3421E_STATE_BUS_RESUME)) {
		return -EBUSY;
	}

	ret = max3421e_write_byte(dev, MAX3421E_REG_HCTL, MAX3421E_BUSRST);
	atomic_set_bit(&priv->state, MAX3421E_STATE_BUS_RESET);

	return ret;
}

/* Signal bus resume event, 20ms K-state + low-speed EOP */
static int max3421e_bus_resume(const struct device *dev)
{
	struct max3421e_data *priv = uhc_get_private(dev);
	int ret;

	if (atomic_test_bit(&priv->state, MAX3421E_STATE_BUS_RESET)) {
		return -EBUSY;
	}

	ret = max3421e_write_byte(dev, MAX3421E_REG_HCTL, MAX3421E_SIGRSM);
	atomic_set_bit(&priv->state, MAX3421E_STATE_BUS_RESUME);

	return ret;
}

static int max3421e_enqueue(const struct device *dev,
			    struct uhc_transfer *const xfer)
{
	return uhc_xfer_append(dev, xfer);
}

static int max3421e_dequeue(const struct device *dev,
			    struct uhc_transfer *const xfer)
{
	/* TODO */
	return 0;
}

static int max3421e_reset(const struct device *dev)
{
	const struct max3421e_config *config = dev->config;
	int ret;

	if (config->dt_rst.port) {
		gpio_pin_set_dt(&config->dt_rst, 1);
		gpio_pin_set_dt(&config->dt_rst, 0);
	} else {
		LOG_DBG("Reset MAX3421E using CHIPRES");
		ret = max3421e_write_byte(dev, MAX3421E_REG_USBCTL, MAX3421E_CHIPRES);
		ret |= max3421e_write_byte(dev, MAX3421E_REG_USBCTL, 0);

		if (ret) {
			return ret;
		}
	}

	for (int i = 0; i < CONFIG_MAX3421E_OSC_WAIT_RETRIES; i++) {
		uint8_t usbirq;

		ret = max3421e_read(dev, MAX3421E_REG_USBIRQ,
				    &usbirq, sizeof(usbirq));

		LOG_DBG("USBIRQ 0x%02x", usbirq);
		if (usbirq & MAX3421E_OSCOKIRQ) {
			return 0;
		}

		k_msleep(3);
	}

	return -EIO;
}

static int max3421e_pinctl_setup(const struct device *dev)
{
	/* Full-Duplex SPI, INT pin edge active, GPX pin signals SOF */
	const uint8_t pinctl = MAX3421E_FDUPSPI | MAX3421E_GPXB | MAX3421E_GPXA;
	uint8_t tmp;
	int ret;

	ret = max3421e_write_byte(dev, MAX3421E_REG_PINCTL, pinctl);
	if (unlikely(ret)) {
		return ret;
	}

	ret = max3421e_read(dev, MAX3421E_REG_PINCTL, &tmp, sizeof(tmp));
	if (unlikely(ret)) {
		return ret;
	}

	if (unlikely(tmp != pinctl)) {
		LOG_ERR("Failed to verify PINCTL register 0x%02x vs 0x%02x",
			pinctl, tmp);
		return -EIO;
	}

	return 0;
}

/*
 * Cache MODE and HIEN register values to avoid having to read it
 * before modifying register bits.
 */
static int max3421e_mode_setup(const struct device *dev)
{
	/*
	 * MODE register defaults:
	 *   host mode, connect internal D+ and D- pulldown resistors to ground
	 */
	const uint8_t mode = MAX3421E_DPPULLDN | MAX3421E_DMPULLDN |
			     MAX3421E_DELAYISO | MAX3421E_HOST;
	struct max3421e_data *priv = uhc_get_private(dev);
	uint8_t tmp;
	int ret;

	ret = max3421e_write_byte(dev, MAX3421E_REG_MODE, mode);
	if (ret) {
		return ret;
	}

	ret = max3421e_read(dev, MAX3421E_REG_MODE, &tmp, sizeof(tmp));
	if (ret) {
		return ret;
	}

	if (tmp != mode) {
		LOG_ERR("Failed to verify MODE register 0x%02x vs 0x%02x",
			mode, tmp);
		return -EIO;
	}

	priv->mode = mode;

	return 0;
}

static int max3421e_hien_setup(const struct device *dev)
{
	/* Host interrupts enabled by default */
	const uint8_t hien = MAX3421E_HXFRDN | MAX3421E_FRAME |
			     MAX3421E_CONDET | MAX3421E_RWU |
			     MAX3421E_BUSEVENT;
	struct max3421e_data *priv = uhc_get_private(dev);
	uint8_t tmp;
	int ret;

	ret = max3421e_write_byte(dev, MAX3421E_REG_HIEN, hien);
	if (ret) {
		return ret;
	}

	ret = max3421e_read(dev, MAX3421E_REG_HIEN, &tmp, sizeof(tmp));
	if (ret) {
		return ret;
	}

	if (tmp != hien) {
		LOG_ERR("Failed to verify HIEN register 0x%02x vs 0x%02x",
			hien, tmp);
		return -EIO;
	}

	priv->hien = hien;

	return 0;
}

static int max3421e_enable_int_output(const struct device *dev)
{
	const uint8_t cpuctl = MAX3421E_IE;
	uint8_t tmp;
	int ret;

	/* Enable MAX3421E INT output pin */
	ret = max3421e_write_byte(dev, MAX3421E_REG_CPUCTL, cpuctl);
	if (ret) {
		return ret;
	}

	ret = max3421e_read(dev, MAX3421E_REG_CPUCTL, &tmp, sizeof(tmp));
	if (ret) {
		return ret;
	}

	if (tmp != cpuctl) {
		LOG_ERR("Failed to verify CPUCTL register 0x%02x vs 0x%02x",
			cpuctl, tmp);
		return -EIO;
	}

	return 0;
}

static int uhc_max3421e_init(const struct device *dev)
{
	struct max3421e_data *priv = uhc_get_private(dev);
	uint8_t rev;
	int ret;

	ret = max3421e_pinctl_setup(dev);
	if (ret) {
		LOG_ERR("Failed to setup pinctl");
		return ret;
	}

	ret = max3421e_read(dev, MAX3421E_REG_REVISION, &rev, sizeof(rev));
	if (ret) {
		LOG_ERR("Failed to read revision");
		return ret;
	}

	ret = max3421e_reset(dev);
	if (ret) {
		LOG_ERR("Failed to reset MAX3421E");
		return ret;
	}

	ret = max3421e_mode_setup(dev);
	if (ret) {
		LOG_ERR("Failed to setup controller mode");
		return ret;
	}

	ret = max3421e_hien_setup(dev);
	if (ret) {
		LOG_ERR("Failed to setup interrupts");
		return ret;
	}

	ret = max3421e_enable_int_output(dev);
	if (ret) {
		LOG_ERR("Failed to enable INT output");
		return ret;
	}

	LOG_INF("REV 0x%x, MODE 0x%02x, HIEN 0x%02x",
		rev, priv->mode, priv->hien);

	priv->addr = 0;

	/* Sample bus if device is already connected */
	return max3421e_write_byte(dev, MAX3421E_REG_HCTL, MAX3421E_SAMPLEBUS);
}

static int uhc_max3421e_enable(const struct device *dev)
{
	/* TODO */
	return 0;
}

static int uhc_max3421e_disable(const struct device *dev)
{
	/* TODO */
	return 0;
}

static int uhc_max3421e_shutdown(const struct device *dev)
{
	/* TODO */
	return 0;
}

static int max3421e_driver_init(const struct device *dev)
{
	const struct max3421e_config *config = dev->config;
	struct uhc_data *data = dev->data;
	struct max3421e_data *priv = data->priv;
	int ret;

	if (config->dt_rst.port) {
		if (!gpio_is_ready_dt(&config->dt_rst)) {
			LOG_ERR("GPIO device %s not ready",
				config->dt_rst.port->name);
			return -EIO;
		}

		ret = gpio_pin_configure_dt(&config->dt_rst,
					    GPIO_OUTPUT_INACTIVE);
		if (ret) {
			LOG_ERR("Failed to configure GPIO pin %u",
				config->dt_rst.pin);
			return ret;
		}
	}

	if (!spi_is_ready_dt(&config->dt_spi)) {
		LOG_ERR("SPI device %s not ready", config->dt_spi.bus->name);
		return -EIO;
	}

	if (!gpio_is_ready_dt(&config->dt_int)) {
		LOG_ERR("GPIO device %s not ready", config->dt_int.port->name);
		return -EIO;
	}

	ret = gpio_pin_configure_dt(&config->dt_int, GPIO_INPUT);
	if (ret) {
		LOG_ERR("Failed to configure GPIO pin %u", config->dt_int.pin);
		return ret;
	}

	gpio_init_callback(&priv->gpio_cb, max3421e_gpio_cb,
			   BIT(config->dt_int.pin));
	ret = gpio_add_callback(config->dt_int.port, &priv->gpio_cb);
	if (ret) {
		return ret;
	}

	ret = gpio_pin_interrupt_configure_dt(&config->dt_int,
					      GPIO_INT_EDGE_TO_ACTIVE);
	if (ret) {
		return ret;
	}

	k_mutex_init(&data->mutex);
	k_thread_create(&drv_stack_data, drv_stack,
			K_KERNEL_STACK_SIZEOF(drv_stack),
			(k_thread_entry_t)uhc_max3421e_thread,
			(void *)dev, NULL, NULL,
			K_PRIO_COOP(2), 0, K_NO_WAIT);
	k_thread_name_set(&drv_stack_data, "uhc_max3421e");

	LOG_DBG("MAX3421E CPU interface initialized");

	return 0;
}

static const struct uhc_api max3421e_uhc_api = {
	.lock = max3421e_lock,
	.unlock = max3421e_unlock,
	.init = uhc_max3421e_init,
	.enable = uhc_max3421e_enable,
	.disable = uhc_max3421e_disable,
	.shutdown = uhc_max3421e_shutdown,

	.bus_reset = max3421e_bus_reset,
	.sof_enable  = max3421e_sof_enable,
	.bus_suspend = max3421e_bus_suspend,
	.bus_resume = max3421e_bus_resume,

	.ep_enqueue = max3421e_enqueue,
	.ep_dequeue = max3421e_dequeue,
};

static struct max3421e_data max3421e_data = {
	.irq_sem = Z_SEM_INITIALIZER(max3421e_data.irq_sem, 0, 1),
};

static struct uhc_data max3421e_uhc_data = {
	.priv = &max3421e_data,
};

static const struct max3421e_config max3421e_cfg = {
	.dt_spi = SPI_DT_SPEC_INST_GET(0, SPI_WORD_SET(8) | SPI_TRANSFER_MSB, 0),
	.dt_int = GPIO_DT_SPEC_INST_GET(0, int_gpios),
	.dt_rst = GPIO_DT_SPEC_INST_GET_OR(0, reset_gpios, {0}),
};

DEVICE_DT_INST_DEFINE(0, max3421e_driver_init, NULL,
		      &max3421e_uhc_data, &max3421e_cfg,
		      POST_KERNEL, 99,
		      &max3421e_uhc_api);
