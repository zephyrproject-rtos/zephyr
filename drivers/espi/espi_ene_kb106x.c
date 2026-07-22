/*
 * Copyright (c) 2026 ENE Technology Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT ene_kb106x_espi

#include <zephyr/drivers/pinctrl.h>
#include <zephyr/kernel.h>
#include <errno.h>
#include <string.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/drivers/espi.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/util.h>

#include "espi_utils.h"
#include <reg/espi.h>
#include <reg/hif.h>
#include <reg/kbc.h>
#include <reg/ec.h>
#include <reg/dbi.h>
#include <reg/mbx.h>
#include <reg/legi.h>
#include <reg/uart.h>

LOG_MODULE_REGISTER(espi, CONFIG_ESPI_LOG_LEVEL);

#define DEFAULT_ECI_PORT_NUM  0x0062
#define DEFAULT_DBI0_PORT_NUM 0x0080
#define DEFAULT_DBI1_PORT_NUM 0x0081

#define DEFAULT_OOB_TIMEOUT 2000
#define DEFAULT_FA_TIMEOUT  2000

#define DEFAULT_ESPIC0CFG 0x00001113

struct espi_kb106x_config {
	struct espi_regs *base_addr;
	struct espivw_regs *vw_addr;
	struct espioob_regs *oob_addr;
	struct espifa_regs *fa_addr;
	struct hif_regs *hif_addr;
	struct kbc_regs *kbc_addr;
	struct eci_regs *eci_addr;
	struct dbi_regs *dbi_addr;
	struct legi_regs *legi_addr;
	struct mbx_regs *mbx_addr;
	struct uart_regs *uart_addr;
	uintptr_t vwtab_addr;
	uint32_t oob_max_timeout;
	uint32_t fa_max_timeout;
	uint32_t eci_port;
	uint32_t dbi0_port;
	uint32_t dbi1_port;
	/* Pin control */
	const struct pinctrl_dev_config *pcfg;
};

struct espi_kb106x_data {
	sys_slist_t callbacks;
	struct k_spinlock pher_reg_lock;
	struct k_spinlock vw_reg_lock;
	struct k_spinlock oob_reg_lock;
	atomic_t oob_status;
	struct k_sem oob_tx_lock;
	struct k_sem oob_rx_lock;
	struct k_spinlock flash_reg_lock;
	atomic_t flash_status;
	struct k_sem flash_lock;
	uint16_t flash_remain_len;
	uint8_t *flash_buf;
	bool plt_rst;
	bool pher_ch_en;
	bool vw_ch_en;
	bool oob_ch_en;
	bool flash_ch_en;
	struct device *dev;
	struct k_work_delayable ch_check_work;
};

struct ene_signal {
	uint8_t offset; /*HW table register offset*/
	uint8_t index;  /*VW index*/
	uint8_t bit;    /*VW data bit*/
	uint8_t dir;    /*VW direction*/
};

#ifdef CONFIG_ESPI_VWIRE_CHANNEL
/* VW signals used in eSPI */
static const struct ene_signal vw_pin[] = {
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
	[ESPI_VWIRE_SIGNAL_SMIOUT] = {ENE_IDX07_OFS, 0x07, BIT(1), ESPI_CONTROLLER_TO_TARGET},
	[ESPI_VWIRE_SIGNAL_NMIOUT] = {ENE_IDX07_OFS, 0x07, BIT(2), ESPI_CONTROLLER_TO_TARGET},
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

static const char *const st_list[] = {
	/* index 02h (In) */
	[ESPI_VWIRE_SIGNAL_SLP_S3] = "VW_SLP_S3",
	[ESPI_VWIRE_SIGNAL_SLP_S4] = "VW_SLP_S4",
	[ESPI_VWIRE_SIGNAL_SLP_S5] = "VW_SLP_S5",
	/* index 03h (In) */
	[ESPI_VWIRE_SIGNAL_SUS_STAT] = "VW_SUS_STAT",
	[ESPI_VWIRE_SIGNAL_PLTRST] = "VW_PLTRST",
	[ESPI_VWIRE_SIGNAL_OOB_RST_WARN] = "VW_OOB_RST_WARN",
	/* index 04h (Out) */
	[ESPI_VWIRE_SIGNAL_OOB_RST_ACK] = "VW_OOB_RST_ACK",
	[ESPI_VWIRE_SIGNAL_WAKE] = "VW_WAKE",
	[ESPI_VWIRE_SIGNAL_PME] = "VW_PME",
	/* index 05h (Out) */
	[ESPI_VWIRE_SIGNAL_TARGET_BOOT_DONE] = "VW_TARGET_BOOT_DONE",
	[ESPI_VWIRE_SIGNAL_ERR_FATAL] = "VW_ERR_FATAL",
	[ESPI_VWIRE_SIGNAL_ERR_NON_FATAL] = "VW_ERR_NON_FATAL",
	[ESPI_VWIRE_SIGNAL_TARGET_BOOT_STS] = "VW_TARGET_BOOT_STS",
	/* index 06h (Out) */
	/* System control interrupt */
	[ESPI_VWIRE_SIGNAL_SCI] = "VW_SCI",
	/* System management interrupt */
	[ESPI_VWIRE_SIGNAL_SMI] = "VW_SMI",
	[ESPI_VWIRE_SIGNAL_RST_CPU_INIT] = "VW_RST_CPU_INIT",
	[ESPI_VWIRE_SIGNAL_HOST_RST_ACK] = "VW_HOST_RST_ACK",
	/* index 07h (In) */
	[ESPI_VWIRE_SIGNAL_HOST_RST_WARN] = "VW_HOST_RST_WARN",
	/* index 40h (Out) */
	[ESPI_VWIRE_SIGNAL_SUS_ACK] = "VW_SUS_ACK",
	[ESPI_VWIRE_SIGNAL_DNX_ACK] = "VW_DNX_ACK",
	/* index 41h (In) */
	[ESPI_VWIRE_SIGNAL_SUS_WARN] = "VW_SUS_WARN",
	[ESPI_VWIRE_SIGNAL_SUS_PWRDN_ACK] = "VW_SUS_PWRDN_ACK",
	[ESPI_VWIRE_SIGNAL_SLP_A] = "VW_SLP_A",
	/* index 42h (In) */
	[ESPI_VWIRE_SIGNAL_SLP_LAN] = "VW_SLP_LAN",
	[ESPI_VWIRE_SIGNAL_SLP_WLAN] = "VW_SLP_WLAN",
	/* index 47h (In) */
	[ESPI_VWIRE_SIGNAL_HOST_C10] = "VW_HOST_C10",
	/* index 4Ah (In) */
	[ESPI_VWIRE_SIGNAL_DNX_WARN] = "VW_DNX_WARN",

	[ESPI_VWIRE_SIGNAL_TARGET_GPIO_0] = "VW_TARGET_GPIO_0",
	[ESPI_VWIRE_SIGNAL_TARGET_GPIO_1] = "VW_TARGET_GPIO_1",
	[ESPI_VWIRE_SIGNAL_TARGET_GPIO_2] = "VW_TARGET_GPIO_2",
	[ESPI_VWIRE_SIGNAL_TARGET_GPIO_3] = "VW_TARGET_GPIO_3",
};
#endif /* CONFIG_ESPI_VWIRE_CHANNEL */

#ifdef CONFIG_ESPI_PERIPHERAL_CHANNEL
#define ESPI_ENE_HCMD_SHM_SIZE 256
#define ESPI_ENE_ACPI_SHM_SIZE 256
/*
 * The ESPI PERIPHERAL EC_HOST_CMD feature consists of two sets of eSPI
 * peripheral I/O functions. First part is one set of commad/data port
 * (default 0x200/0x204), and the other part is ram directly mapping by
 * eSPI I/O cycles(default 0x800).
 *
 * if this feature is enable, ene espi driver will default provide legacy
 * io_0 and to io2ram to support this feature. The mapping ram will also
 * be announced as below with 256 byte aligned (or 8k aligned).
 */
#if defined(CONFIG_ESPI_PERIPHERAL_EC_HOST_CMD)
static uint8_t shm_host_cmd[ESPI_ENE_HCMD_SHM_SIZE] __aligned(256);
#endif /* CONFIG_ESPI_PERIPHERAL_EC_HOST_CMD */

/*
 * The ESPI_PERIPHERAL_ACPI_SHM_REGION feature provide the extra share
 * memory to access by eSPI I/O cycles(default 0x900).
 *
 * if this feature is enable, ene espi driver will default provide
 * mailbox_1 to support this feature. The mapping ram will also be
 * announced as below with 256 byte aligned (or 8k aligned).
 */
#if defined(CONFIG_ESPI_PERIPHERAL_ACPI_SHM_REGION)
static uint8_t shm_acpi_mmap[ESPI_ENE_ACPI_SHM_SIZE] __aligned(256);
#endif /* CONFIG_ESPI_PERIPHERAL_ACPI_SHM_REGION */

#endif /* CONFIG_ESPI_PERIPHERAL_CHANNEL */

/* eSPI function forward declarations */
void espi_custom_opcode_int_set(const struct device *dev, uint16_t port_num, uint8_t enable);
void espi_custom_opcode_outdata_set(const struct device *dev, uint16_t port_num, uint8_t dat);
void espi_kb106x_peripheral_init(const struct device *dev);

/* eSPI api functions */
static int espi_kb106x_configure(const struct device *dev, struct espi_cfg *cfg)
{
	const struct espi_kb106x_config *config = dev->config;
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
	/* set io mode */
	/* always support Single mode, just check dual/quad mode */
	io_mode = (cfg->io_caps >> 1);
	if (io_mode > ESPI_IOMODE_MASK) {
		return -EINVAL;
	}
	/* configure eSPI supported channels */
	channel_caps = cfg->channel_caps & ESPI_CH_SUPPORT_MASK;
	espi->espi_gencfg = (io_mode << ESPI_IOMODE_POS) | (ESPI_ALERT_OD << ESPI_ALERT_POS) |
			    (max_freq << ESPI_FREQ_POS) | channel_caps;

	return 0;
}

static bool espi_kb106x_channel_ready(const struct device *dev, enum espi_channel ch)
{
	const struct espi_kb106x_config *config = dev->config;
	struct espi_regs *espi = config->base_addr;
	bool sts;

	switch (ch) {
	case ESPI_CHANNEL_PERIPHERAL:
		sts = espi->espi_c0cfg & ESPI_CH0_READY;
		break;
	case ESPI_CHANNEL_VWIRE:
		sts = espi->espi_c1cfg & ESPI_CH1_READY;
		break;
	case ESPI_CHANNEL_OOB:
		sts = espi->espi_c2cfg & ESPI_CH2_READY;
		break;
	case ESPI_CHANNEL_FLASH:
		sts = espi->espi_c3cfg & ESPI_CH3_READY;
		break;
	default:
		sts = false;
		break;
	}

	return sts;
}

static int espi_kb106x_manage_callback(const struct device *dev, struct espi_callback *callback,
				       bool set)
{
	struct espi_kb106x_data *data = (struct espi_kb106x_data *)(dev->data);

	return espi_manage_callback(&data->callbacks, callback, set);
}

#ifdef CONFIG_ESPI_PERIPHERAL_CHANNEL
static int espi_kb106x_read_lpc_request(const struct device *dev, enum lpc_peripheral_opcode op,
					uint32_t *data)
{
	const struct espi_kb106x_config *const config = dev->config;
	struct espi_kb106x_data *pher_data = dev->data;
	struct kbc_regs *kbc = config->kbc_addr;
	struct eci_regs *eci = config->eci_addr;
	int err = 0;
	k_spinlock_key_t key;

	LOG_DBG("espi r_opcode:%x", op);
	key = k_spin_lock(&pher_data->pher_reg_lock);
	if (op >= E8042_START_OPCODE && op <= E8042_MAX_OPCODE) {
		switch (op) {
		case E8042_OBF_HAS_CHAR:
			/*
			 * EC has written data back to host. OBF is
			 * automatically cleared after host reads
			 * the data
			 */
			*data = !!(kbc->kbc_sts & KBSTS_OBF);
			break;
		case E8042_IBF_HAS_CHAR:
			*data = !!(kbc->kbc_sts & KBSTS_IBF);
			break;
		case E8042_READ_KB_STS:
			*data = kbc->kbc_sts;
			break;
		default:
			LOG_ERR("invalid kbc r_opcode: %x", op);
			err = -EINVAL;
			break;
		}
	} else if (op >= EACPI_START_OPCODE && op <= EACPI_MAX_OPCODE) {
		switch (op) {
		case EACPI_OBF_HAS_CHAR:
			/*
			 * EC has written data back to host. OBF is
			 * automatically cleared after host reads
			 * the data
			 */
			*data = !!(eci->eci_sts & ECISTS_OBF);
			break;
		case EACPI_IBF_HAS_CHAR:
			*data = !!(eci->eci_sts & ECISTS_IBF);
			break;
		case EACPI_READ_STS:
			*data = eci->eci_sts & (ECISTS_OBF | ECISTS_IBF | ECISTS_BURST_MODE |
						ECISTS_SCI_QUERY_FLAG);
			break;
#ifdef CONFIG_ESPI_PERIPHERAL_ACPI_SHM_REGION
		case EACPI_GET_SHARED_MEMORY:
			*data = (uint32_t)shm_acpi_mmap;
			break;
#endif /* CONFIG_ESPI_PERIPHERAL_ACPI_SHM_REGION */
		default:
			LOG_ERR("invalid acpi r_opcode: %x", op);
			err = -EINVAL;
			break;
		}
	} else if (op >= ECUSTOM_START_OPCODE && op <= ECUSTOM_MAX_OPCODE) {
		/* Other customized op codes */
		switch (op) {
#if defined(CONFIG_ESPI_PERIPHERAL_CUSTOM_OPCODE) && defined(CONFIG_ESPI_PERIPHERAL_EC_HOST_CMD)
		case ECUSTOM_HOST_CMD_GET_PARAM_MEMORY:
			*data = (uint32_t)shm_host_cmd;
			break;
		case ECUSTOM_HOST_CMD_GET_PARAM_MEMORY_SIZE:
			*data = sizeof(shm_host_cmd);
			break;
#endif /* CONFIG_ESPI_PERIPHERAL_CUSTOM_OPCODE && CONFIG_ESPI_PERIPHERAL_EC_HOST_CMD */
		default:
			LOG_ERR("invalid custom r_opcode: %x", op);
			err = -EINVAL;
			break;
		}
	} else {
		LOG_ERR("unsupport r_opcode: %x", op);
		err = -ENOTSUP;
	}
	k_spin_unlock(&pher_data->pher_reg_lock, key);

	return err;
}

static int espi_kb106x_write_lpc_request(const struct device *dev, enum lpc_peripheral_opcode op,
					 uint32_t *data)
{
	const struct espi_kb106x_config *const config = dev->config;
	struct espi_kb106x_data *pher_data = dev->data;
	struct kbc_regs *kbc = config->kbc_addr;
	struct eci_regs *eci = config->eci_addr;
	uint8_t flag;
#if defined(CONFIG_ESPI_PERIPHERAL_CUSTOM_OPCODE)
	uint16_t portnum = *data >> 16;
	uint8_t dat = *data & 0xFF;
#endif /* CONFIG_ESPI_PERIPHERAL_CUSTOM_OPCODE */
	k_spinlock_key_t key;
	int err = 0;

	LOG_DBG("espi w_opcode:%x dat:%x", op, *data);
	key = k_spin_lock(&pher_data->pher_reg_lock);
	if (op >= E8042_START_OPCODE && op <= E8042_MAX_OPCODE) {
		switch (op) {
		case E8042_WRITE_KB_CHAR:
			/* Clear Auxiliary data flag */
			flag = kbc->kbc_sts & ~(KBSTS_OBF | KBSTS_IBF);
			flag &= ~KBSTS_AUX_FLAG;
			kbc->kbc_sts = flag;
			kbc->kbc_odp = (uint8_t)*data;
			break;
		case E8042_WRITE_MB_CHAR:
			/* Set Auxiliary data flag */
			flag = kbc->kbc_sts & ~(KBSTS_OBF | KBSTS_IBF);
			flag |= KBSTS_AUX_FLAG;
			kbc->kbc_sts = flag;
			kbc->kbc_odp = (uint8_t)*data;
			break;
		case E8042_RESUME_IRQ:
			/* Enable KBC IBF interrupt */
			kbc->kbc_ie |= KBC_IBF_EVENT;
			break;
		case E8042_PAUSE_IRQ:
			/* Disable KBC IBF interrupt */
			kbc->kbc_ie &= ~KBC_IBF_EVENT;
			break;
		case E8042_CLEAR_OBF:
			/* Clear OBF flag */
			flag = kbc->kbc_sts & ~(KBSTS_OBF | KBSTS_IBF);
			flag |= KBSTS_OBF;
			kbc->kbc_sts = flag;
			break;
		case E8042_SET_FLAG:
		case E8042_CLEAR_FLAG:
			/* FW shouldn't modify these flags directly */
			flag = kbc->kbc_sts & ~(KBSTS_OBF | KBSTS_IBF);

			if (*data & KBSTS_OBF) {
				if (op == E8042_CLEAR_FLAG) {
					flag |= KBSTS_OBF;
				}
			}

			if (*data & KBSTS_IBF) {
				if (op == E8042_CLEAR_FLAG) {
					flag |= KBSTS_IBF;
				}
			}

			if (*data & KBSTS_SYS_FLAG) {
				if (op == E8042_CLEAR_FLAG) {
					kbc->kbc_cb &= ~KBC_SYSTEM_FLAG;
				} else {
					kbc->kbc_cb |= KBC_SYSTEM_FLAG;
				}
			}

			if (*data & KBSTS_AUX_FLAG) {
				if (op == E8042_CLEAR_FLAG) {
					flag &= ~KBSTS_AUX_FLAG;
				} else {
					flag |= KBSTS_AUX_FLAG;
				}
			}

			if (*data & KBSTS_PARITY_ERROR) {
				if (op == E8042_CLEAR_FLAG) {
					flag &= ~KBSTS_PARITY_ERROR;
				} else {
					flag |= KBSTS_PARITY_ERROR;
				}
			}

			if (*data & KBSTS_TIME_OUT) {
				if (op == E8042_CLEAR_FLAG) {
					flag &= ~KBSTS_TIME_OUT;
				} else {
					flag |= KBSTS_TIME_OUT;
				}
			}
			kbc->kbc_sts = flag;
			break;
		default:
			LOG_ERR("invalid kbc w_opcode: %x", op);
			err = -EINVAL;
			break;
		}
	} else if (op >= EACPI_START_OPCODE && op <= EACPI_MAX_OPCODE) {
		uint8_t ecsts = eci->eci_sts & ~(ECISTS_OBF | ECISTS_IBF | ECISTS_SCI_QUERY_FLAG);

		switch (op) {
		case EACPI_WRITE_CHAR:
			eci->eci_odp = (uint8_t)*data;
			break;
		case EACPI_WRITE_STS:
			if (*data & ECISTS_BURST_MODE) {
				ecsts |= ECISTS_BURST_MODE;
			} else {
				ecsts &= ~ECISTS_BURST_MODE;
			}
			if (*data & ECISTS_SCI_QUERY_FLAG) {
				/* Write nonezero to eci_scid make SCI_FLAG rise */
				eci->eci_scid = ECISTS_SCI_QUERY_FLAG;
			} else {
				ecsts |= ECISTS_SCI_QUERY_FLAG; /* Write 1 to clear */
			}
			eci->eci_sts = ecsts;
			break;
		default:
			LOG_ERR("invalid acpi w_opcode: %x", op);
			err = -EINVAL;
			break;
		}
	} else if (op >= ECUSTOM_START_OPCODE && op <= ECUSTOM_MAX_OPCODE) {
#if defined(CONFIG_ESPI_PERIPHERAL_CUSTOM_OPCODE)
		switch (op) {
		case ECUSTOM_HOST_SUBS_INTERRUPT_EN:
			espi_custom_opcode_int_set(dev, portnum, dat);
			break;
		case ECUSTOM_HOST_CMD_SEND_RESULT:
			espi_custom_opcode_outdata_set(dev, portnum, dat);
			break;
		default:
			LOG_ERR("invalid custom w_opcode: %x", op);
			err = -EINVAL;
			break;
		}
#endif /* CONFIG_ESPI_PERIPHERAL_CUSTOM_OPCODE */
	} else {
		LOG_ERR("unsupport w_opcode: %x", op);
		err = -ENOTSUP;
	}
	k_spin_unlock(&pher_data->pher_reg_lock, key);

	return err;
}
#endif /* CONFIG_ESPI_PERIPHERAL_CHANNEL */

#ifdef CONFIG_ESPI_VWIRE_CHANNEL
static int espi_kb106x_vwtab_set(const struct device *dev, enum espi_vwire_signal signal,
				 uint8_t level)
{
	const struct espi_kb106x_config *config = dev->config;
	struct ene_signal signal_info = vw_pin[signal];
	uint8_t *vwtab = (uint8_t *)(config->vwtab_addr + signal_info.offset);
	uint8_t vwdata = *vwtab;

	if (level) {
		vwdata |= signal_info.bit;
	} else {
		vwdata &= ~signal_info.bit;
	}
	*vwtab = vwdata | (signal_info.bit << ESPIVW_VALIDBIT_POS);
	return 0;
}

static int espi_kb106x_send_vwire(const struct device *dev, enum espi_vwire_signal signal,
				  uint8_t level)
{
	const struct espi_kb106x_config *config = dev->config;
	struct espi_kb106x_data *data = dev->data;
	struct espivw_regs *espivw = config->vw_addr;
	struct ene_signal signal_info = vw_pin[signal];
	uint8_t *vwtab = (uint8_t *)(config->vwtab_addr + signal_info.offset);
	uint8_t vwdata;
	k_spinlock_key_t key;

	if (signal >= ESPI_VWIRE_SIGNAL_COUNT) {
		LOG_ERR("Invalid VW: %d", signal);
		return -EINVAL;
	}

	if (!vw_pin[signal].index) {
		LOG_ERR("%s signal %d is invalid", __func__, signal);
		return -EIO;
	}

	key = k_spin_lock(&data->vw_reg_lock);

	vwdata = *vwtab;
	if (level) {
		vwdata |= signal_info.bit;
	} else {
		vwdata &= ~signal_info.bit;
	}
	vwdata |= (signal_info.bit << ESPIVW_VALIDBIT_POS);

	if (signal_info.dir == ESPI_TARGET_TO_CONTROLLER) {
		espivw->espivw_tx = (signal_info.index << ESPIVW_TXINDEX_POS) | vwdata;
	}

	if (IS_ENABLED(CONFIG_ESPI_KB106X_VWIRE_ENABLE_SEND_CHECK)) {
		if (!WAIT_FOR(!IS_BIT_SET(espivw->espivw_tf[(signal_info.offset) / 32],
					  (signal_info.offset) % 32),
			      CONFIG_ESPI_KB106X_WIRE_SEND_TIMEOUT_US, NULL)) {
			LOG_ERR("%s signal %d timeout", __func__, signal);
			return -ETIMEDOUT;
		}
	}

	k_spin_unlock(&data->vw_reg_lock, key);
	return 0;
}

static int espi_kb106x_receive_vwire(const struct device *dev, enum espi_vwire_signal signal,
				     uint8_t *level)
{
	const struct espi_kb106x_config *config = dev->config;
	struct espi_kb106x_data *data = dev->data;
	struct espivw_regs *espivw = config->vw_addr;
	struct ene_signal signal_info = vw_pin[signal];
	uint8_t *vwtab = (uint8_t *)(espivw->espivw_idx + signal_info.offset);
	k_spinlock_key_t key;

	if (signal >= ESPI_VWIRE_SIGNAL_COUNT) {
		LOG_ERR("Invalid VW: %d", signal);
		return -EINVAL;
	}
	if (!vw_pin[signal].index) {
		LOG_ERR("%s signal %d is invalid", __func__, signal);
		return -EIO;
	}

	key = k_spin_lock(&data->vw_reg_lock);

	*level = !!(*vwtab & signal_info.bit);
	/*
	 * if read vwire(rx index), clear the rx index valid bit (W1C).
	 * if read vwire(tx index), not clear tx index valid bit.
	 */
	if (signal_info.dir == ESPI_CONTROLLER_TO_TARGET) {
		*vwtab = (signal_info.bit) << ESPIVW_VALIDBIT_POS;
	}

	k_spin_unlock(&data->vw_reg_lock, key);

	return 0;
}
#endif /* CONFIG_ESPI_VWIRE_CHANNEL */

#ifdef CONFIG_ESPI_OOB_CHANNEL
static int espi_kb106x_send_oob(const struct device *dev, struct espi_oob_packet *pckt)
{
	int ret = 0;
	const struct espi_kb106x_config *config = dev->config;
	struct espi_kb106x_data *data = dev->data;
	struct espi_regs *espi = config->base_addr;
	struct espioob_regs *espioob = config->oob_addr;
	k_spinlock_key_t key;

	if (!(espi->espi_c2cfg & ESPI_CH2_ENABLE)) {
		LOG_ERR("OOB channel is disabled");
		return -EIO;
	}

	/* check and set busy status */
	if (atomic_test_and_set_bit(&data->oob_status, 1)) {
		return -EBUSY;
	}
	if (pckt->len > ESPIOOB_BUFSIZE) {
		LOG_ERR("%s insufficient space", __func__);
		ret = -EINVAL;
	}

	key = k_spin_lock(&data->oob_reg_lock);

	memcpy((void *)espioob->espioob_dat, pckt->buf, pckt->len);
	espioob->espioob_tx = pckt->len;
	espioob->espioob_tx |= ESPIOOB_TX_ISSUE;

	k_spin_unlock(&data->oob_reg_lock, key);

	/* if it call from other isr will not wait. busy status will be clear by isr */
	if (k_is_in_isr()) {
		/* clear busy status, if oob send has error */
		if (ret) {
			atomic_clear(&data->oob_status);
		}
		return ret;
	}

	/* wait until isr or timeout */
	ret = k_sem_take(&data->oob_tx_lock, K_MSEC(config->oob_max_timeout));
	/* clear busy state */
	atomic_clear(&data->oob_status);

	if (ret == -EAGAIN) {
		return -ETIMEDOUT;
	}

	if (espioob->espioob_ef) {
		LOG_ERR("OOB Tx failed (Error Flag:%x)", espioob->espioob_ef);
		/* Clear all error flag */
		espioob->espioob_ef = espioob->espioob_ef;
		return -EIO;
	}

	return 0;
}

static int espi_kb106x_receive_oob(const struct device *dev, struct espi_oob_packet *pckt)
{
	const struct espi_kb106x_config *config = dev->config;
	struct espi_kb106x_data *data = dev->data;
	struct espioob_regs *espioob = config->oob_addr;
	uint32_t rcvd_len;
#ifndef CONFIG_ESPI_OOB_CHANNEL_RX_ASYNC
	int ret = 0;
#endif /* CONFIG_ESPI_OOB_CHANNEL_RX_ASYNC */
	k_spinlock_key_t key;

	if (espioob->espioob_ef) {
		return -EIO;
	}
	/* if not set RX_ASYNC, wait the Rx event after send_oob */
#ifndef CONFIG_ESPI_OOB_CHANNEL_RX_ASYNC
	/* Wait until ISR or timeout */
	if (k_is_in_isr()) {
		ret = k_sem_take(&data->oob_rx_lock, K_NO_WAIT);
	} else {
		ret = k_sem_take(&data->oob_rx_lock, K_MSEC(config->oob_max_timeout));
	}

	if (ret == -EAGAIN) {
		return -ETIMEDOUT;
	} else if (ret != 0) {
		return -EIO;
	}
#endif
	key = k_spin_lock(&data->oob_reg_lock);

	/* Check if buffer passed to driver can fit the received buffer */
	rcvd_len = espioob->espioob_rl;

	if (rcvd_len > pckt->len) {
		LOG_ERR("space rcvd %d vs %d", rcvd_len, pckt->len);
		k_spin_unlock(&data->oob_reg_lock, key);
		return -EIO;
	}

	pckt->len = rcvd_len;
	memcpy(pckt->buf, (void *)espioob->espioob_dat, pckt->len);

	k_spin_unlock(&data->oob_reg_lock, key);

	return 0;
}
#endif /* CONFIG_ESPI_OOB_CHANNEL */

#ifdef CONFIG_ESPI_FLASH_CHANNEL
static int espi_kb106x_flash_read(const struct device *dev, struct espi_flash_packet *pckt)
{
	int ret;
	const struct espi_kb106x_config *config = dev->config;
	struct espi_kb106x_data *data = dev->data;
	struct espi_regs *espi = config->base_addr;
	struct espifa_regs *espifa = config->fa_addr;
	uint32_t remain_len;
	k_spinlock_key_t key;

	if (!(espi->espi_c3cfg & ESPI_CH3_ENABLE)) {
		LOG_ERR("Flash channel is disabled");
		return -EIO;
	}

	if (k_is_in_isr()) {
		LOG_ERR("Flash channel not support use in isr");
		return -ENOTSUP;
	}

	/* check and set busy status */
	if (atomic_test_and_set_bit(&data->flash_status, 1)) {
		return -EBUSY;
	}

	key = k_spin_lock(&data->flash_reg_lock);

	data->flash_remain_len = pckt->len;
	data->flash_buf = pckt->buf;
	if (data->flash_remain_len > ESPIFA_ENE_BUFSIZE) {
		remain_len = ESPIFA_ENE_BUFSIZE;
	} else {
		remain_len = data->flash_remain_len;
	}

	espifa->espifa_ba = pckt->flash_addr;
	espifa->espifa_cnt = remain_len & 0x3F;
	espifa->espifa_ptcl = ESPIFA_READ | ESPIFA_ACC_ADDR;

	k_spin_unlock(&data->flash_reg_lock, key);

	/* Wait until ISR or timeout */
	ret = k_sem_take(&data->flash_lock, K_MSEC(config->fa_max_timeout));
	/* clear busy status */
	atomic_clear(&data->flash_status);
	if (ret == -EAGAIN) {
		LOG_ERR("%s timeout", __func__);
		return -ETIMEDOUT;
	}

	if (espifa->espifa_ef) {
		LOG_ERR("FLASH Tx failed (Error Flag:%x)", espifa->espifa_ef);
		/* Clear all error flag */
		espifa->espifa_ef = espifa->espifa_ef;
		return -EIO;
	}

	return 0;
}

static int espi_kb106x_flash_write(const struct device *dev, struct espi_flash_packet *pckt)
{
	int ret;
	const struct espi_kb106x_config *config = dev->config;
	struct espi_kb106x_data *data = dev->data;
	struct espi_regs *espi = config->base_addr;
	struct espifa_regs *espifa = config->fa_addr;
	uint16_t len;
	k_spinlock_key_t key;

	if (!(espi->espi_c3cfg & ESPI_CH3_ENABLE)) {
		LOG_ERR("Flash channel is disabled");
		return -EIO;
	}

	if (k_is_in_isr()) {
		LOG_ERR("Flash channel not support use in isr");
		return -ENOTSUP;
	}

	/* check and set busy status */
	if (atomic_test_and_set_bit(&data->flash_status, 1)) {
		return -EBUSY;
	}

	key = k_spin_lock(&data->flash_reg_lock);

	data->flash_remain_len = pckt->len;
	data->flash_buf = pckt->buf;
	if (data->flash_remain_len > ESPIFA_ENE_BUFSIZE) {
		len = ESPIFA_ENE_BUFSIZE;
	} else {
		len = data->flash_remain_len;
	}
	memcpy((void *)espifa->espifa_dat, pckt->buf, len);
	data->flash_remain_len -= len;
	data->flash_buf += len;

	espifa->espifa_ba = pckt->flash_addr;
	espifa->espifa_cnt = len & 0x3F;
	espifa->espifa_ptcl = ESPIFA_WRITE | ESPIFA_ACC_ADDR;

	k_spin_unlock(&data->flash_reg_lock, key);

	/* Wait until ISR or timeout */
	ret = k_sem_take(&data->flash_lock, K_MSEC(config->fa_max_timeout));
	/* clear busy status */
	atomic_clear(&data->flash_status);
	if (ret == -EAGAIN) {
		LOG_ERR("%s timeout", __func__);
		return -ETIMEDOUT;
	}

	if (espifa->espifa_ef) {
		LOG_ERR("FLASH Tx failed (Error Flag:%x)", espifa->espifa_ef);
		/* Clear all error flag */
		espifa->espifa_ef = espifa->espifa_ef;
		return -EIO;
	}

	return 0;
}

static int espi_kb106x_flash_erase(const struct device *dev, struct espi_flash_packet *pckt)
{
	int ret;
	const struct espi_kb106x_config *config = dev->config;
	struct espi_kb106x_data *data = dev->data;
	struct espi_regs *espi = config->base_addr;
	struct espifa_regs *espifa = config->fa_addr;
	k_spinlock_key_t key;

	if (!(espi->espi_c3cfg & ESPI_CH3_ENABLE)) {
		LOG_ERR("Flash channel is disabled");
		return -EIO;
	}

	if (k_is_in_isr()) {
		LOG_ERR("Flash channel not support use in isr");
		return -ENOTSUP;
	}

	/* check and set busy status */
	if (atomic_test_and_set_bit(&data->flash_status, 1)) {
		return -EBUSY;
	}

	key = k_spin_lock(&data->flash_reg_lock);

	espifa->espifa_ba = pckt->flash_addr;
	espifa->espifa_ptcl = ESPIFA_ERASE;

	k_spin_unlock(&data->flash_reg_lock, key);

	/* Wait until ISR or timeout */
	ret = k_sem_take(&data->flash_lock, K_MSEC(config->fa_max_timeout));

	/* clear busy status */
	atomic_clear(&data->flash_status);
	if (ret == -EAGAIN) {
		LOG_ERR("%s timeout", __func__);
		return -ETIMEDOUT;
	}

	if (espifa->espifa_ef) {
		LOG_ERR("FLASH Tx failed (Error Flag:%x)", espifa->espifa_ef);
		/* Clear all error flag */
		espifa->espifa_ef = espifa->espifa_ef;
		return -EIO;
	}

	return 0;
}
#endif /* CONFIG_ESPI_FLASH_CHANNEL */

static DEVICE_API(espi, espi_kb106x_driver_api) = {
	.config = espi_kb106x_configure,
	.get_channel_status = espi_kb106x_channel_ready,
	.manage_callback = espi_kb106x_manage_callback,
#ifdef CONFIG_ESPI_PERIPHERAL_CHANNEL
	.read_lpc_request = espi_kb106x_read_lpc_request,
	.write_lpc_request = espi_kb106x_write_lpc_request,
#endif
#ifdef CONFIG_ESPI_VWIRE_CHANNEL
	.send_vwire = espi_kb106x_send_vwire,
	.receive_vwire = espi_kb106x_receive_vwire,
#endif
#ifdef CONFIG_ESPI_OOB_CHANNEL
	.send_oob = espi_kb106x_send_oob,
	.receive_oob = espi_kb106x_receive_oob,
#endif
#ifdef CONFIG_ESPI_FLASH_CHANNEL
	.flash_read = espi_kb106x_flash_read,
	.flash_write = espi_kb106x_flash_write,
	.flash_erase = espi_kb106x_flash_erase,
#endif
};

/* eSPI local functions */
#ifdef CONFIG_ESPI_PERIPHERAL_8042_KBC
static void kbc_kb106x_isr(const struct device *dev)
{
	const struct espi_kb106x_config *const config = dev->config;
	struct espi_kb106x_data *data = dev->data;
	struct kbc_regs *kbc = config->kbc_addr;
	struct espi_event evt = {
		.evt_type = ESPI_BUS_PERIPHERAL_NOTIFICATION,
		.evt_details = ESPI_PERIPHERAL_8042_KBC,
		.evt_data = ESPI_PERIPHERAL_NODATA,
	};
	struct espi_evt_data_kbc *kbc_evt = (struct espi_evt_data_kbc *)&evt.evt_data;

	if (kbc->kbc_pf & KBC_IBF_EVENT) {
		/* Clear IBF */
		kbc->kbc_pf = KBC_IBF_EVENT;
		/* KBC Input Buffer Full event */
		kbc_evt->evt = HOST_KBC_EVT_IBF;
		/*
		 * The data in KBC Input Buffer
		 * Indicates if the host sent a command or data.
		 * 0 = data 1 = Command.
		 */
		if (kbc->kbc_sts & KBSTS_ADDRESS_64) {
			kbc_evt->data = kbc->kbc_cmd;
			kbc_evt->type = 1;
		} else {
			kbc_evt->data = kbc->kbc_idp;
			kbc_evt->type = 0;
		}
		LOG_INF("kbc ibf (type:%x data: %x)", kbc_evt->type, kbc_evt->data);
		espi_send_callbacks(&data->callbacks, dev, evt);
		/* Clear Status registr IBF (notify host event finish) */
		kbc->kbc_sts = (kbc->kbc_sts & ~(KBSTS_OBF | KBSTS_IBF)) | KBSTS_IBF;
	}

	if (kbc->kbc_pf & KBC_OBE_EVENT) {
		/* Clear OBE event flag */
		kbc->kbc_pf = KBC_OBE_EVENT;
		/*
		 * Notify application that host already read out data. The application
		 * might need to clear status register via espi_api_lpc_write_request()
		 * with E8042_CLEAR_FLAG opcode in callback.
		 */
		kbc_evt->evt = HOST_KBC_EVT_OBE;
		kbc_evt->data = 0;
		kbc_evt->type = 0;
		LOG_INF("kbc obe (type:%x data: %x)", kbc_evt->type, kbc_evt->data);
		espi_send_callbacks(&data->callbacks, dev, evt);
	}
}
#endif /* CONFIG_ESPI_PERIPHERAL_8042_KBC */

#ifdef CONFIG_ESPI_PERIPHERAL_HOST_IO
static void eci_kb106x_isr(const struct device *dev)
{
	const struct espi_kb106x_config *const config = dev->config;
	struct espi_kb106x_data *data = dev->data;
	struct eci_regs *eci = config->eci_addr;
	struct espi_event evt = {
		.evt_type = ESPI_BUS_PERIPHERAL_NOTIFICATION,
		.evt_details = ESPI_PERIPHERAL_HOST_IO,
		.evt_data = ESPI_PERIPHERAL_NODATA,
	};
	struct espi_evt_data_acpi *acpi_evt = (struct espi_evt_data_acpi *)&evt.evt_data;

	if (eci->eci_pf & ECI_IBF_EVENT) {
		/* Clear IBF */
		eci->eci_pf = ECI_IBF_EVENT;
		/*
		 * Indicates if the host sent a command or data.
		 * 0 = data
		 * 1 = Command.
		 */
		if (eci->eci_sts & ECISTS_ADDRESS_CMD_PORT) {
			acpi_evt->data = eci->eci_cmd;
			acpi_evt->type = 1;
		} else {
			acpi_evt->data = eci->eci_idp;
			acpi_evt->type = 0;
		}
		LOG_INF("ec ibf (type:%x data: %x)", acpi_evt->type, acpi_evt->data);
		espi_send_callbacks(&data->callbacks, dev, evt);
		/* Clear Status registr IBF (notify host event finish) */
		eci->eci_sts = (eci->eci_sts & ~(ECISTS_OBF | ECISTS_IBF | ECISTS_SCI_QUERY_FLAG)) |
			       ECISTS_IBF;
	}

	if (eci->eci_pf & ECI_OBE_EVENT) {
		/* clear obe event flag */
		eci->eci_pf = ECI_OBE_EVENT;
	}
}
#endif /* CONFIG_ESPI_PERIPHERAL_HOST_IO */

#ifdef CONFIG_ESPI_PERIPHERAL_DEBUG_PORT_80
static void dbi_kb106x_isr(const struct device *dev)
{
	const struct espi_kb106x_config *const config = dev->config;
	struct espi_kb106x_data *data = dev->data;
	struct dbi_regs *dbi0 = config->dbi_addr;
	struct dbi_regs *dbi1 = dbi0 + 1;

	if (dbi0->dbi_pf & DBI_RX_EVENT) {
		struct espi_event evt = {
			.evt_type = ESPI_BUS_PERIPHERAL_NOTIFICATION,
			.evt_details =
				(ESPI_PERIPHERAL_INDEX_0 << 16) | ESPI_PERIPHERAL_DEBUG_PORT80,
			.evt_data = ESPI_PERIPHERAL_NODATA,
		};

		dbi0->dbi_pf = DBI_RX_EVENT;
		evt.evt_data = dbi0->dbi_idp;
		LOG_INF("dbi0 (data: %02x)", evt.evt_data);
		espi_send_callbacks(&data->callbacks, dev, evt);
	}

	if (dbi1->dbi_pf & DBI_RX_EVENT) {
		struct espi_event evt = {
			.evt_type = ESPI_BUS_PERIPHERAL_NOTIFICATION,
			.evt_details =
				(ESPI_PERIPHERAL_INDEX_1 << 16) | ESPI_PERIPHERAL_DEBUG_PORT80,
			.evt_data = ESPI_PERIPHERAL_NODATA,
		};

		dbi1->dbi_pf = DBI_RX_EVENT;
		evt.evt_data = dbi1->dbi_idp;
		LOG_INF("dbi1 (data: %02x)", evt.evt_data);
		espi_send_callbacks(&data->callbacks, dev, evt);
	}
}
#endif /* CONFIG_ESPI_PERIPHERAL_DEBUG_PORT_80 */

#ifdef CONFIG_ESPI_PERIPHERAL_KB106X_MAILBOX
static void mbx_kb106x_isr(const struct device *dev)
{
	const struct espi_kb106x_config *const config = dev->config;
	struct espi_kb106x_data *data = dev->data;
	struct espi_event evt = {.evt_type = ESPI_BUS_PERIPHERAL_NOTIFICATION};
	struct mbx_regs *mbx = (struct mbx_regs *)config->mbx_addr;

	mbx = (struct mbx_regs *)config->mbx_addr;
	if (mbx->mbx_pf & MBX_NOTIFY_EVENT) {
		mbx->mbx_pf = MBX_NOTIFY_EVENT;
		evt.evt_details = ESPI_PERIPHERAL_HOST_IO_PVT;
		evt.evt_data = mbx->mbx_ainfo;
		LOG_INF("mbx1 (details: %x data: %x)", evt.evt_details, evt.evt_data);
		espi_send_callbacks(&data->callbacks, dev, evt);
	}
}
#endif /* CONFIG_ESPI_PERIPHERAL_KB106X_MAILBOX */

#ifdef CONFIG_ESPI_PERIPHERAL_KB106X_LEGACY_IO
static void legi_kb106x_isr(const struct device *dev)
{
	const struct espi_kb106x_config *const config = dev->config;
	struct espi_kb106x_data *data = dev->data;
	struct espi_event evt = {.evt_type = ESPI_BUS_PERIPHERAL_NOTIFICATION};
	struct legi_regs *legi = (struct legi_regs *)config->legi_addr;

	legi = (struct legi_regs *)config->legi_addr;
	if (legi->legi_pf & LEGI_IBF_EVENT) {
		/* Clear IBF event flag */
		legi->legi_pf = LEGI_IBF_EVENT;
		evt.evt_details = ESPI_PERIPHERAL_EC_HOST_CMD;
		if (legi->legi_sts & LEGI_INDICATOR_FLAG) {
			evt.evt_data = legi->legi_cmd;
		} else {
			evt.evt_data = legi->legi_idp;
		}
		LOG_INF("legi0 ibf (hcmd data: %x)", evt.evt_data);
		espi_send_callbacks(&data->callbacks, dev, evt);
#ifndef CONFIG_ESPI_KB106X_LEGI_CLR_IBF_AFTER_OUTPUT_DATA_READY
		/* Clear Status registr IBF (notify host event finish) */
		legi->legi_sts = LEGI_IBF_EVENT;
#endif /* CONFIG_ESPI_KB106X_LEGI_CLR_IBF_AFTER_OUTPUT_DATA_READY */
	}

	if (legi->legi_pf & LEGI_OBE_EVENT) {
		/* Clear OBE event flag */
		legi->legi_pf = LEGI_OBE_EVENT;
	}
}
#endif /* CONFIG_ESPI_PERIPHERAL_KB106X_LEGACY_IO */

#ifdef CONFIG_ESPI_VWIRE_CHANNEL
static void espi_vw_notify(const struct device *dev, enum espi_vwire_signal signal)
{
	const struct espi_kb106x_config *config = dev->config;
	struct espi_kb106x_data *data = dev->data;
	struct espi_regs *espi = config->base_addr;
	uint8_t wire = 0;

	/* read vwire value and clear current valid bit */
	espi_kb106x_receive_vwire(dev, signal, &wire);
	struct espi_event evt = {
		.evt_type = ESPI_BUS_EVENT_VWIRE_RECEIVED,
		.evt_details = signal,
		.evt_data = wire,
	};

	switch (signal) {
#ifdef CONFIG_ESPI_AUTOMATIC_WARNING_ACKNOWLEDGE
	case ESPI_VWIRE_SIGNAL_HOST_RST_WARN:
		espi_kb106x_send_vwire(dev, ESPI_VWIRE_SIGNAL_HOST_RST_ACK, wire);
		break;
	case ESPI_VWIRE_SIGNAL_SUS_WARN:
		espi_kb106x_send_vwire(dev, ESPI_VWIRE_SIGNAL_SUS_ACK, wire);
		break;
	case ESPI_VWIRE_SIGNAL_OOB_RST_WARN:
		espi_kb106x_send_vwire(dev, ESPI_VWIRE_SIGNAL_OOB_RST_ACK, wire);
		break;
#endif /* CONFIG_ESPI_AUTOMATIC_WARNING_ACKNOWLEDGE */

	case ESPI_VWIRE_SIGNAL_PLTRST:
		if (wire != data->plt_rst) {
			data->plt_rst = wire;
			if (!wire) {
				espi->espi_c0cfg = DEFAULT_ESPIC0CFG;
				/* reset peripheral module setting */
				espi_kb106x_peripheral_init(dev);
				/* reset index6 table value to default */
				espi_kb106x_vwtab_set(dev, ESPI_VWIRE_SIGNAL_SCI, 1);
				espi_kb106x_vwtab_set(dev, ESPI_VWIRE_SIGNAL_SMI, 1);
				espi_kb106x_vwtab_set(dev, ESPI_VWIRE_SIGNAL_RST_CPU_INIT, 1);
				espi_kb106x_vwtab_set(dev, ESPI_VWIRE_SIGNAL_HOST_RST_ACK, 0);
				/* reset index7 table value to default */
				espi_kb106x_vwtab_set(dev, ESPI_VWIRE_SIGNAL_NMIOUT, 1);
				espi_kb106x_vwtab_set(dev, ESPI_VWIRE_SIGNAL_SMIOUT, 1);
				espi_kb106x_vwtab_set(dev, ESPI_VWIRE_SIGNAL_HOST_RST_WARN, 0);
			}
			espi_send_callbacks(&data->callbacks, dev, evt);
		}
		break;
	default:
		espi_send_callbacks(&data->callbacks, dev, evt);
		break;
	}
	LOG_INF("espi_vw %s (data: %x)", st_list[signal], wire);
}

static void espi_vw_kb106x_isr(const struct device *dev)
{
	const struct espi_kb106x_config *config = dev->config;
	struct espivw_regs *espivw = config->vw_addr;
	uint8_t *vwtab = (uint8_t *)(espivw->espivw_idx);
	uint8_t vwdata;
	uint8_t read_index;

	/* eSPI VW Rx event */
	if (espivw->espivw_pf & ESPIVW_RX_EVENT) {
		espivw->espivw_pf = ESPIVW_RX_EVENT;
		while (espivw->espivw_rxv & ESPIVW_RX_VALID_FLAG) {
			read_index = espivw->espivw_rxi;
			espivw->espivw_rxv = ESPIVW_RX_VALID_FLAG;
			for (int i = 0; i < ARRAY_SIZE(vw_pin); i++) {
				if (vw_pin[i].index == read_index) {
					/* get all valid bit */
					vwdata = vwtab[vw_pin[i].offset] >> ESPIVW_VALIDBIT_POS;
					if (vw_pin[i].bit & vwdata) {
						/* notify vw event */
						espi_vw_notify(dev, i);
					}
				}
			}
		}
	}

	/* eSPI VW Tx event */
	if (espivw->espivw_pf & ESPIVW_TX_EVENT) {
		espivw->espivw_pf = ESPIVW_TX_EVENT;
		/* Clear All Tx valid flag */
		for (int i = 0; i < ARRAY_SIZE(vw_pin); i++) {
			if ((vw_pin[i].dir == ESPI_TARGET_TO_CONTROLLER) &&
			    (vwtab[vw_pin[i].offset] & ESPIVW_VALIDBIT_MASK)) {
				vwtab[vw_pin[i].offset] &= ~ESPIVW_VALIDBIT_MASK;
			}
		}
	}
}
#endif /* CONFIG_ESPI_VWIRE_CHANNEL */

#ifdef CONFIG_ESPI_OOB_CHANNEL
static void espi_oob_kb106x_isr(const struct device *dev)
{
	const struct espi_kb106x_config *config = dev->config;
	struct espi_kb106x_data *data = dev->data;
	struct espioob_regs *espioob = config->oob_addr;

	/*eSPI OOB Disable event*/
	if (espioob->espioob_pf & ESPIOOB_DISABLE_EVENT) {
		atomic_clear(&data->oob_status);
		k_sem_give(&data->oob_tx_lock);
		espioob->espioob_pf = ESPIOOB_TX_EVENT | ESPIOOB_DISABLE_EVENT;
		k_work_schedule(&data->ch_check_work, K_NO_WAIT);
	}

	/* eSPI OOB Tx finish */
	if (espioob->espioob_pf & ESPIOOB_TX_EVENT) {
		atomic_clear(&data->oob_status);
		k_sem_give(&data->oob_tx_lock);
		espioob->espioob_pf = ESPIOOB_TX_EVENT;
	}

	/* eSPI OOB Rx finish */
	if (espioob->espioob_pf & ESPIOOB_RX_EVENT) {
#ifdef CONFIG_ESPI_OOB_CHANNEL_RX_ASYNC
		struct espi_event evt = {
			.evt_details = ESPI_CHANNEL_OOB,
			.evt_data = 0,
		};
		evt.evt_type = ESPI_BUS_EVENT_OOB_RECEIVED;
		evt.evt_details = espioob->espioob_rl & ENE_ESPIOOB_RXLEN_MASK;
		espi_send_callbacks(&data->callbacks, dev, evt);
#else
		k_sem_give(&data->oob_rx_lock);
#endif
		espioob->espioob_pf = ESPIOOB_RX_EVENT;
	}
}
#endif /* CONFIG_ESPI_OOB_CHANNEL */

#ifdef CONFIG_ESPI_FLASH_CHANNEL
static void espi_flash_kb106x_isr(const struct device *dev)
{
	const struct espi_kb106x_config *config = dev->config;
	struct espi_kb106x_data *data = dev->data;
	struct espi_regs *espi = config->base_addr;
	struct espifa_regs *espifa = config->fa_addr;
	uint8_t release_lock;
	uint32_t len;

	/* eSPI Flash Disable event */
	if (espifa->espifa_pf & ESPIFA_DISABLE_EVENT) {
		release_lock = 0;
		/* Flash Access Channel is disable now */
		if (!(espi->espi_c3cfg & ESPI_CH3_ENABLE)) {
			release_lock = 1;
		}
		/* No new protocol have issued when channel enable */
		if ((!(espi->espi_sta & ESPI_FLASH_NP_AVAIL)) &&
		    (!(espifa->espifa_pf & ESPIFA_TX_FINISH_EVENT))) {
			release_lock = 1;
		}

		if (release_lock) {
			k_sem_give(&data->flash_lock);
			k_work_schedule(&data->ch_check_work, K_NO_WAIT);
		}
		espifa->espifa_pf = ESPIFA_DISABLE_EVENT | ESPIFA_TX_FINISH_EVENT;
	}

	/* eSPI Flash protocol finish event */
	if (espifa->espifa_pf & ESPIFA_TX_FINISH_EVENT) {
		espifa->espifa_pf = ESPIFA_TX_FINISH_EVENT;
	}

	/* eSPI Flash Write/Erase Completion event */
	if (espifa->espifa_pf & ESPIFA_WRITE_ERASE_COMPLETE_EVENT) {
		if ((espifa->espifa_ptcl & 0x03) == ESPIFA_WRITE) {

			if (data->flash_remain_len) {
				if (data->flash_remain_len > ESPIFA_ENE_BUFSIZE) {
					len = ESPIFA_ENE_BUFSIZE;
				} else {
					len = data->flash_remain_len;
				}
				/* copy write data */
				memcpy((void *)espifa->espifa_dat, data->flash_buf, len);
				data->flash_remain_len -= len;
				data->flash_buf += len;
				/* zero value means the maximum length */
				espifa->espifa_cnt = len & (ESPIFA_ENE_BUFSIZE - 1);
				espifa->espifa_ptcl |= ESPIFA_ACC_ADDR;
			} else {
				k_sem_give(&data->flash_lock);
			}

		} else {
			k_sem_give(&data->flash_lock);
		}
		espifa->espifa_pf = ESPIFA_WRITE_ERASE_COMPLETE_EVENT;
	}

	/* eSPI Flash Read Completion event */
	if (espifa->espifa_pf & ESPIFA_READ_COMPLETE_EVENT) {
		/* copy read data */
		memcpy(data->flash_buf, (void *)espifa->espifa_dat, espifa->espifa_len);
		data->flash_remain_len -= espifa->espifa_len;
		data->flash_buf += espifa->espifa_len;
		if (data->flash_remain_len) {
			if (data->flash_remain_len >= ESPIFA_ENE_BUFSIZE) {
				len = ESPIFA_ENE_BUFSIZE;
			} else {
				len = data->flash_remain_len;
			}
			/* zero value means the maximum length */
			espifa->espifa_cnt = len & (ESPIFA_ENE_BUFSIZE - 1);
			espifa->espifa_ptcl |= ESPIFA_ACC_ADDR;
		} else {
			k_sem_give(&data->flash_lock);
		}
		espifa->espifa_pf = ESPIFA_READ_COMPLETE_EVENT;
	}

	/* eSPI Flash Un-success Completion event */
	if (espifa->espifa_pf & ESPIFA_UNSUCCESS_EVENT) {
		k_sem_give(&data->flash_lock);
		espifa->espifa_pf = ESPIFA_UNSUCCESS_EVENT;
	}
}
#endif /* CONFIG_ESPI_FLASH_CHANNEL */

static const struct device *const espi_device = DEVICE_DT_GET(DT_NODELABEL(espi0));
static struct gpio_callback espi_reset_cb;
static const struct gpio_dt_spec espirst = GPIO_DT_SPEC_GET(DT_NODELABEL(espi0), rst_gpios);
void espi_reset_kb106x_isr(const struct device *dev, struct gpio_callback *cb, uint32_t pins)
{
	struct espi_kb106x_data *data = espi_device->data;
	struct espi_event evt = {
		.evt_type = ESPI_BUS_RESET,
		.evt_details = 0,
		.evt_data = 0,
	};
	bool espi_reset = gpio_pin_get(dev, (find_msb_set(pins) - 1));

	ARG_UNUSED(cb);
	evt.evt_data = espi_reset;
	if (!espi_reset) {

		/* initail channel enable default value */
		data->pher_ch_en = 0;
		data->vw_ch_en = 0;
		data->oob_ch_en = 0;
		data->flash_ch_en = 0;
		/* initail plt_rst default value */
		data->plt_rst = 0;
	}
	espi_send_callbacks(&data->callbacks, espi_device, evt);
}

#ifdef CONFIG_ESPI_AUTOMATIC_BOOT_DONE_ACKNOWLEDGE
static void send_slave_bootdone(const struct device *dev)
{
	int ret;
	uint8_t boot_done;

	ret = espi_kb106x_receive_vwire(dev, ESPI_VWIRE_SIGNAL_TARGET_BOOT_DONE, &boot_done);
	if (!ret && !boot_done) {
		/* SLAVE_BOOT_DONE & SLAVE_LOAD_STS have to be sent together */
		espi_kb106x_send_vwire(dev, ESPI_VWIRE_SIGNAL_TARGET_BOOT_STS, 1);
		espi_kb106x_send_vwire(dev, ESPI_VWIRE_SIGNAL_TARGET_BOOT_DONE, 1);
	}
}
#endif /* CONFIG_ESPI_AUTOMATIC_BOOT_DONE_ACKNOWLEDGE */

static void espi_ch_periodic_handle(struct k_work *work)
{
	struct espi_kb106x_data *data = CONTAINER_OF((struct k_work_delayable *)work,
						     struct espi_kb106x_data, ch_check_work);
	const struct device *dev = data->dev;
	struct espi_event evt = {
		.evt_type = ESPI_BUS_EVENT_CHANNEL_READY,
		.evt_details = 0,
		.evt_data = 0,
	};

#ifdef CONFIG_ESPI_PERIPHERAL_CHANNEL
	evt.evt_data = espi_get_channel_status(dev, ESPI_CHANNEL_PERIPHERAL);
	if (evt.evt_data != data->pher_ch_en) {
		data->pher_ch_en = evt.evt_data;
		evt.evt_details = ESPI_CHANNEL_PERIPHERAL;
		espi_send_callbacks(&data->callbacks, dev, evt);
	}
#endif /* CONFIG_ESPI_PERIPHERAL_CHANNEL */

#ifdef CONFIG_ESPI_VWIRE_CHANNEL
	evt.evt_data = espi_get_channel_status(dev, ESPI_CHANNEL_VWIRE);
	if (evt.evt_data != data->vw_ch_en) {
		data->vw_ch_en = evt.evt_data;
		evt.evt_details = ESPI_CHANNEL_VWIRE;
		espi_send_callbacks(&data->callbacks, dev, evt);
#ifdef CONFIG_ESPI_AUTOMATIC_BOOT_DONE_ACKNOWLEDGE
		if (data->vw_ch_en) {
			send_slave_bootdone(dev);
		}
#endif /* CONFIG_ESPI_AUTOMATIC_BOOT_DONE_ACKNOWLEDGE */
	}
#endif /* CONFIG_ESPI_PERIPHERAL_CHANNEL */

#ifdef CONFIG_ESPI_OOB_CHANNEL
	evt.evt_data = espi_get_channel_status(dev, ESPI_CHANNEL_OOB);
	if (evt.evt_data != data->oob_ch_en) {
		data->oob_ch_en = evt.evt_data;
		evt.evt_details = ESPI_CHANNEL_OOB;
		espi_send_callbacks(&data->callbacks, dev, evt);
	}
#endif /* CONFIG_ESPI_OOB_CHANNEL */

#ifdef CONFIG_ESPI_FLASH_CHANNEL
	evt.evt_data = espi_get_channel_status(dev, ESPI_CHANNEL_FLASH);
	if (evt.evt_data != data->flash_ch_en) {
		data->flash_ch_en = evt.evt_data;
		evt.evt_details = ESPI_CHANNEL_FLASH;
		espi_send_callbacks(&data->callbacks, dev, evt);
	}
#endif /* CONFIG_ESPI_FLASH_CHANNEL */

	k_work_schedule(&data->ch_check_work, K_MSEC(10));
}

#ifdef CONFIG_ESPI_PERIPHERAL_CUSTOM_OPCODE
void espi_custom_opcode_int_set(const struct device *dev, uint16_t port_num, uint8_t enable)
{
	const struct espi_kb106x_config *config = dev->config;
#if CONFIG_ESPI_PERIPHERAL_HOST_IO
	struct eci_regs *eci = (struct eci_regs *)config->eci_addr;
#endif /* CONFIG_ESPI_PERIPHERAL_HOST_IO */
#ifdef CONFIG_ESPI_PERIPHERAL_DEBUG_PORT_80
	struct dbi_regs *dbi = (struct dbi_regs *)config->dbi_addr;
#endif /* CONFIG_ESPI_PERIPHERAL_DEBUG_PORT_80 */
#ifdef CONFIG_ESPI_PERIPHERAL_KB106X_LEGACY_IO
	struct legi_regs *legi = (struct legi_regs *)config->legi_addr;
#endif /* CONFIG_ESPI_PERIPHERAL_KB106X_LEGACY_IO */
#ifdef CONFIG_ESPI_PERIPHERAL_KB106X_MAILBOX
	struct mbx_regs *mbx = (struct mbx_regs *)config->mbx_addr;
#endif /* CONFIG_ESPI_PERIPHERAL_KB106X_MAILBOX */

/* peripheral channel - kbc io (60/64) */
#ifdef CONFIG_ESPI_PERIPHERAL_8042_KBC
	if (!port_num) {
		if (enable) {
			irq_enable(DT_INST_IRQ_BY_NAME(0, kbc, irq));
		} else {
			irq_disable(DT_INST_IRQ_BY_NAME(0, kbc, irq));
		}
	}
#endif /* CONFIG_ESPI_PERIPHERAL_8042_KBC */

/* peripheral channel - ec io (62/66) */
#if CONFIG_ESPI_PERIPHERAL_HOST_IO
	if ((!port_num) || (port_num == config->eci_port)) {
		if (enable) {
			eci->eci_ie |= ECI_IBF_EVENT | ECI_OBE_EVENT;
		} else {
			eci->eci_ie &= ~(ECI_IBF_EVENT | ECI_OBE_EVENT);
		}
	}
#endif /* CONFIG_ESPI_PERIPHERAL_HOST_IO */

/* peripheral channel - debug port (80/81) */
#ifdef CONFIG_ESPI_PERIPHERAL_DEBUG_PORT_80
	if ((!port_num) || (port_num == config->dbi0_port)) {
		dbi = (struct dbi_regs *)config->dbi_addr;
		if (enable) {
			dbi->dbi_ie |= DBI_RX_EVENT;
		} else {
			dbi->dbi_ie &= ~(DBI_RX_EVENT);
		}
	}
	if ((!port_num) || (port_num == config->dbi1_port)) {
		dbi = (struct dbi_regs *)config->dbi_addr + 1;
		if (enable) {
			dbi->dbi_ie |= DBI_RX_EVENT;
		} else {
			dbi->dbi_ie &= ~(DBI_RX_EVENT);
		}
	}
#endif /* CONFIG_ESPI_PERIPHERAL_DEBUG_PORT_80 */

/* peripheral channel - ene legacy io (68/6c, 78/7c) */
#ifdef CONFIG_ESPI_PERIPHERAL_KB106X_LEGACY_IO
	if ((!port_num) || (port_num == CONFIG_ESPI_PERIPHERAL_HOST_CMD_DATA_PORT_NUM)) {
		legi = (struct legi_regs *)config->legi_addr;
		if (enable) {
			legi->legi_ie |= LEGI_IBF_EVENT | LEGI_OBE_EVENT;
		} else {
			legi->legi_ie &= ~(LEGI_IBF_EVENT | LEGI_OBE_EVENT);
		}
	}
#endif /* CONFIG_ESPI_PERIPHERAL_KB106X_LEGACY_IO */

/* peripheral channel - ene mailbox io */
#ifdef CONFIG_ESPI_PERIPHERAL_KB106X_MAILBOX
	if ((!port_num) || (port_num == CONFIG_ESPI_PERIPHERAL_ACPI_SHM_REGION_PORT_NUM)) {
		mbx = (struct mbx_regs *)config->mbx_addr;
		if (enable) {
			mbx->mbx_ie |= MBX_NOTIFY_EVENT;
		} else {
			mbx->mbx_ie &= ~(MBX_NOTIFY_EVENT);
		}
	}
#endif /* CONFIG_ESPI_PERIPHERAL_KB106X_MAILBOX */
}

void espi_custom_opcode_outdata_set(const struct device *dev, uint16_t port_num, uint8_t dat)
{
	const struct espi_kb106x_config *config = dev->config;
#ifdef CONFIG_ESPI_PERIPHERAL_KB106X_LEGACY_IO
	struct legi_regs *legi = (struct legi_regs *)config->legi_addr;
#endif /* CONFIG_ESPI_PERIPHERAL_KB106X_LEGACY_IO */
#ifdef CONFIG_ESPI_PERIPHERAL_DEBUG_PORT_80
	struct dbi_regs *dbi = (struct dbi_regs *)config->dbi_addr;
#endif /* CONFIG_ESPI_PERIPHERAL_DEBUG_PORT_80 */

/* Write result to the data byte. */
#ifdef CONFIG_ESPI_PERIPHERAL_DEBUG_PORT_80
	if (port_num == config->dbi0_port) {
		dbi = (struct dbi_regs *)config->dbi_addr;
		dbi->dbi_odp = dat;
		return;
	}
	if (port_num == config->dbi1_port) {
		dbi = (struct dbi_regs *)config->dbi_addr + 1;
		dbi->dbi_odp = dat;
		return;
	}
#endif /* CONFIG_ESPI_PERIPHERAL_DEBUG_PORT_80 */

#ifdef CONFIG_ESPI_PERIPHERAL_EC_HOST_CMD
#ifdef CONFIG_ESPI_PERIPHERAL_KB106X_LEGACY_IO
	legi = (struct legi_regs *)config->legi_addr;
	legi->legi_odp = dat;
#ifdef CONFIG_ESPI_KB106X_LEGI_CLR_IBF_AFTER_OUTPUT_DATA_READY
	/* Clear Status registr IBF (notify host event finish) */
	legi->legi_sts = LEGI_IBF_EVENT;
#endif /* CONFIG_ESPI_KB106X_LEGI_CLR_IBF_AFTER_OUTPUT_DATA_READY */
#endif /* CONFIG_ESPI_PERIPHERAL_KB106X_LEGACY_IO */
#endif /* CONFIG_ESPI_PERIPHERAL_EC_HOST_CMD */
}
#endif /* CONFIG_ESPI_PERIPHERAL_CUSTOM_OPCODE */

#ifdef CONFIG_ESPI_PERIPHERAL_CHANNEL
void espi_kb106x_peripheral_init(const struct device *dev)
{
	const struct espi_kb106x_config *config = dev->config;
#ifdef CONFIG_ESPI_PERIPHERAL_8042_KBC
	struct kbc_regs *kbc = config->kbc_addr;
#endif /* CONFIG_ESPI_PERIPHERAL_8042_KBC */
#if CONFIG_ESPI_PERIPHERAL_HOST_IO
	struct eci_regs *eci = config->eci_addr;
#endif /* CONFIG_ESPI_PERIPHERAL_HOST_IO */
#ifdef CONFIG_ESPI_PERIPHERAL_DEBUG_PORT_80
	struct dbi_regs *dbi = config->dbi_addr;
#endif /* CONFIG_ESPI_PERIPHERAL_DEBUG_PORT_80 */
#ifdef CONFIG_ESPI_PERIPHERAL_KB106X_LEGACY_IO
	struct legi_regs *legi = (struct legi_regs *)config->legi_addr;
	uint16_t port_iobase;
	uint16_t port_offset;
#endif /* CONFIG_ESPI_PERIPHERAL_KB106X_LEGACY_IO */
#if (CONFIG_ESPI_PERIPHERAL_KB106X_HOST_TO_RAM)
	struct hif_regs *hif = config->hif_addr;
	uint32_t ram_base;
	uint32_t port_base;
#endif /* CONFIG_ESPI_PERIPHERAL_KB106X_HOST_TO_RAM */
#ifdef CONFIG_ESPI_PERIPHERAL_KB106X_MAILBOX
	struct mbx_regs *mbx = (struct mbx_regs *)config->mbx_addr;
	uint16_t mbx_port_base;
	uint32_t mbx_ram_base;
#endif /* CONFIG_ESPI_PERIPHERAL_KB106X_MAILBOX */

/* peripheral channel - kbc io (60/64) */
#ifdef CONFIG_ESPI_PERIPHERAL_8042_KBC
	if (IS_ENABLED(CONFIG_ESPI_PERIPHERAL_8042_KBC)) {
		IRQ_CONNECT(DT_INST_IRQ_BY_NAME(0, kbc, irq), DT_INST_IRQ_BY_NAME(0, kbc, priority),
			    kbc_kb106x_isr, DEVICE_DT_INST_GET(0), 0);
		irq_enable(DT_INST_IRQ_BY_NAME(0, kbc, irq));
		/* clear stsatus flag */
		kbc->kbc_sts |= KBSTS_OBF | KBSTS_IBF;
		/* Enable IBF OBE interrupt */
		kbc->kbc_ie |= KBC_OBE_EVENT | KBC_IBF_EVENT;
		/* Enable SIRQ 12 and SIRQ 1. */
		kbc->kbc_cb |= KBC_IRQ1_ENABLE | KBC_IRQ12_ENABLE;
		/* Enable kbc and auto clear output buffer after read */
		kbc->kbc_cfg |= KBC_FUNCTION_ENABLE | KBC_OUTPUT_READ_CLR_ENABLE;
	}
#endif /* CONFIG_ESPI_PERIPHERAL_8042_KBC */

/* peripheral channel - ec io (62/66) */
#if CONFIG_ESPI_PERIPHERAL_HOST_IO
	if (IS_ENABLED(CONFIG_ESPI_PERIPHERAL_HOST_IO)) {
		IRQ_CONNECT(DT_INST_IRQ_BY_NAME(0, eci, irq), DT_INST_IRQ_BY_NAME(0, eci, priority),
			    eci_kb106x_isr, DEVICE_DT_INST_GET(0), 0);
		irq_enable(DT_INST_IRQ_BY_NAME(0, eci, irq));
		eci->eci_ie |= ECI_IBF_EVENT | ECI_OBE_EVENT;
		eci->eci_pf = ECI_IBF_EVENT | ECI_OBE_EVENT;
		eci->eci_cfg =
			((config->eci_port & ECI_BASE_MASK) << ECI_BASE_POS) | ECI_FUNCTION_ENABLE;
	}
#endif /* CONFIG_ESPI_PERIPHERAL_HOST_IO */

/* peripheral channel - debug port (80/81) */
#ifdef CONFIG_ESPI_PERIPHERAL_DEBUG_PORT_80
	if (IS_ENABLED(CONFIG_ESPI_PERIPHERAL_DEBUG_PORT_80)) {
		IRQ_CONNECT(DT_INST_IRQ_BY_NAME(0, dbi, irq), DT_INST_IRQ_BY_NAME(0, dbi, priority),
			    dbi_kb106x_isr, DEVICE_DT_INST_GET(0), 0);
		irq_enable(DT_INST_IRQ_BY_NAME(0, dbi, irq));
		/* debug port index 0 */
		dbi->dbi_pf = DBI_RX_EVENT;
		dbi->dbi_ie |= DBI_RX_EVENT;
		dbi->dbi_cfg = (config->dbi0_port << DBI_BASE_POS) | DBI_FUNCTION_ENABLE;
		dbi = config->dbi_addr + 1;
		/* debug port index 1 */
		dbi->dbi_pf = DBI_RX_EVENT;
		dbi->dbi_ie |= DBI_RX_EVENT;
		dbi->dbi_cfg = (config->dbi0_port << DBI_BASE_POS) | DBI_FUNCTION_ENABLE;
	}
#endif /* CONFIG_ESPI_PERIPHERAL_DEBUG_PORT_80 */

/* peripheral channel - ene legacy io (68/6c, 78/7c) */
#ifdef CONFIG_ESPI_PERIPHERAL_KB106X_LEGACY_IO
	if (IS_ENABLED(CONFIG_ESPI_PERIPHERAL_KB106X_LEGACY_IO)) {
		legi = (struct legi_regs *)config->legi_addr;
		port_iobase = CONFIG_ESPI_PERIPHERAL_HOST_CMD_DATA_PORT_NUM;
		port_offset = 3;
		legi->legi_ie |= LEGI_IBF_EVENT | LEGI_OBE_EVENT;
		legi->legi_pf = LEGI_IBF_EVENT | LEGI_OBE_EVENT;
		legi->legi_cfg = (port_iobase << LEGI_BASE_POS) | (port_offset << LEGI_OFFSET_POS) |
				 LEGI_FUNCTION_ENABLE;

		IRQ_CONNECT(DT_INST_IRQ_BY_NAME(0, legi, irq),
			    DT_INST_IRQ_BY_NAME(0, legi, priority), legi_kb106x_isr,
			    DEVICE_DT_INST_GET(0), 0);
		irq_enable(DT_INST_IRQ_BY_NAME(0, legi, irq));
	}
#endif /* CONFIG_ESPI_PERIPHERAL_KB106X_LEGACY_IO */

/* peripheral channel - ene host to ram */
#ifdef CONFIG_ESPI_PERIPHERAL_KB106X_HOST_TO_RAM
	if (IS_ENABLED(CONFIG_ESPI_PERIPHERAL_KB106X_HOST_TO_RAM)) {
		ram_base = (uint32_t)shm_host_cmd;
		port_base = CONFIG_ESPI_PERIPHERAL_HOST_CMD_PARAM_PORT_NUM;
		hif->ios_ie = ram_base & IO2RAM_RAM_BASE_MASK;
		hif->ios_cfg = (port_base << IO2RAM_IO_BASE_POS) | (0 << IO2RAM_NOTIFY_OFFSET_POS) |
			       H2RAM_FUNCTION_ENABLE;
		/* setting io2ram read/write protect region */
		hif->ios_rwp = 0;
	}
#endif /* CONFIG_ESPI_PERIPHERAL_KB106X_HOST_TO_RAM */

/* peripheral channel - ene mailbox io */
#ifdef CONFIG_ESPI_PERIPHERAL_KB106X_MAILBOX
	if (IS_ENABLED(CONFIG_ESPI_PERIPHERAL_KB106X_MAILBOX)) {
		mbx = (struct mbx_regs *)config->mbx_addr;
		mbx_ram_base = (uint32_t)shm_acpi_mmap;
		mbx_port_base = CONFIG_ESPI_PERIPHERAL_ACPI_SHM_REGION_PORT_NUM;
		mbx->mbx_rba = mbx_ram_base;
		mbx->mbx_cfg = (mbx_port_base << MBX_IO_BASE_POS) | MBX_FUNCTION_ENABLE;
		mbx->mbx_pf = MBX_NOTIFY_EVENT;

		IRQ_CONNECT(DT_INST_IRQ_BY_NAME(0, mbx, irq), DT_INST_IRQ_BY_NAME(0, mbx, priority),
			    mbx_kb106x_isr, DEVICE_DT_INST_GET(0), 0);
		irq_enable(DT_INST_IRQ_BY_NAME(0, mbx, irq));
	}
#endif /* CONFIG_ESPI_PERIPHERAL_KB106X_MAILBOX */
}
#endif /* CONFIG_ESPI_PERIPHERAL_CHANNEL */

static int espi_kb106x_init(const struct device *dev)
{
	int ret;
	const struct espi_kb106x_config *config = dev->config;
	struct espi_kb106x_data *data = dev->data;
#ifdef CONFIG_ESPI_VWIRE_CHANNEL
	struct espivw_regs *espivw = config->vw_addr;
	uint16_t *vwblk_base_dir = (uint16_t *)&espivw->espivw_b10;
	uint8_t vw_base;
	uint8_t vw_num;
#endif /* CONFIG_ESPI_VWIRE_CHANNEL */
#ifdef CONFIG_ESPI_OOB_CHANNEL
	struct espioob_regs *espioob = config->oob_addr;
#endif /* CONFIG_ESPI_OOB_CHANNEL */
#ifdef CONFIG_ESPI_FLASH_CHANNEL
	struct espifa_regs *espifa = config->fa_addr;
#endif /* CONFIG_ESPI_FLASH_CHANNEL */

	ret = pinctrl_apply_state(config->pcfg, PINCTRL_STATE_DEFAULT);
	if (ret != 0) {
		return ret;
	}

	/* keep device pointer */
	data->dev = (struct device *)dev;
	/* initial periodic work for espi status check  */
	k_work_init_delayable(&data->ch_check_work, espi_ch_periodic_handle);
	k_work_schedule(&data->ch_check_work, K_MSEC(10));

	/* initail channel enable default value */
	data->pher_ch_en = 0;
	data->vw_ch_en = 0;
	data->oob_ch_en = 0;
	data->flash_ch_en = 0;

#ifdef CONFIG_ESPI_PERIPHERAL_CHANNEL
	espi_kb106x_peripheral_init(dev);
#endif /* CONFIG_ESPI_PERIPHERAL_CHANNEL */

#ifdef CONFIG_ESPI_VWIRE_CHANNEL
	/* virtual vwire channel */
	IRQ_CONNECT(DT_INST_IRQ_BY_NAME(0, vwire, irq), DT_INST_IRQ_BY_NAME(0, vwire, priority),
		    espi_vw_kb106x_isr, DEVICE_DT_INST_GET(0), 0);
	irq_enable(DT_INST_IRQ_BY_NAME(0, vwire, irq));

	/* Initial VW block base and direction */
	vwblk_base_dir[0] = ESPIVW_B0_BASE;
	vwblk_base_dir[1] = ESPIVW_B1_BASE;
	vwblk_base_dir[2] = ESPIVW_B2_BASE;
	vwblk_base_dir[3] = ESPIVW_B3_BASE;
	for (int i = 0; i < ARRAY_SIZE(vw_pin); i++) {
		if (vw_pin[i].dir == ESPI_CONTROLLER_TO_TARGET) {
			continue;
		}
		vw_base = vw_pin[i].index & ESPIVW_INDEXBASE_MASK;
		vw_num = (vw_pin[i].index & ESPIVW_INDEXNUM_MASK) + ESPIVW_BLK_DIR_POS;
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
	espivw->espivw_pf = ESPIVW_TX_EVENT | ESPIVW_RX_EVENT;
	espivw->espivw_ie |= ESPIVW_TX_EVENT | ESPIVW_RX_EVENT;

	/* initail plt_rst default value */
	data->plt_rst = 0;
#endif /* CONFIG_ESPI_VWIRE_CHANNEL */

#ifdef CONFIG_ESPI_OOB_CHANNEL
	/* OOB channel */
	IRQ_CONNECT(DT_INST_IRQ_BY_NAME(0, oob, irq), DT_INST_IRQ_BY_NAME(0, oob, priority),
		    espi_oob_kb106x_isr, DEVICE_DT_INST_GET(0), 0);
	irq_enable(DT_INST_IRQ_BY_NAME(0, oob, irq));
	espioob->espioob_pf = ESPIOOB_TX_EVENT | ESPIOOB_RX_EVENT | ESPIOOB_DISABLE_EVENT;
	espioob->espioob_ie |= ESPIOOB_TX_EVENT | ESPIOOB_RX_EVENT | ESPIOOB_DISABLE_EVENT;
	k_sem_init(&data->oob_tx_lock, 0, 1);
#ifndef CONFIG_ESPI_OOB_CHANNEL_RX_ASYNC
	k_sem_init(&data->oob_rx_lock, 0, 1);
#endif /* CONFIG_ESPI_OOB_CHANNEL_RX_ASYNC */
#endif /* CONFIG_ESPI_OOB_CHANNEL */

#ifdef CONFIG_ESPI_FLASH_CHANNEL
	/* Flash channel */
	IRQ_CONNECT(DT_INST_IRQ_BY_NAME(0, flash, irq), DT_INST_IRQ_BY_NAME(0, flash, priority),
		    espi_flash_kb106x_isr, DEVICE_DT_INST_GET(0), 0);
	irq_enable(DT_INST_IRQ_BY_NAME(0, flash, irq));
	espifa->espifa_pf = ESPIFA_TX_FINISH_EVENT | ESPIFA_WRITE_ERASE_COMPLETE_EVENT |
			    ESPIFA_READ_COMPLETE_EVENT | ESPIFA_UNSUCCESS_EVENT |
			    ESPIFA_DISABLE_EVENT;
	espifa->espifa_ie |= ESPIFA_TX_FINISH_EVENT | ESPIFA_WRITE_ERASE_COMPLETE_EVENT |
			     ESPIFA_READ_COMPLETE_EVENT | ESPIFA_UNSUCCESS_EVENT |
			     ESPIFA_DISABLE_EVENT;
	k_sem_init(&data->flash_lock, 0, 1);
#endif /* CONFIG_ESPI_FLASH_CHANNEL */

	/* Initial eSPI Reset ISR (gpio isr connection) */
	gpio_pin_configure_dt(&espirst, GPIO_INPUT);
	gpio_pin_interrupt_configure_dt(&espirst, GPIO_INT_EDGE_BOTH);
	gpio_init_callback(&espi_reset_cb, espi_reset_kb106x_isr, BIT(espirst.pin));
	gpio_add_callback(espirst.port, &espi_reset_cb);

	return 0;
}

PINCTRL_DT_INST_DEFINE(0);
static struct espi_kb106x_data espi_kb106x_data_0;
static const struct espi_kb106x_config espi_kb106x_config_0 = {
	.base_addr = (struct espi_regs *)DT_INST_REG_ADDR(0),
	.vw_addr = (struct espivw_regs *)DT_INST_PROP(0, espivw_reg),
	.vwtab_addr = (uintptr_t)DT_INST_PROP(0, espivw_tab),
	.oob_addr = (struct espioob_regs *)DT_INST_PROP(0, espioob_reg),
	.fa_addr = (struct espifa_regs *)DT_INST_PROP(0, espifa_reg),
	.hif_addr = (struct hif_regs *)DT_INST_PROP(0, hif_reg),
	.kbc_addr = (struct kbc_regs *)DT_INST_PROP(0, kbc_reg),
	.eci_addr = (struct eci_regs *)DT_INST_PROP(0, eci_reg),
	.dbi_addr = (struct dbi_regs *)DT_INST_PROP(0, dbi_reg),
	.legi_addr = (struct legi_regs *)DT_INST_PROP(0, legi_reg),
	.mbx_addr = (struct mbx_regs *)DT_INST_PROP(0, mbx_reg),
	.uart_addr = (struct uart_regs *)DT_INST_PROP(0, uart_reg),
	.oob_max_timeout = DT_INST_PROP_OR(0, oob_max_timeout, DEFAULT_OOB_TIMEOUT),
	.fa_max_timeout = DT_INST_PROP_OR(0, fa_max_timeout, DEFAULT_FA_TIMEOUT),
	.eci_port = DT_INST_PROP_OR(0, eci_port, DEFAULT_ECI_PORT_NUM),
	.dbi0_port = DT_INST_PROP_OR(0, dbi0_port, DEFAULT_DBI0_PORT_NUM),
	.dbi1_port = DT_INST_PROP_OR(0, dbi1_port, DEFAULT_DBI1_PORT_NUM),
	.pcfg = PINCTRL_DT_INST_DEV_CONFIG_GET(0),
};
DEVICE_DT_INST_DEFINE(0, espi_kb106x_init, NULL, &espi_kb106x_data_0, &espi_kb106x_config_0,
		      PRE_KERNEL_2, CONFIG_ESPI_INIT_PRIORITY, &espi_kb106x_driver_api);
