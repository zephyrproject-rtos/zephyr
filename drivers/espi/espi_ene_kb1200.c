/*
 * Copyright (c) 2024 ENE Technology Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT ene_kb1200_espi

#include <zephyr/kernel.h>
#include <errno.h>
#include <string.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/drivers/espi.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/logging/log.h>

#include "espi_utils.h"
#include <reg/espi.h>
#include <reg/hif.h>
#include <reg/kbc.h>
#include <reg/eci.h>
#include <reg/dbi.h>

LOG_MODULE_REGISTER(espi, CONFIG_ESPI_LOG_LEVEL);

struct espi_kb1200_config {
	struct espi_regs *base_addr;
	struct espivw_regs *vw_addr;
	struct espioob_regs *oob_addr;
	struct espifa_regs *fa_addr;
	struct hif_regs *hif_addr;
	struct kbc_regs *kbc_addr;
	struct eci_regs *eci_addr;
	struct dbi_regs *dbi_addr;
	uintptr_t vwtab_addr;
	const struct pinctrl_dev_config *pcfg;
};

struct espi_kb1200_data {
	sys_slist_t callbacks;
	struct k_sem oob_tx_lock;
	struct k_sem oob_rx_lock;
	struct k_sem flash_lock;
};

struct ene_signal {
	uint8_t offset; /* HW table register offset */
	uint8_t index;  /* VW index */
	uint8_t bit;    /* VW data bit */
	uint8_t dir;    /* VW direction */
};

#ifdef CONFIG_ESPI_VWIRE_CHANNEL
/* VW signals used in eSPI */
static const struct ene_signal vw_in[] = {
	/* index 02h (In) */
	[ESPI_VWIRE_SIGNAL_SLP_S3] = {ENE_IDX02_OFS, 0x02, BIT(0), ESPI_CONTROLLER_TO_TARGET},
	[ESPI_VWIRE_SIGNAL_SLP_S4] = {ENE_IDX02_OFS, 0x02, BIT(1), ESPI_CONTROLLER_TO_TARGET},
	[ESPI_VWIRE_SIGNAL_SLP_S5] = {ENE_IDX02_OFS, 0x02, BIT(2), ESPI_CONTROLLER_TO_TARGET},
	/* index 03h (In) */
	[ESPI_VWIRE_SIGNAL_SUS_STAT] = {ENE_IDX03_OFS, 0x03, BIT(0), ESPI_CONTROLLER_TO_TARGET},
	[ESPI_VWIRE_SIGNAL_PLTRST] = {ENE_IDX03_OFS, 0x03, BIT(1), ESPI_CONTROLLER_TO_TARGET},
	[ESPI_VWIRE_SIGNAL_OOB_RST_WARN] = {ENE_IDX03_OFS, 0x03, BIT(2), ESPI_CONTROLLER_TO_TARGET},
	/* index 07h (In) */
	[ESPI_VWIRE_SIGNAL_HOST_RST_WARN] = {ENE_IDX07_OFS, 0x07, BIT(0),
					     ESPI_CONTROLLER_TO_TARGET},
	/* index 41h (In) */
	[ESPI_VWIRE_SIGNAL_SUS_WARN] = {ENE_IDX41_OFS, 0x41, BIT(0), ESPI_CONTROLLER_TO_TARGET},
	[ESPI_VWIRE_SIGNAL_SUS_PWRDN_ACK] = {ENE_IDX41_OFS, 0x41, BIT(1),
					     ESPI_CONTROLLER_TO_TARGET},
	[ESPI_VWIRE_SIGNAL_SLP_A] = {ENE_IDX41_OFS, 0x41, BIT(3), ESPI_CONTROLLER_TO_TARGET},
	/* index 42h (In) */
	[ESPI_VWIRE_SIGNAL_SLP_LAN] = {ENE_IDX42_OFS, 0x42, BIT(0), ESPI_CONTROLLER_TO_TARGET},
	[ESPI_VWIRE_SIGNAL_SLP_WLAN] = {ENE_IDX42_OFS, 0x42, BIT(1), ESPI_CONTROLLER_TO_TARGET},
	/* index 47h (In) */
	[ESPI_VWIRE_SIGNAL_HOST_C10] = {ENE_IDX47_OFS, 0x47, BIT(0), ESPI_CONTROLLER_TO_TARGET},
	/* index 4Ah (In) */
	[ESPI_VWIRE_SIGNAL_DNX_WARN] = {ENE_IDX4A_OFS, 0x4A, BIT(1), ESPI_CONTROLLER_TO_TARGET},
};
static const struct ene_signal vw_out[] = {
	/* index 04h (Out) */
	[ESPI_VWIRE_SIGNAL_OOB_RST_ACK] = {ENE_IDX04_OFS, 0x04, BIT(0), ESPI_TARGET_TO_CONTROLLER},
	[ESPI_VWIRE_SIGNAL_WAKE] = {ENE_IDX04_OFS, 0x04, BIT(2), ESPI_TARGET_TO_CONTROLLER},
	[ESPI_VWIRE_SIGNAL_PME] = {ENE_IDX04_OFS, 0x04, BIT(3), ESPI_TARGET_TO_CONTROLLER},
	/* index 05h (Out) */
	[ESPI_VWIRE_SIGNAL_TARGET_BOOT_DONE] = {ENE_IDX05_OFS, 0x05, BIT(0),
						ESPI_TARGET_TO_CONTROLLER},
	[ESPI_VWIRE_SIGNAL_ERR_FATAL] = {ENE_IDX05_OFS, 0x05, BIT(1), ESPI_TARGET_TO_CONTROLLER},
	[ESPI_VWIRE_SIGNAL_ERR_NON_FATAL] = {ENE_IDX05_OFS, 0x05, BIT(2),
					     ESPI_TARGET_TO_CONTROLLER},
	[ESPI_VWIRE_SIGNAL_TARGET_BOOT_STS] = {ENE_IDX05_OFS, 0x05, BIT(3),
					       ESPI_TARGET_TO_CONTROLLER},
	/* index 06h (Out) */
	/* System control interrupt */
	[ESPI_VWIRE_SIGNAL_SCI] = {ENE_IDX06_OFS, 0x06, BIT(0), ESPI_TARGET_TO_CONTROLLER},
	/* System management interrupt */
	[ESPI_VWIRE_SIGNAL_SMI] = {ENE_IDX06_OFS, 0x06, BIT(1), ESPI_TARGET_TO_CONTROLLER},
	[ESPI_VWIRE_SIGNAL_RST_CPU_INIT] = {ENE_IDX06_OFS, 0x06, BIT(2), ESPI_TARGET_TO_CONTROLLER},
	[ESPI_VWIRE_SIGNAL_HOST_RST_ACK] = {ENE_IDX06_OFS, 0x06, BIT(3), ESPI_TARGET_TO_CONTROLLER},
	/* index 40h (Out) */
	[ESPI_VWIRE_SIGNAL_SUS_ACK] = {ENE_IDX40_OFS, 0x40, BIT(0), ESPI_TARGET_TO_CONTROLLER},
	[ESPI_VWIRE_SIGNAL_DNX_ACK] = {ENE_IDX40_OFS, 0x40, BIT(1), ESPI_TARGET_TO_CONTROLLER},

	[ESPI_VWIRE_SIGNAL_TARGET_GPIO_0] = {ENE_IDX60_OFS, 0x60, BIT(1),
					     ESPI_TARGET_TO_CONTROLLER},
	[ESPI_VWIRE_SIGNAL_TARGET_GPIO_1] = {ENE_IDX61_OFS, 0x61, BIT(2),
					     ESPI_TARGET_TO_CONTROLLER},
	[ESPI_VWIRE_SIGNAL_TARGET_GPIO_2] = {ENE_IDX64_OFS, 0x64, BIT(3),
					     ESPI_TARGET_TO_CONTROLLER},
	[ESPI_VWIRE_SIGNAL_TARGET_GPIO_3] = {ENE_IDX67_OFS, 0x67, BIT(0),
					     ESPI_TARGET_TO_CONTROLLER},
};

char *st_list[] = {
	/* index 02h (In) */
	[ESPI_VWIRE_SIGNAL_SLP_S3] = "ESPI_VWIRE_SIGNAL_SLP_S3        ",
	[ESPI_VWIRE_SIGNAL_SLP_S4] = "ESPI_VWIRE_SIGNAL_SLP_S4        ",
	[ESPI_VWIRE_SIGNAL_SLP_S5] = "ESPI_VWIRE_SIGNAL_SLP_S5        ",
	/* index 03h (In) */
	[ESPI_VWIRE_SIGNAL_SUS_STAT] = "ESPI_VWIRE_SIGNAL_SUS_STAT      ",
	[ESPI_VWIRE_SIGNAL_PLTRST] = "ESPI_VWIRE_SIGNAL_PLTRST        ",
	[ESPI_VWIRE_SIGNAL_OOB_RST_WARN] = "ESPI_VWIRE_SIGNAL_OOB_RST_WARN  ",
	/* index 04h (Out) */
	[ESPI_VWIRE_SIGNAL_OOB_RST_ACK] = "ESPI_VWIRE_SIGNAL_OOB_RST_ACK   ",
	[ESPI_VWIRE_SIGNAL_WAKE] = "ESPI_VWIRE_SIGNAL_WAKE          ",
	[ESPI_VWIRE_SIGNAL_PME] = "ESPI_VWIRE_SIGNAL_PME           ",
	/* index 05h (Out) */
	[ESPI_VWIRE_SIGNAL_TARGET_BOOT_DONE] = "ESPI_VWIRE_SIGNAL_TARGET_BOOT_DONE ",
	[ESPI_VWIRE_SIGNAL_ERR_FATAL] = "ESPI_VWIRE_SIGNAL_ERR_FATAL     ",
	[ESPI_VWIRE_SIGNAL_ERR_NON_FATAL] = "ESPI_VWIRE_SIGNAL_ERR_NON_FATAL ",
	[ESPI_VWIRE_SIGNAL_TARGET_BOOT_STS] = "ESPI_VWIRE_SIGNAL_TARGET_BOOT_STS  ",
	/* index 06h (Out) */
	/* System control interrupt */
	[ESPI_VWIRE_SIGNAL_SCI] = "ESPI_VWIRE_SIGNAL_SCI           ",
	/* System management interrupt */
	[ESPI_VWIRE_SIGNAL_SMI] = "ESPI_VWIRE_SIGNAL_SMI           ",
	[ESPI_VWIRE_SIGNAL_RST_CPU_INIT] = "ESPI_VWIRE_SIGNAL_RST_CPU_INIT  ",
	[ESPI_VWIRE_SIGNAL_HOST_RST_ACK] = "ESPI_VWIRE_SIGNAL_HOST_RST_ACK  ",
	/* index 07h (In) */
	[ESPI_VWIRE_SIGNAL_HOST_RST_WARN] = "ESPI_VWIRE_SIGNAL_HOST_RST_WARN ",
	/* index 40h (Out) */
	[ESPI_VWIRE_SIGNAL_SUS_ACK] = "ESPI_VWIRE_SIGNAL_SUS_ACK       ",
	[ESPI_VWIRE_SIGNAL_DNX_ACK] = "ESPI_VWIRE_SIGNAL_DNX_ACK       ",
	/* index 41h (In) */
	[ESPI_VWIRE_SIGNAL_SUS_WARN] = "ESPI_VWIRE_SIGNAL_SUS_WARN      ",
	[ESPI_VWIRE_SIGNAL_SUS_PWRDN_ACK] = "ESPI_VWIRE_SIGNAL_SUS_PWRDN_ACK ",
	[ESPI_VWIRE_SIGNAL_SLP_A] = "ESPI_VWIRE_SIGNAL_SLP_A         ",
	/* index 42h (In) */
	[ESPI_VWIRE_SIGNAL_SLP_LAN] = "ESPI_VWIRE_SIGNAL_SLP_LAN       ",
	[ESPI_VWIRE_SIGNAL_SLP_WLAN] = "ESPI_VWIRE_SIGNAL_SLP_WLAN      ",
	/* index 47h (In) */
	[ESPI_VWIRE_SIGNAL_HOST_C10] = "ESPI_VWIRE_SIGNAL_HOST_C10      ",
	/* index 4Ah (In) */
	[ESPI_VWIRE_SIGNAL_DNX_WARN] = "ESPI_VWIRE_SIGNAL_DNX_WARN      ",

	[ESPI_VWIRE_SIGNAL_TARGET_GPIO_0] = "ESPI_VWIRE_SIGNAL_TARGET_GPIO_0    ",
	[ESPI_VWIRE_SIGNAL_TARGET_GPIO_1] = "ESPI_VWIRE_SIGNAL_TARGET_GPIO_1    ",
	[ESPI_VWIRE_SIGNAL_TARGET_GPIO_2] = "ESPI_VWIRE_SIGNAL_TARGET_GPIO_2    ",
	[ESPI_VWIRE_SIGNAL_TARGET_GPIO_3] = "ESPI_VWIRE_SIGNAL_TARGET_GPIO_3    ",
};
#endif /* CONFIG_ESPI_VWIRE_CHANNEL */

#ifdef CONFIG_ESPI_PERIPHERAL_CHANNEL
#if defined(CONFIG_ESPI_PERIPHERAL_EC_HOST_CMD) && defined(CONFIG_ESPI_PERIPHERAL_ACPI_SHM_REGION)
/*
 * Host IO to SRAM (IO2RAM) memory mapping.
 * This feature allows host access EC's memory directly by eSPI I/O cycles.
 * Mapping range is 128 bytes and base address is adjustable.
 * Eg. the I/O cycle 800h~8ffh from host can be mapped to x800h~x8ffh.
 * Linker script will make the pool 128 aligned.
 */
static uint8_t shm_acpi_mmap[ENE_ESPI_IO2RAM_SIZE_MAX] __aligned(128);
#endif /* CONFIG_ESPI_PERIPHERAL_ACPI_SHM_REGION */
#endif /* CONFIG_ESPI_PERIPHERAL_CHANNEL */

/* eSPI api functions */
static int espi_kb1200_configure(const struct device *dev, struct espi_cfg *cfg)
{
	const struct espi_kb1200_config *config = dev->config;
	struct espi_regs *espi = config->base_addr;
	uint8_t io_mode;
	uint8_t channel_caps;
	uint8_t max_freq;

	/* Set frequency */
	switch (cfg->max_freq) {
	case 20:
		max_freq = ESPI_FREQ_MAX_20M;
		break;
	case 25:
		max_freq = ESPI_FREQ_MAX_25M;
		break;
	case 33:
		max_freq = ESPI_FREQ_MAX_33M;
		break;
	case 50:
		max_freq = ESPI_FREQ_MAX_50M;
		break;
	case 66:
		max_freq = ESPI_FREQ_MAX_66M;
		break;
	default:
		return -EINVAL;
	}
	/* Set IO mode */
	/* always support Single mode, just check dual/quad mode */
	io_mode = (cfg->io_caps >> 1);
	if (io_mode > ESPI_IOMODE_MASK) {
		return -EINVAL;
	}
	/* Configure eSPI supported channels */
	channel_caps = cfg->channel_caps & ESPI_CH_SUPPORT_MASK;
	espi->ESPIGENCFG = (io_mode << ESPI_IOMODE_POS) | (ESPI_ALERT_OD << ESPI_ALERT_POS) |
			   (max_freq << ESPI_FREQ_POS) | channel_caps;

	return 0;
}

static bool espi_kb1200_channel_ready(const struct device *dev, enum espi_channel ch)
{
	const struct espi_kb1200_config *config = dev->config;
	struct espi_regs *espi = config->base_addr;
	bool sts;

	switch (ch) {
	case ESPI_CHANNEL_PERIPHERAL:
		sts = espi->ESPIC0CFG & ESPI_CH0_READY;
		break;
	case ESPI_CHANNEL_VWIRE:
		sts = espi->ESPIC1CFG & ESPI_CH1_READY;
		break;
	case ESPI_CHANNEL_OOB:
		sts = espi->ESPIC2CFG & ESPI_CH2_READY;
		break;
	case ESPI_CHANNEL_FLASH:
		sts = espi->ESPIC3CFG & ESPI_CH3_READY;
		break;
	default:
		sts = false;
		break;
	}

	return sts;
}

static int espi_kb1200_manage_callback(const struct device *dev, struct espi_callback *callback,
				       bool set)
{
	struct espi_kb1200_data *data = (struct espi_kb1200_data *)(dev->data);

	return espi_manage_callback(&data->callbacks, callback, set);
}

#ifdef CONFIG_ESPI_PERIPHERAL_CHANNEL
static int espi_kb1200_read_lpc_request(const struct device *dev, enum lpc_peripheral_opcode op,
					uint32_t *data)
{
	const struct espi_kb1200_config *const config = dev->config;

	if (op >= E8042_START_OPCODE && op <= E8042_MAX_OPCODE) {
		struct kbc_regs *kbc = config->kbc_addr;

		switch (op) {
		case E8042_OBF_HAS_CHAR:
			/*
			 * EC has written data back to host. OBF is
			 * automatically cleared after host reads
			 * the data
			 */
			*data = !!(kbc->KBCSTS & KBSTS_OBF);
			break;
		case E8042_IBF_HAS_CHAR:
			*data = !!(kbc->KBCSTS & KBSTS_IBF);
			break;
		case E8042_READ_KB_STS:
			*data = kbc->KBCSTS;
			break;
		default:
			return -EINVAL;
		}
	} else if (op >= EACPI_START_OPCODE && op <= EACPI_MAX_OPCODE) {
		struct eci_regs *eci = config->eci_addr;

		switch (op) {
		case EACPI_OBF_HAS_CHAR:
			/*
			 * EC has written data back to host. OBF is
			 * automatically cleared after host reads
			 * the data
			 */
			*data = !!(eci->ECISTS & ECISTS_OBF);
			break;
		case EACPI_IBF_HAS_CHAR:
			*data = !!(eci->ECISTS & ECISTS_IBF);
			break;
		case EACPI_READ_STS:
			*data = eci->ECISTS;
			break;
#ifdef CONFIG_ESPI_PERIPHERAL_ACPI_SHM_REGION
		case EACPI_GET_SHARED_MEMORY:
			*data = (uint32_t)shm_acpi_mmap;
			break;
#endif /* CONFIG_ESPI_PERIPHERAL_ACPI_SHM_REGION */
		default:
			return -EINVAL;
		}
	} else {
		return -ENOTSUP;
	}

	return 0;
}

static int espi_kb1200_write_lpc_request(const struct device *dev, enum lpc_peripheral_opcode op,
					 uint32_t *data)
{
	const struct espi_kb1200_config *const config = dev->config;

	if (op >= E8042_START_OPCODE && op <= E8042_MAX_OPCODE) {
		struct kbc_regs *kbc = config->kbc_addr;

		switch (op) {
		case E8042_WRITE_KB_CHAR:
			/* Clear Auxiliary data flag */
			kbc->KBCSTS &= ~KBSTS_AUX_FLAG;
			kbc->KBCODP = (uint8_t)*data;
			/*
			 * Enable OBE interrupt after putting data in
			 * data register.
			 */
			kbc->KBCIE |= KBC_OBF_EVENT;
			break;
		case E8042_WRITE_MB_CHAR:
			/* Set Auxiliary data flag */
			kbc->KBCSTS |= KBSTS_AUX_FLAG;
			kbc->KBCODP = (uint8_t)*data;
			/*
			 * Enable OBE interrupt after putting data in
			 * data register.
			 */
			kbc->KBCIE |= KBC_OBF_EVENT;
			break;
		case E8042_RESUME_IRQ:
			/* Enable KBC IBF interrupt */
			kbc->KBCIE |= KBC_IBF_EVENT;
			break;
		case E8042_PAUSE_IRQ:
			/* Disable KBC IBF interrupt */
			kbc->KBCIE &= ~KBC_IBF_EVENT;
			break;
		case E8042_CLEAR_OBF:
			/* Clear OBF flag */
			kbc->KBCSTS |= KBC_OBF_EVENT;
			break;
		case E8042_SET_FLAG:
			/* FW shouldn't modify these flags directly */
			*data &= ~(KBSTS_OBF | KBSTS_IBF | KBSTS_AUX_FLAG);
			*data = kbc->KBCSTS | (*data);
			kbc->KBCSTS = (*data) & ~(KBSTS_OBF | KBSTS_IBF);
			break;
		case E8042_CLEAR_FLAG:
			/* FW shouldn't modify these flags directly */
			*data &= ~(KBSTS_OBF | KBSTS_IBF | KBSTS_AUX_FLAG);
			*data = kbc->KBCSTS & ~(*data);
			kbc->KBCSTS = (*data) & ~(KBSTS_OBF | KBSTS_IBF);
			break;
		default:
			return -EINVAL;
		}
	} else if (op >= EACPI_START_OPCODE && op <= EACPI_MAX_OPCODE) {
		struct eci_regs *eci = config->eci_addr;

		switch (op) {
		case EACPI_WRITE_CHAR:
			eci->ECIODP = (uint8_t)*data;
			break;
		case EACPI_WRITE_STS:
			eci->ECISTS = (uint8_t)*data;
			break;
		default:
			return -EINVAL;
		}
	} else {
		return -ENOTSUP;
	}

	return 0;
}
#endif /* CONFIG_ESPI_PERIPHERAL_CHANNEL */

#ifdef CONFIG_ESPI_VWIRE_CHANNEL
static int espi_kb1200_send_vwire(const struct device *dev, enum espi_vwire_signal signal,
				  uint8_t level)
{
	const struct espi_kb1200_config *config = dev->config;
	struct espivw_regs *espivw = config->vw_addr;
	struct ene_signal signal_info = vw_out[signal];
	uint8_t *vwtab = (uint8_t *)(config->vwtab_addr + signal_info.offset);
	uint8_t vwdata;

	if (signal >= ESPI_VWIRE_SIGNAL_COUNT) {
		LOG_ERR("Invalid VW: %d", signal);
		return -EINVAL;
	}

	if (!vw_out[signal].index) {
		LOG_ERR("%s signal %d is invalid", __func__, signal);
		return -EIO;
	}

	vwdata = *vwtab;
	if (level) {
		vwdata |= signal_info.bit;
	} else {
		vwdata &= ~signal_info.bit;
	}
	vwdata |= (signal_info.bit << ESPIVW_VALIDBIT_POS);

	if (signal_info.dir == ESPI_TARGET_TO_CONTROLLER) {
		espivw->ESPIVWTX = (signal_info.index << ESPIVW_TXINDEX_POS) | vwdata;
	}
	return 0;
}

static int espi_kb1200_receive_vwire(const struct device *dev, enum espi_vwire_signal signal,
				     uint8_t *level)
{
	const struct espi_kb1200_config *config = dev->config;
	struct espivw_regs *espivw = config->vw_addr;
	struct ene_signal signal_info = vw_in[signal];
	uint8_t *vwtab = (uint8_t *)(espivw->ESPIVWIDX + signal_info.offset);
	uint8_t vwdata;

	if (signal >= ESPI_VWIRE_SIGNAL_COUNT) {
		LOG_ERR("Invalid VW: %d", signal);
		return -EINVAL;
	}
	if ((!vw_out[signal].index) && (!vw_in[signal].index)) {
		LOG_ERR("%s signal %d is invalid", __func__, signal);
		return -EIO;
	}
	vwdata = *vwtab >> ESPIVW_VALIDBIT_POS;
	vwdata &= *vwtab;
	*level = !!(vwdata & signal_info.bit);
	/* clear valid bit */
	*vwtab = (signal_info.bit) << ESPIVW_VALIDBIT_POS;
	return 0;
}
#endif /* CONFIG_ESPI_VWIRE_CHANNEL */

#ifdef CONFIG_ESPI_OOB_CHANNEL
static int espi_kb1200_send_oob(const struct device *dev, struct espi_oob_packet *pckt)
{
	int ret;
	const struct espi_kb1200_config *config = dev->config;
	struct espi_kb1200_data *data = dev->data;
	struct espi_regs *espi = config->base_addr;
	struct espioob_regs *espioob = config->oob_addr;

	if (!(espi->ESPIC2CFG & ESPI_CH2_ENABLE)) {
		LOG_ERR("OOB channel is disabled");
		return -EIO;
	}

	if (pckt->len > ESPIOOB_BUFSIZE) {
		LOG_ERR("%s insufficient space", __func__);
		return -EINVAL;
	}

	memcpy((void *)espioob->ESPIOOBDAT, pckt->buf, pckt->len);
	espioob->ESPIOOBTX = pckt->len;
	espioob->ESPIOOBTX |= ESPIOOB_TX_ISSUE;

	/* Wait until ISR or timeout */
	ret = k_sem_take(&data->oob_tx_lock, K_MSEC(ESPIOOB_MAX_TIMEOUT));
	if (ret == -EAGAIN) {
		return -ETIMEDOUT;
	}

	if (espioob->ESPIOOBEF) {
		LOG_ERR("OOB Tx failed (Error Flag:%x)", espioob->ESPIOOBEF);
		/* Clear all error flag */
		espioob->ESPIOOBEF = espioob->ESPIOOBEF;
		return -EIO;
	}

	return 0;
}

static int espi_kb1200_receive_oob(const struct device *dev, struct espi_oob_packet *pckt)
{
	const struct espi_kb1200_config *config = dev->config;
	struct espioob_regs *espioob = config->oob_addr;

	if (espioob->ESPIOOBEF) {
		return -EIO;
	}
	/* if not set RX_ASYNC, wait the Rx event after send_oob */
#ifndef CONFIG_ESPI_OOB_CHANNEL_RX_ASYNC
	int ret;
	struct espi_kb1200_data *data = dev->data;

	/* Wait until ISR or timeout */
	ret = k_sem_take(&data->oob_rx_lock, K_MSEC(ESPIOOB_MAX_TIMEOUT));
	if (ret == -EAGAIN) {
		return -ETIMEDOUT;
	}
#endif
	/* Check if buffer passed to driver can fit the received buffer */
	uint32_t rcvd_len = espioob->ESPIOOBRL;

	if (rcvd_len > pckt->len) {
		LOG_ERR("space rcvd %d vs %d", rcvd_len, pckt->len);
		return -EIO;
	}

	pckt->len = rcvd_len;
	memcpy(pckt->buf, (void *)espioob->ESPIOOBDAT, pckt->len);

	return 0;
}
#endif /* CONFIG_ESPI_OOB_CHANNEL */

#ifdef CONFIG_ESPI_FLASH_CHANNEL
#define CONFIG_ESPI_FLASH_BUFFER_SIZE ESPIFA_BUFSIZE
static int espi_kb1200_flash_read(const struct device *dev, struct espi_flash_packet *pckt)
{
	int ret;
	const struct espi_kb1200_config *config = dev->config;
	struct espi_kb1200_data *data = dev->data;
	struct espi_regs *espi = config->base_addr;
	struct espifa_regs *espifa = config->fa_addr;

	if (!(espi->ESPIC3CFG & ESPI_CH3_ENABLE)) {
		LOG_ERR("Flash channel is disabled");
		return -EIO;
	}

	if (pckt->len > CONFIG_ESPI_FLASH_BUFFER_SIZE) {
		LOG_ERR("Invalid size request");
		return -EINVAL;
	}

	espifa->ESPIFABA = pckt->flash_addr;
	espifa->ESPIFACNT = pckt->len;
	espifa->ESPIFAPTCL = ESPIFA_READ;

	/* Wait until ISR or timeout */
	ret = k_sem_take(&data->flash_lock, K_MSEC(ESPIFA_MAX_TIMEOUT));
	if (ret == -EAGAIN) {
		LOG_ERR("%s timeout", __func__);
		return -ETIMEDOUT;
	}

	if (espifa->ESPIFAEF) {
		LOG_ERR("FLASH Tx failed (Error Flag:%x)", espifa->ESPIFAEF);
		/* Clear all error flag */
		espifa->ESPIFAEF = espifa->ESPIFAEF;
		return -EIO;
	}

	memcpy(pckt->buf, (void *)espifa->ESPIFADAT, pckt->len);

	return 0;
}

static int espi_kb1200_flash_write(const struct device *dev, struct espi_flash_packet *pckt)
{
	int ret;
	const struct espi_kb1200_config *config = dev->config;
	struct espi_kb1200_data *data = dev->data;
	struct espi_regs *espi = config->base_addr;
	struct espifa_regs *espifa = config->fa_addr;

	if (pckt->len > CONFIG_ESPI_FLASH_BUFFER_SIZE) {
		LOG_ERR("Packet length %d is too big", pckt->len);
		return -ENOMEM;
	}

	if (!(espi->ESPIC3CFG & ESPI_CH3_ENABLE)) {
		LOG_ERR("Flash channel is disabled");
		return -EIO;
	}

	memcpy((void *)espifa->ESPIFADAT, pckt->buf, pckt->len);

	espifa->ESPIFABA = pckt->flash_addr;
	espifa->ESPIFACNT = pckt->len;
	espifa->ESPIFAPTCL = ESPIFA_WRITE;

	/* Wait until ISR or timeout */
	ret = k_sem_take(&data->flash_lock, K_MSEC(ESPIFA_MAX_TIMEOUT));
	if (ret == -EAGAIN) {
		LOG_ERR("%s timeout", __func__);
		return -ETIMEDOUT;
	}

	if (espifa->ESPIFAEF) {
		LOG_ERR("FLASH Tx failed (Error Flag:%x)", espifa->ESPIFAEF);
		/* Clear all error flag */
		espifa->ESPIFAEF = espifa->ESPIFAEF;
		return -EIO;
	}

	return 0;
}

static int espi_kb1200_flash_erase(const struct device *dev, struct espi_flash_packet *pckt)
{
	int ret;
	const struct espi_kb1200_config *config = dev->config;
	struct espi_kb1200_data *data = dev->data;
	struct espi_regs *espi = config->base_addr;
	struct espifa_regs *espifa = config->fa_addr;

	if (!(espi->ESPIC3CFG & ESPI_CH3_ENABLE)) {
		LOG_ERR("Flash channel is disabled");
		return -EIO;
	}

	espifa->ESPIFABA = pckt->flash_addr;
	espifa->ESPIFAPTCL = ESPIFA_ERASE;

	/* Wait until ISR or timeout */
	ret = k_sem_take(&data->flash_lock, K_MSEC(ESPIFA_MAX_TIMEOUT));
	if (ret == -EAGAIN) {
		LOG_ERR("%s timeout", __func__);
		return -ETIMEDOUT;
	}

	if (espifa->ESPIFAEF) {
		LOG_ERR("FLASH Tx failed (Error Flag:%x)", espifa->ESPIFAEF);
		/* Clear all error flag */
		espifa->ESPIFAEF = espifa->ESPIFAEF;
		return -EIO;
	}

	return 0;
}
#endif /* CONFIG_ESPI_FLASH_CHANNEL */

static const struct espi_driver_api espi_kb1200_driver_api = {
	.config = espi_kb1200_configure,
	.get_channel_status = espi_kb1200_channel_ready,
	.manage_callback = espi_kb1200_manage_callback,
#ifdef CONFIG_ESPI_PERIPHERAL_CHANNEL
	.read_lpc_request = espi_kb1200_read_lpc_request,
	.write_lpc_request = espi_kb1200_write_lpc_request,
#endif
#ifdef CONFIG_ESPI_VWIRE_CHANNEL
	.send_vwire = espi_kb1200_send_vwire,
	.receive_vwire = espi_kb1200_receive_vwire,
#endif
#ifdef CONFIG_ESPI_OOB_CHANNEL
	.send_oob = espi_kb1200_send_oob,
	.receive_oob = espi_kb1200_receive_oob,
#endif
#ifdef CONFIG_ESPI_FLASH_CHANNEL
	.flash_read = espi_kb1200_flash_read,
	.flash_write = espi_kb1200_flash_write,
	.flash_erase = espi_kb1200_flash_erase,
#endif
};

#if defined(CONFIG_ESPI_PERIPHERAL_8042_KBC)
static void kbc_kb1200_isr(const struct device *dev)
{
	const struct espi_kb1200_config *const config = dev->config;
	struct espi_kb1200_data *data = dev->data;
	struct kbc_regs *kbc = config->kbc_addr;
	struct espi_event evt = {
		.evt_type = ESPI_BUS_PERIPHERAL_NOTIFICATION,
		.evt_details = ESPI_PERIPHERAL_8042_KBC,
		.evt_data = ESPI_PERIPHERAL_NODATA,
	};
	struct espi_evt_data_kbc *kbc_evt = (struct espi_evt_data_kbc *)&evt.evt_data;

	if (kbc->KBCPF & KBC_IBF_EVENT) {
		/* Clear IBF */
		kbc->KBCPF = KBC_IBF_EVENT;
		/* KBC Input Buffer Full event */
		kbc_evt->evt = HOST_KBC_EVT_IBF;
		/*
		 * The data in KBC Input Buffer
		 * Indicates if the host sent a command or data.
		 * 0 = data 1 = Command.
		 */
		if (kbc->KBCSTS & KBSTS_ADDRESS_64) {
			kbc_evt->data = kbc->KBCCMD;
			kbc_evt->type = 1;
		} else {
			kbc_evt->data = kbc->KBCIDP;
			kbc_evt->type = 0;
		}
		espi_send_callbacks(&data->callbacks, dev, evt);
		/* Clear Status registr IBF (notify host event finish) */
		kbc->KBCSTS = (kbc->KBCSTS & ~(KBSTS_OBF | KBSTS_IBF)) | KBSTS_IBF;
	}

	if (kbc->KBCPF & KBC_OBF_EVENT) {
		/* Disable KBC OBE interrupt first */
		kbc->KBCIE &= ~KBC_OBF_EVENT;
		/* Clear OBF */
		kbc->KBCPF = KBC_OBF_EVENT;
		/*
		 * Notify application that host already read out data. The application
		 * might need to clear status register via espi_api_lpc_write_request()
		 * with E8042_CLEAR_FLAG opcode in callback.
		 */
		kbc_evt->evt = HOST_KBC_EVT_OBE;
		kbc_evt->data = 0;
		kbc_evt->type = 0;
		espi_send_callbacks(&data->callbacks, dev, evt);
	}
}
#endif

#if defined(CONFIG_ESPI_PERIPHERAL_HOST_IO)
static void ec_kb1200_isr(const struct device *dev)
{
	const struct espi_kb1200_config *const config = dev->config;
	struct espi_kb1200_data *data = dev->data;
	struct eci_regs *eci = config->eci_addr;
	struct espi_event evt = {
		.evt_type = ESPI_BUS_PERIPHERAL_NOTIFICATION,
		.evt_details = ESPI_PERIPHERAL_HOST_IO,
		.evt_data = ESPI_PERIPHERAL_NODATA,
	};
	struct espi_evt_data_acpi *acpi_evt = (struct espi_evt_data_acpi *)&evt.evt_data;

	if (eci->ECIPF & ECI_IBF_EVENT) {
		/* Clear IBF */
		eci->ECIPF = ECI_IBF_EVENT;
		/*
		 * Indicates if the host sent a command or data.
		 * 0 = data
		 * 1 = Command.
		 */
		if (eci->ECISTS & ECISTS_ADDRESS_CMD_PORT) {
			acpi_evt->data = eci->ECICMD;
			acpi_evt->type = 1;
		} else {
			acpi_evt->data = eci->ECIIDP;
			acpi_evt->type = 0;
		}

		espi_send_callbacks(&data->callbacks, dev, evt);
		/* Clear Status registr IBF (notify host event finish) */
		eci->ECISTS = (eci->ECISTS & ~(ECISTS_OBF | ECISTS_IBF)) | ECISTS_IBF;
	}

	if (eci->ECIPF & ECI_OBF_EVENT) {
		/* Clear OBF */
		eci->ECIPF = ECI_OBF_EVENT;
		espi_send_callbacks(&data->callbacks, dev, evt);
	}
}
#endif

#if defined(CONFIG_ESPI_PERIPHERAL_EC_HOST_CMD) && defined(CONFIG_ESPI_PERIPHERAL_ACPI_SHM_REGION)
static void iotosram_kb1200_isr(const struct device *dev)
{
	const struct espi_kb1200_config *const config = dev->config;
	struct espi_kb1200_data *data = dev->data;
	struct hif_regs *hif = config->hif_addr;
	struct espi_event evt = {
		.evt_type = ESPI_BUS_PERIPHERAL_NOTIFICATION,
		.evt_details = ESPI_PERIPHERAL_EC_HOST_CMD,
		.evt_data = ESPI_PERIPHERAL_NODATA,
	};

	if (hif->IOSPF & IO2SRAM_WRITE_EVENT) {
		hif->IOSPF = IO2SRAM_WRITE_EVENT;
		evt.evt_data = shm_acpi_mmap[0];
		espi_send_callbacks(&data->callbacks, dev, evt);
	}
}
#endif

#if defined(CONFIG_ESPI_PERIPHERAL_DEBUG_PORT_80)
static void dbi_kb1200_isr(const struct device *dev)
{
	const struct espi_kb1200_config *const config = dev->config;
	struct espi_kb1200_data *data = dev->data;
	struct dbi_regs *dbi0 = config->dbi_addr;
	struct dbi_regs *dbi1 = dbi0 + 1;

	if (dbi0->DBIPF & DBI_RX_EVENT) {
		struct espi_event evt = {
			.evt_type = ESPI_BUS_PERIPHERAL_NOTIFICATION,
			.evt_details =
				(ESPI_PERIPHERAL_INDEX_0 << 16) | ESPI_PERIPHERAL_DEBUG_PORT80,
			.evt_data = ESPI_PERIPHERAL_NODATA,
		};

		dbi0->DBIPF = DBI_RX_EVENT;
		evt.evt_data = dbi0->DBIIDP;
		espi_send_callbacks(&data->callbacks, dev, evt);
	}

	if (dbi1->DBIPF & DBI_RX_EVENT) {
		struct espi_event evt = {
			.evt_type = ESPI_BUS_PERIPHERAL_NOTIFICATION,
			.evt_details =
				(ESPI_PERIPHERAL_INDEX_1 << 16) | ESPI_PERIPHERAL_DEBUG_PORT80,
			.evt_data = ESPI_PERIPHERAL_NODATA,
		};

		dbi1->DBIPF = DBI_RX_EVENT;
		evt.evt_data = dbi1->DBIIDP;
		espi_send_callbacks(&data->callbacks, dev, evt);
	}
}
#endif

#ifdef CONFIG_ESPI_VWIRE_CHANNEL
static void espi_vw_kb1200_isr(const struct device *dev)
{
	const struct espi_kb1200_config *config = dev->config;
	struct espi_kb1200_data *data = dev->data;
	struct espivw_regs *espivw = config->vw_addr;
	uint8_t *vwtab = (uint8_t *)(espivw->ESPIVWIDX);

	/* eSPI VW Rx event */
	if (espivw->ESPIVWPF & ESPIVW_RX_EVENT) {
		espivw->ESPIVWPF = ESPIVW_RX_EVENT;
		while (espivw->ESPIVWRXV & ESPIVW_RX_VALID_FLAG) {
			struct espi_event evt = {
				.evt_type = ESPI_BUS_EVENT_VWIRE_RECEIVED,
				.evt_details = 0,
				.evt_data = 0,
			};
			uint8_t vwdata;
			uint8_t read_index;

			read_index = espivw->ESPIVWRXI;
			espivw->ESPIVWRXV = ESPIVW_RX_VALID_FLAG;
			for (int i = 0; i < ARRAY_SIZE(vw_in); i++) {
				if (vw_in[i].index == read_index) {
					/* get all valid bit */
					vwdata = vwtab[vw_in[i].offset] >> ESPIVW_VALIDBIT_POS;
					if (vw_in[i].bit & vwdata) {
						evt.evt_details = i;
						evt.evt_data =
							!!(vwtab[vw_in[i].offset] & vw_in[i].bit);
						espi_send_callbacks(&data->callbacks, dev, evt);
						/* clear current valid bit ? */
						vwtab[vw_in[i].offset] = vw_in[i].bit
									 << ESPIVW_VALIDBIT_POS;
					}
				}
			}
		}
	}

	/* eSPI VW Tx event */
	if (espivw->ESPIVWPF & ESPIVW_TX_EVENT) {
		espivw->ESPIVWPF = ESPIVW_TX_EVENT;
		/* Clear All Tx valid flag */
		for (int i = 0; i < ARRAY_SIZE(vw_out); i++) {
			if (vwtab[vw_out[i].offset] & ESPIVW_VALIDBIT_MASK) {
				vwtab[vw_out[i].offset] &= ~ESPIVW_VALIDBIT_MASK;
			}
		}
	}
}
#endif

#ifdef CONFIG_ESPI_OOB_CHANNEL
static void espi_oob_kb1200_isr(const struct device *dev)
{
	const struct espi_kb1200_config *config = dev->config;
	struct espi_kb1200_data *data = dev->data;
	struct espioob_regs *espioob = config->oob_addr;

	/* eSPI OOB Disable event */
	if (espioob->ESPIOOBPF & ESPIOOB_DISABLE_EVENT) {
		struct espi_event evt = {
			.evt_type = ESPI_BUS_RESET,
			.evt_details = ESPI_CHANNEL_OOB,
			.evt_data = 0,
		};

		k_sem_give(&data->oob_tx_lock);
		espi_send_callbacks(&data->callbacks, dev, evt);
		espioob->ESPIOOBPF = ESPIOOB_TX_EVENT | ESPIOOB_DISABLE_EVENT;
	}

	/* eSPI OOB Tx finish */
	if (espioob->ESPIOOBPF & ESPIOOB_TX_EVENT) {
		struct espi_event evt = {
			.evt_type = ESPI_BUS_EVENT_CHANNEL_READY,
			.evt_details = ESPI_CHANNEL_OOB,
			.evt_data = 0,
		};

		k_sem_give(&data->oob_tx_lock);
		espi_send_callbacks(&data->callbacks, dev, evt);
		espioob->ESPIOOBPF = ESPIOOB_TX_EVENT;
	}

	/* eSPI OOB Rx finish */
	if (espioob->ESPIOOBPF & ESPIOOB_RX_EVENT) {
#ifdef CONFIG_ESPI_OOB_CHANNEL_RX_ASYNC
		struct espi_event evt = {
			.evt_type = ESPI_BUS_EVENT_OOB_RECEIVED,
			.evt_details = 0,
			.evt_data = 0,
		};

		evt.evt_details = espioob->ESPIOOBRL & ENE_ESPIOOB_RXLEN_MASK;
		espi_send_callbacks(&data->callbacks, dev, evt);
#else
		k_sem_give(&data->oob_rx_lock);
#endif
		espioob->ESPIOOBPF = ESPIOOB_RX_EVENT;
	}
}
#endif

#ifdef CONFIG_ESPI_FLASH_CHANNEL
static void espi_flash_kb1200_isr(const struct device *dev)
{
	const struct espi_kb1200_config *config = dev->config;
	struct espi_kb1200_data *data = dev->data;
	struct espi_regs *espi = config->base_addr;
	struct espifa_regs *espifa = config->fa_addr;

	/* eSPI Flash Disable event */
	if (espifa->ESPIFAPF & ESPIFA_DISABLE_EVENT) {
		uint8_t release_lock;

		release_lock = 0;
		/* Flash Access Channel is disable now */
		if (!(espi->ESPIC3CFG & ESPI_CH3_ENABLE)) {
			release_lock = 1;
		}
		/* No new protocol have issued when channel enable */
		if ((!(espi->ESPISTA & ESPI_FLASH_NP_AVAIL)) &&
		    (!(espifa->ESPIFAPF & ESPIFA_TX_FINISH_EVENT))) {
			release_lock = 1;
		}

		if (release_lock) {
			struct espi_event evt = {
				.evt_type = ESPI_BUS_RESET,
				.evt_details = ESPI_CHANNEL_FLASH,
				.evt_data = 0,
			};

			k_sem_give(&data->flash_lock);
			espi_send_callbacks(&data->callbacks, dev, evt);
		}
		espifa->ESPIFAPF = ESPIFA_DISABLE_EVENT | ESPIFA_TX_FINISH_EVENT;
	}

	/* eSPI Flash protocol finish event */
	if (espifa->ESPIFAPF & ESPIFA_TX_FINISH_EVENT) {
		espifa->ESPIFAPF = ESPIFA_TX_FINISH_EVENT;
	}

	/* eSPI Flash Write/Erase Completion event */
	if (espifa->ESPIFAPF & ESPIFA_WRITE_ERASE_COMPLETE_EVENT) {
		struct espi_event evt = {
			.evt_type = ESPI_BUS_EVENT_CHANNEL_READY,
			.evt_details = ESPI_CHANNEL_FLASH,
			.evt_data = 0,
		};

		k_sem_give(&data->flash_lock);
		espifa->ESPIFAPF = ESPIFA_WRITE_ERASE_COMPLETE_EVENT;
		espi_send_callbacks(&data->callbacks, dev, evt);
	}

	/* eSPI Flash Read Completion event */
	if (espifa->ESPIFAPF & ESPIFA_READ_COMPLETE_EVENT) {
		struct espi_event evt = {
			.evt_type = ESPI_BUS_EVENT_CHANNEL_READY,
			.evt_details = ESPI_CHANNEL_FLASH,
			.evt_data = 0,
		};

		k_sem_give(&data->flash_lock);
		espifa->ESPIFAPF = ESPIFA_READ_COMPLETE_EVENT;
		espi_send_callbacks(&data->callbacks, dev, evt);
	}

	/* eSPI Flash Un-success Completion event */
	if (espifa->ESPIFAPF & ESPIFA_UNSUCCESS_EVENT) {
		k_sem_give(&data->flash_lock);
		espifa->ESPIFAPF = ESPIFA_UNSUCCESS_EVENT;
	}
}
#endif

static const struct device *espi_device = DEVICE_DT_GET(DT_NODELABEL(espi0));
static struct gpio_callback espi_reset_cb;
static const struct gpio_dt_spec espirst = GPIO_DT_SPEC_GET(DT_NODELABEL(espi0), gpios);
void espi_reset_kb1200_isr(const struct device *dev, struct gpio_callback *cb, uint32_t pins)
{
	struct espi_kb1200_data *data = espi_device->data;
	struct espi_event evt = {
		.evt_type = ESPI_BUS_RESET,
		.evt_details = 0,
		.evt_data = 0,
	};
	bool espi_reset = gpio_pin_get(dev, (find_msb_set(pins) - 1));

	ARG_UNUSED(cb);
	evt.evt_data = espi_reset;
	espi_send_callbacks(&data->callbacks, dev, evt);
}

static int espi_kb1200_init(const struct device *dev)
{
	int ret;
	const struct espi_kb1200_config *config = dev->config;
#if defined(CONFIG_ESPI_OOB_CHANNEL) || defined(CONFIG_ESPI_FLASH_CHANNEL)
	struct espi_kb1200_data *data = dev->data;
#endif

	/* Configure pin-mux for eSPI bus device */
	ret = pinctrl_apply_state(config->pcfg, PINCTRL_STATE_DEFAULT);
	if (ret < 0) {
		LOG_ERR("eSPI pinctrl setup failed (%d)", ret);
		return ret;
	}

#ifdef CONFIG_ESPI_PERIPHERAL_CHANNEL
#if defined(CONFIG_ESPI_PERIPHERAL_EC_HOST_CMD) || defined(ESPI_PERIPHERAL_ENE_IDX32_0)
	struct hif_regs *hif = config->hif_addr;
#endif

#ifdef ESPI_PERIPHERAL_ENE_IDX32_0
	/* peripheral channel - Index32 IO 0*/
	hif->IDX32CFG = (hif->IDX32CFG & (~INDEX32_0_MASK)) |
			(ESPI_PERIPHERAL_ENE_IDX32_0_PORT_NUM << INDEX32_0_POS) |
			INDEX32_0_FUNCTION_ENABLE;
#endif

#ifdef ESPI_PERIPHERAL_ENE_IDX32_1
	/* peripheral channel - Index32 IO 1*/
	hif->IDX32CFG = (hif->IDX32CFG & (~INDEX32_1_MASK)) |
			(ESPI_PERIPHERAL_ENE_IDX32_1_PORT_NUM << INDEX32_1_POS) |
			INDEX32_1_FUNCTION_ENABLE;
#endif

#ifdef CONFIG_ESPI_PERIPHERAL_8042_KBC
	struct kbc_regs *kbc = config->kbc_addr;

	/* peripheral channel - KBC IO */
	IRQ_CONNECT(DT_INST_IRQ_BY_NAME(0, kbc, irq), DT_INST_IRQ_BY_NAME(0, kbc, priority),
		    kbc_kb1200_isr, DEVICE_DT_INST_GET(0), 0);
	irq_enable(DT_INST_IRQ_BY_NAME(0, kbc, irq));
	/* Set FW_OBF to clear OBF flag in both STATUS and HIKMST to 0 */
	kbc->KBCSTS |= KBSTS_OBF | KBSTS_IBF;
	/* Enable SIRQ 12 and SIRQ 1. */
	kbc->KBCCB |= KBC_IRQ1_ENABLE | KBC_IRQ12_ENABLE;
	/* Enable kbc and auto clear output buffer after read */
	kbc->KBCCFG |= KBC_FUNCTION_ENABLE | KBC_OUTPUT_READ_CLR_ENABLE;
#endif

#if CONFIG_ESPI_PERIPHERAL_HOST_IO
	struct eci_regs *eci = config->eci_addr;

	/* peripheral channel - EC IO */
	IRQ_CONNECT(DT_INST_IRQ_BY_NAME(0, eci, irq), DT_INST_IRQ_BY_NAME(0, eci, priority),
		    ec_kb1200_isr, DEVICE_DT_INST_GET(0), 0);
	irq_enable(DT_INST_IRQ_BY_NAME(0, eci, irq));
	eci->ECIIE |= ECI_IBF_EVENT | ECI_OBF_EVENT;
	eci->ECIPF = ECI_IBF_EVENT | ECI_OBF_EVENT;
	eci->ECICFG |= ECI_FUNCTION_ENABLE;
#endif

#if defined(CONFIG_ESPI_PERIPHERAL_EC_HOST_CMD) && defined(CONFIG_ESPI_PERIPHERAL_ACPI_SHM_REGION)
	/* peripheral channel - IOtoSRAM IO */
	IRQ_CONNECT(DT_INST_IRQ_BY_NAME(0, iotosram, irq),
		    DT_INST_IRQ_BY_NAME(0, iotosram, priority), iotosram_kb1200_isr,
		    DEVICE_DT_INST_GET(0), 0);
	irq_enable(DT_INST_IRQ_BY_NAME(0, iotosram, irq));
	/* IO Cycle to SRAM */
	hif->IOSCFG = (CONFIG_ESPI_PERIPHERAL_ACPI_SHM_REGION_PORT_NUM << IO2SRAM_IO_BASE_POS) |
		      IO2SRAM_FUNCTION_ENABLE;
	hif->IOSIE = ((uint32_t)shm_acpi_mmap & IO2SRAM_SRAM_BASE_MASK) | IO2SRAM_WRITE_EVENT;
#endif /* CONFIG_ESPI_PERIPHERAL_ACPI_SHM_REGION */

#ifdef CONFIG_ESPI_PERIPHERAL_DEBUG_PORT_80
	struct dbi_regs *dbi0 = config->dbi_addr;
	struct dbi_regs *dbi1 = dbi0 + 1;

	/* peripheral channel - Debug Port IO */
	IRQ_CONNECT(DT_INST_IRQ_BY_NAME(0, dbi, irq), DT_INST_IRQ_BY_NAME(0, dbi, priority),
		    dbi_kb1200_isr, DEVICE_DT_INST_GET(0), 0);
	irq_enable(DT_INST_IRQ_BY_NAME(0, dbi, irq));
	/* Port 80 */
	dbi0->DBIPF = DBI_RX_EVENT;
	dbi0->DBIIE |= DBI_RX_EVENT;
	dbi0->DBICFG |= DBI_FUNCTION_ENABLE;
	/* Port 81 */
	dbi1->DBIPF = DBI_RX_EVENT;
	dbi1->DBIIE |= DBI_RX_EVENT;
	dbi1->DBICFG |= DBI_FUNCTION_ENABLE;
#endif
#endif /* CONFIG_ESPI_PERIPHERAL_CHANNEL */

#ifdef CONFIG_ESPI_VWIRE_CHANNEL
	struct espivw_regs *espivw = config->vw_addr;
	uint16_t *vwblk_base_dir = (uint16_t *)&espivw->ESPIVWB10;

	/* virtual vwire channel */
	IRQ_CONNECT(DT_INST_IRQ_BY_NAME(0, vwire, irq), DT_INST_IRQ_BY_NAME(0, vwire, priority),
		    espi_vw_kb1200_isr, DEVICE_DT_INST_GET(0), 0);
	irq_enable(DT_INST_IRQ_BY_NAME(0, vwire, irq));

	/* Initial VW block base and direction */
	vwblk_base_dir[0] = ESPIVW_B0_BASE;
	vwblk_base_dir[1] = ESPIVW_B1_BASE;
	vwblk_base_dir[2] = ESPIVW_B2_BASE;
	vwblk_base_dir[3] = ESPIVW_B3_BASE;
	for (int i = 0; i < ARRAY_SIZE(vw_out); i++) {
		uint8_t vw_base = vw_out[i].index & ESPIVW_INDEXBASE_MASK;
		uint8_t vw_num = (vw_out[i].index & ESPIVW_INDEXNUM_MASK) + ESPIVW_BLK_DIR_POS;

		if (vw_base) {
			int j;

			for (j = 0; j < ESPIVW_BLK_NUM; j++) {
				if (vw_base == (vwblk_base_dir[j] & ESPIVW_BLK_BASE_MASK)) {
					vwblk_base_dir[j] |= BIT(vw_num);
					break;
				}
			}
			if (j == ESPIVW_BLK_NUM) {
				LOG_ERR("Invalid VW vw_base:%d vw_num:%d", vw_base, vw_num);
			}
		}
	}
	espivw->ESPIVWPF = ESPIVW_TX_EVENT | ESPIVW_RX_EVENT;
	espivw->ESPIVWIE |= ESPIVW_TX_EVENT | ESPIVW_RX_EVENT;
#endif /* CONFIG_ESPI_VWIRE_CHANNEL */

#ifdef CONFIG_ESPI_OOB_CHANNEL
	struct espioob_regs *espioob = config->oob_addr;

	/* OOB channel */
	IRQ_CONNECT(DT_INST_IRQ_BY_NAME(0, oob, irq), DT_INST_IRQ_BY_NAME(0, oob, priority),
		    espi_oob_kb1200_isr, DEVICE_DT_INST_GET(0), 0);
	irq_enable(DT_INST_IRQ_BY_NAME(0, oob, irq));
	espioob->ESPIOOBPF = ESPIOOB_TX_EVENT | ESPIOOB_RX_EVENT | ESPIOOB_DISABLE_EVENT;
	espioob->ESPIOOBIE |= ESPIOOB_TX_EVENT | ESPIOOB_RX_EVENT | ESPIOOB_DISABLE_EVENT;
	k_sem_init(&data->oob_tx_lock, 0, 1);
#ifndef CONFIG_ESPI_OOB_CHANNEL_RX_ASYNC
	k_sem_init(&data->oob_rx_lock, 0, 1);
#endif /* CONFIG_ESPI_OOB_CHANNEL_RX_ASYNC */
#endif /* CONFIG_ESPI_OOB_CHANNEL */

#ifdef CONFIG_ESPI_FLASH_CHANNEL
	struct espifa_regs *espifa = config->fa_addr;

	/* Flash channel */
	IRQ_CONNECT(DT_INST_IRQ_BY_NAME(0, flash, irq), DT_INST_IRQ_BY_NAME(0, flash, priority),
		    espi_flash_kb1200_isr, DEVICE_DT_INST_GET(0), 0);
	irq_enable(DT_INST_IRQ_BY_NAME(0, flash, irq));
	espifa->ESPIFAIE = 0xFF;
	k_sem_init(&data->flash_lock, 0, 1);
#endif /* CONFIG_ESPI_FLASH_CHANNEL */

	/* Initial eSPI Reset ISR (gpio isr connection) */
	gpio_pin_configure_dt(&espirst, GPIO_INPUT);
	gpio_pin_interrupt_configure_dt(&espirst, GPIO_INT_EDGE_BOTH);
	gpio_init_callback(&espi_reset_cb, espi_reset_kb1200_isr, BIT(espirst.pin));
	gpio_add_callback(espirst.port, &espi_reset_cb);

	return 0;
}

PINCTRL_DT_INST_DEFINE(0);
static struct espi_kb1200_data espi_kb1200_data_0;
static const struct espi_kb1200_config espi_kb1200_config_0 = {
	.base_addr = (struct espi_regs *)DT_INST_REG_ADDR(0),
	.vw_addr = (struct espivw_regs *)DT_INST_PROP(0, espivw_reg),
	.vwtab_addr = (uintptr_t)DT_INST_PROP(0, espivw_tab),
	.oob_addr = (struct espioob_regs *)DT_INST_PROP(0, espioob_reg),
	.fa_addr = (struct espifa_regs *)DT_INST_PROP(0, espifa_reg),
	.hif_addr = (struct hif_regs *)DT_INST_PROP(0, hif_reg),
	.kbc_addr = (struct kbc_regs *)DT_INST_PROP(0, kbc_reg),
	.eci_addr = (struct eci_regs *)DT_INST_PROP(0, eci_reg),
	.dbi_addr = (struct dbi_regs *)DT_INST_PROP(0, dbi_reg),
	.pcfg = PINCTRL_DT_INST_DEV_CONFIG_GET(0),
};
DEVICE_DT_INST_DEFINE(0, &espi_kb1200_init, NULL, &espi_kb1200_data_0, &espi_kb1200_config_0,
		      PRE_KERNEL_2, CONFIG_ESPI_INIT_PRIORITY, &espi_kb1200_driver_api);
