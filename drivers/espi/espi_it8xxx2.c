/*
 * Copyright (c) 2021 ITE Corporation. All Rights Reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT ite_it8xxx2_espi

#include <assert.h>
#include <zephyr/drivers/espi.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/interrupt_controller/wuc_ite_it8xxx2.h>
#include <zephyr/kernel.h>
#include <soc.h>
#include <soc_dt.h>
#include "soc_espi.h"
#include "espi_utils.h"

#include <zephyr/logging/log.h>
#include <zephyr/irq.h>
LOG_MODULE_REGISTER(espi, CONFIG_ESPI_LOG_LEVEL);

#define ESPI_IT8XXX2_GET_GCTRL_BASE \
	((struct gctrl_it8xxx2_regs *)DT_REG_ADDR(DT_NODELABEL(gctrl)))

#define IT8XXX2_ESPI_IRQ     DT_INST_IRQ_BY_IDX(0, 0, irq)
#define IT8XXX2_ESPI_VW_IRQ  DT_INST_IRQ_BY_IDX(0, 1, irq)
#define IT8XXX2_KBC_IBF_IRQ  DT_INST_IRQ_BY_IDX(0, 2, irq)
#define IT8XXX2_KBC_OBE_IRQ  DT_INST_IRQ_BY_IDX(0, 3, irq)
#define IT8XXX2_PMC1_IBF_IRQ DT_INST_IRQ_BY_IDX(0, 4, irq)
#define IT8XXX2_PORT_80_IRQ  DT_INST_IRQ_BY_IDX(0, 5, irq)
#define IT8XXX2_PMC2_IBF_IRQ DT_INST_IRQ_BY_IDX(0, 6, irq)
#define IT8XXX2_TRANS_IRQ    DT_INST_IRQ_BY_IDX(0, 7, irq)

/* General Capabilities and Configuration 1 */
#define IT8XXX2_ESPI_MAX_FREQ_MASK GENMASK(2, 0)
#define IT8XXX2_ESPI_CAPCFG1_MAX_FREQ_20 0
#define IT8XXX2_ESPI_CAPCFG1_MAX_FREQ_25 1
#define IT8XXX2_ESPI_CAPCFG1_MAX_FREQ_33 2
#define IT8XXX2_ESPI_CAPCFG1_MAX_FREQ_50 3
#define IT8XXX2_ESPI_CAPCFG1_MAX_FREQ_66 4

#define IT8XXX2_ESPI_PC_READY_MASK  BIT(1)
#define IT8XXX2_ESPI_VW_READY_MASK  BIT(1)
#define IT8XXX2_ESPI_OOB_READY_MASK BIT(1)
#define IT8XXX2_ESPI_FC_READY_MASK  BIT(1)

#define IT8XXX2_ESPI_INTERRUPT_ENABLE          BIT(7)
#define IT8XXX2_ESPI_TO_WUC_ENABLE             BIT(4)
#define IT8XXX2_ESPI_VW_INTERRUPT_ENABLE       BIT(7)
#define IT8XXX2_ESPI_INTERRUPT_PUT_PC          BIT(7)

/*
 * VWCTRL2 register:
 * bit4 = 1b: Refers to ESPI_RESET# for PLTRST#.
 */
#define IT8XXX2_ESPI_VW_RESET_PLTRST           BIT(4)

#define IT8XXX2_ESPI_UPSTREAM_ENABLE           BIT(7)
#define IT8XXX2_ESPI_UPSTREAM_GO               BIT(6)
#define IT8XXX2_ESPI_UPSTREAM_INTERRUPT_ENABLE BIT(5)
#define IT8XXX2_ESPI_UPSTREAM_CHANNEL_DISABLE  BIT(2)
#define IT8XXX2_ESPI_UPSTREAM_DONE             BIT(1)
#define IT8XXX2_ESPI_UPSTREAM_BUSY             BIT(0)

#define IT8XXX2_ESPI_CYCLE_TYPE_OOB            0x07

#define IT8XXX2_ESPI_PUT_OOB_STATUS            BIT(7)
#define IT8XXX2_ESPI_PUT_OOB_INTERRUPT_ENABLE  BIT(7)
#define IT8XXX2_ESPI_PUT_OOB_LEN_MASK          GENMASK(6, 0)

#define IT8XXX2_ESPI_INPUT_PAD_GATING          BIT(6)

#define IT8XXX2_ESPI_FLASH_MAX_PAYLOAD_SIZE    64
#define IT8XXX2_ESPI_PUT_FLASH_TAG_MASK        GENMASK(7, 4)
#define IT8XXX2_ESPI_PUT_FLASH_LEN_MASK        GENMASK(6, 0)

struct espi_it8xxx2_wuc {
	/* WUC control device structure */
	const struct device *wucs;
	/* WUC pin mask */
	uint8_t mask;
};

struct espi_it8xxx2_config {
	uintptr_t base_espi_slave;
	uintptr_t base_espi_vw;
	uintptr_t base_espi_queue1;
	uintptr_t base_espi_queue0;
	uintptr_t base_ec2i;
	uintptr_t base_kbc;
	uintptr_t base_pmc;
	uintptr_t base_smfi;
	const struct espi_it8xxx2_wuc wuc;
};

struct espi_it8xxx2_data {
	sys_slist_t callbacks;
#ifdef CONFIG_ESPI_OOB_CHANNEL
	struct k_sem oob_upstream_go;
#endif
#ifdef CONFIG_ESPI_FLASH_CHANNEL
	struct k_sem flash_upstream_go;
	uint8_t put_flash_cycle_type;
	uint8_t put_flash_tag;
	uint8_t put_flash_len;
	uint8_t flash_buf[IT8XXX2_ESPI_FLASH_MAX_PAYLOAD_SIZE];
#endif
};

struct vw_channel_t {
	uint8_t  vw_index;      /* VW index of signal */
	uint8_t  level_mask;    /* level bit of signal */
	uint8_t  valid_mask;    /* valid bit of signal */
};

struct vwidx_isr_t {
	void (*vwidx_isr)(const struct device *dev, uint8_t update_flag);
	uint8_t vw_index;
};

enum espi_ch_enable_isr_type {
	DEASSERTED_FLAG = 0,
	ASSERTED_FLAG = 1,
};

struct espi_isr_t {
	void (*espi_isr)(const struct device *dev, bool enable);
	enum espi_ch_enable_isr_type isr_type;
};

struct espi_vw_signal_t {
	enum espi_vwire_signal signal;
	void (*vw_signal_isr)(const struct device *dev);
};

/* EC2I bridge and PNPCFG devices */
static const struct ec2i_t kbc_settings[] = {
	/* Select logical device 06h(keyboard) */
	{HOST_INDEX_LDN, LDN_KBC_KEYBOARD},
	/* Set IRQ=01h for logical device */
	{HOST_INDEX_IRQNUMX, 0x01},
	/* Configure IRQTP for KBC. */
	/*
	 * Interrupt request type select (IRQTP) for KBC.
	 * bit 1, 0: IRQ request is buffered and applied to SERIRQ
	 *        1: IRQ request is inverted before being applied to SERIRQ
	 * bit 0, 0: Edge triggered mode
	 *        1: Level triggered mode
	 *
	 * This interrupt configuration should the same on both host and EC side
	 */
	{HOST_INDEX_IRQTP, 0x02},
	/* Enable logical device */
	{HOST_INDEX_LDA, 0x01},

#ifdef CONFIG_ESPI_IT8XXX2_PNPCFG_DEVICE_KBC_MOUSE
	/* Select logical device 05h(mouse) */
	{HOST_INDEX_LDN, LDN_KBC_MOUSE},
	/* Set IRQ=0Ch for logical device */
	{HOST_INDEX_IRQNUMX, 0x0C},
	/* Enable logical device */
	{HOST_INDEX_LDA, 0x01},
#endif
};

static const struct ec2i_t pmc1_settings[] = {
	/* Select logical device 11h(PM1 ACPI) */
	{HOST_INDEX_LDN, LDN_PMC1},
	/* Set IRQ=00h for logical device */
	{HOST_INDEX_IRQNUMX, 0x00},
	/* Enable logical device */
	{HOST_INDEX_LDA, 0x01},
};

#ifdef CONFIG_ESPI_PERIPHERAL_EC_HOST_CMD
#define IT8XXX2_ESPI_HC_DATA_PORT_MSB \
	((CONFIG_ESPI_PERIPHERAL_HOST_CMD_DATA_PORT_NUM >> 8) & 0xff)
#define IT8XXX2_ESPI_HC_DATA_PORT_LSB \
	(CONFIG_ESPI_PERIPHERAL_HOST_CMD_DATA_PORT_NUM & 0xff)
#define IT8XXX2_ESPI_HC_CMD_PORT_MSB \
	(((CONFIG_ESPI_PERIPHERAL_HOST_CMD_DATA_PORT_NUM + 4) >> 8) & 0xff)
#define IT8XXX2_ESPI_HC_CMD_PORT_LSB \
	((CONFIG_ESPI_PERIPHERAL_HOST_CMD_DATA_PORT_NUM + 4) & 0xff)
static const struct ec2i_t pmc2_settings[] = {
	/* Select logical device 12h(PM2 host command) */
	{HOST_INDEX_LDN, LDN_PMC2},
	/* I/O Port Base Address (data/command ports) */
	{HOST_INDEX_IOBAD0_MSB, IT8XXX2_ESPI_HC_DATA_PORT_MSB},
	{HOST_INDEX_IOBAD0_LSB, IT8XXX2_ESPI_HC_DATA_PORT_LSB},
	{HOST_INDEX_IOBAD1_MSB, IT8XXX2_ESPI_HC_CMD_PORT_MSB},
	{HOST_INDEX_IOBAD1_LSB, IT8XXX2_ESPI_HC_CMD_PORT_LSB},
	/* Set IRQ=00h for logical device */
	{HOST_INDEX_IRQNUMX, 0x00},
	/* Enable logical device */
	{HOST_INDEX_LDA, 0x01},
};
#endif

#if defined(CONFIG_ESPI_PERIPHERAL_EC_HOST_CMD) || \
	defined(CONFIG_ESPI_PERIPHERAL_ACPI_SHM_REGION)
/*
 * Host to RAM (H2RAM) memory mapping.
 * This feature allows host access EC's memory directly by eSPI I/O cycles.
 * Mapping range is 4K bytes and base address is adjustable.
 * Eg. the I/O cycle 800h~8ffh from host can be mapped to x800h~x8ffh.
 * Linker script of h2ram.ld will make the pool 4K aligned.
 */
#define IT8XXX2_ESPI_H2RAM_POOL_SIZE_MAX 0x1000
#define IT8XXX2_ESPI_H2RAM_OFFSET_MASK   GENMASK(3, 0)

#if defined(CONFIG_ESPI_PERIPHERAL_ACPI_SHM_REGION)
#define H2RAM_ACPI_SHM_MAX ((CONFIG_ESPI_IT8XXX2_ACPI_SHM_H2RAM_SIZE) + \
			(CONFIG_ESPI_PERIPHERAL_ACPI_SHM_REGION_PORT_NUM))
#if (H2RAM_ACPI_SHM_MAX > IT8XXX2_ESPI_H2RAM_POOL_SIZE_MAX)
#error "ACPI shared memory region out of h2ram"
#endif
#endif /* CONFIG_ESPI_PERIPHERAL_ACPI_SHM_REGION */

#if defined(CONFIG_ESPI_PERIPHERAL_EC_HOST_CMD)
#define H2RAM_EC_HOST_CMD_MAX ((CONFIG_ESPI_IT8XXX2_HC_H2RAM_SIZE) + \
			(CONFIG_ESPI_PERIPHERAL_HOST_CMD_PARAM_PORT_NUM))
#if (H2RAM_EC_HOST_CMD_MAX > IT8XXX2_ESPI_H2RAM_POOL_SIZE_MAX)
#error "EC host command parameters out of h2ram"
#endif
#endif /* CONFIG_ESPI_PERIPHERAL_EC_HOST_CMD */

#if defined(CONFIG_ESPI_PERIPHERAL_EC_HOST_CMD) && \
	defined(CONFIG_ESPI_PERIPHERAL_ACPI_SHM_REGION)
#if (MIN(H2RAM_ACPI_SHM_MAX, H2RAM_EC_HOST_CMD_MAX) > \
	MAX(CONFIG_ESPI_PERIPHERAL_ACPI_SHM_REGION_PORT_NUM, \
		CONFIG_ESPI_PERIPHERAL_HOST_CMD_PARAM_PORT_NUM))
#error "ACPI and HC sections of h2ram overlap"
#endif
#endif

static uint8_t h2ram_pool[MAX(H2RAM_ACPI_SHM_MAX, H2RAM_EC_HOST_CMD_MAX)]
					__attribute__((section(".h2ram_pool")));

#define H2RAM_WINDOW_SIZE(ram_size) ((find_msb_set((ram_size) / 16) - 1) & 0x7)

static const struct ec2i_t smfi_settings[] = {
	/* Select logical device 0Fh(SMFI) */
	{HOST_INDEX_LDN, LDN_SMFI},
	/* Internal RAM base address on eSPI I/O space */
	{HOST_INDEX_DSLDC6, 0x00},
	/* Enable H2RAM eSPI I/O cycle */
	{HOST_INDEX_DSLDC7, 0x01},
	/* Enable logical device */
	{HOST_INDEX_LDA, 0x01},
};

static void smfi_it8xxx2_init(const struct device *dev)
{
	const struct espi_it8xxx2_config *const config = dev->config;
	struct smfi_it8xxx2_regs *const smfi_reg =
		(struct smfi_it8xxx2_regs *)config->base_smfi;
	struct gctrl_it8xxx2_regs *const gctrl = ESPI_IT8XXX2_GET_GCTRL_BASE;
	uint8_t h2ram_offset;

	/* Set the host to RAM cycle address offset */
	h2ram_offset = ((uint32_t)h2ram_pool & 0xffff) /
				IT8XXX2_ESPI_H2RAM_POOL_SIZE_MAX;
	gctrl->GCTRL_H2ROFSR =
		(gctrl->GCTRL_H2ROFSR & ~IT8XXX2_ESPI_H2RAM_OFFSET_MASK) |
		h2ram_offset;

#ifdef CONFIG_ESPI_PERIPHERAL_EC_HOST_CMD
	memset(&h2ram_pool[CONFIG_ESPI_PERIPHERAL_HOST_CMD_PARAM_PORT_NUM], 0,
			CONFIG_ESPI_IT8XXX2_HC_H2RAM_SIZE);
	/* Set host RAM window 0 base address */
	smfi_reg->SMFI_HRAMW0BA =
		(CONFIG_ESPI_PERIPHERAL_HOST_CMD_PARAM_PORT_NUM >> 4) & 0xff;
	/* Set host RAM window 0 size. (allow R/W) */
	smfi_reg->SMFI_HRAMW0AAS =
		H2RAM_WINDOW_SIZE(CONFIG_ESPI_IT8XXX2_HC_H2RAM_SIZE);
	/* Enable window 0, H2RAM through IO cycle */
	smfi_reg->SMFI_HRAMWC |= (SMFI_H2RAMPS | SMFI_H2RAMW0E);
#endif

#ifdef CONFIG_ESPI_PERIPHERAL_ACPI_SHM_REGION
	memset(&h2ram_pool[CONFIG_ESPI_PERIPHERAL_ACPI_SHM_REGION_PORT_NUM], 0,
			CONFIG_ESPI_IT8XXX2_ACPI_SHM_H2RAM_SIZE);
	/* Set host RAM window 1 base address */
	smfi_reg->SMFI_HRAMW1BA =
		(CONFIG_ESPI_PERIPHERAL_ACPI_SHM_REGION_PORT_NUM >> 4) & 0xff;
	/* Set host RAM window 1 size. (read-only) */
	smfi_reg->SMFI_HRAMW1AAS =
		H2RAM_WINDOW_SIZE(CONFIG_ESPI_IT8XXX2_ACPI_SHM_H2RAM_SIZE) |
		SMFI_HRAMWXWPE_ALL;
	/* Enable window 1, H2RAM through IO cycle */
	smfi_reg->SMFI_HRAMWC |= (SMFI_H2RAMPS | SMFI_H2RAMW1E);
#endif
}
#endif /* CONFIG_ESPI_PERIPHERAL_EC_HOST_CMD ||
	* CONFIG_ESPI_PERIPHERAL_ACPI_SHM_REGION
	*/

static void ec2i_it8xxx2_wait_status_cleared(const struct device *dev,
						uint8_t mask)
{
	const struct espi_it8xxx2_config *const config = dev->config;
	struct ec2i_regs *const ec2i = (struct ec2i_regs *)config->base_ec2i;

	while (ec2i->IBCTL & mask) {
		;
	}
}

static void ec2i_it8xxx2_write_pnpcfg(const struct device *dev,
					enum ec2i_access sel, uint8_t data)
{
	const struct espi_it8xxx2_config *const config = dev->config;
	struct ec2i_regs *const ec2i = (struct ec2i_regs *)config->base_ec2i;

	/* bit0: EC to I-Bus access enabled. */
	ec2i->IBCTL |= EC2I_IBCTL_CSAE;
	/*
	 * Wait that both CRIB and CWIB bits in IBCTL register
	 * are cleared.
	 */
	ec2i_it8xxx2_wait_status_cleared(dev, EC2I_IBCTL_CRWIB);
	/* Enable EC access to the PNPCFG registers */
	ec2i->IBMAE |= EC2I_IBMAE_CFGAE;
	/* Set indirect host I/O offset. */
	ec2i->IHIOA = sel;
	/* Write the data to IHD register */
	ec2i->IHD = data;
	/* Wait the CWIB bit in IBCTL cleared. */
	ec2i_it8xxx2_wait_status_cleared(dev, EC2I_IBCTL_CWIB);
	/* Disable EC access to the PNPCFG registers. */
	ec2i->IBMAE &= ~EC2I_IBMAE_CFGAE;
	/* Disable EC to I-Bus access. */
	ec2i->IBCTL &= ~EC2I_IBCTL_CSAE;
}

static void ec2i_it8xxx2_write(const struct device *dev,
				enum host_pnpcfg_index index, uint8_t data)
{
	/* Set index */
	ec2i_it8xxx2_write_pnpcfg(dev, EC2I_ACCESS_INDEX, index);
	/* Set data */
	ec2i_it8xxx2_write_pnpcfg(dev, EC2I_ACCESS_DATA, data);
}

static void pnpcfg_it8xxx2_configure(const struct device *dev,
					const struct ec2i_t *settings,
					size_t entries)
{
	for (size_t i = 0; i < entries; i++) {
		ec2i_it8xxx2_write(dev, settings[i].index_port,
					settings[i].data_port);
	}
}

#define PNPCFG(_s)						\
	pnpcfg_it8xxx2_configure(dev, _s##_settings, ARRAY_SIZE(_s##_settings))

static void pnpcfg_it8xxx2_init(const struct device *dev)
{
	const struct espi_it8xxx2_config *const config = dev->config;
	struct ec2i_regs *const ec2i = (struct ec2i_regs *)config->base_ec2i;
	struct gctrl_it8xxx2_regs *const gctrl = ESPI_IT8XXX2_GET_GCTRL_BASE;

	/* The register pair to access PNPCFG is 004Eh and 004Fh */
	gctrl->GCTRL_BADRSEL = 0x1;
	/* Host access is disabled */
	ec2i->LSIOHA |= 0x3;
	/* configure pnpcfg devices */
	if (IS_ENABLED(CONFIG_ESPI_PERIPHERAL_8042_KBC)) {
		PNPCFG(kbc);
	}
	if (IS_ENABLED(CONFIG_ESPI_PERIPHERAL_HOST_IO)) {
		PNPCFG(pmc1);
	}
#ifdef CONFIG_ESPI_PERIPHERAL_EC_HOST_CMD
	PNPCFG(pmc2);
#endif
#if defined(CONFIG_ESPI_PERIPHERAL_EC_HOST_CMD) || \
	defined(CONFIG_ESPI_PERIPHERAL_ACPI_SHM_REGION)
	PNPCFG(smfi);
#endif
}

/* KBC (port 60h/64h) */
#ifdef CONFIG_ESPI_PERIPHERAL_8042_KBC
static void kbc_it8xxx2_ibf_isr(const struct device *dev)
{
	const struct espi_it8xxx2_config *const config = dev->config;
	struct espi_it8xxx2_data *const data = dev->data;
	struct kbc_regs *const kbc_reg = (struct kbc_regs *)config->base_kbc;
	struct espi_event evt = {
		ESPI_BUS_PERIPHERAL_NOTIFICATION,
		ESPI_PERIPHERAL_8042_KBC,
		ESPI_PERIPHERAL_NODATA
	};
	struct espi_evt_data_kbc *kbc_evt =
			(struct espi_evt_data_kbc *)&evt.evt_data;

	/* KBC Input Buffer Full event */
	kbc_evt->evt = HOST_KBC_EVT_IBF;
	/*
	 * Indicates if the host sent a command or data.
	 * 0 = data
	 * 1 = Command.
	 */
	kbc_evt->type = !!(kbc_reg->KBHISR & KBC_KBHISR_A2_ADDR);
	/* The data in KBC Input Buffer */
	kbc_evt->data = kbc_reg->KBHIDIR;

	espi_send_callbacks(&data->callbacks, dev, evt);
}

static void kbc_it8xxx2_obe_isr(const struct device *dev)
{
	const struct espi_it8xxx2_config *const config = dev->config;
	struct espi_it8xxx2_data *const data = dev->data;
	struct kbc_regs *const kbc_reg = (struct kbc_regs *)config->base_kbc;
	struct espi_event evt = {
		ESPI_BUS_PERIPHERAL_NOTIFICATION,
		ESPI_PERIPHERAL_8042_KBC,
		ESPI_PERIPHERAL_NODATA
	};
	struct espi_evt_data_kbc *kbc_evt =
				(struct espi_evt_data_kbc *)&evt.evt_data;

	/* Disable KBC OBE interrupt first */
	kbc_reg->KBHICR &= ~KBC_KBHICR_OBECIE;

	/* Notify application that host already read out data. */
	kbc_evt->evt = HOST_KBC_EVT_OBE;
	kbc_evt->data = 0;
	kbc_evt->type = 0;
	espi_send_callbacks(&data->callbacks, dev, evt);
}

static void kbc_it8xxx2_init(const struct device *dev)
{
	const struct espi_it8xxx2_config *const config = dev->config;
	struct kbc_regs *const kbc_reg = (struct kbc_regs *)config->base_kbc;

	/* Disable KBC serirq IRQ */
	kbc_reg->KBIRQR = 0;

	/*
	 * bit3: Input Buffer Full CPU Interrupt Enable.
	 * bit1: Enable the interrupt to mouse driver in the host processor via
	 *       SERIRQ when the output buffer is full.
	 * bit0: Enable the interrupt to keyboard driver in the host processor
	 *       via SERIRQ when the output buffer is full
	 */
	kbc_reg->KBHICR |=
		(KBC_KBHICR_IBFCIE | KBC_KBHICR_OBFKIE | KBC_KBHICR_OBFMIE);

	/* Input Buffer Full CPU Interrupt Enable. */
	IRQ_CONNECT(IT8XXX2_KBC_IBF_IRQ, 0, kbc_it8xxx2_ibf_isr,
			DEVICE_DT_INST_GET(0), 0);
	irq_enable(IT8XXX2_KBC_IBF_IRQ);

	/* Output Buffer Empty CPU Interrupt Enable */
	IRQ_CONNECT(IT8XXX2_KBC_OBE_IRQ, 0, kbc_it8xxx2_obe_isr,
			DEVICE_DT_INST_GET(0), 0);
	irq_enable(IT8XXX2_KBC_OBE_IRQ);
}
#endif

/* PMC 1 (APCI port 62h/66h) */
#ifdef CONFIG_ESPI_PERIPHERAL_HOST_IO
static void pmc1_it8xxx2_ibf_isr(const struct device *dev)
{
	const struct espi_it8xxx2_config *const config = dev->config;
	struct espi_it8xxx2_data *const data = dev->data;
	struct pmc_regs *const pmc_reg = (struct pmc_regs *)config->base_pmc;
	struct espi_event evt = {
		ESPI_BUS_PERIPHERAL_NOTIFICATION,
		ESPI_PERIPHERAL_HOST_IO,
		ESPI_PERIPHERAL_NODATA
	};
	struct espi_evt_data_acpi *acpi_evt =
				(struct espi_evt_data_acpi *)&evt.evt_data;

	/*
	 * Indicates if the host sent a command or data.
	 * 0 = data
	 * 1 = Command.
	 */
	acpi_evt->type = !!(pmc_reg->PM1STS & PMC_PM1STS_A2_ADDR);
	/* Set processing flag before reading command byte */
	pmc_reg->PM1STS |= PMC_PM1STS_GPF;
	acpi_evt->data = pmc_reg->PM1DI;

	espi_send_callbacks(&data->callbacks, dev, evt);
}

static void pmc1_it8xxx2_init(const struct device *dev)
{
	const struct espi_it8xxx2_config *const config = dev->config;
	struct pmc_regs *const pmc_reg = (struct pmc_regs *)config->base_pmc;

	/* Enable pmc1 input buffer full interrupt */
	pmc_reg->PM1CTL |= PMC_PM1CTL_IBFIE;
	IRQ_CONNECT(IT8XXX2_PMC1_IBF_IRQ, 0, pmc1_it8xxx2_ibf_isr,
			DEVICE_DT_INST_GET(0), 0);
	irq_enable(IT8XXX2_PMC1_IBF_IRQ);
}
#endif

/* Port 80 */
#ifdef CONFIG_ESPI_PERIPHERAL_DEBUG_PORT_80
static void port80_it8xxx2_isr(const struct device *dev)
{
	struct espi_it8xxx2_data *const data = dev->data;
	struct gctrl_it8xxx2_regs *const gctrl = ESPI_IT8XXX2_GET_GCTRL_BASE;
	struct espi_event evt = {
		ESPI_BUS_PERIPHERAL_NOTIFICATION,
		(ESPI_PERIPHERAL_INDEX_0 << 16) | ESPI_PERIPHERAL_DEBUG_PORT80,
		ESPI_PERIPHERAL_NODATA
	};

	evt.evt_data = gctrl->GCTRL_P80HDR;
	/* Write 1 to clear this bit */
	gctrl->GCTRL_P80H81HSR |= BIT(0);

	espi_send_callbacks(&data->callbacks, dev, evt);
}

static void port80_it8xxx2_init(const struct device *dev)
{
	ARG_UNUSED(dev);
	struct gctrl_it8xxx2_regs *const gctrl = ESPI_IT8XXX2_GET_GCTRL_BASE;

	/* Accept Port 80h Cycle */
	gctrl->GCTRL_SPCTRL1 |= IT8XXX2_GCTRL_ACP80;
	IRQ_CONNECT(IT8XXX2_PORT_80_IRQ, 0, port80_it8xxx2_isr,
			DEVICE_DT_INST_GET(0), 0);
	irq_enable(IT8XXX2_PORT_80_IRQ);
}
#endif

#ifdef CONFIG_ESPI_PERIPHERAL_EC_HOST_CMD
/* PMC 2 (Host command port CONFIG_ESPI_PERIPHERAL_HOST_CMD_DATA_PORT_NUM) */
static void pmc2_it8xxx2_ibf_isr(const struct device *dev)
{
	const struct espi_it8xxx2_config *const config = dev->config;
	struct espi_it8xxx2_data *const data = dev->data;
	struct pmc_regs *const pmc_reg = (struct pmc_regs *)config->base_pmc;
	struct espi_event evt = {
		ESPI_BUS_PERIPHERAL_NOTIFICATION,
		ESPI_PERIPHERAL_EC_HOST_CMD,
		ESPI_PERIPHERAL_NODATA
	};

	/* Set processing flag before reading command byte */
	pmc_reg->PM2STS |= PMC_PM2STS_GPF;
	evt.evt_data = pmc_reg->PM2DI;

	espi_send_callbacks(&data->callbacks, dev, evt);
}

static void pmc2_it8xxx2_init(const struct device *dev)
{
	const struct espi_it8xxx2_config *const config = dev->config;
	struct pmc_regs *const pmc_reg = (struct pmc_regs *)config->base_pmc;

	/* Dedicated interrupt for PMC2 */
	pmc_reg->MBXCTRL |= PMC_MBXCTRL_DINT;
	/* Enable pmc2 input buffer full interrupt */
	pmc_reg->PM2CTL |= PMC_PM2CTL_IBFIE;
	IRQ_CONNECT(IT8XXX2_PMC2_IBF_IRQ, 0, pmc2_it8xxx2_ibf_isr,
			DEVICE_DT_INST_GET(0), 0);
	irq_enable(IT8XXX2_PMC2_IBF_IRQ);
}
#endif

/* eSPI api functions */
#define VW_CHAN(signal, index, level, valid) \
	[signal] = {.vw_index = index, .level_mask = level, .valid_mask = valid}

/* VW signals used in eSPI */
static const struct vw_channel_t vw_channel_list[] = {
	VW_CHAN(ESPI_VWIRE_SIGNAL_SLP_S3,        0x02, BIT(0), BIT(4)),
	VW_CHAN(ESPI_VWIRE_SIGNAL_SLP_S4,        0x02, BIT(1), BIT(5)),
	VW_CHAN(ESPI_VWIRE_SIGNAL_SLP_S5,        0x02, BIT(2), BIT(6)),
	VW_CHAN(ESPI_VWIRE_SIGNAL_OOB_RST_WARN,  0x03, BIT(2), BIT(6)),
	VW_CHAN(ESPI_VWIRE_SIGNAL_PLTRST,        0x03, BIT(1), BIT(5)),
	VW_CHAN(ESPI_VWIRE_SIGNAL_SUS_STAT,      0x03, BIT(0), BIT(4)),
	VW_CHAN(ESPI_VWIRE_SIGNAL_NMIOUT,        0x07, BIT(2), BIT(6)),
	VW_CHAN(ESPI_VWIRE_SIGNAL_SMIOUT,        0x07, BIT(1), BIT(5)),
	VW_CHAN(ESPI_VWIRE_SIGNAL_HOST_RST_WARN, 0x07, BIT(0), BIT(4)),
	VW_CHAN(ESPI_VWIRE_SIGNAL_SLP_A,         0x41, BIT(3), BIT(7)),
	VW_CHAN(ESPI_VWIRE_SIGNAL_SUS_PWRDN_ACK, 0x41, BIT(1), BIT(5)),
	VW_CHAN(ESPI_VWIRE_SIGNAL_SUS_WARN,      0x41, BIT(0), BIT(4)),
	VW_CHAN(ESPI_VWIRE_SIGNAL_SLP_WLAN,      0x42, BIT(1), BIT(5)),
	VW_CHAN(ESPI_VWIRE_SIGNAL_SLP_LAN,       0x42, BIT(0), BIT(4)),
	VW_CHAN(ESPI_VWIRE_SIGNAL_HOST_C10,      0x47, BIT(0), BIT(4)),
	VW_CHAN(ESPI_VWIRE_SIGNAL_DNX_WARN,      0x4a, BIT(1), BIT(5)),
	VW_CHAN(ESPI_VWIRE_SIGNAL_PME,           0x04, BIT(3), BIT(7)),
	VW_CHAN(ESPI_VWIRE_SIGNAL_WAKE,          0x04, BIT(2), BIT(6)),
	VW_CHAN(ESPI_VWIRE_SIGNAL_OOB_RST_ACK,   0x04, BIT(0), BIT(4)),
	VW_CHAN(ESPI_VWIRE_SIGNAL_SLV_BOOT_STS,  0x05, BIT(3), BIT(7)),
	VW_CHAN(ESPI_VWIRE_SIGNAL_ERR_NON_FATAL, 0x05, BIT(2), BIT(6)),
	VW_CHAN(ESPI_VWIRE_SIGNAL_ERR_FATAL,     0x05, BIT(1), BIT(5)),
	VW_CHAN(ESPI_VWIRE_SIGNAL_SLV_BOOT_DONE, 0x05, BIT(0), BIT(4)),
	VW_CHAN(ESPI_VWIRE_SIGNAL_HOST_RST_ACK,  0x06, BIT(3), BIT(7)),
	VW_CHAN(ESPI_VWIRE_SIGNAL_RST_CPU_INIT,  0x06, BIT(2), BIT(6)),
	VW_CHAN(ESPI_VWIRE_SIGNAL_SMI,           0x06, BIT(1), BIT(5)),
	VW_CHAN(ESPI_VWIRE_SIGNAL_SCI,           0x06, BIT(0), BIT(4)),
	VW_CHAN(ESPI_VWIRE_SIGNAL_DNX_ACK,       0x40, BIT(1), BIT(5)),
	VW_CHAN(ESPI_VWIRE_SIGNAL_SUS_ACK,       0x40, BIT(0), BIT(4)),
};

static int espi_it8xxx2_configure(const struct device *dev,
					struct espi_cfg *cfg)
{
	const struct espi_it8xxx2_config *const config = dev->config;
	struct espi_slave_regs *const slave_reg =
		(struct espi_slave_regs *)config->base_espi_slave;
	uint8_t capcfg1 = 0;

	/* Set frequency */
	switch (cfg->max_freq) {
	case 20:
		capcfg1 = IT8XXX2_ESPI_CAPCFG1_MAX_FREQ_20;
		break;
	case 25:
		capcfg1 = IT8XXX2_ESPI_CAPCFG1_MAX_FREQ_25;
		break;
	case 33:
		capcfg1 = IT8XXX2_ESPI_CAPCFG1_MAX_FREQ_33;
		break;
	case 50:
		capcfg1 = IT8XXX2_ESPI_CAPCFG1_MAX_FREQ_50;
		break;
	case 66:
		capcfg1 = IT8XXX2_ESPI_CAPCFG1_MAX_FREQ_66;
		break;
	default:
		return -EINVAL;
	}
	slave_reg->GCAPCFG1 =
		(slave_reg->GCAPCFG1 & ~IT8XXX2_ESPI_MAX_FREQ_MASK) | capcfg1;

	/*
	 * Configure eSPI I/O mode. (Register read only)
	 * Supported I/O mode : single, dual and quad.
	 */

	/* Configure eSPI supported channels. (Register read only)
	 * Supported channels: peripheral, virtual wire, OOB, and flash access.
	 */

	return 0;
}

static bool espi_it8xxx2_channel_ready(const struct device *dev,
					enum espi_channel ch)
{
	const struct espi_it8xxx2_config *const config = dev->config;
	struct espi_slave_regs *const slave_reg =
		(struct espi_slave_regs *)config->base_espi_slave;
	bool sts = false;

	switch (ch) {
	case ESPI_CHANNEL_PERIPHERAL:
		sts = slave_reg->CH_PC_CAPCFG3 & IT8XXX2_ESPI_PC_READY_MASK;
		break;
	case ESPI_CHANNEL_VWIRE:
		sts = slave_reg->CH_VW_CAPCFG3 & IT8XXX2_ESPI_VW_READY_MASK;
		break;
	case ESPI_CHANNEL_OOB:
		sts = slave_reg->CH_OOB_CAPCFG3 & IT8XXX2_ESPI_OOB_READY_MASK;
		break;
	case ESPI_CHANNEL_FLASH:
		sts = slave_reg->CH_FLASH_CAPCFG3 & IT8XXX2_ESPI_FC_READY_MASK;
		break;
	default:
		break;
	}

	return sts;
}

static int espi_it8xxx2_send_vwire(const struct device *dev,
			enum espi_vwire_signal signal, uint8_t level)
{
	const struct espi_it8xxx2_config *const config = dev->config;
	struct espi_vw_regs *const vw_reg =
		(struct espi_vw_regs *)config->base_espi_vw;
	uint8_t vw_index = vw_channel_list[signal].vw_index;
	uint8_t level_mask = vw_channel_list[signal].level_mask;
	uint8_t valid_mask = vw_channel_list[signal].valid_mask;

	if (signal > ARRAY_SIZE(vw_channel_list)) {
		return -EIO;
	}

	if (level) {
		vw_reg->VW_INDEX[vw_index] |= level_mask;
	} else {
		vw_reg->VW_INDEX[vw_index] &= ~level_mask;
	}

	vw_reg->VW_INDEX[vw_index] |= valid_mask;

	return 0;
}

static int espi_it8xxx2_receive_vwire(const struct device *dev,
				  enum espi_vwire_signal signal, uint8_t *level)
{
	const struct espi_it8xxx2_config *const config = dev->config;
	struct espi_vw_regs *const vw_reg =
		(struct espi_vw_regs *)config->base_espi_vw;
	uint8_t vw_index = vw_channel_list[signal].vw_index;
	uint8_t level_mask = vw_channel_list[signal].level_mask;
	uint8_t valid_mask = vw_channel_list[signal].valid_mask;

	if (signal > ARRAY_SIZE(vw_channel_list)) {
		return -EIO;
	}

	if (vw_reg->VW_INDEX[vw_index] & valid_mask) {
		*level = !!(vw_reg->VW_INDEX[vw_index] & level_mask);
	} else {
		/* Not valid */
		*level = 0;
	}

	return 0;
}

static int espi_it8xxx2_manage_callback(const struct device *dev,
				    struct espi_callback *callback, bool set)
{
	struct espi_it8xxx2_data *const data = dev->data;

	return espi_manage_callback(&data->callbacks, callback, set);
}

static int espi_it8xxx2_read_lpc_request(const struct device *dev,
				     enum lpc_peripheral_opcode op,
				     uint32_t *data)
{
	const struct espi_it8xxx2_config *const config = dev->config;

	if (op >= E8042_START_OPCODE && op <= E8042_MAX_OPCODE) {
		struct kbc_regs *const kbc_reg =
			(struct kbc_regs *)config->base_kbc;

		switch (op) {
		case E8042_OBF_HAS_CHAR:
			/* EC has written data back to host. OBF is
			 * automatically cleared after host reads
			 * the data
			 */
			*data = !!(kbc_reg->KBHISR & KBC_KBHISR_OBF);
			break;
		case E8042_IBF_HAS_CHAR:
			*data = !!(kbc_reg->KBHISR & KBC_KBHISR_IBF);
			break;
		case E8042_READ_KB_STS:
			*data = kbc_reg->KBHISR;
			break;
		default:
			return -EINVAL;
		}
	} else if (op >= EACPI_START_OPCODE && op <= EACPI_MAX_OPCODE) {
		struct pmc_regs *const pmc_reg =
			(struct pmc_regs *)config->base_pmc;

		switch (op) {
		case EACPI_OBF_HAS_CHAR:
			/* EC has written data back to host. OBF is
			 * automatically cleared after host reads
			 * the data
			 */
			*data = !!(pmc_reg->PM1STS & PMC_PM1STS_OBF);
			break;
		case EACPI_IBF_HAS_CHAR:
			*data = !!(pmc_reg->PM1STS & PMC_PM1STS_IBF);
			break;
		case EACPI_READ_STS:
			*data = pmc_reg->PM1STS;
			break;
#ifdef CONFIG_ESPI_PERIPHERAL_ACPI_SHM_REGION
		case EACPI_GET_SHARED_MEMORY:
			*data = (uint32_t)&h2ram_pool[
			CONFIG_ESPI_PERIPHERAL_ACPI_SHM_REGION_PORT_NUM];
			break;
#endif /* CONFIG_ESPI_PERIPHERAL_ACPI_SHM_REGION */
		default:
			return -EINVAL;
		}
	}
#ifdef CONFIG_ESPI_PERIPHERAL_CUSTOM_OPCODE
	else if (op >= ECUSTOM_START_OPCODE && op <= ECUSTOM_MAX_OPCODE) {

		switch (op) {
		case ECUSTOM_HOST_CMD_GET_PARAM_MEMORY:
			*data = (uint32_t)&h2ram_pool[
				CONFIG_ESPI_PERIPHERAL_HOST_CMD_PARAM_PORT_NUM];
			break;
		case ECUSTOM_HOST_CMD_GET_PARAM_MEMORY_SIZE:
			*data = CONFIG_ESPI_IT8XXX2_HC_H2RAM_SIZE;
			break;
		default:
			return -EINVAL;
		}
	}
#endif /* CONFIG_ESPI_PERIPHERAL_CUSTOM_OPCODE */
	else {
		return -ENOTSUP;
	}

	return 0;
}

static int espi_it8xxx2_write_lpc_request(const struct device *dev,
				      enum lpc_peripheral_opcode op,
				      uint32_t *data)
{
	const struct espi_it8xxx2_config *const config = dev->config;

	if (op >= E8042_START_OPCODE && op <= E8042_MAX_OPCODE) {
		struct kbc_regs *const kbc_reg =
			(struct kbc_regs *)config->base_kbc;

		switch (op) {
		case E8042_WRITE_KB_CHAR:
			kbc_reg->KBHIKDOR = (*data & 0xff);
			/*
			 * Enable OBE interrupt after putting data in
			 * data register.
			 */
			kbc_reg->KBHICR |= KBC_KBHICR_OBECIE;
			break;
		case E8042_WRITE_MB_CHAR:
			kbc_reg->KBHIMDOR = (*data & 0xff);
			/*
			 * Enable OBE interrupt after putting data in
			 * data register.
			 */
			kbc_reg->KBHICR |= KBC_KBHICR_OBECIE;
			break;
		case E8042_RESUME_IRQ:
			/* Enable KBC IBF interrupt */
			kbc_reg->KBHICR |= KBC_KBHICR_IBFCIE;
			break;
		case E8042_PAUSE_IRQ:
			/* Disable KBC IBF interrupt */
			kbc_reg->KBHICR &= ~KBC_KBHICR_IBFCIE;
			break;
		case E8042_CLEAR_OBF:
			/*
			 * After enabling IBF/OBF clear mode, we have to make
			 * sure that IBF interrupt is not triggered before
			 * disabling the clear mode. Or the interrupt will keep
			 * triggering until the watchdog is reset.
			 */
			unsigned int key = irq_lock();
			/*
			 * When IBFOBFCME is enabled, write 1 to COBF bit to
			 * clear KBC OBF.
			 */
			kbc_reg->KBHICR |= KBC_KBHICR_IBFOBFCME;
			kbc_reg->KBHICR |= KBC_KBHICR_COBF;
			kbc_reg->KBHICR &= ~KBC_KBHICR_COBF;
			/* Disable clear mode */
			kbc_reg->KBHICR &= ~KBC_KBHICR_IBFOBFCME;
			irq_unlock(key);
			break;
		case E8042_SET_FLAG:
			kbc_reg->KBHISR |= (*data & 0xff);
			break;
		case E8042_CLEAR_FLAG:
			kbc_reg->KBHISR &= ~(*data & 0xff);
			break;
		default:
			return -EINVAL;
		}
	} else if (op >= EACPI_START_OPCODE && op <= EACPI_MAX_OPCODE) {
		struct pmc_regs *const pmc_reg =
			(struct pmc_regs *)config->base_pmc;

		switch (op) {
		case EACPI_WRITE_CHAR:
			pmc_reg->PM1DO = (*data & 0xff);
			break;
		case EACPI_WRITE_STS:
			pmc_reg->PM1STS = (*data & 0xff);
			break;
		default:
			return -EINVAL;
		}
	}
#ifdef CONFIG_ESPI_PERIPHERAL_CUSTOM_OPCODE
	else if (op >= ECUSTOM_START_OPCODE && op <= ECUSTOM_MAX_OPCODE) {
		struct pmc_regs *const pmc_reg =
			(struct pmc_regs *)config->base_pmc;

		switch (op) {
		/* Enable/Disable PMC1 (port 62h/66h) interrupt */
		case ECUSTOM_HOST_SUBS_INTERRUPT_EN:
			if (*data) {
				pmc_reg->PM1CTL |= PMC_PM1CTL_IBFIE;
			} else {
				pmc_reg->PM1CTL &= ~PMC_PM1CTL_IBFIE;
			}
			break;
		case ECUSTOM_HOST_CMD_SEND_RESULT:
			/* Write result to data output port (set OBF status) */
			pmc_reg->PM2DO = (*data & 0xff);
			/* Clear processing flag */
			pmc_reg->PM2STS &= ~PMC_PM2STS_GPF;
			break;
		default:
			return -EINVAL;
		}
	}
#endif /* CONFIG_ESPI_PERIPHERAL_CUSTOM_OPCODE */
	else {
		return -ENOTSUP;
	}

	return 0;
}

#ifdef CONFIG_ESPI_OOB_CHANNEL
/* eSPI cycle type field */
#define ESPI_OOB_CYCLE_TYPE          0x21
#define ESPI_OOB_TAG                 0x00
#define ESPI_OOB_TIMEOUT_MS          200

/* eSPI tag + len[11:8] field */
#define ESPI_TAG_LEN_FIELD(tag, len) \
		   ((((tag) & 0xF) << 4) | (((len) >> 8) & 0xF))

struct espi_oob_msg_packet {
	uint8_t data_byte[0];
};

static int espi_it8xxx2_send_oob(const struct device *dev,
				struct espi_oob_packet *pckt)
{
	const struct espi_it8xxx2_config *const config = dev->config;
	struct espi_slave_regs *const slave_reg =
		(struct espi_slave_regs *)config->base_espi_slave;
	struct espi_queue1_regs *const queue1_reg =
		(struct espi_queue1_regs *)config->base_espi_queue1;
	struct espi_oob_msg_packet *oob_pckt =
		(struct espi_oob_msg_packet *)pckt->buf;

	if (!(slave_reg->CH_OOB_CAPCFG3 & IT8XXX2_ESPI_OOB_READY_MASK)) {
		LOG_ERR("%s: OOB channel isn't ready", __func__);
		return -EIO;
	}

	if (slave_reg->ESUCTRL0 & IT8XXX2_ESPI_UPSTREAM_BUSY) {
		LOG_ERR("%s: OOB upstream busy", __func__);
		return -EIO;
	}

	if (pckt->len > ESPI_IT8XXX2_OOB_MAX_PAYLOAD_SIZE) {
		LOG_ERR("%s: Out of OOB queue space", __func__);
		return -EINVAL;
	}

	/* Set cycle type */
	slave_reg->ESUCTRL1 = IT8XXX2_ESPI_CYCLE_TYPE_OOB;
	/* Set tag and length[11:8] */
	slave_reg->ESUCTRL2 = ESPI_TAG_LEN_FIELD(0, pckt->len);
	/* Set length [7:0] */
	slave_reg->ESUCTRL3 = pckt->len & 0xff;

	/* Set data byte */
	for (int i = 0; i < pckt->len; i++) {
		queue1_reg->UPSTREAM_DATA[i] = oob_pckt->data_byte[i];
	}

	/* Set upstream enable */
	slave_reg->ESUCTRL0 |= IT8XXX2_ESPI_UPSTREAM_ENABLE;
	/* Set upstream go */
	slave_reg->ESUCTRL0 |= IT8XXX2_ESPI_UPSTREAM_GO;

	return 0;
}

static int espi_it8xxx2_receive_oob(const struct device *dev,
				struct espi_oob_packet *pckt)
{
	const struct espi_it8xxx2_config *const config = dev->config;
	struct espi_slave_regs *const slave_reg =
		(struct espi_slave_regs *)config->base_espi_slave;
	struct espi_queue0_regs *const queue0_reg =
		(struct espi_queue0_regs *)config->base_espi_queue0;
	struct espi_oob_msg_packet *oob_pckt =
		(struct espi_oob_msg_packet *)pckt->buf;
	uint8_t oob_len;

	if (!(slave_reg->CH_OOB_CAPCFG3 & IT8XXX2_ESPI_OOB_READY_MASK)) {
		LOG_ERR("%s: OOB channel isn't ready", __func__);
		return -EIO;
	}

#ifndef CONFIG_ESPI_OOB_CHANNEL_RX_ASYNC
	struct espi_it8xxx2_data *const data = dev->data;
	int ret;

	/* Wait until receive OOB message or timeout */
	ret = k_sem_take(&data->oob_upstream_go, K_MSEC(ESPI_OOB_TIMEOUT_MS));
	if (ret == -EAGAIN) {
		LOG_ERR("%s: Timeout", __func__);
		return -ETIMEDOUT;
	}
#endif

	/* Get length */
	oob_len = (slave_reg->ESOCTRL4 & IT8XXX2_ESPI_PUT_OOB_LEN_MASK);
	/*
	 * Buffer passed to driver isn't enough.
	 * The first three bytes of buffer are cycle type, tag, and length.
	 */
	if (oob_len > pckt->len) {
		LOG_ERR("%s: Out of rx buf %d vs %d", __func__,
			oob_len, pckt->len);
		return -EINVAL;
	}

	pckt->len = oob_len;
	/* Get data byte */
	for (int i = 0; i < oob_len; i++) {
		oob_pckt->data_byte[i] = queue0_reg->PUT_OOB_DATA[i];
	}

	return 0;
}

static void espi_it8xxx2_oob_init(const struct device *dev)
{
	const struct espi_it8xxx2_config *const config = dev->config;
	struct espi_slave_regs *const slave_reg =
		(struct espi_slave_regs *)config->base_espi_slave;

#ifndef CONFIG_ESPI_OOB_CHANNEL_RX_ASYNC
	struct espi_it8xxx2_data *const data = dev->data;

	k_sem_init(&data->oob_upstream_go, 0, 1);
#endif

	/* Upstream interrupt enable */
	slave_reg->ESUCTRL0 |= IT8XXX2_ESPI_UPSTREAM_INTERRUPT_ENABLE;

	/* PUT_OOB interrupt enable */
	slave_reg->ESOCTRL1 |= IT8XXX2_ESPI_PUT_OOB_INTERRUPT_ENABLE;
}
#endif

#ifdef CONFIG_ESPI_FLASH_CHANNEL
#define ESPI_FLASH_TAG                      0x01
#define ESPI_FLASH_READ_TIMEOUT_MS          200
#define ESPI_FLASH_WRITE_TIMEOUT_MS         500
#define ESPI_FLASH_ERASE_TIMEOUT_MS         1000

/* Successful completion without data */
#define ESPI_IT8XXX2_PUT_FLASH_C_SCWOD      0
/* Successful completion with data */
#define ESPI_IT8XXX2_PUT_FLASH_C_SCWD       4

enum espi_flash_cycle_type {
	IT8XXX2_ESPI_CYCLE_TYPE_FLASH_READ = 0x08,
	IT8XXX2_ESPI_CYCLE_TYPE_FLASH_WRITE = 0x09,
	IT8XXX2_ESPI_CYCLE_TYPE_FLASH_ERASE = 0x0A,
};

static int espi_it8xxx2_flash_trans(const struct device *dev,
				struct espi_flash_packet *pckt,
				enum espi_flash_cycle_type tran)
{
	const struct espi_it8xxx2_config *const config = dev->config;
	struct espi_slave_regs *const slave_reg =
		(struct espi_slave_regs *)config->base_espi_slave;
	struct espi_queue1_regs *const queue1_reg =
		(struct espi_queue1_regs *)config->base_espi_queue1;

	if (!(slave_reg->CH_FLASH_CAPCFG3 & IT8XXX2_ESPI_FC_READY_MASK)) {
		LOG_ERR("%s: Flash channel isn't ready (tran:%d)",
			__func__, tran);
		return -EIO;
	}

	if (slave_reg->ESUCTRL0 & IT8XXX2_ESPI_UPSTREAM_BUSY) {
		LOG_ERR("%s: Upstream busy (tran:%d)", __func__, tran);
		return -EIO;
	}

	if (pckt->len > IT8XXX2_ESPI_FLASH_MAX_PAYLOAD_SIZE) {
		LOG_ERR("%s: Invalid size request (tran:%d)", __func__, tran);
		return -EINVAL;
	}

	/* Set cycle type */
	slave_reg->ESUCTRL1 = tran;
	/* Set tag and length[11:8] */
	slave_reg->ESUCTRL2 = (ESPI_FLASH_TAG << 4);
	/*
	 * Set length [7:0]
	 * Note: for erasing, the least significant 3 bit of the length field
	 * specifies the size of the block to be erased:
	 * 001b: 4 Kbytes
	 * 010b: 64Kbytes
	 * 100b: 128 Kbytes
	 * 101b: 256 Kbytes
	 */
	slave_reg->ESUCTRL3 = pckt->len;
	/* Set flash address */
	queue1_reg->UPSTREAM_DATA[0] = (pckt->flash_addr >> 24) & 0xff;
	queue1_reg->UPSTREAM_DATA[1] = (pckt->flash_addr >> 16) & 0xff;
	queue1_reg->UPSTREAM_DATA[2] = (pckt->flash_addr >> 8) & 0xff;
	queue1_reg->UPSTREAM_DATA[3] = pckt->flash_addr & 0xff;

	return 0;
}

static int espi_it8xxx2_flash_read(const struct device *dev,
					struct espi_flash_packet *pckt)
{
	const struct espi_it8xxx2_config *const config = dev->config;
	struct espi_it8xxx2_data *const data = dev->data;
	struct espi_slave_regs *const slave_reg =
		(struct espi_slave_regs *)config->base_espi_slave;
	int ret;

	ret = espi_it8xxx2_flash_trans(dev, pckt,
					IT8XXX2_ESPI_CYCLE_TYPE_FLASH_READ);
	if (ret) {
		return ret;
	}

	/* Set upstream enable */
	slave_reg->ESUCTRL0 |= IT8XXX2_ESPI_UPSTREAM_ENABLE;
	/* Set upstream go */
	slave_reg->ESUCTRL0 |= IT8XXX2_ESPI_UPSTREAM_GO;

	/* Wait until upstream done or timeout */
	ret = k_sem_take(&data->flash_upstream_go,
			K_MSEC(ESPI_FLASH_READ_TIMEOUT_MS));
	if (ret == -EAGAIN) {
		LOG_ERR("%s: Timeout", __func__);
		return -ETIMEDOUT;
	}

	if (data->put_flash_cycle_type != ESPI_IT8XXX2_PUT_FLASH_C_SCWD) {
		LOG_ERR("%s: Unsuccessful completion", __func__);
		return -EIO;
	}

	memcpy(pckt->buf, data->flash_buf, pckt->len);

	LOG_INF("%s: read (%d) bytes from flash over espi", __func__,
		data->put_flash_len);

	return 0;
}

static int espi_it8xxx2_flash_write(const struct device *dev,
					struct espi_flash_packet *pckt)
{
	const struct espi_it8xxx2_config *const config = dev->config;
	struct espi_it8xxx2_data *const data = dev->data;
	struct espi_slave_regs *const slave_reg =
		(struct espi_slave_regs *)config->base_espi_slave;
	struct espi_queue1_regs *const queue1_reg =
		(struct espi_queue1_regs *)config->base_espi_queue1;
	int ret;

	ret = espi_it8xxx2_flash_trans(dev, pckt,
					IT8XXX2_ESPI_CYCLE_TYPE_FLASH_WRITE);
	if (ret) {
		return ret;
	}

	/* Set data byte */
	for (int i = 0; i < pckt->len; i++) {
		queue1_reg->UPSTREAM_DATA[4 + i] = pckt->buf[i];
	}

	/* Set upstream enable */
	slave_reg->ESUCTRL0 |= IT8XXX2_ESPI_UPSTREAM_ENABLE;
	/* Set upstream go */
	slave_reg->ESUCTRL0 |= IT8XXX2_ESPI_UPSTREAM_GO;

	/* Wait until upstream done or timeout */
	ret = k_sem_take(&data->flash_upstream_go,
			K_MSEC(ESPI_FLASH_WRITE_TIMEOUT_MS));
	if (ret == -EAGAIN) {
		LOG_ERR("%s: Timeout", __func__);
		return -ETIMEDOUT;
	}

	if (data->put_flash_cycle_type != ESPI_IT8XXX2_PUT_FLASH_C_SCWOD) {
		LOG_ERR("%s: Unsuccessful completion", __func__);
		return -EIO;
	}

	return 0;
}

static int espi_it8xxx2_flash_erase(const struct device *dev,
					struct espi_flash_packet *pckt)
{
	const struct espi_it8xxx2_config *const config = dev->config;
	struct espi_it8xxx2_data *const data = dev->data;
	struct espi_slave_regs *const slave_reg =
		(struct espi_slave_regs *)config->base_espi_slave;
	int ret;

	ret = espi_it8xxx2_flash_trans(dev, pckt,
					IT8XXX2_ESPI_CYCLE_TYPE_FLASH_ERASE);
	if (ret) {
		return ret;
	}

	/* Set upstream enable */
	slave_reg->ESUCTRL0 |= IT8XXX2_ESPI_UPSTREAM_ENABLE;
	/* Set upstream go */
	slave_reg->ESUCTRL0 |= IT8XXX2_ESPI_UPSTREAM_GO;

	/* Wait until upstream done or timeout */
	ret = k_sem_take(&data->flash_upstream_go,
			K_MSEC(ESPI_FLASH_ERASE_TIMEOUT_MS));
	if (ret == -EAGAIN) {
		LOG_ERR("%s: Timeout", __func__);
		return -ETIMEDOUT;
	}

	if (data->put_flash_cycle_type != ESPI_IT8XXX2_PUT_FLASH_C_SCWOD) {
		LOG_ERR("%s: Unsuccessful completion", __func__);
		return -EIO;
	}

	return 0;
}

static void espi_it8xxx2_flash_upstream_done_isr(const struct device *dev)
{
	const struct espi_it8xxx2_config *const config = dev->config;
	struct espi_it8xxx2_data *const data = dev->data;
	struct espi_slave_regs *const slave_reg =
		(struct espi_slave_regs *)config->base_espi_slave;
	struct espi_queue1_regs *const queue1_reg =
		(struct espi_queue1_regs *)config->base_espi_queue1;

	data->put_flash_cycle_type = slave_reg->ESUCTRL6;
	data->put_flash_tag = slave_reg->ESUCTRL7 &
				IT8XXX2_ESPI_PUT_FLASH_TAG_MASK;
	data->put_flash_len = slave_reg->ESUCTRL8 &
				IT8XXX2_ESPI_PUT_FLASH_LEN_MASK;

	if (slave_reg->ESUCTRL1 == IT8XXX2_ESPI_CYCLE_TYPE_FLASH_READ) {
		if (data->put_flash_len > IT8XXX2_ESPI_FLASH_MAX_PAYLOAD_SIZE) {
			LOG_ERR("%s: Invalid size (%d)", __func__,
							data->put_flash_len);
		} else {
			for (int i = 0; i < data->put_flash_len; i++) {
				data->flash_buf[i] =
					queue1_reg->UPSTREAM_DATA[i];
			}
		}
	}

	k_sem_give(&data->flash_upstream_go);
}

static void espi_it8xxx2_flash_init(const struct device *dev)
{
	const struct espi_it8xxx2_config *const config = dev->config;
	struct espi_it8xxx2_data *const data = dev->data;
	struct espi_slave_regs *const slave_reg =
		(struct espi_slave_regs *)config->base_espi_slave;

	k_sem_init(&data->flash_upstream_go, 0, 1);

	/* Upstream interrupt enable */
	slave_reg->ESUCTRL0 |= IT8XXX2_ESPI_UPSTREAM_INTERRUPT_ENABLE;
}
#endif /* CONFIG_ESPI_FLASH_CHANNEL */

/* eSPI driver registration */
static int espi_it8xxx2_init(const struct device *dev);

static const struct espi_driver_api espi_it8xxx2_driver_api = {
	.config = espi_it8xxx2_configure,
	.get_channel_status = espi_it8xxx2_channel_ready,
	.send_vwire = espi_it8xxx2_send_vwire,
	.receive_vwire = espi_it8xxx2_receive_vwire,
	.manage_callback = espi_it8xxx2_manage_callback,
	.read_lpc_request = espi_it8xxx2_read_lpc_request,
	.write_lpc_request = espi_it8xxx2_write_lpc_request,
#ifdef CONFIG_ESPI_OOB_CHANNEL
	.send_oob = espi_it8xxx2_send_oob,
	.receive_oob = espi_it8xxx2_receive_oob,
#endif
#ifdef CONFIG_ESPI_FLASH_CHANNEL
	.flash_read = espi_it8xxx2_flash_read,
	.flash_write = espi_it8xxx2_flash_write,
	.flash_erase = espi_it8xxx2_flash_erase,
#endif
};

static void espi_it8xxx2_vw_notify_system_state(const struct device *dev,
				enum espi_vwire_signal signal)
{
	struct espi_it8xxx2_data *const data = dev->data;
	struct espi_event evt = {ESPI_BUS_EVENT_VWIRE_RECEIVED, 0, 0};
	uint8_t level = 0;

	espi_it8xxx2_receive_vwire(dev, signal, &level);

	evt.evt_details = signal;
	evt.evt_data = level;
	espi_send_callbacks(&data->callbacks, dev, evt);
}

static void espi_vw_signal_no_isr(const struct device *dev)
{
	ARG_UNUSED(dev);
}

static const struct espi_vw_signal_t vwidx2_signals[] = {
	{ESPI_VWIRE_SIGNAL_SLP_S3, NULL},
	{ESPI_VWIRE_SIGNAL_SLP_S4, NULL},
	{ESPI_VWIRE_SIGNAL_SLP_S5, NULL},
};

static void espi_it8xxx2_vwidx2_isr(const struct device *dev,
					uint8_t updated_flag)
{
	for (int i = 0; i < ARRAY_SIZE(vwidx2_signals); i++) {
		enum espi_vwire_signal vw_signal = vwidx2_signals[i].signal;

		if (updated_flag & vw_channel_list[vw_signal].level_mask) {
			espi_it8xxx2_vw_notify_system_state(dev, vw_signal);
		}
	}
}

static void espi_vw_oob_rst_warn_isr(const struct device *dev)
{
	uint8_t level = 0;

	espi_it8xxx2_receive_vwire(dev, ESPI_VWIRE_SIGNAL_OOB_RST_WARN, &level);
	espi_it8xxx2_send_vwire(dev, ESPI_VWIRE_SIGNAL_OOB_RST_ACK, level);
}

static void espi_vw_pltrst_isr(const struct device *dev)
{
	uint8_t pltrst = 0;

	espi_it8xxx2_receive_vwire(dev, ESPI_VWIRE_SIGNAL_PLTRST, &pltrst);

	if (pltrst) {
		espi_it8xxx2_send_vwire(dev, ESPI_VWIRE_SIGNAL_SMI, 1);
		espi_it8xxx2_send_vwire(dev, ESPI_VWIRE_SIGNAL_SCI, 1);
		espi_it8xxx2_send_vwire(dev, ESPI_VWIRE_SIGNAL_HOST_RST_ACK, 1);
		espi_it8xxx2_send_vwire(dev, ESPI_VWIRE_SIGNAL_RST_CPU_INIT, 1);
	}

	LOG_INF("VW PLTRST_L %sasserted", pltrst ? "de" : "");
}

static const struct espi_vw_signal_t vwidx3_signals[] = {
	{ESPI_VWIRE_SIGNAL_OOB_RST_WARN, espi_vw_oob_rst_warn_isr},
	{ESPI_VWIRE_SIGNAL_PLTRST,       espi_vw_pltrst_isr},
};

static void espi_it8xxx2_vwidx3_isr(const struct device *dev,
					uint8_t updated_flag)
{
	for (int i = 0; i < ARRAY_SIZE(vwidx3_signals); i++) {
		enum espi_vwire_signal vw_signal = vwidx3_signals[i].signal;

		if (updated_flag & vw_channel_list[vw_signal].level_mask) {
			vwidx3_signals[i].vw_signal_isr(dev);
			espi_it8xxx2_vw_notify_system_state(dev, vw_signal);
		}
	}
}

static void espi_vw_host_rst_warn_isr(const struct device *dev)
{
	uint8_t level = 0;

	espi_it8xxx2_receive_vwire(dev,
		ESPI_VWIRE_SIGNAL_HOST_RST_WARN, &level);
	espi_it8xxx2_send_vwire(dev, ESPI_VWIRE_SIGNAL_HOST_RST_ACK, level);
}

static const struct espi_vw_signal_t vwidx7_signals[] = {
	{ESPI_VWIRE_SIGNAL_HOST_RST_WARN, espi_vw_host_rst_warn_isr},
};

static void espi_it8xxx2_vwidx7_isr(const struct device *dev,
					uint8_t updated_flag)
{
	for (int i = 0; i < ARRAY_SIZE(vwidx7_signals); i++) {
		enum espi_vwire_signal vw_signal = vwidx7_signals[i].signal;

		if (updated_flag & vw_channel_list[vw_signal].level_mask) {
			vwidx7_signals[i].vw_signal_isr(dev);
			espi_it8xxx2_vw_notify_system_state(dev, vw_signal);
		}
	}
}

static void espi_vw_sus_warn_isr(const struct device *dev)
{
	uint8_t level = 0;

	espi_it8xxx2_receive_vwire(dev, ESPI_VWIRE_SIGNAL_SUS_WARN, &level);
	espi_it8xxx2_send_vwire(dev, ESPI_VWIRE_SIGNAL_SUS_ACK, level);
}

static const struct espi_vw_signal_t vwidx41_signals[] = {
	{ESPI_VWIRE_SIGNAL_SUS_WARN,      espi_vw_sus_warn_isr},
	{ESPI_VWIRE_SIGNAL_SUS_PWRDN_ACK, espi_vw_signal_no_isr},
	{ESPI_VWIRE_SIGNAL_SLP_A,         espi_vw_signal_no_isr},
};

static void espi_it8xxx2_vwidx41_isr(const struct device *dev,
					uint8_t updated_flag)
{
	for (int i = 0; i < ARRAY_SIZE(vwidx41_signals); i++) {
		enum espi_vwire_signal vw_signal = vwidx41_signals[i].signal;

		if (updated_flag & vw_channel_list[vw_signal].level_mask) {
			vwidx41_signals[i].vw_signal_isr(dev);
			espi_it8xxx2_vw_notify_system_state(dev, vw_signal);
		}
	}
}

static const struct espi_vw_signal_t vwidx42_signals[] = {
	{ESPI_VWIRE_SIGNAL_SLP_LAN, NULL},
	{ESPI_VWIRE_SIGNAL_SLP_WLAN, NULL},
};

static void espi_it8xxx2_vwidx42_isr(const struct device *dev,
					uint8_t updated_flag)
{
	for (int i = 0; i < ARRAY_SIZE(vwidx42_signals); i++) {
		enum espi_vwire_signal vw_signal = vwidx42_signals[i].signal;

		if (updated_flag & vw_channel_list[vw_signal].level_mask) {
			espi_it8xxx2_vw_notify_system_state(dev, vw_signal);
		}
	}
}

static void espi_it8xxx2_vwidx43_isr(const struct device *dev,
					uint8_t updated_flag)
{
	ARG_UNUSED(dev);
	/*
	 * We haven't send callback to system because there is no index 43
	 * virtual wire signal is listed in enum espi_vwire_signal.
	 */
	LOG_INF("vw isr %s is ignored!", __func__);
}

static void espi_it8xxx2_vwidx44_isr(const struct device *dev,
					uint8_t updated_flag)
{
	ARG_UNUSED(dev);
	/*
	 * We haven't send callback to system because there is no index 44
	 * virtual wire signal is listed in enum espi_vwire_signal.
	 */
	LOG_INF("vw isr %s is ignored!", __func__);
}

static const struct espi_vw_signal_t vwidx47_signals[] = {
	{ESPI_VWIRE_SIGNAL_HOST_C10, NULL},
};
static void espi_it8xxx2_vwidx47_isr(const struct device *dev,
					uint8_t updated_flag)
{
	for (int i = 0; i < ARRAY_SIZE(vwidx47_signals); i++) {
		enum espi_vwire_signal vw_signal = vwidx47_signals[i].signal;

		if (updated_flag & vw_channel_list[vw_signal].level_mask) {
			espi_it8xxx2_vw_notify_system_state(dev, vw_signal);
		}
	}
}

/*
 * The ISR of espi VW interrupt in array needs to match bit order in
 * ESPI VW VWCTRL1 register.
 */
static const struct vwidx_isr_t vwidx_isr_list[] = {
	[0] = {espi_it8xxx2_vwidx2_isr,  0x02},
	[1] = {espi_it8xxx2_vwidx3_isr,  0x03},
	[2] = {espi_it8xxx2_vwidx7_isr,  0x07},
	[3] = {espi_it8xxx2_vwidx41_isr, 0x41},
	[4] = {espi_it8xxx2_vwidx42_isr, 0x42},
	[5] = {espi_it8xxx2_vwidx43_isr, 0x43},
	[6] = {espi_it8xxx2_vwidx44_isr, 0x44},
	[7] = {espi_it8xxx2_vwidx47_isr, 0x47},
};

/*
 * This is used to record the previous VW valid/level field state to discover
 * changes. Then do following sequence only when state is changed.
 */
static uint8_t vwidx_cached_flag[ARRAY_SIZE(vwidx_isr_list)];

static void espi_it8xxx2_reset_vwidx_cache(const struct device *dev)
{
	const struct espi_it8xxx2_config *const config = dev->config;
	struct espi_vw_regs *const vw_reg =
		(struct espi_vw_regs *)config->base_espi_vw;

	/* reset vwidx_cached_flag */
	for (int i = 0; i < ARRAY_SIZE(vwidx_isr_list); i++) {
		vwidx_cached_flag[i] =
			vw_reg->VW_INDEX[vwidx_isr_list[i].vw_index];
	}
}

static void espi_it8xxx2_vw_isr(const struct device *dev)
{
	const struct espi_it8xxx2_config *const config = dev->config;
	struct espi_vw_regs *const vw_reg =
		(struct espi_vw_regs *)config->base_espi_vw;
	uint8_t vwidx_updated = vw_reg->VWCTRL1;

	/* write-1 to clear */
	vw_reg->VWCTRL1 = vwidx_updated;

	for (int i = 0; i < ARRAY_SIZE(vwidx_isr_list); i++) {
		if (vwidx_updated & BIT(i)) {
			uint8_t vw_flag;

			vw_flag = vw_reg->VW_INDEX[vwidx_isr_list[i].vw_index];
			vwidx_isr_list[i].vwidx_isr(dev,
					vwidx_cached_flag[i] ^ vw_flag);
			vwidx_cached_flag[i] = vw_flag;
		}
	}
}

static void espi_it8xxx2_ch_notify_system_state(const struct device *dev,
						enum espi_channel ch, bool en)
{
	struct espi_it8xxx2_data *const data = dev->data;
	struct espi_event evt = {
		.evt_type = ESPI_BUS_EVENT_CHANNEL_READY,
		.evt_details = ch,
		.evt_data = en,
	};

	espi_send_callbacks(&data->callbacks, dev, evt);
}

/*
 * Peripheral channel enable asserted flag.
 * A 0-to-1 or 1-to-0 transition on "Peripheral Channel Enable" bit.
 */
static void espi_it8xxx2_peripheral_ch_en_isr(const struct device *dev,
						bool enable)
{
	espi_it8xxx2_ch_notify_system_state(dev,
					ESPI_CHANNEL_PERIPHERAL, enable);
}

/*
 * VW channel enable asserted flag.
 * A 0-to-1 or 1-to-0 transition on "Virtual Wire Channel Enable" bit.
 */
static void espi_it8xxx2_vw_ch_en_isr(const struct device *dev, bool enable)
{
	espi_it8xxx2_ch_notify_system_state(dev, ESPI_CHANNEL_VWIRE, enable);
}

/*
 * OOB message channel enable asserted flag.
 * A 0-to-1 or 1-to-0 transition on "OOB Message Channel Enable" bit.
 */
static void espi_it8xxx2_oob_ch_en_isr(const struct device *dev, bool enable)
{
	espi_it8xxx2_ch_notify_system_state(dev, ESPI_CHANNEL_OOB, enable);
}

/*
 * Flash channel enable asserted flag.
 * A 0-to-1 or 1-to-0 transition on "Flash Access Channel Enable" bit.
 */
static void espi_it8xxx2_flash_ch_en_isr(const struct device *dev, bool enable)
{
	if (enable) {
		espi_it8xxx2_send_vwire(dev, ESPI_VWIRE_SIGNAL_SLV_BOOT_STS, 1);
		espi_it8xxx2_send_vwire(dev,
					ESPI_VWIRE_SIGNAL_SLV_BOOT_DONE, 1);
	}

	espi_it8xxx2_ch_notify_system_state(dev, ESPI_CHANNEL_FLASH, enable);
}

static void espi_it8xxx2_put_pc_status_isr(const struct device *dev)
{
	const struct espi_it8xxx2_config *const config = dev->config;
	struct espi_slave_regs *const slave_reg =
		(struct espi_slave_regs *)config->base_espi_slave;

	/*
	 * TODO: To check cycle type (bit[3-0] at ESPCTRL0) and make
	 * corresponding modification if needed.
	 */
	LOG_INF("isr %s is ignored!", __func__);

	/* write-1-clear to release PC_FREE */
	slave_reg->ESPCTRL0 = IT8XXX2_ESPI_INTERRUPT_PUT_PC;
}

#ifdef CONFIG_ESPI_OOB_CHANNEL
static void espi_it8xxx2_upstream_channel_disable_isr(const struct device *dev)
{
	const struct espi_it8xxx2_config *const config = dev->config;
	struct espi_slave_regs *const slave_reg =
		(struct espi_slave_regs *)config->base_espi_slave;

	LOG_INF("isr %s is ignored!", __func__);

	/* write-1 to clear this bit */
	slave_reg->ESUCTRL0 |= IT8XXX2_ESPI_UPSTREAM_CHANNEL_DISABLE;
}

static void espi_it8xxx2_put_oob_status_isr(const struct device *dev)
{
	const struct espi_it8xxx2_config *const config = dev->config;
	struct espi_it8xxx2_data *const data = dev->data;
	struct espi_slave_regs *const slave_reg =
		(struct espi_slave_regs *)config->base_espi_slave;
#ifdef CONFIG_ESPI_OOB_CHANNEL_RX_ASYNC
	struct espi_event evt = { .evt_type = ESPI_BUS_EVENT_OOB_RECEIVED,
				  .evt_details = 0,
				  .evt_data = 0 };
#endif

	/* Write-1 to clear this bit for the next coming posted transaction. */
	slave_reg->ESOCTRL0 |= IT8XXX2_ESPI_PUT_OOB_STATUS;

#ifndef CONFIG_ESPI_OOB_CHANNEL_RX_ASYNC
	k_sem_give(&data->oob_upstream_go);
#else
	/* Additional detail is length field of PUT_OOB message packet. */
	evt.evt_details = (slave_reg->ESOCTRL4 & IT8XXX2_ESPI_PUT_OOB_LEN_MASK);
	espi_send_callbacks(&data->callbacks, dev, evt);
#endif
}
#endif

#if defined(CONFIG_ESPI_OOB_CHANNEL) || defined(CONFIG_ESPI_FLASH_CHANNEL)
static void espi_it8xxx2_upstream_done_isr(const struct device *dev)
{
	const struct espi_it8xxx2_config *const config = dev->config;
	struct espi_slave_regs *const slave_reg =
		(struct espi_slave_regs *)config->base_espi_slave;

#ifdef CONFIG_ESPI_FLASH_CHANNEL
	/* cycle type is flash read, write, or erase */
	if (slave_reg->ESUCTRL1 != IT8XXX2_ESPI_CYCLE_TYPE_OOB) {
		espi_it8xxx2_flash_upstream_done_isr(dev);
	}
#endif

	/* write-1 to clear this bit */
	slave_reg->ESUCTRL0 |= IT8XXX2_ESPI_UPSTREAM_DONE;
	/* upstream disable */
	slave_reg->ESUCTRL0 &= ~IT8XXX2_ESPI_UPSTREAM_ENABLE;
}
#endif

/*
 * The ISR of espi interrupt event in array need to be matched bit order in
 * IT8XXX2 ESPI ESGCTRL0 register.
 */
static const struct espi_isr_t espi_isr_list[] = {
	[0] = {espi_it8xxx2_peripheral_ch_en_isr, ASSERTED_FLAG},
	[1] = {espi_it8xxx2_vw_ch_en_isr,         ASSERTED_FLAG},
	[2] = {espi_it8xxx2_oob_ch_en_isr,        ASSERTED_FLAG},
	[3] = {espi_it8xxx2_flash_ch_en_isr,      ASSERTED_FLAG},
	[4] = {espi_it8xxx2_peripheral_ch_en_isr, DEASSERTED_FLAG},
	[5] = {espi_it8xxx2_vw_ch_en_isr,         DEASSERTED_FLAG},
	[6] = {espi_it8xxx2_oob_ch_en_isr,        DEASSERTED_FLAG},
	[7] = {espi_it8xxx2_flash_ch_en_isr,      DEASSERTED_FLAG},
};

static void espi_it8xxx2_isr(const struct device *dev)
{
	const struct espi_it8xxx2_config *const config = dev->config;
	struct espi_slave_regs *const slave_reg =
		(struct espi_slave_regs *)config->base_espi_slave;
	/* get espi interrupt events */
	uint8_t espi_event = slave_reg->ESGCTRL0;
#if defined(CONFIG_ESPI_OOB_CHANNEL) || defined(CONFIG_ESPI_FLASH_CHANNEL)
	uint8_t espi_upstream = slave_reg->ESUCTRL0;
#endif

	/* write-1 to clear */
	slave_reg->ESGCTRL0 = espi_event;

	/* process espi interrupt events */
	for (int i = 0; i < ARRAY_SIZE(espi_isr_list); i++) {
		if (espi_event & BIT(i)) {
			espi_isr_list[i].espi_isr(dev,
				espi_isr_list[i].isr_type);
		}
	}

	/*
	 * bit7: the peripheral has received a peripheral posted/completion.
	 * This bit indicates the peripheral has received a packet from eSPI
	 * peripheral channel.
	 */
	if (slave_reg->ESPCTRL0 & IT8XXX2_ESPI_INTERRUPT_PUT_PC) {
		espi_it8xxx2_put_pc_status_isr(dev);
	}

#ifdef CONFIG_ESPI_OOB_CHANNEL
	/*
	 * The corresponding channel of the eSPI upstream transaction is
	 * disabled.
	 */
	if (espi_upstream & IT8XXX2_ESPI_UPSTREAM_CHANNEL_DISABLE) {
		espi_it8xxx2_upstream_channel_disable_isr(dev);
	}

	/* The eSPI slave has received a PUT_OOB message. */
	if (slave_reg->ESOCTRL0 & IT8XXX2_ESPI_PUT_OOB_STATUS) {
		espi_it8xxx2_put_oob_status_isr(dev);
	}
#endif

	/* eSPI oob and flash channels use the same interrupt of upstream. */
#if defined(CONFIG_ESPI_OOB_CHANNEL) || defined(CONFIG_ESPI_FLASH_CHANNEL)
	/* The eSPI upstream transaction is done. */
	if (espi_upstream & IT8XXX2_ESPI_UPSTREAM_DONE) {
		espi_it8xxx2_upstream_done_isr(dev);
	}
#endif
}

void espi_it8xxx2_enable_pad_ctrl(const struct device *dev, bool enable)
{
	const struct espi_it8xxx2_config *const config = dev->config;
	struct espi_slave_regs *const slave_reg =
		(struct espi_slave_regs *)config->base_espi_slave;

	if (enable) {
		/* Enable eSPI pad. */
		slave_reg->ESGCTRL2 &= ~IT8XXX2_ESPI_INPUT_PAD_GATING;
	} else {
		/* Disable eSPI pad. */
		slave_reg->ESGCTRL2 |= IT8XXX2_ESPI_INPUT_PAD_GATING;
	}
}

void espi_it8xxx2_enable_trans_irq(const struct device *dev, bool enable)
{
	const struct espi_it8xxx2_config *const config = dev->config;

	if (enable) {
		irq_enable(IT8XXX2_TRANS_IRQ);
	} else {
		irq_disable(IT8XXX2_TRANS_IRQ);
		/* Clear pending interrupt */
		it8xxx2_wuc_clear_status(config->wuc.wucs, config->wuc.mask);
	}
}

static void espi_it8xxx2_trans_isr(const struct device *dev)
{
	/*
	 * This interrupt is only used to wake up CPU, there is no need to do
	 * anything in the isr in addition to disable interrupt.
	 */
	espi_it8xxx2_enable_trans_irq(dev, false);
}

void espi_it8xxx2_espi_reset_isr(const struct device *port,
				struct gpio_callback *cb, uint32_t pins)
{
	struct espi_it8xxx2_data *const data = ESPI_IT8XXX2_SOC_DEV->data;
	struct espi_event evt = {ESPI_BUS_RESET, 0, 0};
	bool espi_reset = gpio_pin_get(port, (find_msb_set(pins) - 1));

	if (!(espi_reset)) {
		/* Reset vwidx_cached_flag[] when espi_reset# asserted. */
		espi_it8xxx2_reset_vwidx_cache(ESPI_IT8XXX2_SOC_DEV);
	}

	evt.evt_data = espi_reset;
	espi_send_callbacks(&data->callbacks, ESPI_IT8XXX2_SOC_DEV, evt);

	LOG_INF("eSPI reset %sasserted", espi_reset ? "de" : "");
}

/* eSPI reset# is enabled on GPD2 */
#define ESPI_IT8XXX2_ESPI_RESET_PORT DEVICE_DT_GET(DT_NODELABEL(gpiod))
#define ESPI_IT8XXX2_ESPI_RESET_PIN  2
static void espi_it8xxx2_enable_reset(void)
{
	struct gpio_it8xxx2_regs *const gpio_regs = GPIO_IT8XXX2_REG_BASE;
	static struct gpio_callback espi_reset_cb;

	/* eSPI reset is enabled on GPD2 */
	gpio_regs->GPIO_GCR =
		(gpio_regs->GPIO_GCR & ~IT8XXX2_GPIO_GCR_ESPI_RST_EN_MASK) |
		(IT8XXX2_GPIO_GCR_ESPI_RST_D2 << IT8XXX2_GPIO_GCR_ESPI_RST_POS);
	/* enable eSPI reset isr */
	gpio_init_callback(&espi_reset_cb, espi_it8xxx2_espi_reset_isr,
				BIT(ESPI_IT8XXX2_ESPI_RESET_PIN));
	gpio_add_callback(ESPI_IT8XXX2_ESPI_RESET_PORT, &espi_reset_cb);
	gpio_pin_interrupt_configure(ESPI_IT8XXX2_ESPI_RESET_PORT,
					ESPI_IT8XXX2_ESPI_RESET_PIN,
					GPIO_INT_MODE_EDGE | GPIO_INT_TRIG_BOTH);
}

static struct espi_it8xxx2_data espi_it8xxx2_data_0;
static const struct espi_it8xxx2_config espi_it8xxx2_config_0 = {
	.base_espi_slave = DT_INST_REG_ADDR_BY_IDX(0, 0),
	.base_espi_vw = DT_INST_REG_ADDR_BY_IDX(0, 1),
	.base_espi_queue0 = DT_INST_REG_ADDR_BY_IDX(0, 2),
	.base_espi_queue1 = DT_INST_REG_ADDR_BY_IDX(0, 3),
	.base_ec2i = DT_INST_REG_ADDR_BY_IDX(0, 4),
	.base_kbc = DT_INST_REG_ADDR_BY_IDX(0, 5),
	.base_pmc = DT_INST_REG_ADDR_BY_IDX(0, 6),
	.base_smfi = DT_INST_REG_ADDR_BY_IDX(0, 7),
	.wuc = IT8XXX2_DT_WUC_ITEMS_FUNC(0, 0),
};

DEVICE_DT_INST_DEFINE(0, &espi_it8xxx2_init, NULL,
		    &espi_it8xxx2_data_0, &espi_it8xxx2_config_0,
		    PRE_KERNEL_2, CONFIG_ESPI_INIT_PRIORITY,
		    &espi_it8xxx2_driver_api);

static int espi_it8xxx2_init(const struct device *dev)
{
	const struct espi_it8xxx2_config *const config = dev->config;
	struct espi_vw_regs *const vw_reg =
		(struct espi_vw_regs *)config->base_espi_vw;
	struct espi_slave_regs *const slave_reg =
		(struct espi_slave_regs *)config->base_espi_slave;
	struct gctrl_it8xxx2_regs *const gctrl = ESPI_IT8XXX2_GET_GCTRL_BASE;

	/* configure VCC detector */
	gctrl->GCTRL_RSTS = (gctrl->GCTRL_RSTS &
			~(IT8XXX2_GCTRL_VCCDO_MASK | IT8XXX2_GCTRL_HGRST)) |
			(IT8XXX2_GCTRL_VCCDO_VCC_ON | IT8XXX2_GCTRL_GRST);

	/* enable PNPCFG devices */
	pnpcfg_it8xxx2_init(dev);

#ifdef CONFIG_ESPI_PERIPHERAL_8042_KBC
	/* enable kbc port (60h/64h) */
	kbc_it8xxx2_init(dev);
#endif
#ifdef CONFIG_ESPI_PERIPHERAL_HOST_IO
	/* enable pmc1 for ACPI port (62h/66h) */
	pmc1_it8xxx2_init(dev);
#endif
#ifdef CONFIG_ESPI_PERIPHERAL_DEBUG_PORT_80
	/* Accept Port 80h Cycle */
	port80_it8xxx2_init(dev);
#endif
#if defined(CONFIG_ESPI_PERIPHERAL_EC_HOST_CMD) || \
	defined(CONFIG_ESPI_PERIPHERAL_ACPI_SHM_REGION)
	smfi_it8xxx2_init(dev);
#endif
#ifdef CONFIG_ESPI_PERIPHERAL_EC_HOST_CMD
	/* enable pmc2 for host command port */
	pmc2_it8xxx2_init(dev);
#endif

	/* Reset vwidx_cached_flag[] at initialization */
	espi_it8xxx2_reset_vwidx_cache(dev);

	/* Enable espi vw interrupt */
	vw_reg->VWCTRL0 |= IT8XXX2_ESPI_VW_INTERRUPT_ENABLE;
	IRQ_CONNECT(IT8XXX2_ESPI_VW_IRQ, 0, espi_it8xxx2_vw_isr,
			DEVICE_DT_INST_GET(0), 0);
	irq_enable(IT8XXX2_ESPI_VW_IRQ);

	/* Reset PLTRST# virtual wire signal during eSPI reset */
	vw_reg->VWCTRL2 |= IT8XXX2_ESPI_VW_RESET_PLTRST;

#ifdef CONFIG_ESPI_OOB_CHANNEL
	espi_it8xxx2_oob_init(dev);
#endif

#ifdef CONFIG_ESPI_FLASH_CHANNEL
	espi_it8xxx2_flash_init(dev);
#endif

	/* Enable espi interrupt */
	slave_reg->ESGCTRL1 |= IT8XXX2_ESPI_INTERRUPT_ENABLE;
	IRQ_CONNECT(IT8XXX2_ESPI_IRQ, 0, espi_it8xxx2_isr,
			DEVICE_DT_INST_GET(0), 0);
	irq_enable(IT8XXX2_ESPI_IRQ);

	/* enable interrupt and reset from eSPI_reset# */
	espi_it8xxx2_enable_reset();

	/*
	 * Enable eSPI to WUC.
	 * If an eSPI transaction is accepted, WU42 interrupt will be asserted.
	 */
	slave_reg->ESGCTRL2 |= IT8XXX2_ESPI_TO_WUC_ENABLE;

	/* Enable WU42 of WUI */
	it8xxx2_wuc_clear_status(config->wuc.wucs, config->wuc.mask);
	it8xxx2_wuc_enable(config->wuc.wucs, config->wuc.mask);
	/*
	 * Only register isr here, the interrupt only need to be enabled
	 * before CPU and RAM clocks gated in the idle function.
	 */
	IRQ_CONNECT(IT8XXX2_TRANS_IRQ, 0, espi_it8xxx2_trans_isr,
			DEVICE_DT_INST_GET(0), 0);

	return 0;
}
