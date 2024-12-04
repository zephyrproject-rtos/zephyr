/*
 * req->column_addr_length,
 *
 * Copyright 2024 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT nxp_flexspi

#include <zephyr/logging/log.h>
#include <zephyr/drivers/mspi.h>
#include <zephyr/drivers/mspi/nxp_flexspi.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/drivers/reset.h>
#include <zephyr/sys/barrier.h>
#include <zephyr/pm/device.h>
#include <soc.h>

/*
 * NOTE: If CONFIG_FLASH_MCUX_FLEXSPI_XIP is selected, Any external functions
 * called while interacting with the flexspi MUST be relocated to SRAM or ITCM
 * at runtime, so that the chip does not access the flexspi to read program
 * instructions while it is being written to
 */
#if defined(CONFIG_FLASH_MCUX_FLEXSPI_XIP) && (CONFIG_MSPI_LOG_LEVEL > 0)
#warning "Enabling mspi driver logging and XIP mode simultaneously can cause \
	read-while-write hazards. This configuration is not recommended."
#endif

LOG_MODULE_REGISTER(memc_flexspi, CONFIG_MSPI_LOG_LEVEL);

/*
 * Custom pin control state- this state indicates that the pin setting
 * will not affect XIP operation, IE no pins used for XIP will be configured.
 */
#define PINCTRL_STATE_SAFE PINCTRL_STATE_PRIV_START

/*
 * Some IP revisions do not contain certain registers. For these cases,
 * we provide empty definitions for register macros, so that the
 * driver code itself is cleaner
 */
#if !DT_INST_PROP(0, nxp_combination_supported)
#define FLEXSPI_MCR0_COMBINATIONEN_MASK 0x0
#define FLEXSPI_MCR0_COMBINATIONEN(x) 0x0
#endif

#if !DT_INST_PROP(0, nxp_sckb_invert_supported)
#define FLEXSPI_MCR2_SCKBDIFFOPT_MASK 0x0
#define FLEXSPI_MCR2_SCKBDIFFOPT(x) 0x0
#endif

#if !DT_INST_PROP(0, nxp_diff_rxclk_supported)
#define FLEXSPI_MCR2_RXCLKSRC_B_MASK 0x0
#define FLEXSPI_MCR2_RXCLKSRC_B(x) 0x0
#endif

#if !DT_INST_PROP(0, nxp_addrshift_supported)
#define FLEXSPI_FLSHCR0_ADDRSHIFT_MASK 0x0
#endif

#define FLEXSPI_PORT_COUNT (ARRAY_SIZE(FLEXSPI->FLSHCR0))

#define FLEXSPI_LUT_KEY_VAL   0x5AF05AF0

/*
 * FlexSPI instructions are 2 bytes each, and are stored in a LUT.
 * 8 instructions (16 bytes) are considered a "sequence". FlexSPI LUT execution
 * can only start on a sequence, so for a 128 byte LUT array we can program
 * up to 16 instruction sequences simultaneously.
 *
 * We reserve two sequences for each FlexSPI port, one for the XIP read and
 * one for the XIP write sequence. We then use two sequences as a "dynamic"
 * LUT, which is used for transfer requests. The dynamic LUT uses the first
 * two sequences, and each port uses subsequent blocks of 2 sequences for
 * XIP. In total we use 10 sequences on a 4 port FlexSPI device.
 *
 * The LUT array is accessed as a `uint32_t`, so we define indices
 * as offsets into that array. Each sequence uses 4 indices of the array.
 */

/* Dynamic lut occupies 2 sequences, 8 indices */
#define FLEXSPI_DYNAMIC_LUT_IDX 8
#define FLEXSPI_DYNAMIC_LUT_SEQ 2
/* 2 sequence offset (8 indices) for the "dynamic lut" */
#define FLEXSPI_PORT_READ_LUT_SEQ(port) (FLEXSPI_DYNAMIC_LUT_SEQ + ((port) * 2))
#define FLEXSPI_PORT_READ_LUT_IDX(port) (FLEXSPI_DYNAMIC_LUT_IDX + ((port) * 8))
/* 2 sequence offset (8 indices) for the "dynamic lut", plus 1 for the read lut */
#define FLEXSPI_PORT_WRITE_LUT_SEQ(port) (FLEXSPI_DYNAMIC_LUT_SEQ + 1 + ((port) * 2))
#define FLEXSPI_PORT_WRITE_LUT_IDX(port) (FLEXSPI_DYNAMIC_LUT_IDX + 4 + ((port) * 8))

/*
 * Default flash size we use, permits addressing 128MB on each FlexSPI device
 * if the IP instance has 4 ports.
 */
#define FLEXSPI_DEFAULT_SIZE 0x20000

/*
 * Mask of configuration parameters that would require us to reload a
 * port's READ LUT
 */
#define NXP_FLEXSPI_READ_CFG_MASK (MSPI_DEVICE_CONFIG_IO_MODE | \
				   MSPI_DEVICE_CONFIG_DATA_RATE | \
				   MSPI_DEVICE_CONFIG_RX_DUMMY | \
				   MSPI_DEVICE_CONFIG_READ_CMD | \
				   MSPI_DEVICE_CONFIG_CMD_LEN | \
				   MSPI_DEVICE_CONFIG_ADDR_LEN | \
				   MSPI_DEVICE_CONFIG_CADDR_LEN | \
				   MSPI_DEVICE_CONFIG_RD_MODE_LEN | \
				   MSPI_DEVICE_CONFIG_RD_MODE_BITS)

/*
 * Mask of configuration parameters that would require us to reload a
 * port's WRITE LUT
 */
#define NXP_FLEXSPI_WRITE_CFG_MASK (MSPI_DEVICE_CONFIG_IO_MODE | \
				    MSPI_DEVICE_CONFIG_DATA_RATE | \
				    MSPI_DEVICE_CONFIG_TX_DUMMY | \
				    MSPI_DEVICE_CONFIG_WRITE_CMD | \
				    MSPI_DEVICE_CONFIG_CMD_LEN | \
				    MSPI_DEVICE_CONFIG_ADDR_LEN | \
				    MSPI_DEVICE_CONFIG_CADDR_LEN | \
				    MSPI_DEVICE_CONFIG_WR_MODE_LEN | \
				    MSPI_DEVICE_CONFIG_WR_MODE_BITS)

/* Structure for FlexSPI AHB buffer configuration */
struct nxp_flexspi_ahb_buf_cfg {
	uint16_t prefetch;
	uint16_t priority;
	uint16_t master_id;
	uint16_t buf_size;
} __packed;

struct nxp_flexspi_config {
	/* Base address of controller */
	FLEXSPI_Type *base;
	/* Clock device and subsystem */
	const struct device *clock_dev;
	clock_control_subsys_t clock_subsys;
	/* Reset control line */
	const struct reset_dt_spec reset;
	/* Pin control config */
	const struct pinctrl_dev_config *pincfg;
	/* Default mspi config */
	const struct mspi_dt_spec default_config;
	/* AHB base address for memory-mapped access */
	uint8_t *ahb_base;
	/* Is the FlexSPI being used for XIP ? */
	bool xip;
	/* FlexSPI port used for XIP (if XIP is enabled) */
	uint8_t xip_port;
	/* Enable AHB bus bufferable write access support (AHBCR[BUFFERABLEEN]) */
	bool ahb_bufferable;
	/* Enable AHB bus cachable read access support (AHBCR[CACHABLEEN]) */
	bool ahb_cacheable;
	/* AHB Read Prefetch Enable (AHBCR[PREFETCHEN]) */
	bool ahb_prefetch;
	/* AHB Read Address option bit (AHBCR[READADDROPT]) */
	bool ahb_read_addr_opt;
	/* Combine port A and port B data pins (MCR0[COMBINATIONEN]) */
	bool combination_mode;
	/* RX sample clock selection MCR0[RXCLKSRC] */
	uint8_t rx_sample_clock;
	/* Use SCLK_B as inverted SCLK_A output (MCR2[SCKBDIFFOPT]) */
	bool sck_differential_clock;
	/* RX sample clock for port B (only on some IP revs, MCR2[RXCLKSRC_B]) */
	uint8_t rx_sample_clock_b;
	/* Number of AHB buffer configurations */
	uint8_t buf_cfg_cnt;
	/* AHB buffer configuration array */
	struct nxp_flexspi_ahb_buf_cfg *buf_cfg;
};

/*
 * Port specific configuration data. This data is set within the
 * nxp_flexspi_dev_config function, but will be used by the
 * transceive function as well
 */
struct nxp_flexspi_port_cfg {
	/* MSPI device configuration */
	struct mspi_dev_cfg dev_cfg;
	/* Size of the device in bytes */
	uint32_t size;
	/* Number of pads and opcode to use for command phase */
	uint8_t cmd_pads;
	uint8_t cmd_opcode;
	/* Number of pads and opcode to use for address phase */
	uint8_t addr_pads;
	uint8_t addr_opcode;
	/* Opcode to use during address phase to send column address */
	uint8_t caddr_opcode;
	/* Number of pads to use for data phase */
	uint8_t data_pads;
	/* Opcode for dummy cycles (in data phase) */
	uint8_t dummy_opcode;
	/* Opcode for writing data (in data phase) */
	uint8_t write_opcode;
	/* Opcode for reading data (in data phase) */
	uint8_t read_opcode;
	/* Number of sequences used for reads */
	uint8_t read_seq_num;
	/* Number of sequences used for reads */
	uint8_t write_seq_num;
};

struct nxp_flexspi_data {
	struct nxp_flexspi_port_cfg port_cfg[FLEXSPI_PORT_COUNT];
};

/* Wait for the FlexSPI bus to be idle */
static ALWAYS_INLINE void nxp_flexspi_wait_bus_idle(FLEXSPI_Type *base)
{
	uint32_t reg;

	/* Wait for FlexSPI arbitrator and sequence engine to be idle */
	do {
		reg = base->STS0;
	} while (((reg & FLEXSPI_STS0_ARBIDLE_MASK) == 0) ||
	       ((reg & FLEXSPI_STS0_SEQIDLE_MASK) == 0));
}

/* Helper to unlock FlexSPI LUT */
static ALWAYS_INLINE void nxp_flexspi_unlock_lut(FLEXSPI_Type *base)
{
	if (!IS_ENABLED(FSL_FEATURE_FLEXSPI_LUTKEY_IS_RO)) {
		base->LUTKEY = FLEXSPI_LUT_KEY_VAL;
	}
	base->LUTCR = FLEXSPI_LUTCR_UNLOCK_MASK;
}

/* Helper to lock FlexSPI LUT */
static ALWAYS_INLINE void nxp_flexspi_lock_lut(FLEXSPI_Type *base)
{
	if (!IS_ENABLED(FSL_FEATURE_FLEXSPI_LUTKEY_IS_RO)) {
		base->LUTKEY = FLEXSPI_LUT_KEY_VAL;
	}
	base->LUTCR = FLEXSPI_LUTCR_LOCK_MASK;
}

/*
 * Helper to update LUT
 * @p base: FlexSPI base address
 * @p seq_offset: Sequence offset to write LUT instructions at
 * @p instr_buf: Buffer with instructions to write to flexspi
 * @p instr_count: count of 2 byte instructions to write to flexspi
 */
static void __ramfunc nxp_flexspi_update_lut(FLEXSPI_Type *base,
					     uint32_t seq_offset,
					     uint16_t *instr_buf,
					     uint32_t instr_count)
{
	uint32_t tmp = 0;
	uint32_t idx_offset = seq_offset * 4;
	uint32_t i = 0;

	nxp_flexspi_unlock_lut(base);
	while (i < instr_count) {
		if ((instr_count - i) >= 2) {
			base->LUT[idx_offset] = ((uint32_t *)instr_buf)[i / 2];
			i += 2;
		} else {
			tmp = instr_buf[i] & 0xFFFF;
			base->LUT[idx_offset] = tmp;
			i += 1;
		}
		idx_offset++;
	}
	nxp_flexspi_lock_lut(base);
}

/*
 * Enter critical FlexSPI section- no flash access can be performed
 * after this function is called, until `nxp_flexspi_exit_critical` is called
 */
static void __ramfunc nxp_flexspi_enter_critical(FLEXSPI_Type *base, int *irq_key)
{
	/*
	 * Lock IRQs- control flow can't be interrupted until we exit
	 * critical section
	 */
	*irq_key = irq_lock();
	/* Wait for the bus to be idle */
	nxp_flexspi_wait_bus_idle(base);
	/* Make sure prefetch will stop execution here */
	barrier_dsync_fence_full();
	barrier_isync_fence_full();
}

/*
 * Exit critical FlexSPI section- once this is called, flash access can be
 * performed again.
 */
static void __ramfunc nxp_flexspi_exit_critical(FLEXSPI_Type *base, int irq_key)
{
	/* Wait for the bus to be idle */
	nxp_flexspi_wait_bus_idle(base);
	/* Force flush of instruction cache */
	barrier_isync_fence_full();
	/* Reenable irqs */
	irq_unlock(irq_key);
}

/*
 * Helper function to configure a FLEXSPI LUT
 * @p port: Configuration for this FLEXSPI port
 * @p lut_buf: buffer to load LUT into
 * @p lut_count: number of instructions lut_buf can hold (2 bytes each)
 * @p cmd: Command to send during command phase
 * @p cmd_len: Length of command in bytes
 * @p addr_len: address length for address phase
 * @p caddr_len: column address length for address phase
 * @p mode_len: number of mode bits to send after address
 * @p mode_bits: mode bits to send after address
 * @p dummy_cnt: number of dummy clocks to send after mode bits
 * @p data_present: set to true if an opcode should be sent during data phase
 * @p data_opcode: opcode to send during data phase
 * @return number of instructions loaded into lut_buf
 * @return -ENOMEM if lut_buf is not large enough
 */
static int __ramfunc nxp_flexspi_setup_lut(struct nxp_flexspi_port_cfg *port,
					   uint16_t *lut_buf, int lut_count,
					   uint32_t cmd, uint8_t cmd_len,
					   uint8_t addr_len, uint8_t caddr_len,
					   uint8_t mode_len, uint8_t mode_bits,
					   uint8_t dummy_cnt, bool data_present,
					   uint8_t data_opcode)
{
	int instr_idx = 0U;
	uint8_t mode_opcode = 0U;

	/* Command phase */
	while (cmd_len-- > 0) {
		lut_buf[instr_idx++] = (FLEXSPI_LUT_OPERAND0((cmd >> (cmd_len * 8)) & 0xFF) |
					FLEXSPI_LUT_NUM_PADS0(port->cmd_pads) |
					FLEXSPI_LUT_OPCODE0(port->cmd_opcode));
		if (instr_idx >= lut_count) {
			return -ENOMEM;
		}
	}
	if (addr_len != 0) {
		/* Address phase */
		lut_buf[instr_idx++] = (FLEXSPI_LUT_OPERAND0(addr_len) |
					FLEXSPI_LUT_NUM_PADS0(port->addr_pads) |
					FLEXSPI_LUT_OPCODE0(port->addr_opcode));
		if (instr_idx >= lut_count) {
			return -ENOMEM;
		}
	}
	if (caddr_len != 0) {
		lut_buf[instr_idx++] = (FLEXSPI_LUT_OPERAND0(caddr_len) |
					FLEXSPI_LUT_NUM_PADS0(port->addr_pads) |
					FLEXSPI_LUT_OPCODE0(port->caddr_opcode));
		if (instr_idx >= lut_count) {
			return -ENOMEM;
		}
	}
	/* Calculate mode opcode */
	if (mode_len != 0) {
		switch (mode_len) {
		case 2:
			mode_opcode = FLEXSPI_OP_MODE2_SDR;
			break;
		case 4:
			mode_opcode = FLEXSPI_OP_MODE4_SDR;
			break;
		case 8:
			mode_opcode = FLEXSPI_OP_MODE8_SDR;
			break;
		default:
			return -ENOTSUP;
		}
		/*
		 * For dual mode (full DDR) or S_D_D (DDR addr/data) we should
		 * send mode bits with DDR opcode
		 */
		switch (port->dev_cfg.data_rate) {
		case MSPI_DATA_RATE_DUAL:
			__fallthrough;
		case MSPI_DATA_RATE_S_D_D:
			mode_opcode |= BIT(5);
			__fallthrough;
		case MSPI_DATA_RATE_S_S_D:
			__fallthrough;
		case MSPI_DATA_RATE_SINGLE:
			break;
		default:
			return -ENOTSUP;
		}
		lut_buf[instr_idx++] = (FLEXSPI_LUT_OPERAND0(mode_bits) |
					FLEXSPI_LUT_NUM_PADS0(port->addr_pads) |
					FLEXSPI_LUT_OPCODE0(mode_opcode));
		if (instr_idx >= lut_count) {
			return -ENOMEM;
		}
	}
	if (dummy_cnt != 0) {
		lut_buf[instr_idx++] = (FLEXSPI_LUT_OPERAND0(dummy_cnt) |
					FLEXSPI_LUT_NUM_PADS0(port->addr_pads) |
					FLEXSPI_LUT_OPCODE0(port->dummy_opcode));
		if (instr_idx >= lut_count) {
			return -ENOMEM;
		}
	}
	if (data_present) {
		/* Data phase */
		lut_buf[instr_idx++] = (FLEXSPI_LUT_OPERAND0(0x0) |
					FLEXSPI_LUT_NUM_PADS0(port->data_pads) |
					FLEXSPI_LUT_OPCODE0(data_opcode));
		if (instr_idx >= lut_count) {
			return -ENOMEM;
		}
	}
	lut_buf[instr_idx++] = 0U; /* FlexSPI STOP command */
	return instr_idx;
}

/*
 * Helper function to check the FlexSPI INTR register for errors
 */
static ALWAYS_INLINE int nxp_flexspi_check_clear_error(FLEXSPI_Type *base,
						       uint32_t err_reg)
{
	uint32_t err_status = err_reg & (FLEXSPI_INTR_SEQTIMEOUT_MASK |
					FLEXSPI_INTR_IPCMDERR_MASK |
					FLEXSPI_INTR_IPCMDGE_MASK);
	if (err_status != 0) {
		/* Clear error flags */
		base->INTR = err_status;
		return -EIO;
	}

	return 0;
}

/*
 * Helper function for blocking transfers using IP command
 */
static int __ramfunc nxp_flexspi_tx(FLEXSPI_Type *base, uint8_t *buffer,
				    uint32_t len)
{
	uint32_t watermark = 8U;
	uint32_t fifo_idx, intr_reg;
	uint32_t byte_cnt = len;

	/* Fill FlexSPI TX FIFO. We set watermark to 8 bytes */
	base->IPTXFCR = 0x0;

	while (byte_cnt > 0) {
		/* Wait for space to be available in TX FIFO */
		intr_reg = base->INTR;
		while ((intr_reg & FLEXSPI_INTR_IPTXWE_MASK) == 0) {
			/* Poll register */
			intr_reg = base->INTR;
		}

		/* Check for errors */
		if (nxp_flexspi_check_clear_error(base, intr_reg)) {
			return -EIO;
		}

		/*
		 * Note that the data being written should be in RAM. If it is
		 * not, errors may occur as the FlexSPI tries to access data in
		 * external flash while also sending IP commands that may cause
		 * that flash device to not respond to reads
		 */
		if (byte_cnt >= watermark) {
			/* Fill up to watermark */
			for (fifo_idx = 0; fifo_idx < watermark / 4; fifo_idx++) {
				base->TFDR[fifo_idx] = *(uint32_t *)buffer;
				byte_cnt -= 4U;
				buffer += 4U;
			}
		} else {
			for (fifo_idx = 0; fifo_idx < (byte_cnt / 4); fifo_idx++) {
				/* Write in 4 byte chunks */
				base->TFDR[fifo_idx] = *(uint32_t *)buffer;
				buffer += 4U;
			}
			byte_cnt -= 4U * fifo_idx;

			if (byte_cnt != 0) {
				/* Write unaligned data into FIFO */
				uint32_t tmp = 0U;

				for (uint32_t j = 0; j < byte_cnt; j++) {
					tmp |= (((uint32_t)*buffer++) << (8 * j));
				}
				byte_cnt = 0U;
				base->TFDR[fifo_idx] = tmp;
			}
		}

		/* Push data into IP TX FIFO */
		base->INTR = FLEXSPI_INTR_IPTXWE_MASK;
	}
	return len;
}

/*
 * Helper function to run IP command RX from FLEXSPI
 */
static int __ramfunc nxp_flexspi_rx(FLEXSPI_Type *base, uint8_t *buffer,
				    uint32_t len)
{
	uint32_t fifo_idx, fill_bytes, intr_reg;
	uint32_t watermark = 8U;
	uint32_t byte_cnt = len;

	/* Read from FlexSPI RX FIFO. We set watermark to 8 bytes. */
	base->IPTXFCR = 0x0;

	while (byte_cnt > 0) {
		/* Wait for there to be data in FIFO */
		intr_reg = base->INTR;
		while ((intr_reg & FLEXSPI_INTR_IPRXWA_MASK) == 0) {
			/* Poll register */
			intr_reg = base->INTR;
		}
		/* Check for errors */
		if (nxp_flexspi_check_clear_error(base, intr_reg)) {
			return -EIO;
		}
		if (byte_cnt < watermark) {
			/*
			 * Poll on the FILL field. Field counts valid
			 * data entries in RX fifo in 64 bit increments
			 */
			do {
				if (nxp_flexspi_check_clear_error(base, base->INTR)) {
					return -EIO;
				}
				fill_bytes = FIELD_GET(base->IPRXFSTS,
						       FLEXSPI_IPRXFSTS_FILL_MASK) * 8U;
			} while (byte_cnt > fill_bytes);
		}

		/* Check for errors once more */
		if (nxp_flexspi_check_clear_error(base, base->INTR)) {
			return -EIO;
		}

		if (byte_cnt >= watermark) {
			/* Read up to the watermark from RX FIFO */
			for (fifo_idx = 0; fifo_idx < watermark / 4; fifo_idx++) {
				*(uint32_t *)buffer = base->RFDR[fifo_idx];
				byte_cnt -= 4U;
				buffer += 4U;
			}
		} else {
			for (fifo_idx = 0; fifo_idx < (byte_cnt / 4); fifo_idx++) {
				/* Write in 4 byte chunks */
				*(uint32_t *)buffer = base->RFDR[fifo_idx];
			}
			byte_cnt -= 4U * fifo_idx;

			if (byte_cnt != 0) {
				/* Write unaligned data into FIFO */
				uint32_t tmp = base->RFDR[fifo_idx];

				for (uint32_t j = 0; j < byte_cnt; j++) {
					*buffer++ = ((uint8_t)(tmp >> (8U * j)) & 0xFFU);
				}
				byte_cnt = 0U;
			}
		}
		/* Set IPRXWA bit to pop RX FIFO data */
		base->INTR = FLEXSPI_INTR_IPRXWA_MASK;
	}
	return len;
}

/*
 * Common initialization code for FlexSPI, run regardless of if FlexSPI
 * is being used for XIP
 * NOTE: Critical function- this code must execute from RAM
 */
static int __ramfunc nxp_flexspi_common_config(const struct mspi_dt_spec *spec)
{
	const struct nxp_flexspi_config *config = spec->bus->config;
	FLEXSPI_Type *base = config->base;
	volatile uint32_t mcr0, mcr2, ahbcr;
	volatile uint32_t ahbrxbufcr[ARRAY_SIZE(base->AHBRXBUFCR0)];
	int irq_key;
	volatile uint8_t buf_cfg_count = config->buf_cfg_cnt;
	volatile bool xip = config->xip;

	if (spec->config.op_mode != MSPI_OP_MODE_CONTROLLER) {
		/* FlexSPI only supports controller mode */
		return -ENOTSUP;
	}

	if (spec->config.duplex != MSPI_HALF_DUPLEX) {
		/* FlexSPI only supports half duplex */
		return -ENOTSUP;
	}

	if ((spec->config.ce_group != NULL) ||
	    (spec->config.num_ce_gpios != 0)) {
		/* FlexSPI does not support GPIO chip select lines */
		return -ENOTSUP;
	}

	/*
	 * Precalculate all register values, since can't access config
	 * structure in critical section
	 */
	mcr0 = base->MCR0;
	mcr0 &= ~(FLEXSPI_MCR0_COMBINATIONEN_MASK | FLEXSPI_MCR0_RXCLKSRC_MASK);
	mcr0 |= FLEXSPI_MCR0_COMBINATIONEN(config->combination_mode) |
		FLEXSPI_MCR0_RXCLKSRC(config->rx_sample_clock);
	/*
	 * Clear ARDFEN and ATDFEN, because we will access FlexSPI RX/TX FIFO
	 * via IP bus when writing data
	 */
	mcr0 &= ~(FLEXSPI_MCR0_ATDFEN_MASK | FLEXSPI_MCR0_ARDFEN_MASK);
	mcr2 = base->MCR2;
	mcr2 &= ~(FLEXSPI_MCR2_SCKBDIFFOPT_MASK | FLEXSPI_MCR2_RXCLKSRC_B_MASK);
	mcr2 |= FLEXSPI_MCR2_SCKBDIFFOPT(config->sck_differential_clock) |
		FLEXSPI_MCR2_RXCLKSRC_B(config->rx_sample_clock_b);
	ahbcr = base->AHBCR;
	ahbcr &= ~(FLEXSPI_AHBCR_BUFFERABLEEN_MASK |
		   FLEXSPI_AHBCR_CACHABLEEN_MASK |
		   FLEXSPI_AHBCR_PREFETCHEN_MASK |
		   FLEXSPI_AHBCR_READADDROPT_MASK);
	ahbcr |= FLEXSPI_AHBCR_BUFFERABLEEN(config->ahb_bufferable) |
		FLEXSPI_AHBCR_CACHABLEEN(config->ahb_cacheable) |
		FLEXSPI_AHBCR_PREFETCHEN(config->ahb_prefetch) |
		FLEXSPI_AHBCR_READADDROPT(config->ahb_read_addr_opt);

	if (buf_cfg_count > ARRAY_SIZE(ahbrxbufcr)) {
		LOG_ERR("Maximum RX buffer configuration count exceeded");
		return -ENOBUFS;
	}

	for (uint8_t i = 0; i < buf_cfg_count; i++) {
		ahbrxbufcr[i] = base->AHBRXBUFCR0[i];
		ahbrxbufcr[i] &= ~(FLEXSPI_AHBRXBUFCR0_PREFETCHEN_MASK |
				   FLEXSPI_AHBRXBUFCR0_PRIORITY_MASK |
				   FLEXSPI_AHBRXBUFCR0_MSTRID_MASK |
				   FLEXSPI_AHBRXBUFCR0_BUFSZ_MASK);
		ahbrxbufcr[i] |= FLEXSPI_AHBRXBUFCR0_PREFETCHEN(config->buf_cfg[i].prefetch) |
			FLEXSPI_AHBRXBUFCR0_PRIORITY(config->buf_cfg[i].priority) |
			FLEXSPI_AHBRXBUFCR0_MSTRID(config->buf_cfg[i].master_id) |
			FLEXSPI_AHBRXBUFCR0_BUFSZ(config->buf_cfg[i].buf_size);
	}

	if (xip) {
		/*
		 * Enter critical section- we cannot access flash until FlexSPI is
		 * reconfigured
		 */
		nxp_flexspi_enter_critical(base, &irq_key);
	}
	/* Disable module */
	base->MCR0 |= FLEXSPI_MCR0_MDIS_MASK;
	/* Configure module */
	base->MCR0 = mcr0;
	base->MCR2 = mcr2;
	base->AHBCR = ahbcr;
	for (uint8_t i = 0; i < buf_cfg_count; i++) {
		base->AHBRXBUFCR0[i] = ahbrxbufcr[i];
	}
	/* Reenable module */
	base->MCR0 &= ~FLEXSPI_MCR0_MDIS_MASK;
	/* Issue software reset */
	base->MCR0 |= FLEXSPI_MCR0_SWRESET_MASK;
	while (base->MCR0 & FLEXSPI_MCR0_SWRESET_MASK) {
		/* Wait for hardware to clear bit */
	}
	if (xip) {
		/* Exit critical section- flash access should now be possible */
		nxp_flexspi_exit_critical(base, irq_key);
	}
	return 0;
}

/* Configure a FlexSPI instance that is not being used for XIP */
static int nxp_flexspi_normal_config(const struct mspi_dt_spec *spec)
{
	const struct nxp_flexspi_config *config = spec->bus->config;
	int ret;

	if (config->reset.dev != NULL) {
		if (!device_is_ready(config->reset.dev)) {
			LOG_ERR("Reset device not ready");
			return -ENODEV;
		}

		ret = reset_line_toggle(config->reset.dev, config->reset.id);
		if (ret < 0) {
			return ret;
		}
	}

	if (!device_is_ready(config->clock_dev)) {
		LOG_ERR("Clock device not ready");
		return -ENODEV;
	}

	/* Enable FlexSPI clock */
	ret = clock_control_on(config->clock_dev, config->clock_subsys);
	if (ret < 0) {
		return ret;
	}

	ret = pinctrl_apply_state(config->pincfg, PINCTRL_STATE_DEFAULT);
	if (ret < 0) {
		return ret;
	}

	return nxp_flexspi_common_config(spec);
}

/* Configure a FlexSPI instance that is being used for XIP */
static int __ramfunc nxp_flexspi_safe_config(const struct mspi_dt_spec *spec)
{
	const struct nxp_flexspi_config *config = spec->bus->config;
	FLEXSPI_Type *base = config->base;
	int ret, seq_count, seq_id, reg, irq_key;
	uint32_t lut_seq[4];
	volatile uint8_t xip_port = config->xip_port;

	/*
	 * If running in XIP, copy the LUT used for read by the port "xip_port"
	 * to the offset this driver will use, and reconfigure the "xip_port"
	 * to use this new LUT location. This way, XIP support configured
	 * by the bootloader can be preserved
	 */
	reg = base->FLSHCR2[xip_port];
	seq_count = (FIELD_GET(reg, FLEXSPI_FLSHCR2_ARDSEQNUM_MASK)) + 1;
	if (seq_count > 2) {
		LOG_ERR("Cannot init FLEXSPI safely, XIP read "
			"requires more than 16 instructions");
		return -ENOTSUP;
	}
	seq_id = FIELD_GET(reg, FLEXSPI_FLSHCR2_ARDSEQID_MASK);

	/* Enter critical section */
	nxp_flexspi_enter_critical(base, &irq_key);
	for (uint8_t i = 0; i < seq_count; i++) {
		/* Copy sequence from LUT */
		for (uint8_t j = 0; j < ARRAY_SIZE(lut_seq); j++) {
			lut_seq[j] = base->LUT[j + (seq_id * 8)];
		}
		/* 8 instructions per sequence */
		nxp_flexspi_update_lut(base, FLEXSPI_PORT_READ_LUT_SEQ(xip_port),
				       (uint16_t *)lut_seq, 8);
	}
	/* Reprogram the sequence ID */
	reg &= ~FLEXSPI_FLSHCR2_ARDSEQID_MASK;
	reg |= FLEXSPI_FLSHCR2_ARDSEQID(FLEXSPI_PORT_READ_LUT_SEQ(xip_port));

	/*
	 * Disable module. Must be done after LUT copy is completed, as we
	 * can't access LUT RAM while module is disabled.
	 */
	base->MCR0 |= FLEXSPI_MCR0_MDIS_MASK;
	/* Configure module */
	base->FLSHCR2[xip_port] = reg;
	/* Reenable module and issue SW reset */
	base->MCR0 &= ~FLEXSPI_MCR0_MDIS_MASK;
	base->MCR0 |= FLEXSPI_MCR0_SWRESET_MASK;
	while (base->MCR0 & FLEXSPI_MCR0_SWRESET_MASK) {
		/* Wait for hardware to clear bit */
	}
	nxp_flexspi_exit_critical(base, irq_key);

	/*
	 * Apply "safe" pinctrl state, if one is defined.
	 * Note we don't error out if no state is defined, many SOCs
	 * won't define one as the bootrom will configure pins
	 */
	ret = pinctrl_apply_state(config->pincfg, PINCTRL_STATE_SAFE);
	if (ret < 0 && ret != -ENOENT) {
		return ret;
	}

	return nxp_flexspi_common_config(spec);
}

static int nxp_flexspi_config(const struct mspi_dt_spec *spec)
{
	const struct nxp_flexspi_config *config = spec->bus->config;

	if (config->xip) {
		return nxp_flexspi_safe_config(spec);
	}
	return nxp_flexspi_normal_config(spec);
}

#define NXP_FLEXSPI_UPDATE_CFG(mask, field)                                    \
	if (param_mask & mask) {                                               \
		port_cfg->field = new_cfg->field;                              \
	}
/*
 * Helper function to update copy of device configuration stored
 * within a port based on the param_mask
 */
static void __ramfunc nxp_flexspi_update_dev_cfg(struct mspi_dev_cfg *port_cfg,
						 const struct mspi_dev_cfg *new_cfg,
						 const enum mspi_dev_cfg_mask param_mask)
{
	NXP_FLEXSPI_UPDATE_CFG(MSPI_DEVICE_CONFIG_CE_NUM, ce_num);
	NXP_FLEXSPI_UPDATE_CFG(MSPI_DEVICE_CONFIG_FREQUENCY, freq);
	NXP_FLEXSPI_UPDATE_CFG(MSPI_DEVICE_CONFIG_IO_MODE, io_mode);
	NXP_FLEXSPI_UPDATE_CFG(MSPI_DEVICE_CONFIG_DATA_RATE, data_rate);
	NXP_FLEXSPI_UPDATE_CFG(MSPI_DEVICE_CONFIG_CPP, cpp);
	NXP_FLEXSPI_UPDATE_CFG(MSPI_DEVICE_CONFIG_ENDIAN, endian);
	NXP_FLEXSPI_UPDATE_CFG(MSPI_DEVICE_CONFIG_CE_POL, ce_polarity);
	NXP_FLEXSPI_UPDATE_CFG(MSPI_DEVICE_CONFIG_DQS, dqs_enable);
	NXP_FLEXSPI_UPDATE_CFG(MSPI_DEVICE_CONFIG_RX_DUMMY, rx_dummy);
	NXP_FLEXSPI_UPDATE_CFG(MSPI_DEVICE_CONFIG_TX_DUMMY, tx_dummy);
	NXP_FLEXSPI_UPDATE_CFG(MSPI_DEVICE_CONFIG_READ_CMD, read_cmd);
	NXP_FLEXSPI_UPDATE_CFG(MSPI_DEVICE_CONFIG_WRITE_CMD, write_cmd);
	NXP_FLEXSPI_UPDATE_CFG(MSPI_DEVICE_CONFIG_CMD_LEN, cmd_length);
	NXP_FLEXSPI_UPDATE_CFG(MSPI_DEVICE_CONFIG_ADDR_LEN, addr_length);
	NXP_FLEXSPI_UPDATE_CFG(MSPI_DEVICE_CONFIG_MEM_BOUND, mem_boundary);
	NXP_FLEXSPI_UPDATE_CFG(MSPI_DEVICE_CONFIG_BREAK_TIME, time_to_break);
	NXP_FLEXSPI_UPDATE_CFG(MSPI_DEVICE_CONFIG_RD_MODE_LEN, read_mode_length);
	NXP_FLEXSPI_UPDATE_CFG(MSPI_DEVICE_CONFIG_RD_MODE_BITS, read_mode_bits);
	NXP_FLEXSPI_UPDATE_CFG(MSPI_DEVICE_CONFIG_WR_MODE_LEN, write_mode_length);
	NXP_FLEXSPI_UPDATE_CFG(MSPI_DEVICE_CONFIG_WR_MODE_BITS, write_mode_bits);
}


static int __ramfunc nxp_flexspi_dev_config(const struct device *controller,
					    const struct mspi_dev_id *dev_id,
					    const enum mspi_dev_cfg_mask param_mask,
					    const struct mspi_dev_cfg *cfg)
{
	uint16_t instruction_buf[8];
	const struct nxp_flexspi_config *config = controller->config;
	struct nxp_flexspi_data *data = controller->data;
	FLEXSPI_Type *base = config->base;
	struct nxp_flexspi_port_cfg *port;
	uint32_t reg;
	int irq_key, ret = 0;
	/*
	 * Note- these variables are volatile because we need them to be
	 * present on the stack- we will access these from the critical
	 * section of this code, where we can't read flash.
	 */
	volatile uint16_t port_idx = dev_id->dev_idx;
	volatile bool xip = config->xip;

	if (port_idx >= FLEXSPI_PORT_COUNT) {
		return -ENOTSUP;
	}

	port = &data->port_cfg[port_idx];
	nxp_flexspi_update_dev_cfg(&port->dev_cfg, cfg, param_mask);

	if (port->dev_cfg.endian != MSPI_XFER_LITTLE_ENDIAN) {
		return -ENOTSUP;
	}

	if (port->dev_cfg.ce_polarity != MSPI_CE_ACTIVE_LOW) {
		return -ENOTSUP;
	}

	if (port->dev_cfg.cpp != MSPI_CPP_MODE_0) {
		return -ENOTSUP;
	}

	/* Check if requested address shift is supported */
	switch (port->dev_cfg.addr_shift) {
	case 0:
	case 1:
		break;
	case 5:
		if (IS_ENABLED(DT_INST_PROP(0, nxp_addrshift_supported))) {
			break;
		}
		__fallthrough;
	default:
		return -ENOTSUP;
	}

	port->size = FLEXSPI_DEFAULT_SIZE;

	/* Calculate pad counts */
	switch (port->dev_cfg.io_mode) {
	case MSPI_IO_MODE_SINGLE:
		port->cmd_pads = port->addr_pads = port->data_pads = FLEXSPI_1PAD;
		break;
	case MSPI_IO_MODE_DUAL:
		port->cmd_pads = port->addr_pads = port->data_pads = FLEXSPI_2PAD;
		break;
	case MSPI_IO_MODE_DUAL_1_1_2:
		port->cmd_pads = port->addr_pads = FLEXSPI_1PAD;
		port->data_pads = FLEXSPI_2PAD;
		break;
	case MSPI_IO_MODE_DUAL_1_2_2:
		port->cmd_pads = FLEXSPI_1PAD;
		port->addr_pads = port->data_pads = FLEXSPI_2PAD;
		break;
	case MSPI_IO_MODE_QUAD:
		port->cmd_pads = port->addr_pads = port->data_pads = FLEXSPI_4PAD;
		break;
	case MSPI_IO_MODE_QUAD_1_1_4:
		port->cmd_pads = port->addr_pads = FLEXSPI_1PAD;
		port->data_pads = FLEXSPI_4PAD;
		break;
	case MSPI_IO_MODE_QUAD_1_4_4:
		port->cmd_pads = FLEXSPI_1PAD;
		port->addr_pads = port->data_pads = FLEXSPI_4PAD;
		break;
	case MSPI_IO_MODE_OCTAL:
		port->cmd_pads = port->addr_pads = port->data_pads = FLEXSPI_8PAD;
		break;
	case MSPI_IO_MODE_OCTAL_1_1_8:
		port->cmd_pads = port->addr_pads = FLEXSPI_1PAD;
		port->data_pads = FLEXSPI_8PAD;
		break;
	case MSPI_IO_MODE_OCTAL_1_8_8:
		port->cmd_pads = FLEXSPI_1PAD;
		port->addr_pads = port->data_pads = FLEXSPI_8PAD;
		break;
	default:
		return -ENOTSUP;
	}

	/* Set default SDR opcodes */
	port->cmd_opcode = FLEXSPI_OP_CMD_SDR;
	port->addr_opcode = FLEXSPI_OP_RADDR_SDR;
	port->caddr_opcode = FLEXSPI_OP_CADDR_SDR;
	port->dummy_opcode = FLEXSPI_OP_DUMMY_SDR;
	port->read_opcode = FLEXSPI_OP_READ_SDR;
	port->write_opcode = FLEXSPI_OP_WRITE_SDR;

	/* Calculate XIP opcodes. DDR opcodes set bit 5. */
	switch (port->dev_cfg.data_rate) {
	case MSPI_DATA_RATE_DUAL:
		port->cmd_opcode |= BIT(5);
		__fallthrough;
	case MSPI_DATA_RATE_S_D_D:
		port->addr_opcode |= BIT(5);
		port->caddr_opcode |= BIT(5);
		port->dummy_opcode |= BIT(5);
		__fallthrough;
	case MSPI_DATA_RATE_S_S_D:
		port->write_opcode |= BIT(5);
		port->read_opcode |= BIT(5);
		__fallthrough;
	case MSPI_DATA_RATE_SINGLE:
		break;
	default:
		return -ENOTSUP;
	}

	if (xip) {
		/*
		 * Enter critical region to reconfigure FlexSPI. No flash
		 * access may be performed until we exit the critical section.
		 */
		nxp_flexspi_enter_critical(base, &irq_key);
	}
	if (param_mask & MSPI_DEVICE_CONFIG_FREQUENCY) {
		ret = clock_control_set_rate(config->clock_dev,
					     config->clock_subsys,
					     (clock_control_subsys_rate_t)port->dev_cfg.freq);
		if (ret < 0) {
			goto out;
		}
	}

	/*
	 * Initialize device to 128MB. This way, we can access up to 128MB on
	 * each external device (when using 4 ports) by default. FlexSPI
	 * supports a maximum of 512MB of addressable flash across all ports.
	 * We will adjust this if the device performs XIP configuration.
	 */
	base->FLSHCR0[port_idx] = FLEXSPI_FLSHCR0_FLSHSZ(FLEXSPI_DEFAULT_SIZE);
	if (param_mask & MSPI_DEVICE_CONFIG_ADDR_SHIFT) {
		if ((port->dev_cfg.addr_shift == 5) &&
		    IS_ENABLED(DT_INST_PROP(0, nxp_addrshift_supported))) {
			base->FLSHCR0[port_idx] |= FLEXSPI_FLSHCR0_ADDRSHIFT_MASK;
		}
	}
	reg = base->FLSHCR1[port_idx];
	if (param_mask & MSPI_DEVICE_CONFIG_ADDR_SHIFT) {
		reg &= ~FLEXSPI_FLSHCR1_WA_MASK;
		if (port->dev_cfg.addr_shift == 1) {
			reg |= FLEXSPI_FLSHCR1_WA_MASK;
		}
	}
	if (param_mask & MSPI_DEVICE_CONFIG_CADDR_LEN) {
		reg &= ~FLEXSPI_FLSHCR1_CAS_MASK;
		reg |= FLEXSPI_FLSHCR1_CAS(port->dev_cfg.column_addr_length);
	}
	base->FLSHCR1[port_idx] = reg;
	if (param_mask & MSPI_DEVICE_CONFIG_DQS) {
		reg = base->FLSHCR4;
		reg &= ~(FLEXSPI_FLSHCR4_WMOPT1_MASK |
			 FLEXSPI_FLSHCR4_WMENA_MASK |
			 FLEXSPI_FLSHCR4_WMENB_MASK);
		if (port->dev_cfg.dqs_enable) {
			if (port_idx >= 2) {
				/* Port B, set WMENB */
				reg |= FLEXSPI_FLSHCR4_WMENB_MASK;
			} else {
				/* Port A, set WMENA */
				reg |= FLEXSPI_FLSHCR4_WMENA_MASK;
			}
		}
		base->FLSHCR4 = reg;
	}

	/* Configure XIP lut settings */
	if (param_mask & NXP_FLEXSPI_READ_CFG_MASK) {
		if (xip) {
			/*
			 * Check the configuration mask. We will only
			 * set the configuration if the user provided settings
			 * for all read options. Otherwise, we can't be sure
			 * XIP will be possible after this setting is applied.
			 */
			if ((param_mask & NXP_FLEXSPI_READ_CFG_MASK) !=
			    NXP_FLEXSPI_READ_CFG_MASK) {
				/* Not possible to safely reconfigure */
				return -ENOTSUP;
			}
		}
		/* Setup read LUT */
		ret = nxp_flexspi_setup_lut(port, instruction_buf,
					    ARRAY_SIZE(instruction_buf),
					    port->dev_cfg.read_cmd,
					    port->dev_cfg.cmd_length,
					    port->dev_cfg.addr_length,
					    port->dev_cfg.column_addr_length,
					    port->dev_cfg.read_mode_length,
					    port->dev_cfg.read_mode_bits,
					    port->dev_cfg.rx_dummy,
					    true,
					    port->read_opcode);
		if (ret < 0) {
			goto out;
		}
		/* Each sequence has 8 instructions */
		port->read_seq_num = DIV_ROUND_UP(ret, 8);
		nxp_flexspi_update_lut(base, FLEXSPI_PORT_READ_LUT_SEQ(port_idx),
				       instruction_buf, ret);
	}
	if (param_mask & NXP_FLEXSPI_WRITE_CFG_MASK) {
		/* Setup write LUT */
		ret = nxp_flexspi_setup_lut(port, instruction_buf,
					    ARRAY_SIZE(instruction_buf),
					    port->dev_cfg.write_cmd,
					    port->dev_cfg.cmd_length,
					    port->dev_cfg.addr_length,
					    port->dev_cfg.column_addr_length,
					    port->dev_cfg.write_mode_length,
					    port->dev_cfg.write_mode_bits,
					    port->dev_cfg.tx_dummy,
					    true,
					    port->write_opcode);
		if (ret < 0) {
			goto out;
		}
		/* Each sequence has 8 instructions */
		port->write_seq_num = DIV_ROUND_UP(ret, 8);
		nxp_flexspi_update_lut(base, FLEXSPI_PORT_WRITE_LUT_SEQ(port_idx),
				       instruction_buf, ret);
	}
out:
	nxp_flexspi_exit_critical(base, irq_key);
	return ret;
}

static int nxp_flexspi_transceive(const struct device *controller,
				  const struct mspi_dev_id *dev_id,
				  const struct mspi_xfer *req)
{
	/* Instruction buffer covers two sequences */
	uint16_t instruction_buf[16];
	const struct nxp_flexspi_config *config = controller->config;
	struct nxp_flexspi_data *data = controller->data;
	FLEXSPI_Type *base = config->base;
	struct nxp_flexspi_port_cfg *port;
	int irq_key, ret;
	/* Stack variables that we can access in critical code section */
	volatile uint16_t port_idx = dev_id->dev_idx;
	volatile bool xip = config->xip;
	struct mspi_xfer_packet tmp_pkt;

	if (req->async) {
		/* No support for async transfers */
		return -ENOTSUP;
	}

	if (port_idx >= FLEXSPI_PORT_COUNT) {
		return -ENOTSUP;
	}

	port = &data->port_cfg[port_idx];

	/*
	 * For each packet, install LUT into dynamic LUT slot, and
	 * issue a transfer using that LUT
	 */
	for (uint32_t i = 0; i < req->num_packet; i++) {
		uint8_t dummy, mode_bits, mode_len, data_opcode;

		/* Copy packet into stack variable for critical section access */
		memcpy(&tmp_pkt, &req->packets[i], sizeof(tmp_pkt));

		/* Configure LUT for this packet */
		if (tmp_pkt.dir == MSPI_RX) {
			dummy = req->rx_dummy;
			mode_bits = req->read_mode_bits;
			mode_len = req->read_mode_length;
			data_opcode = port->read_opcode;
		} else {
			dummy = req->tx_dummy;
			mode_bits = req->write_mode_bits;
			mode_len = req->write_mode_length;
			data_opcode = port->write_opcode;
		}

		ret = nxp_flexspi_setup_lut(port, instruction_buf,
					    ARRAY_SIZE(instruction_buf),
					    tmp_pkt.cmd, req->cmd_length,
					    req->addr_length,
					    req->column_addr_length,
					    mode_len, mode_bits,
					    dummy,
					    (tmp_pkt.num_bytes != 0),
					    data_opcode);
		if (ret < 0) {
			return ret;
		}

		if (xip) {
			/*
			 * Enter critical region to load LUT and send transfer.
			 * No flash access may be performed until we exit the
			 * critical section.
			 */
			nxp_flexspi_enter_critical(base, &irq_key);
		}

		nxp_flexspi_update_lut(base, 0, instruction_buf, ret);

		/* Clear errors from prior transfers */
		base->INTR = (FLEXSPI_INTR_AHBCMDERR_MASK |
			FLEXSPI_INTR_IPCMDERR_MASK | FLEXSPI_INTR_AHBCMDGE_MASK |
			FLEXSPI_INTR_IPCMDGE_MASK | FLEXSPI_INTR_IPCMDDONE_MASK);

		/* Clear sequence pointer */
		base->FLSHCR2[port_idx] |= FLEXSPI_FLSHCR2_CLRINSTRPTR_MASK;
		/* Reset RX/TX FIFOs */
		base->IPTXFCR |= FLEXSPI_IPTXFCR_CLRIPTXF_MASK;
		base->IPRXFCR |= FLEXSPI_IPRXFCR_CLRIPRXF_MASK;

		/* Set SFAR with command address */
		base->IPCR0 = tmp_pkt.address;
		/*
		 * Dynamic LUT is always first sequence in flash.
		 * We can use ret to calculate sequence count.
		 */
		base->IPCR1 = FLEXSPI_IPCR1_IDATSZ(tmp_pkt.num_bytes) |
				FLEXSPI_IPCR1_ISEQID(0x0) |
				FLEXSPI_IPCR1_ISEQNUM(DIV_ROUND_UP(ret, 8) - 1);
		/* Trigger IP command */
		base->IPCMD |= FLEXSPI_IPCMD_TRG_MASK;

		if (tmp_pkt.dir == MSPI_TX) {
			ret = nxp_flexspi_tx(base, tmp_pkt.data_buf,
					     tmp_pkt.num_bytes);
		} else {
			ret = nxp_flexspi_rx(base, tmp_pkt.data_buf,
					     tmp_pkt.num_bytes);
		}

		while (!(base->INTR & FLEXSPI_INTR_IPCMDDONE_MASK)) {
			/* Wait for the IP command to complete */
		}

		if (nxp_flexspi_check_clear_error(base, base->INTR)) {
			return -EIO;
		}

		if (xip) {
			/* Exit critical section */
			nxp_flexspi_exit_critical(base, irq_key);
		}

		if (ret < 0) {
			return ret;
		}
	}
	return 0;
}

static int nxp_flexspi_xip_config(const struct device *controller,
				  const struct mspi_dev_id *dev_id,
				  const struct mspi_xip_cfg *xip_cfg)
{
	const struct nxp_flexspi_config *config = controller->config;
	struct nxp_flexspi_data *data = controller->data;
	FLEXSPI_Type *base = config->base;
	struct nxp_flexspi_port_cfg *port;
	uint32_t flshcr0, flshcr2;
	int irq_key;
	uint16_t port_idx = dev_id->dev_idx;
	volatile bool xip = config->xip;

	if (port_idx >= FLEXSPI_PORT_COUNT) {
		return -ENOTSUP;
	}

	port = &data->port_cfg[port_idx];
	flshcr0 = base->FLSHCR0[port_idx];
	flshcr2 = base->FLSHCR2[port_idx];

	/* Set flash size in KiB */
	flshcr0 &= ~FLEXSPI_FLSHCR0_FLSHSZ_MASK;
	flshcr0 |= FLEXSPI_FLSHCR0_FLSHSZ(xip_cfg->size / 1024);


	if (xip_cfg->permission == MSPI_XIP_READ_WRITE) {
		/* Configure write support */
		flshcr2 &= ~(FLEXSPI_FLSHCR2_AWRSEQID_MASK | FLEXSPI_FLSHCR2_AWRSEQNUM_MASK);
		flshcr2 |= FLEXSPI_FLSHCR2_AWRSEQID(FLEXSPI_PORT_WRITE_LUT_SEQ(port_idx)) |
			FLEXSPI_FLSHCR2_AWRSEQNUM(port->write_seq_num - 1);
	}

	/* Enable read support */
	flshcr2 &= ~(FLEXSPI_FLSHCR2_ARDSEQID_MASK | FLEXSPI_FLSHCR2_ARDSEQNUM_MASK);
	flshcr2 |= FLEXSPI_FLSHCR2_ARDSEQID(FLEXSPI_PORT_READ_LUT_SEQ(port_idx)) |
		FLEXSPI_FLSHCR2_ARDSEQNUM(port->read_seq_num - 1);

	if (xip) {
		/*
		 * Enter critical section- we cannot access flash until FlexSPI is
		 * reconfigured
		 */
		nxp_flexspi_enter_critical(base, &irq_key);
	}
	/* Disable module */
	base->MCR0 |= FLEXSPI_MCR0_MDIS_MASK;

	base->FLSHCR0[port_idx] = flshcr0;
	base->FLSHCR2[port_idx] = flshcr2;

	/* Reenable module, issue software reset */
	base->MCR0 &= ~FLEXSPI_MCR0_MDIS_MASK;
	base->MCR0 |= FLEXSPI_MCR0_SWRESET_MASK;
	while (base->MCR0 & FLEXSPI_MCR0_SWRESET_MASK) {
		/* Wait for hardware to clear bit */
	}
	if (xip) {
		/* Exit critical section- flash access should now be possible */
		nxp_flexspi_exit_critical(base, irq_key);
	}
	return 0;
}

struct mspi_driver_api nxp_flexspi_driver_api = {
	.config = nxp_flexspi_config,
	.dev_config = nxp_flexspi_dev_config,
	.transceive = nxp_flexspi_transceive,
	.xip_config = nxp_flexspi_xip_config,
};

/* Initialize the FlexSPI module */
static int nxp_flexspi_init(const struct device *dev)
{
	const struct nxp_flexspi_config *config = dev->config;
	struct nxp_flexspi_data *data = dev->data;
	/*
	 * Sensible set of defaults for each MSPI device port. Note not
	 * all devices will work with these settings. However, the way
	 * the FlexSPI LUTs work mean that we will sometimes have to assume
	 * settings for the device (IE if the user configures the
	 * address length but does not set a READ command, we would
	 * still need to update the full LUT)
	 */
	const struct mspi_dev_cfg default_cfg = {
		.ce_num = 0,
		.freq = MHZ(30), /* Most flash chips support this frequency */
		.io_mode = MSPI_IO_MODE_SINGLE,
		.data_rate = MSPI_DATA_RATE_SINGLE,
		.cpp = MSPI_CPP_MODE_0,
		.endian = MSPI_XFER_LITTLE_ENDIAN,
		.ce_polarity = MSPI_CE_ACTIVE_LOW,
		.dqs_enable = false,
	};

	/* Set default device configurations for FlexSPI */
	for (uint8_t i = 0; i < FLEXSPI_PORT_COUNT; i++) {
		memcpy(&data->port_cfg[i].dev_cfg, &default_cfg,
		       sizeof(data->port_cfg[i].dev_cfg));
	}

	return mspi_config(&config->default_config);
}

/* Default FlexSPI MSPI configuration */
#define FLEXSPI_MSPI_CONFIG                                                    \
	{                                                                      \
		.channel_num = 0,                                              \
		.op_mode = MSPI_OP_MODE_CONTROLLER,                            \
		.duplex = MSPI_HALF_DUPLEX,                                    \
		.dqs_support = true,                                           \
		.sw_multi_periph = false,                                      \
		.ce_group = NULL,                                              \
		.num_ce_gpios = 0,                                             \
		.num_periph = 0,                                               \
		.max_freq = 0,                                                 \
		.re_init = false,                                              \
	}

#if defined(CONFIG_XIP) && defined(CONFIG_FLASH_MCUX_FLEXSPI_XIP)
/* Checks if image flash base address is in the FlexSPI AHB base region */
#define NXP_FLEXSPI_CFG_XIP(node_id)                                           \
	((CONFIG_FLASH_BASE_ADDRESS) >= DT_REG_ADDR_BY_IDX(node_id, 1)) &&     \
	((CONFIG_FLASH_BASE_ADDRESS) < (DT_REG_ADDR_BY_IDX(node_id, 1) +       \
					DT_REG_SIZE_BY_IDX(node_id, 1)))

#else
#define NXP_FLEXSPI_CFG_XIP(node_id) false
#endif

#ifdef CONFIG_PM_DEVICE
static int nxp_flexspi_pm_action(const struct device *dev,
				 enum pm_device_action action)
{
	const struct nxp_flexspi_config *config = dev->config;
	int ret;

	switch (action) {
	case PM_DEVICE_ACTION_RESUME:
		if (config->xip) {
			ret = pinctrl_apply_state(dev->pincfg,
						  PINCTRL_STATE_SAFE);
		} else {
			ret = pinctrl_apply_state(dev->pincfg,
						  PINCTRL_STATE_DEFAULT);
		}
		if (ret < 0 && ret != -ENOENT) {
			return ret;
		}
		break;
	case PM_DEVICE_ACTION_SUSPEND:
		ret = pinctrl_apply_state(dev->pincfg, PINCTRL_STATE_SLEEP);
		if (ret < 0 && ret != -ENOENT) {
			return ret;
		}
		break;
	default:
		return -ENOTSUP;
	}

	return 0;
}
#endif


#define NXP_FLEXSPI_DEFINE(n)                                                  \
	PINCTRL_DT_INST_DEFINE(n);                                             \
	static uint16_t  buf_cfg_##n[] =                                       \
		DT_INST_PROP_OR(n, nxp_rx_buffer_config, {0});                 \
	static const struct nxp_flexspi_config nxp_flexspi_config_##n = {      \
		.base = (FLEXSPI_Type *)DT_INST_REG_ADDR(n),                   \
		.clock_dev = DEVICE_DT_GET(DT_INST_CLOCKS_CTLR(n)),            \
		.clock_subsys = (clock_control_subsys_t)                       \
			DT_INST_CLOCKS_CELL(n, name),                          \
		.xip = NXP_FLEXSPI_CFG_XIP(DT_DRV_INST(n)),                    \
		.xip_port = DT_INST_PROP(n, nxp_xip_port),                     \
		.reset = RESET_DT_SPEC_INST_GET_OR(n, {}),                     \
		.pincfg = PINCTRL_DT_INST_DEV_CONFIG_GET(n),                   \
		.default_config = {                                            \
			.bus = DEVICE_DT_GET(DT_DRV_INST(n)),                  \
			.config = FLEXSPI_MSPI_CONFIG,                         \
		},                                                             \
		.ahb_base = (uint8_t *)DT_INST_REG_ADDR_BY_IDX(n, 1),          \
		.ahb_bufferable = DT_INST_PROP(n, nxp_ahb_bufferable),         \
		.ahb_cacheable = DT_INST_PROP(n, nxp_ahb_cacheable),           \
		.ahb_prefetch = DT_INST_PROP(n, nxp_ahb_prefetch),             \
		.ahb_read_addr_opt = DT_INST_PROP(n, nxp_ahb_read_addr_opt),   \
		.combination_mode = DT_INST_PROP(n, nxp_combination_mode),     \
		.rx_sample_clock = DT_INST_ENUM_IDX(n, nxp_rx_clock_source),   \
		.sck_differential_clock = DT_INST_PROP(n, nxp_sck_differential_clock),\
		.rx_sample_clock_b = DT_INST_ENUM_IDX(n, nxp_rx_clock_source_b),\
		.buf_cfg = (struct nxp_flexspi_ahb_buf_cfg *)buf_cfg_##n,      \
		.buf_cfg_cnt = sizeof(buf_cfg_##n) /                           \
			sizeof(struct nxp_flexspi_ahb_buf_cfg),                \
	};                                                                     \
	static struct nxp_flexspi_data nxp_flexspi_data_##n;                   \
                                                                               \
	PM_DEVICE_DT_INST_DEFINE(n, nxp_flexspi_pm_action);                    \
                                                                               \
	DEVICE_DT_INST_DEFINE(n,                                               \
			      nxp_flexspi_init,                                \
			      PM_DEVICE_DT_INST_GET(n),                        \
			      &nxp_flexspi_data_##n,                           \
			      &nxp_flexspi_config_##n,                         \
			      POST_KERNEL,                                     \
			      CONFIG_MSPI_INIT_PRIORITY,                       \
			      &nxp_flexspi_driver_api);

DT_INST_FOREACH_STATUS_OKAY(NXP_FLEXSPI_DEFINE)
