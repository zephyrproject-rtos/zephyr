/*
 * Copyright 2026 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT nxp_espi

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/espi.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/logging/log.h>
#ifdef CONFIG_ESPI_FLASH_CHANNEL
#include <zephyr/drivers/flash.h>
#endif

#include <fsl_espi.h>
#include "espi_utils.h"

LOG_MODULE_REGISTER(espi, CONFIG_ESPI_LOG_LEVEL);

#define PORT_NONE (-1)

/* KBC IRQ numbers for IRQPUSH */
#define KBC_IRQ_KEYBOARD 1U
#define KBC_IRQ_MOUSE    12U

struct espi_nxp_vw_map {
	enum espi_vwire_signal signal;
	uint8_t bit;
};

/* WIREWO bit positions for target->controller VWire signals. */
static const struct espi_nxp_vw_map wirewo_map[] = {
	{ ESPI_VWIRE_SIGNAL_OOB_RST_ACK,      0 },
	{ ESPI_VWIRE_SIGNAL_WAKE,             1 },  /* WAKE when !S0. */
	{ ESPI_VWIRE_SIGNAL_SCI,              1 },  /* SCI when S0. */
	{ ESPI_VWIRE_SIGNAL_PME,              2 },
	{ ESPI_VWIRE_SIGNAL_SMI,              3 },
	{ ESPI_VWIRE_SIGNAL_HOST_RST_ACK,     6 },
	{ ESPI_VWIRE_SIGNAL_SUS_ACK,          7 },
	{ ESPI_VWIRE_SIGNAL_TARGET_GPIO_0,    8 },
	{ ESPI_VWIRE_SIGNAL_TARGET_GPIO_1,    9 },
	{ ESPI_VWIRE_SIGNAL_TARGET_GPIO_2,    10 },
	{ ESPI_VWIRE_SIGNAL_TARGET_GPIO_3,    11 },
	{ ESPI_VWIRE_SIGNAL_TARGET_GPIO_4,    12 },
	{ ESPI_VWIRE_SIGNAL_TARGET_GPIO_5,    13 },
	{ ESPI_VWIRE_SIGNAL_TARGET_GPIO_6,    14 },
	{ ESPI_VWIRE_SIGNAL_TARGET_GPIO_7,    15 },
	{ ESPI_VWIRE_SIGNAL_TARGET_BOOT_DONE, 16 },
	{ ESPI_VWIRE_SIGNAL_ERR_FATAL,        17 },
};

static const struct espi_nxp_vw_map wirero_map[] = {
	{ ESPI_VWIRE_SIGNAL_SLP_S3,        0 },
	{ ESPI_VWIRE_SIGNAL_SLP_S4,        1 },
	{ ESPI_VWIRE_SIGNAL_SLP_S5,        2 },
	{ ESPI_VWIRE_SIGNAL_SUS_STAT,      3 },
	{ ESPI_VWIRE_SIGNAL_PLTRST,        4 },
	{ ESPI_VWIRE_SIGNAL_OOB_RST_WARN,  5 },
	{ ESPI_VWIRE_SIGNAL_HOST_RST_WARN, 6 },
	{ ESPI_VWIRE_SIGNAL_SUS_WARN,      7 },
	{ ESPI_VWIRE_SIGNAL_SUS_PWRDN_ACK, 8 },
	{ ESPI_VWIRE_SIGNAL_SLP_A,         9 },
	{ ESPI_VWIRE_SIGNAL_SLP_LAN,       10 },
	{ ESPI_VWIRE_SIGNAL_SLP_WLAN,      11 },
	{ ESPI_VWIRE_SIGNAL_HOST_C10,      20 },
};

struct espi_nxp_port_cfg {
	uint8_t type;
	uint8_t ram_size;
	uint8_t idx_offset;
	uint16_t io_addr;
	uint32_t ram_offset;
};

struct espi_nxp_config {
	ESPI_Type *base;
	const struct device *clock_dev;
	clock_control_subsys_t clock_subsys;
	const struct pinctrl_dev_config *pincfg;
	void (*irq_config_func)(const struct device *dev);
	uint32_t ram_base_addr;
	uint16_t map_base0;
	uint16_t map_base1;
	const struct espi_nxp_port_cfg *ports;
	uint8_t port_count;
#ifdef CONFIG_ESPI_FLASH_CHANNEL
	const struct device *flash_dev;
#endif
};

struct espi_nxp_data {
	espi_handle_t handle;
	sys_slist_t callbacks;
	uint32_t wirewo_shadow;
	uint32_t wirero_prev;
	int8_t acpi_port;
	int8_t acpi_idx_port;
	int8_t custom_port;
	int8_t oob_port;
	int8_t flash_port;
#ifdef CONFIG_ESPI_PERIPHERAL_CHANNEL
	uint8_t kbc_status;
	uint8_t acpi_status;
#endif
#ifdef CONFIG_ESPI_PERIPHERAL_8042_KBC
	bool kbc_irq_enabled;
#endif
#ifdef CONFIG_ESPI_OOB_CHANNEL
	struct k_sem oob_rx_lock;
#endif
#ifdef CONFIG_ESPI_FLASH_CHANNEL
	const struct device *dev;
	struct k_work taf_work;
	struct {
		uint32_t addr;
		uint32_t len;
		uint8_t *data;
		uint8_t tag;
		espi_flash_trans_type_t type;
		bool read_start;
	} taf_req;
#endif
};

static inline void espi_nxp_set_sstcl(ESPI_Type *base, uint8_t port,
				      uint32_t sstcl)
{
	base->PORT[port].IRULESTAT = (base->PORT[port].IRULESTAT &
				     ~ESPI_IRULESTAT_SSTCL_MASK) |
				    ESPI_IRULESTAT_SSTCL(sstcl);
}

static inline void espi_nxp_clear_hstall(ESPI_Type *base)
{
	if (ESPI_GetStatusFlags(base) & ESPI_MSTAT_HSTALL_MASK) {
		ESPI_ClearStatusFlags(base, ESPI_MSTAT_HSTALL_MASK);
	}
}

static void espi_nxp_fire_callbacks(const struct device *dev,
				     struct espi_event evt)
{
	struct espi_nxp_data *data = dev->data;

	espi_send_callbacks(&data->callbacks, dev, evt);
}

static int espi_nxp_config(const struct device *dev, struct espi_cfg *cfg)
{
	const struct espi_nxp_config *config = dev->config;
	ESPI_Type *base = config->base;
	uint32_t espicap = base->ESPICAP;

	espicap &= ~ESPI_ESPICAP_SPICAP_MASK;
	if (cfg->io_caps & ESPI_IO_MODE_QUAD_LINES) {
		espicap |= ESPI_ESPICAP_SPICAP(3);
	} else if (cfg->io_caps & ESPI_IO_MODE_DUAL_LINES) {
		espicap |= ESPI_ESPICAP_SPICAP(1);
	} else {
		/* Zero means single mode. */
	}

	espicap &= ~ESPI_ESPICAP_MAXSPD_MASK;
	if (cfg->max_freq >= 66) {
		espicap |= ESPI_ESPICAP_MAXSPD(4);
	} else if (cfg->max_freq >= 50) {
		espicap |= ESPI_ESPICAP_MAXSPD(3);
	} else if (cfg->max_freq >= 33) {
		espicap |= ESPI_ESPICAP_MAXSPD(2);
	} else if (cfg->max_freq >= 25) {
		espicap |= ESPI_ESPICAP_MAXSPD(1);
	} else {
		/* Zero means 20 MHz. */
	}

	if (cfg->channel_caps & ESPI_CHANNEL_OOB) {
		espicap |= ESPI_ESPICAP_OOBOK_MASK;
	}
	if (cfg->channel_caps & ESPI_CHANNEL_PERIPHERAL) {
		espicap |= ESPI_ESPICAP_MEMMX(1);
	}
#ifdef CONFIG_ESPI_FLASH_CHANNEL
	if (cfg->channel_caps & ESPI_CHANNEL_FLASH) {
		espicap |= ESPI_ESPICAP_SAF_MASK | ESPI_ESPICAP_FLASHMX(1) |
			   ESPI_ESPICAP_SAFERA(1);
	}
#endif
	espicap |= ESPI_ESPICAP_ALPIN_MASK;

	base->ESPICAP = espicap;
	return 0;
}

static bool espi_nxp_get_channel_status(const struct device *dev,
					 enum espi_channel ch)
{
	const struct espi_nxp_config *config = dev->config;
	uint32_t espicfg = config->base->ESPICFG;

	switch (ch) {
	case ESPI_CHANNEL_PERIPHERAL:
		return (espicfg & ESPI_ESPICFG_MEMENA_MASK) != 0U;
	case ESPI_CHANNEL_VWIRE:
		return (espicfg & ESPI_ESPICFG_VWOK_MASK) != 0U;
	case ESPI_CHANNEL_OOB:
		return (espicfg & ESPI_ESPICFG_OOBOK_MASK) != 0U;
	case ESPI_CHANNEL_FLASH:
		return (espicfg & ESPI_ESPICFG_FLSHOK_MASK) != 0U;
	default:
		return false;
	}
}

static int espi_nxp_send_vwire(const struct device *dev,
				enum espi_vwire_signal signal, uint8_t level)
{
	const struct espi_nxp_config *config = dev->config;
	struct espi_nxp_data *data = dev->data;

	for (size_t i = 0; i < ARRAY_SIZE(wirewo_map); i++) {
		if (wirewo_map[i].signal == signal) {
			unsigned int key = irq_lock();

			if (level) {
				data->wirewo_shadow |= BIT(wirewo_map[i].bit);
			} else {
				data->wirewo_shadow &= ~BIT(wirewo_map[i].bit);
			}
			config->base->WIREWO = data->wirewo_shadow |
					       ESPI_WIREWO_DONE_MASK;
			irq_unlock(key);
			return 0;
		}
	}
	return -EINVAL;
}

static int espi_nxp_receive_vwire(const struct device *dev,
				   enum espi_vwire_signal signal,
				   uint8_t *level)
{
	const struct espi_nxp_config *config = dev->config;
	struct espi_nxp_data *data = dev->data;

	for (size_t i = 0; i < ARRAY_SIZE(wirewo_map); i++) {
		if (wirewo_map[i].signal == signal) {
			*level = (data->wirewo_shadow >> wirewo_map[i].bit) & 1U;
			return 0;
		}
	}

	uint32_t wirero = ESPI_GetVWire(config->base);

	for (size_t i = 0; i < ARRAY_SIZE(wirero_map); i++) {
		if (wirero_map[i].signal == signal) {
			*level = (wirero >> wirero_map[i].bit) & 1U;
			return 0;
		}
	}
	return -EINVAL;
}

static int espi_nxp_manage_callback(const struct device *dev,
				     struct espi_callback *callback, bool set)
{
	struct espi_nxp_data *data = dev->data;

	return espi_manage_callback(&data->callbacks, callback, set);
}

#ifdef CONFIG_ESPI_PERIPHERAL_CHANNEL

static int espi_nxp_read_lpc_request(const struct device *dev,
				      enum lpc_peripheral_opcode op,
				      uint32_t *data)
{
	const struct espi_nxp_config *config = dev->config;
	struct espi_nxp_data *drv = dev->data;
	ESPI_Type *base = config->base;

	ARG_UNUSED(config);
	ARG_UNUSED(drv);
	ARG_UNUSED(base);

	switch (op) {
#ifdef CONFIG_ESPI_PERIPHERAL_HOST_IO
	case EACPI_OBF_HAS_CHAR:
		if (drv->acpi_port == PORT_NONE) {
			return -ENOTSUP;
		}
		*data = (ESPI_GetPortStatus(base, drv->acpi_port) &
			 ESPI_STAT_INTWR_MASK) ? 1U : 0U;
		return 0;
	case EACPI_IBF_HAS_CHAR:
		if (drv->acpi_port == PORT_NONE) {
			return -ENOTSUP;
		}
		*data = (ESPI_GetPortStatus(base, drv->acpi_port) &
			 ESPI_STAT_INTRD_MASK) ? 1U : 0U;
		return 0;
	case EACPI_READ_STS:
		*data = drv->acpi_status;
		return 0;
#endif
#ifdef CONFIG_ESPI_PERIPHERAL_8042_KBC
	case E8042_OBF_HAS_CHAR:
		if (drv->acpi_idx_port == PORT_NONE) {
			return -ENOTSUP;
		}
		*data = (ESPI_GetPortStatus(base, drv->acpi_idx_port) &
			 ESPI_STAT_INTWR_MASK) ? 1U : 0U;
		return 0;
	case E8042_IBF_HAS_CHAR:
		if (drv->acpi_idx_port == PORT_NONE) {
			return -ENOTSUP;
		}
		*data = (ESPI_GetPortStatus(base, drv->acpi_idx_port) &
			 ESPI_STAT_INTRD_MASK) ? 1U : 0U;
		return 0;
	case E8042_READ_KB_STS:
		*data = drv->kbc_status;
		return 0;
#endif
#ifdef CONFIG_ESPI_PERIPHERAL_CUSTOM_OPCODE
	case ECUSTOM_HOST_CMD_GET_PARAM_MEMORY:
		if (drv->custom_port == PORT_NONE) {
			return -ENOTSUP;
		}
		*data = config->ram_base_addr +
			config->ports[drv->custom_port].ram_offset;
		return 0;
	case ECUSTOM_HOST_CMD_GET_PARAM_MEMORY_SIZE:
		if (drv->custom_port == PORT_NONE) {
			return -ENOTSUP;
		}
		*data = 4U << config->ports[drv->custom_port].ram_size;
		return 0;
#endif
	default:
		return -ENOTSUP;
	}
}

static int espi_nxp_write_lpc_request(const struct device *dev,
				       enum lpc_peripheral_opcode op,
				       uint32_t *data)
{
	const struct espi_nxp_config *config = dev->config;
	struct espi_nxp_data *drv = dev->data;
	ESPI_Type *base = config->base;

	ARG_UNUSED(config);
	ARG_UNUSED(drv);
	ARG_UNUSED(base);

	switch (op) {
#ifdef CONFIG_ESPI_PERIPHERAL_HOST_IO
	case EACPI_WRITE_CHAR:
		if (drv->acpi_port == PORT_NONE) {
			return -ENOTSUP;
		}
		ESPI_WritePortData(base, drv->acpi_port, *data & 0xFFU);
		espi_nxp_clear_hstall(base);
		return 0;
	case EACPI_WRITE_STS:
		drv->acpi_status = *data & 0xFFU;
		return 0;
#endif
#ifdef CONFIG_ESPI_PERIPHERAL_8042_KBC
	case E8042_WRITE_KB_CHAR:
		if (drv->acpi_idx_port == PORT_NONE) {
			return -ENOTSUP;
		}
		ESPI_WritePortData(base, drv->acpi_idx_port, *data & 0xFFU);
		if (drv->kbc_irq_enabled) {
			ESPI_PushIrq(base, KBC_IRQ_KEYBOARD);
		}
		espi_nxp_clear_hstall(base);
		return 0;
	case E8042_WRITE_MB_CHAR:
		if (drv->acpi_idx_port == PORT_NONE) {
			return -ENOTSUP;
		}
		ESPI_WritePortData(base, drv->acpi_idx_port, *data & 0xFFU);
		if (drv->kbc_irq_enabled) {
			ESPI_PushIrq(base, KBC_IRQ_MOUSE);
		}
		espi_nxp_clear_hstall(base);
		return 0;
	case E8042_SET_FLAG:
		drv->kbc_status |= *data & 0xFFU;
		return 0;
	case E8042_CLEAR_FLAG:
		drv->kbc_status &= ~(*data & 0xFFU);
		return 0;
	case E8042_RESUME_IRQ:
		drv->kbc_irq_enabled = true;
		return 0;
	case E8042_PAUSE_IRQ:
		drv->kbc_irq_enabled = false;
		return 0;
	case E8042_CLEAR_OBF:
		if (drv->acpi_idx_port == PORT_NONE) {
			return -ENOTSUP;
		}
		espi_nxp_set_sstcl(base, drv->acpi_idx_port,
				   kESPI_SSTCL_ACPIClrRdPend);
		return 0;
#endif
#ifdef CONFIG_ESPI_PERIPHERAL_CUSTOM_OPCODE
	case ECUSTOM_HOST_CMD_SEND_RESULT:
		if (drv->custom_port == PORT_NONE) {
			return -ENOTSUP;
		}
		ESPI_WritePortData(base, drv->custom_port, *data & 0xFFU);
		espi_nxp_set_sstcl(base, drv->custom_port,
				   kESPI_SSTCL_MailboxWrEmpty);
		espi_nxp_clear_hstall(base);
		return 0;
#endif
	default:
		return -ENOTSUP;
	}
}

#endif /* CONFIG_ESPI_PERIPHERAL_CHANNEL */

#ifdef CONFIG_ESPI_OOB_CHANNEL

static int espi_nxp_send_oob(const struct device *dev,
			      struct espi_oob_packet *pckt)
{
	const struct espi_nxp_config *config = dev->config;
	struct espi_nxp_data *drv = dev->data;
	ESPI_Type *base = config->base;

	if (drv->oob_port == PORT_NONE || pckt->len == 0U) {
		return -EINVAL;
	}

	status_t ret = ESPI_SendOOB(base, &drv->handle,
				    pckt->buf, pckt->len, false);

	return (ret == kStatus_Success) ? 0 : -EMSGSIZE;
}

static int espi_nxp_receive_oob(const struct device *dev,
				 struct espi_oob_packet *pckt)
{
	const struct espi_nxp_config *config = dev->config;
	struct espi_nxp_data *drv = dev->data;
	ESPI_Type *base = config->base;
	uint32_t len;
	uint8_t port;
	int ret;

	if (drv->oob_port == PORT_NONE) {
		return -ENOTSUP;
	}
	port = drv->oob_port;

	ret = k_sem_take(&drv->oob_rx_lock, K_MSEC(CONFIG_ESPI_NXP_TIMEOUT_MS));
	if (ret == -EAGAIN) {
		LOG_ERR("OOB receive timeout");
		return -ETIMEDOUT;
	}

	len = ESPI_GetPortMsgLen(base, port);
	if (len > pckt->len) {
		return -EMSGSIZE;
	}

	memcpy(pckt->buf, ESPI_GetPortRamBuffer(base, port), len);
	pckt->len = len;
	espi_nxp_set_sstcl(base, port, kESPI_SSTCL_OOBRecvEmpty);

	return 0;
}

#endif /* CONFIG_ESPI_OOB_CHANNEL */

#ifdef CONFIG_ESPI_FLASH_CHANNEL

static int espi_nxp_flash_read(const struct device *dev,
				struct espi_flash_packet *pckt)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(pckt);

	return -ENOTSUP;
}

static int espi_nxp_flash_write(const struct device *dev,
				 struct espi_flash_packet *pckt)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(pckt);

	return -ENOTSUP;
}

static int espi_nxp_flash_erase(const struct device *dev,
				 struct espi_flash_packet *pckt)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(pckt);

	return -ENOTSUP;
}

static void espi_nxp_taf_work(struct k_work *work)
{
	struct espi_nxp_data *data = CONTAINER_OF(work, struct espi_nxp_data,
						  taf_work);
	const struct espi_nxp_config *config = data->dev->config;
	ESPI_Type *base = config->base;
	uint8_t port = data->handle.safPort;
	int ret;

	if (data->taf_req.type ==
	    (espi_flash_trans_type_t)kESPI_WRSTAT_EraseRequest) {
		ESPI_SetFlashCompletion(base, port, data->taf_req.tag,
			(uint32_t)kESPI_SSTCL_SAFReqAccepted,
			(uint32_t)kESPI_SAFReadMiddle);
		ret = flash_erase(config->flash_dev, data->taf_req.addr,
				  data->taf_req.len);
		if (ret < 0) {
			LOG_ERR("SAF erase failed: %d", ret);
		}
		ESPI_SetFlashOpLen(base, port,
			(uint32_t)kESPI_OMFLEN_SAFCompletionNoData,
			data->taf_req.len);
		ESPI_SetFlashCompletion(base, port, data->taf_req.tag,
			(uint32_t)kESPI_SSTCL_SAFCompletion,
			(uint32_t)kESPI_SAFReadMiddle);
	} else if (data->taf_req.type ==
		   (espi_flash_trans_type_t)kESPI_WRSTAT_WriteRequest) {
		ESPI_SetFlashCompletion(base, port, data->taf_req.tag,
			(uint32_t)kESPI_SSTCL_SAFReqAccepted,
			(uint32_t)kESPI_SAFReadMiddle);
		ret = flash_write(config->flash_dev, data->taf_req.addr,
				  data->taf_req.data, data->taf_req.len);
		if (ret < 0) {
			LOG_ERR("SAF write failed: %d", ret);
		}
		ESPI_SetFlashOpLen(base, port,
			(uint32_t)kESPI_OMFLEN_SAFCompletionNoData,
			data->taf_req.len);
		ESPI_SetFlashCompletion(base, port, data->taf_req.tag,
			(uint32_t)kESPI_SSTCL_SAFCompletion,
			(uint32_t)kESPI_SAFReadMiddle);
	} else if (data->taf_req.type ==
		   (espi_flash_trans_type_t)kESPI_WRSTAT_ReadRequest &&
		   data->taf_req.read_start) {
		ESPI_SetFlashCompletion(base, port, data->taf_req.tag,
			(uint32_t)kESPI_SSTCL_SAFReqAccepted,
			(uint32_t)kESPI_SAFReadMiddle);

		uint32_t max_payload = ESPI_GetFlashMaxPayload(base);
		uint32_t remaining = data->taf_req.len;
		uint32_t offset = 0U;

		while (remaining > 0U) {
			uint32_t chunk = MIN(remaining, max_payload);
			espi_saf_rx_completion_type_t rxComplType;

			if (data->taf_req.len <= max_payload) {
				rxComplType = kESPI_SAFReadOnly;
			} else if (remaining == data->taf_req.len) {
				rxComplType = kESPI_SAFReadFirst;
			} else if (remaining == chunk) {
				rxComplType = kESPI_SAFReadLast;
			} else {
				rxComplType = kESPI_SAFReadMiddle;
			}

			ret = flash_read(config->flash_dev,
					 data->taf_req.addr + offset,
					 data->taf_req.data, chunk);
			if (ret < 0) {
				LOG_ERR("SAF read failed: %d", ret);
				break;
			}
			ESPI_SetFlashOpLen(base, port,
				(uint32_t)kESPI_OMFLEN_SAFCompletionWithData,
				chunk);
			ESPI_SetFlashCompletion(base, port, data->taf_req.tag,
				(uint32_t)kESPI_SSTCL_SAFCompletion,
				(uint32_t)rxComplType);
			offset += chunk;
			remaining -= chunk;
		}
	} else {
		/* No action needed. */
	}
}

static void espi_nxp_flash_ops(ESPI_Type *base, espi_handle_t *handle,
			       espi_flash_request_t *req, void *userData)
{
	const struct device *dev = userData;
	struct espi_nxp_data *data = dev->data;

	ARG_UNUSED(base);
	ARG_UNUSED(handle);

	data->taf_req.type = req->type;
	data->taf_req.addr = req->addr;
	data->taf_req.len = req->length;
	data->taf_req.data = req->data;
	data->taf_req.tag = req->tag;
	data->taf_req.read_start = req->readStart;
	k_work_submit(&data->taf_work);
}

#endif /* CONFIG_ESPI_FLASH_CHANNEL */

static void espi_nxp_common_cb(ESPI_Type *base, uint32_t status,
			       void *userData)
{
	const struct device *dev = userData;
	struct espi_nxp_data *data = dev->data;

	if (status & kESPI_CrcErrorFlag) {
		LOG_ERR("eSPI CRC error detected");
	}

	if (status & kESPI_BusResetFlag) {
		data->wirewo_shadow = 0U;
		data->wirero_prev = 0U;
		espi_nxp_fire_callbacks(dev,
			(struct espi_event){
				.evt_type = ESPI_BUS_RESET,
				.evt_data = (ESPI_GetStatusFlags(base) &
							 kESPI_InResetFlag) ? 0U : 1U,
			});
	}

	if (status & kESPI_WireChangeFlag) {
		uint32_t wirero = ESPI_GetVWire(base);
		uint32_t changed = wirero ^ data->wirero_prev;

		data->wirero_prev = wirero;

		for (size_t i = 0; i < ARRAY_SIZE(wirero_map); i++) {
			if (!(changed & BIT(wirero_map[i].bit))) {
				continue;
			}
			uint8_t level = (wirero >> wirero_map[i].bit) & 1U;

			if (IS_ENABLED(CONFIG_ESPI_AUTOMATIC_WARNING_ACKNOWLEDGE)) {
				switch (wirero_map[i].signal) {
				case ESPI_VWIRE_SIGNAL_HOST_RST_WARN:
					espi_nxp_send_vwire(dev,
						ESPI_VWIRE_SIGNAL_HOST_RST_ACK,
						level);
					break;
				case ESPI_VWIRE_SIGNAL_SUS_WARN:
					espi_nxp_send_vwire(dev,
						ESPI_VWIRE_SIGNAL_SUS_ACK,
						level);
					break;
				case ESPI_VWIRE_SIGNAL_OOB_RST_WARN:
					espi_nxp_send_vwire(dev,
						ESPI_VWIRE_SIGNAL_OOB_RST_ACK,
						level);
					break;
				default:
					break;
				}
			}

			espi_nxp_fire_callbacks(dev, (struct espi_event){
				.evt_type    = ESPI_BUS_EVENT_VWIRE_RECEIVED,
				.evt_details = wirero_map[i].signal,
				.evt_data    = level,
			});
		}
	}

#ifdef CONFIG_ESPI_PERIPHERAL_DEBUG_PORT_80
	if (status & kESPI_Port80InterruptFlag) {
		espi_p80_status_t p80;

		ESPI_GetPort80Status(base, &p80);
		espi_nxp_fire_callbacks(dev, (struct espi_event){
			.evt_type    = ESPI_BUS_PERIPHERAL_NOTIFICATION,
			.evt_details = ESPI_PERIPHERAL_DEBUG_PORT80,
			.evt_data    = p80.currentCode,
		});
	}
#endif
}

static void espi_nxp_port_cb(ESPI_Type *base, espi_handle_t *handle,
			     uint32_t port, uint32_t status, void *userData)
{
	const struct device *dev = userData;
	struct espi_nxp_data *data = dev->data;

	ARG_UNUSED(handle);

	if (status & ESPI_STAT_INTERR_MASK) {
		espi_port_error_t err;

		ESPI_GetPortErrorStatus(base, port, status, &err);
		LOG_ERR("eSPI port %u error: %d", port, err);
	}

#ifdef CONFIG_ESPI_PERIPHERAL_CHANNEL
#ifdef CONFIG_ESPI_PERIPHERAL_HOST_IO
	if ((int8_t)port == data->acpi_port) {
		uint32_t idx, pdata;

		ESPI_GetEndpointData(base, port, &idx, &pdata);
		espi_nxp_fire_callbacks(dev, (struct espi_event){
			.evt_type    = ESPI_BUS_PERIPHERAL_NOTIFICATION,
			.evt_details = ESPI_PERIPHERAL_HOST_IO,
			.evt_data    = pdata & 0xFFU,
		});
		return;
	}
#endif
#ifdef CONFIG_ESPI_PERIPHERAL_CUSTOM_OPCODE
	if ((int8_t)port == data->custom_port) {
		espi_nxp_fire_callbacks(dev, (struct espi_event){
			.evt_type    = ESPI_BUS_PERIPHERAL_NOTIFICATION,
#ifdef CONFIG_ESPI_PERIPHERAL_EC_HOST_CMD
			.evt_details = ESPI_PERIPHERAL_EC_HOST_CMD,
#else
			.evt_details = ESPI_PERIPHERAL_HOST_IO_PVT,
#endif
			.evt_data    = 0,
		});
		espi_nxp_set_sstcl(base, port, kESPI_SSTCL_MailboxWrEmpty);
		return;
	}
#endif
#ifdef CONFIG_ESPI_PERIPHERAL_8042_KBC
	if ((int8_t)port == data->acpi_idx_port) {
		uint32_t idx, pdata;

		ESPI_GetEndpointData(base, port, &idx, &pdata);
		struct espi_evt_data_kbc kbc_evt = {
			.type = ESPI_PERIPHERAL_8042_KBC,
			.data = pdata & 0xFFU,
			.evt  = (status & ESPI_STAT_INTSPC0_MASK) ? 1U : 0U,
		};
		uint32_t evt_data;

		memcpy(&evt_data, &kbc_evt, sizeof(evt_data));
		espi_nxp_fire_callbacks(dev, (struct espi_event){
			.evt_type    = ESPI_BUS_PERIPHERAL_NOTIFICATION,
			.evt_details = ESPI_PERIPHERAL_8042_KBC,
			.evt_data    = evt_data,
		});
		return;
	}
#endif
#endif /* CONFIG_ESPI_PERIPHERAL_CHANNEL */

#ifdef CONFIG_ESPI_OOB_CHANNEL
	if ((int8_t)port == data->oob_port) {
		k_sem_give(&data->oob_rx_lock);
		espi_nxp_fire_callbacks(dev, (struct espi_event){
			.evt_type    = ESPI_BUS_EVENT_OOB_RECEIVED,
			.evt_details = ESPI_GetPortMsgLen(base, port),
			.evt_data    = 0,
		});
		return;
	}
#endif
}

static void espi_nxp_isr(const struct device *dev)
{
	struct espi_nxp_data *data = dev->data;
	const struct espi_nxp_config *config = dev->config;

	ESPI_TransferHandleIRQ(config->base, &data->handle);
}

static int espi_nxp_init(const struct device *dev)
{
	const struct espi_nxp_config *config = dev->config;
	espi_port_config_t espi_ports[ESPI_PORT_COUNT] = {0};
	struct espi_nxp_data *data = dev->data;
	ESPI_Type *base = config->base;
	espi_config_t espi_cfg;
	int ret;

	data->acpi_port   = PORT_NONE;
	data->acpi_idx_port = PORT_NONE;
	data->custom_port = PORT_NONE;
	data->oob_port    = PORT_NONE;
	data->flash_port  = PORT_NONE;
#ifdef CONFIG_ESPI_PERIPHERAL_8042_KBC
	data->kbc_irq_enabled = true;
#endif
#ifdef CONFIG_ESPI_OOB_CHANNEL
	k_sem_init(&data->oob_rx_lock, 0, 1);
#endif
#ifdef CONFIG_ESPI_FLASH_CHANNEL
	data->dev = dev;
	k_work_init(&data->taf_work, espi_nxp_taf_work);
#endif

	ret = clock_control_on(config->clock_dev, config->clock_subsys);
	if (ret < 0) {
		LOG_ERR("Failed to enable ESPI clock: %d", ret);
		return ret;
	}

	ret = pinctrl_apply_state(config->pincfg, PINCTRL_STATE_DEFAULT);
	if (ret < 0) {
		LOG_ERR("Failed to apply eSPI pinctrl: %d", ret);
		return ret;
	}

#ifdef CONFIG_ESPI_FLASH_CHANNEL
	if (config->flash_dev == NULL || !device_is_ready(config->flash_dev)) {
		LOG_ERR("Flash device not available for TAF");
		return -ENODEV;
	}
#endif

	if (config->port_count > ESPI_PORT_COUNT) {
		LOG_ERR("eSPI ports count is over the limitation: %u (max %u)",
			config->port_count, ESPI_PORT_COUNT);
		return -EINVAL;
	}

	for (uint8_t i = 0; i < config->port_count; i++) {
		const struct espi_nxp_port_cfg *p = &config->ports[i];

		espi_ports[i] = (espi_port_config_t){
			.type       = p->type,
			.direction  = (p->type == (uint8_t)kESPI_PortType_ACPIIndexData)
				      ? 1U : 0U,
			.ramOffset  = p->ram_offset,
			.ramSize    = (espi_ram_size_t)p->ram_size,
			.addrOffset = p->io_addr,
			.idxOffset  = p->idx_offset,
		};

		switch (p->type) {
		case (uint8_t)kESPI_PortType_ACPIEndpoint:
			data->acpi_port = i;
			break;
		case (uint8_t)kESPI_PortType_ACPIIndexData:
			data->acpi_idx_port = i;
			break;
		case (uint8_t)kESPI_PortType_MailboxSingle:
			data->custom_port = i;
			break;
		case (uint8_t)kESPI_PortType_MailboxOOBSplit:
			data->oob_port = i;
			break;
#ifdef CONFIG_ESPI_FLASH_CHANNEL
		case (uint8_t)kESPI_PortType_BusMasterFlashSingle:
			data->flash_port = i;
			break;
#endif
		default:
			break;
		}
	}

	ESPI_GetDefaultConfig(&espi_cfg);
	espi_cfg.ramBaseAddr    = config->ram_base_addr;
	espi_cfg.base0Addr      = config->map_base0;
	espi_cfg.base1Addr      = config->map_base1;
	espi_cfg.portConfig     = espi_ports;
	espi_cfg.portCount      = config->port_count;
	espi_cfg.enableP80      = IS_ENABLED(CONFIG_ESPI_PERIPHERAL_DEBUG_PORT_80);
	espi_cfg.enableAlertPin = true;
	espi_cfg.enableOOB      = (data->oob_port != PORT_NONE);
	espi_cfg.enableSAF      = IS_ENABLED(CONFIG_ESPI_FLASH_CHANNEL) &&
				  (data->flash_port != PORT_NONE);
	ESPI_Init(base, &espi_cfg);

	espi_callback_config_t cb_cfg = {
		.commonCallback = espi_nxp_common_cb,
		.portCallback = espi_nxp_port_cb,
	};
	ESPI_CreateHandle(base, &data->handle, &espi_cfg,
			  &cb_cfg, (void *)dev);

#ifdef CONFIG_ESPI_FLASH_CHANNEL
	ESPI_FlashCreateHandle(base, &data->handle,
			       espi_nxp_flash_ops, UINT32_MAX);
#endif

	/* Configure per-port interrupt enables and stall settings. */
	for (uint8_t i = 0; i < config->port_count; i++) {
		const struct espi_nxp_port_cfg *p = &config->ports[i];

		if (p->type == (uint8_t)kESPI_PortType_ACPIEndpoint ||
		    p->type == (uint8_t)kESPI_PortType_ACPIIndexData) {
			base->PORT[i].CFG |= ESPI_CFG_STALLRD_MASK |
					     ESPI_CFG_STALLWR_MASK;
		}

		ESPI_EnablePortInterrupts(base, i,
			kESPI_PortErrorInterrupt | kESPI_PortReadInterrupt |
			kESPI_PortWriteInterrupt | kESPI_PortSpec0Interrupt |
			kESPI_PortSpec1Interrupt | kESPI_PortSpec2Interrupt |
			kESPI_PortSpec3Interrupt);
	}

	ESPI_EnableInterrupts(base,
		kESPI_PortInterruptEnable | kESPI_BusResetInterruptEnable |
		kESPI_WireChangeInterruptEnable | kESPI_HostStallInterruptEnable |
		kESPI_CrcErrorInterruptEnable | kESPI_Port80InterruptEnable);

	config->irq_config_func(dev);

	return 0;
}

static DEVICE_API(espi, espi_nxp_driver_api) = {
	.config             = espi_nxp_config,
	.get_channel_status = espi_nxp_get_channel_status,
	.send_vwire         = espi_nxp_send_vwire,
	.receive_vwire      = espi_nxp_receive_vwire,
	.manage_callback    = espi_nxp_manage_callback,
#ifdef CONFIG_ESPI_PERIPHERAL_CHANNEL
	.read_lpc_request   = espi_nxp_read_lpc_request,
	.write_lpc_request  = espi_nxp_write_lpc_request,
#endif
#ifdef CONFIG_ESPI_OOB_CHANNEL
	.send_oob           = espi_nxp_send_oob,
	.receive_oob        = espi_nxp_receive_oob,
#endif
#ifdef CONFIG_ESPI_FLASH_CHANNEL
	.flash_read         = espi_nxp_flash_read,
	.flash_write        = espi_nxp_flash_write,
	.flash_erase        = espi_nxp_flash_erase,
#endif
};

#ifdef CONFIG_ESPI_FLASH_CHANNEL
#define ESPI_NXP_FLASH_DEV(n)						\
	.flash_dev = DEVICE_DT_GET_OR_NULL(				\
		DT_INST_PHANDLE(n, flash_dev)),
#else
#define ESPI_NXP_FLASH_DEV(n)
#endif

#define ESPI_NXP_PORT_CFG(child)					\
	{								\
		.type       = DT_PROP(child, port_type),		\
		.io_addr    = DT_PROP_OR(child, io_addr, 0),		\
		.ram_offset = DT_PROP_OR(child, ram_offset, 0),		\
		.ram_size   = DT_PROP_OR(child, ram_size, 0),		\
		.idx_offset = DT_PROP_OR(child, idx_offset, 0),		\
	},

#define ESPI_NXP_IRQ_CONFIG(n)						\
	static void espi_nxp_irq_config_##n(const struct device *dev)	\
	{								\
		IRQ_CONNECT(DT_INST_IRQN(n),				\
			    DT_INST_IRQ(n, priority),			\
			    espi_nxp_isr,				\
			    DEVICE_DT_INST_GET(n), 0);			\
		irq_enable(DT_INST_IRQN(n));				\
	}

#define ESPI_NXP_INIT(n)						\
	PINCTRL_DT_INST_DEFINE(n);					\
	ESPI_NXP_IRQ_CONFIG(n)						\
									\
	static const struct espi_nxp_port_cfg				\
		espi_nxp_ports_##n[] = {				\
		DT_INST_FOREACH_CHILD(n, ESPI_NXP_PORT_CFG)		\
	};								\
									\
	static const struct espi_nxp_config espi_nxp_config_##n = {	\
		.base = (ESPI_Type *)DT_INST_REG_ADDR(n),		\
		.clock_dev = DEVICE_DT_GET(DT_INST_CLOCKS_CTLR(n)),	\
		.clock_subsys = (clock_control_subsys_t)		\
			DT_INST_CLOCKS_CELL(n, name),			\
		.pincfg = PINCTRL_DT_INST_DEV_CONFIG_GET(n),		\
		.irq_config_func = espi_nxp_irq_config_##n,		\
		.ram_base_addr = DT_INST_PROP(n, ram_base_addr),	\
		.map_base0 = DT_INST_PROP(n, map_base0_addr),		\
		.map_base1 = DT_INST_PROP(n, map_base1_addr),		\
		.ports = espi_nxp_ports_##n,				\
		.port_count = ARRAY_SIZE(espi_nxp_ports_##n),		\
		ESPI_NXP_FLASH_DEV(n)					\
	};								\
									\
	static struct espi_nxp_data espi_nxp_data_##n;			\
									\
	DEVICE_DT_INST_DEFINE(n,					\
			      espi_nxp_init,				\
			      NULL,					\
			      &espi_nxp_data_##n,			\
			      &espi_nxp_config_##n,			\
			      POST_KERNEL,				\
			      CONFIG_ESPI_INIT_PRIORITY,		\
			      &espi_nxp_driver_api);

DT_INST_FOREACH_STATUS_OKAY(ESPI_NXP_INIT)
