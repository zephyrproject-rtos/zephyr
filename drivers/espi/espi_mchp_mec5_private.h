/*
 * Copyright (c) 2024 Microchip Technology Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_ESPI_MCHP_MEC5_PRIVATE_H_
#define ZEPHYR_DRIVERS_ESPI_MCHP_MEC5_PRIVATE_H_

#include <zephyr/device.h>
#include <zephyr/kernel.h>
#include <zephyr/drivers/espi.h>

#ifdef CONFIG_ESPI_PERIPHERAL_EC_HOST_CMD
#ifdef CONFIG_ESPI_PERIPHERAL_ACPI_SHM_REGION
#define MEC5_ACPI_EC_HCMD_SHM_SIZE ((CONFIG_ESPI_MEC5_PERIPHERAL_HOST_CMD_PARAM_SIZE) +\
				    (CONFIG_ESPI_MEC5_PERIPHERAL_ACPI_SHD_MEM_SIZE))
#define MEC5_ACPI_EC_HCMD_SHM_HOFS 0
#define MEC5_ACPI_EC_HCMD_SHM_SOFS (CONFIG_ESPI_MEC5_PERIPHERAL_HOST_CMD_PARAM_SIZE)
#define MEC5_ACPI_EC_HCMD_SHM_RD_SIZE (MEC5_ACPI_EC_HCMD_SHM_SIZE)
#define MEC5_ACPI_EC_HCMD_SHM_WR_SIZE (CONFIG_ESPI_MEC5_PERIPHERAL_HOST_CMD_PARAM_SIZE)
#else
#define MEC5_ACPI_EC_HCMD_SHM_SIZE (CONFIG_ESPI_MEC5_PERIPHERAL_HOST_CMD_PARAM_SIZE)
#define MEC5_ACPI_EC_HCMD_SHM_HOFS 0
#define MEC5_ACPI_EC_HCMD_SHM_SOFS -1
#define MEC5_ACPI_EC_HCMD_SHM_RD_SIZE (MEC5_ACPI_EC_HCMD_SHM_SIZE)
#define MEC5_ACPI_EC_HCMD_SHM_WR_SIZE (MEC5_ACPI_EC_HCMD_SHM_SIZE)
#endif
#endif

/* MEC5 device configuration structure. */
struct espi_mec5_dev_config {
	struct espi_io_regs *iob;
	struct espi_mem_regs *memb;
	struct espi_vw_regs *vwb;
	uint32_t membar_hi;
	uint32_t srambar_hi;
	uint16_t cfg_io_addr;
	const struct pinctrl_dev_config *pcfg;
	void (*irq_cfg_func)(const struct device *espi_dev);
};

/* begin eSPI driver data structure definitions */
#if defined(CONFIG_ESPI_OOB_CHANNEL)
#define MEC5_MAX_OOB_TIMEOUT_MS 200u

/* OOB channel supported. Requires >= 4-byte aligned RX and TX buffers */
struct espi_mec5_oob_data {
	struct k_sem tx_sync;
#ifndef CONFIG_ESPI_OOB_CHANNEL_RX_ASYNC
	struct k_sem rx_sync;
#endif
	uint8_t tx_tag;
	uint8_t rx_tag;
	uint32_t rx_status;
	uint32_t tx_status;
	uint32_t rx_mem[CONFIG_ESPI_OOB_BUFFER_SIZE / 4]; /* 128 bytes */
	uint32_t tx_mem[CONFIG_ESPI_OOB_BUFFER_SIZE / 4];
};
#endif

#if defined(CONFIG_ESPI_FLASH_CHANNEL)
#define MEC5_MAX_FC_TIMEOUT_MS 1000u

struct espi_mec5_fc_data {
	struct k_sem flash_lock;
	uint32_t fc_status;
	uint32_t fc_mem[CONFIG_ESPI_FLASH_BUFFER_SIZE >> 2]; /* must be >= 4 byte aligned */
};
#endif

struct espi_mec5_data {
	volatile uint8_t espi_reset_cnt;
	volatile uint8_t espi_reset_asserted;
	volatile uint8_t pltrst_asserted;
	volatile uint8_t slp_s3_edge;
	volatile uint8_t slp_s4_edge;
	volatile uint8_t slp_s5_edge;
	struct espi_callback vwcb;
	sys_slist_t callbacks;
#ifdef CONFIG_ESPI_PERIPHERAL_EC_HOST_CMD
	uint8_t hcmd_sram[MEC5_ACPI_EC_HCMD_SHM_SIZE] __aligned(4);
#endif
#if defined(CONFIG_ESPI_OOB_CHANNEL)
	struct espi_mec5_oob_data oob_data;
#endif
#if defined(CONFIG_ESPI_FLASH_CHANNEL)
	struct espi_mec5_fc_data fc_data;
#endif
};
/* end eSPI driver data definitions */

#define ESPI_LD_HA_FLAG_SZ_POS		0
#define ESPI_LD_HA_FLAG_SZ_MSK		0xfu
#define ESPI_LD_HA_FLAG_SZ_8_BITS	1
#define ESPI_LD_HA_FLAG_SZ_16_BITS	2
#define ESPI_LD_HA_FLAG_SZ_32_BITS	4
#define ESPI_LD_HA_FLAG_SZ_48_BITS	6
#define ESPI_LD_HA_FLAG_MEM_POS		7

struct espi_ld_host_addr {
	uint32_t haddr_lsw;
	uint16_t haddr_msh;
	uint8_t flags;
	uint8_t ldn;
};

void espi_mec5_send_callbacks(const struct device *dev, struct espi_event evt);

#define ESPI_MEC5_BAR_CFG_LDN_POS	0
#define ESPI_MEC5_BAR_CFG_LDN_MSK	0xffu
#define ESPI_MEC5_BAR_CFG_EN_POS	8
#define ESPI_MEC5_BAR_CFG_MEM_BAR_POS	9

int espi_mec5_bar_config(const struct device *espi_dev, uint32_t haddr, uint32_t cfg);

#define ESPI_MEC5_SIRQ_CFG_LDN_POS	0
#define ESPI_MEC5_SIRQ_CFG_LDN_MSK	0xffu
#define ESPI_MEC5_SIRQ_CFG_LDN_IDX_POS	8
#define ESPI_MEC5_SIRQ_CFG_LDN_IDX_MSK	0xf00u
#define ESPI_MEC5_SIRQ_CFG_SLOT_POS	12
#define ESPI_MEC5_SIRQ_CFG_SLOT_MSK	0xf000u

int espi_mec5_sirq_config(const struct device *espi_dev, uint32_t cfg);

int espi_mec5_get_ld_host_addr(const struct device *espi_dev, const struct device *ld_dev,
			       struct espi_ld_host_addr *ldha);

uint32_t espi_mec5_shm_addr_get(const struct device *espi_dev, enum lpc_peripheral_opcode op);
uint32_t espi_mec_shm_size_get(const struct device *espi_dev, enum lpc_peripheral_opcode op);

/* Flash Channel */
#ifdef CONFIG_ESPI_FLASH_CHANNEL
int mec5_espi_fc_read(const struct device *dev, struct espi_flash_packet *pckt);
int mec5_espi_fc_write(const struct device *dev, struct espi_flash_packet *pckt);
int mec5_espi_fc_erase(const struct device *dev, struct espi_flash_packet *pckt);
void mec5_espi_fc_irq_connect(const struct device *espi_dev);
#endif

/* OOB Channel */
#ifdef CONFIG_ESPI_OOB_CHANNEL
int mec5_espi_oob_upstream(const struct device *dev, struct espi_oob_packet *pckt);
int mec5_espi_oob_downstream(const struct device *dev, struct espi_oob_packet *pckt);
void mec5_espi_oob_irq_connect(const struct device *espi_dev);
#endif

#endif /* ZEPHYR_DRIVERS_ESPI_MCHP_MEC5_PRIVATE_H_ */
