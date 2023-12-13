#include <zephyr/kernel.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/sdhc.h>
#include <zephyr/sd/sd_spec.h>
#include <zephyr/sys/device_mmio.h>

#include "xlnx_zynq_sdhc.h"
#include "zephyr/cache.h"
#include <zephyr/device.h>

#ifdef CONFIG_PINCTRL
#include <zephyr/drivers/pinctrl.h>
#endif

#define DT_DRV_COMPAT xlnx_zynq_sdhc

#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(sdhc, CONFIG_SDHC_LOG_LEVEL);

#ifdef CONFIG_XLNX_ZYNQ_SDHC_HOST_ADMA_DESC_SIZE
#define ADMA_DESC_SIZE CONFIG_XLNX_ZYNQ_SDHC_HOST_ADMA_DESC_SIZE
#else
#define ADMA_DESC_SIZE 32
#endif
/*----------------------------------------------------------------------------------------
			STRUCTURE DEFINITIONS
----------------------------------------------------------------------------------------*/
#define DEV_CFG(dev) ((const struct zynq_sdhc_config *const)((dev)->config))
#define DEV_REG(dev) ((volatile struct zynq_sdhc_reg *)((uintptr_t)DEVICE_MMIO_GET(dev)))

typedef void (*zynq_sdhc_isr_cb_t)(const struct device *dev);

struct zynq_sdhc_config {
	// mm_reg_t base;
	DEVICE_MMIO_ROM;
#ifdef CONFIG_PINCTRL
	const struct pinctrl_dev_config *pincfg;
#endif

	zynq_sdhc_isr_cb_t config_func;
	uint32_t clock_freq;
	uint32_t max_bus_freq;
	uint32_t min_bus_freq;
	uint32_t power_delay_ms;
	uint8_t hs200_mode: 1;
	uint8_t hs400_mode: 1;
	uint8_t dw_4bit: 1;
	uint8_t dw_8bit: 1;
};

struct zynq_sdhc_cmd_config {
	struct sdhc_command *sdhc_cmd;
	uint32_t cmd_idx;
	enum zynq_sdhc_cmd_type cmd_type;
	bool data_present;
	bool idx_check_en;
	bool crc_check_en;
};

struct zynq_sdhc_data {
	DEVICE_MMIO_RAM;
	struct sdhc_host_props props;
	uint32_t rca;
	struct sdhc_io host_io;
	struct k_sem lock;
	struct k_event irq_event;

	bool card_present;

	enum sd_spec_version hc_ver;
	enum sdhc_bus_width bus_width;
	enum zynq_sdhc_slot_type slot_type;
	uint64_t host_caps;
#if defined(CONFIG_XLNX_ZYNQ_SDHC_HOST_ADMA)
	uint8_t xfer_flag;
#endif
	adma_desc_t adma_desc_tbl[ADMA_DESC_SIZE] __aligned(32);
};

/*----------------------------------------------------------------------------------------
			PROTOTYPES
----------------------------------------------------------------------------------------*/

// static const struct device *const slcr = DEVICE_DT_GET(DT_INST_PHANDLE(0, syscon));

static void configure_status_interrupts_enable_signals(const struct device *dev);
static void configure_status_interrupts_disable_signals(const struct device *dev);
static void clear_interrupts(const struct device *dev);

static int zynq_sdhc_host_sw_reset(const struct device *dev, enum zynq_sdhc_swrst sr);
static int zynq_sdhc_dma_init(const struct device *dev, struct sdhc_data *data, bool read);
static int zynq_sdhc_init_xfr(const struct device *dev, struct sdhc_data *data, bool read);

static int read_data_port(const struct device *dev, struct sdhc_data *sdhc);
static int write_data_port(const struct device *dev, struct sdhc_data *sdhc);
static int wait_xfr_poll_complete(const struct device *dev, uint32_t time_out);

static int zynq_sdhc_set_voltage(const struct device *dev, enum sd_voltage signal_voltage);
static int zynq_sdhc_set_power(const struct device *dev, enum sdhc_power state);

static uint32_t zynq_get_clock_speed(enum sdhc_clock_speed speed);
static bool zynq_sdhc_clock_set(const struct device *dev, enum sdhc_clock_speed speed);
static bool zynq_sdhc_enable_clock(const struct device *dev);
static bool zynq_sdhc_disable_clock(const struct device *dev);
static uint16_t zynq_sdhc_calc_clock(const struct device *dev, uint32_t tgt_freq);
static uint16_t zynq_sdhc_calc_timeout(const struct device *dev, uint32_t timeout_ms);

static int set_timing(const struct device *dev, enum sdhc_timing_mode timing);

static int zynq_sdhc_set_io(const struct device *dev, struct sdhc_io *ios);

#if defined(CONFIG_XLNX_ZYNQ_SDHC_HOST_INTR)
static int wait_for_cmd_complete(struct zynq_sdhc_data *sdhc_data, uint32_t time_out);
#else
static int poll_cmd_complete(const struct device *dev, uint32_t time_out);
#endif

static int zynq_sdhc_host_send_cmd(const struct device *dev,
				   const struct zynq_sdhc_cmd_config *config);
static int zynq_sdhc_send_cmd_no_data(const struct device *dev, struct sdhc_command *cmd);
static int zynq_sdhc_send_cmd_data(const struct device *dev, struct sdhc_command *cmd,
				   struct sdhc_data *data, bool read);
static uint16_t zynq_sdhc_generate_transfer_mode(const struct device *dev, struct sdhc_data *data,
						 bool read);
/*----------------------------------------------------------------------------------------*/

static int zynq_sdhc_set_voltage(const struct device *dev, enum sd_voltage signal_voltage)
{
	volatile struct zynq_sdhc_reg *regs = DEV_REG(dev);
	struct zynq_sdhc_data *sdhc = dev->data;
	bool power_state = regs->power_ctrl & ZYNQ_SDHC_HOST_POWER_CTRL_SD_BUS_POWER ? true : false;
	uint64_t host_caps = sdhc->host_caps;
	int ret = 0;
	uint8_t power_level = 0;

	if (power_state) {
		/* Turn OFF Bus Power before config clock */
		regs->power_ctrl &= ~ZYNQ_SDHC_HOST_POWER_CTRL_SD_BUS_POWER;
	}

	switch (signal_voltage) {
	case SD_VOL_3_3_V:
		if (host_caps & ZYNQ_SDHC_HOST_VOL_3_3_V_SUPPORT) {
			if (sdhc->hc_ver == SD_SPEC_VER3_0) {
				regs->host_ctrl2 &= ~(ZYNQ_SDHC_HOST_CTRL2_1P8V_SIG_EN
						      << ZYNQ_SDHC_HOST_CTRL2_1P8V_SIG_LOC);
			}
			/* 3.3v voltage select */
			power_level = ZYNQ_SDHC_HOST_VOL_3_3_V_SELECT;
			LOG_DBG("3.3V Selected for MMC Card");
		} else {
			LOG_ERR("3.3V not supported by MMC Host");
			ret = -ENOTSUP;
		}
		break;

	case SD_VOL_3_0_V:
		if (host_caps & ZYNQ_SDHC_HOST_VOL_3_0_V_SUPPORT) {
			if (sdhc->hc_ver == SD_SPEC_VER3_0) {
				regs->host_ctrl2 &= ~(ZYNQ_SDHC_HOST_CTRL2_1P8V_SIG_EN
						      << ZYNQ_SDHC_HOST_CTRL2_1P8V_SIG_LOC);
			}
			/* 3.0v voltage select */
			power_level = ZYNQ_SDHC_HOST_VOL_3_0_V_SELECT;
			LOG_DBG("3.0V Selected for MMC Card");
		} else {
			LOG_ERR("3.0V not supported by MMC Host");
			ret = -ENOTSUP;
		}
		break;

	case SD_VOL_1_8_V:
		if (host_caps & ZYNQ_SDHC_HOST_VOL_1_8_V_SUPPORT) {
			if (sdhc->hc_ver == SD_SPEC_VER3_0) {
				regs->host_ctrl2 |= ZYNQ_SDHC_HOST_CTRL2_1P8V_SIG_EN
						    << ZYNQ_SDHC_HOST_CTRL2_1P8V_SIG_LOC;
			}
			/* 1.8v voltage select */
			power_level = ZYNQ_SDHC_HOST_VOL_1_8_V_SELECT;
			LOG_DBG("1.8V Selected for MMC Card");
		} else {
			LOG_ERR("1.8V not supported by MMC Host");
			ret = -ENOTSUP;
		}
		break;

	default:
		ret = -EINVAL;
	}

	/* Turn ON Bus Power */
	regs->power_ctrl = power_state ? (power_level | ZYNQ_SDHC_HOST_POWER_CTRL_SD_BUS_POWER)
				       : (power_level);

	return ret;
}

static int zynq_sdhc_set_power(const struct device *dev, enum sdhc_power state)
{
	struct zynq_sdhc_data *sdhc = dev->data;

	if (state == SDHC_POWER_ON) {
		/* Turn ON Bus Power */
		if (sdhc->hc_ver == SD_SPEC_VER3_0) {
			const uint8_t write_val =
				(XSDPS_PC_BUS_VSEL_3V3_MASK | XSDPS_PC_BUS_PWR_MASK) &
				~XSDPS_PC_EMMC_HW_RST_MASK;
			zynq_sdhc_write8(dev, XSDPS_POWER_CTRL_OFFSET, write_val);
		} else {
			const uint8_t write_val =
				(XSDPS_PC_BUS_VSEL_3V3_MASK | XSDPS_PC_BUS_PWR_MASK);
			zynq_sdhc_write8(dev, XSDPS_POWER_CTRL_OFFSET, write_val);
		}
	} else {
		/* Turn OFF Bus Power */
		if (sdhc->hc_ver == SD_SPEC_VER3_0) {
			zynq_sdhc_write8(dev, XSDPS_POWER_CTRL_OFFSET, XSDPS_PC_EMMC_HW_RST_MASK);
		} else {
			zynq_sdhc_write8(dev, XSDPS_POWER_CTRL_OFFSET, 0x0);
		}
	}

	k_msleep(1u);

	return 0;
}

static bool zynq_sdhc_disable_clock(const struct device *dev)
{
	uint32_t present_state = zynq_sdhc_read32(dev, XSDPS_PRES_STATE_OFFSET);
	uint16_t clk_reg = zynq_sdhc_read16(dev, XSDPS_CLK_CTRL_OFFSET);

	if (present_state & ZYNQ_SDHC_HOST_PSTATE_CMD_INHIBIT) {
		LOG_ERR("present_state:%x", present_state);
		return false;
	}
	if (present_state & ZYNQ_SDHC_HOST_PSTATE_DAT_INHIBIT) {
		LOG_ERR("present_state:%x", present_state);
		return false;
	}

	clk_reg &= ~ZYNQ_SDHC_HOST_INTERNAL_CLOCK_EN;
	zynq_sdhc_write16(dev, XSDPS_CLK_CTRL_OFFSET, clk_reg);

	clk_reg &= ~ZYNQ_SDHC_HOST_SD_CLOCK_EN;
	zynq_sdhc_write16(dev, XSDPS_CLK_CTRL_OFFSET, clk_reg);

	while ((zynq_sdhc_read16(dev, XSDPS_CLK_CTRL_OFFSET) & ZYNQ_SDHC_HOST_SD_CLOCK_EN) != 0) {
		// k_usleep(10);
		k_busy_wait(10);
	}

	return true;
}

static bool zynq_sdhc_enable_clock(const struct device *dev)
{
	volatile struct zynq_sdhc_reg *regs = DEV_REG(dev);

	regs->clock_ctrl |= ZYNQ_SDHC_HOST_INTERNAL_CLOCK_EN;
	/* Wait for the stable Internal Clock */
	while ((regs->clock_ctrl & ZYNQ_SDHC_HOST_INTERNAL_CLOCK_STABLE) == 0) {
		k_busy_wait(10);
	}

	/* Enable SD Clock */
	regs->clock_ctrl |= ZYNQ_SDHC_HOST_SD_CLOCK_EN;
	while ((regs->clock_ctrl & ZYNQ_SDHC_HOST_SD_CLOCK_EN) == 0) {
		k_busy_wait(10);
	}

	return true;
}

static uint16_t zynq_sdhc_calc_timeout(const struct device *dev, uint32_t timeout_ms)
{
	uint16_t timeout_val = 0xeU;
	// struct zynq_sdhc_data *sdhc = dev->data;
	const struct zynq_sdhc_config *cfg = DEV_CFG(dev);
	const uint8_t divider = zynq_sdhc_read8(dev, XSDPS_CLK_CTRL_OFFSET + 1) & 0xFF;
	const uint32_t base_freq = cfg->clock_freq;
	const uint32_t freq = base_freq / (divider);

	for (uint8_t i = 0; i < 0xe; i++) {

		uint32_t multiplier = 1 << (i + 13);
		uint32_t t = multiplier * 1000 / freq;
		if (t >= timeout_ms) {
			timeout_val = i;
			break;
		}
	}
	return timeout_val;
}

static uint16_t zynq_sdhc_calc_clock(const struct device *dev, uint32_t tgt_freq)
{
	uint16_t clock_val = 0U;
	uint16_t div_cnt = 0;
	uint16_t divisor = 0U;
	struct zynq_sdhc_data *sdhc = dev->data;
	const struct zynq_sdhc_config *cfg = DEV_CFG(dev);
	const uint32_t base_freq = cfg->clock_freq;
	if (sdhc->hc_ver == SD_SPEC_VER3_0) {
		if (base_freq <= tgt_freq) {
			divisor = 0;
		} else {
			for (div_cnt = 2U; div_cnt <= XSDPS_CC_EXT_MAX_DIV_CNT; div_cnt += 2U) {
				if (((base_freq) / div_cnt) <= tgt_freq) {
					divisor = div_cnt >> 1;
					break;
				}
			}
		}
	} else {
		for (div_cnt = 1U; div_cnt <= XSDPS_CC_MAX_DIV_CNT; div_cnt <<= 1U) {
			if (((base_freq) / div_cnt) <= tgt_freq) {
				divisor = div_cnt >> 1;
				break;
			}
		}
	}

	clock_val |= ((divisor & XSDPS_CC_SDCLK_FREQ_SEL_MASK) << XSDPS_CC_DIV_SHIFT);
	clock_val |=
		(((divisor >> 8U) & XSDPS_CC_SDCLK_FREQ_SEL_EXT_MASK) << XSDPS_CC_EXT_DIV_SHIFT);

	return clock_val;
}

uint32_t zynq_get_clock_speed(enum sdhc_clock_speed speed)
{
	uint32_t freq = 0;
	switch (speed) {
	case SDMMC_CLOCK_400KHZ:
		freq = 400000;
		break;

	case SD_CLOCK_25MHZ:
	case MMC_CLOCK_26MHZ:
		freq = 25000000;
		break;

	case SD_CLOCK_50MHZ:
	case MMC_CLOCK_52MHZ:
		freq = 50000000;
		break;

	case SD_CLOCK_100MHZ:
		freq = 100000000;
		break;

	case MMC_CLOCK_HS200:
		freq = 200000000;
		break;

	case SD_CLOCK_208MHZ:
	default:
		break;
	}
	return freq;
}

bool zynq_sdhc_clock_set(const struct device *dev, enum sdhc_clock_speed speed)
{
	// uint32_t clock_val = 0U;
	uint16_t clk_reg = 0;
	bool ret = false;

	ret = zynq_sdhc_disable_clock(dev);
	if (!ret) {
		return false;
	}

	/* calc clock */
	clk_reg = zynq_sdhc_calc_clock(dev, zynq_get_clock_speed(speed));
	LOG_DBG("Clock divider for MMC Clk: %d Hz is %d", speed, clk_reg);

	clk_reg |= ZYNQ_SDHC_HOST_INTERNAL_CLOCK_EN;
	zynq_sdhc_write16(dev, XSDPS_CLK_CTRL_OFFSET, (clk_reg));

	/* Wait for the stable Internal Clock */
	while ((zynq_sdhc_read16(dev, XSDPS_CLK_CTRL_OFFSET) &
		ZYNQ_SDHC_HOST_INTERNAL_CLOCK_STABLE) == 0) {
		;
	}

	/* Enable SD Clock */
	clk_reg |= ZYNQ_SDHC_HOST_SD_CLOCK_EN;
	zynq_sdhc_write16(dev, XSDPS_CLK_CTRL_OFFSET, (clk_reg));
	while ((zynq_sdhc_read16(dev, XSDPS_CLK_CTRL_OFFSET) & ZYNQ_SDHC_HOST_SD_CLOCK_EN) == 0) {
		;
	}

	return true;
}

static int set_timing(const struct device *dev, enum sdhc_timing_mode timing)
{
	volatile struct zynq_sdhc_reg *regs = DEV_REG(dev);
	int ret = 0;
	uint8_t mode;

	LOG_DBG("UHS Mode: %d", timing);

	switch (timing) {
	case SDHC_TIMING_LEGACY:
	case SDHC_TIMING_HS:
	case SDHC_TIMING_SDR12:
		mode = ZYNQ_SDHC_HOST_UHSMODE_SDR12;
		break;

	case SDHC_TIMING_SDR25:
		mode = ZYNQ_SDHC_HOST_UHSMODE_SDR25;
		break;

	case SDHC_TIMING_SDR50:
		mode = ZYNQ_SDHC_HOST_UHSMODE_SDR50;
		break;

	case SDHC_TIMING_SDR104:
		mode = ZYNQ_SDHC_HOST_UHSMODE_SDR104;
		break;

	case SDHC_TIMING_DDR50:
	case SDHC_TIMING_DDR52:
		mode = ZYNQ_SDHC_HOST_UHSMODE_DDR50;
		break;

	case SDHC_TIMING_HS400:
	case SDHC_TIMING_HS200:
		mode = ZYNQ_SDHC_HOST_UHSMODE_HS400;
		break;

	default:
		ret = -ENOTSUP;
	}

	if (!ret) {
		if (!zynq_sdhc_disable_clock(dev)) {
			LOG_ERR("Disable clk failed");
			return -EIO;
		}
		regs->host_ctrl2 |= ZYNQ_SDHC_HOST_CTRL2_1P8V_SIG_EN
				    << ZYNQ_SDHC_HOST_CTRL2_1P8V_SIG_LOC;
		SET_BITS(regs->host_ctrl2, ZYNQ_SDHC_HOST_CTRL2_UHS_MODE_SEL_LOC,
			 ZYNQ_SDHC_HOST_CTRL2_UHS_MODE_SEL_MASK, mode);

		zynq_sdhc_enable_clock(dev);
	}

	return ret;
}

static void configure_status_interrupts_enable_signals(const struct device *dev)
{
	// zynq_sdhc_write16(dev, XSDPS_NORM_INTR_STS_EN_OFFSET, ZYNQ_SDHC_HOST_NORMAL_INTR_MASK);
	zynq_sdhc_write16(dev, XSDPS_NORM_INTR_STS_EN_OFFSET,
			  XSDPS_NORM_INTR_ALL_MASK & (~XSDPS_INTR_CARD_MASK));
	zynq_sdhc_write16(dev, XSDPS_ERR_INTR_STS_EN_OFFSET, ZYNQ_SDHC_HOST_ERROR_INTR_MASK);

	zynq_sdhc_write16(dev, XSDPS_NORM_INTR_SIG_EN_OFFSET,
			  XSDPS_NORM_INTR_ALL_MASK & (~XSDPS_INTR_CARD_MASK));
	zynq_sdhc_write16(dev, XSDPS_ERR_INTR_SIG_EN_OFFSET, ZYNQ_SDHC_HOST_ERROR_INTR_MASK);

	zynq_sdhc_write16(dev, XSDPS_TIMEOUT_CTRL_OFFSET, ZYNQ_SDHC_HOST_MAX_TIMEOUT);
}

static void configure_status_interrupts_disable_signals(const struct device *dev)
{
	zynq_sdhc_write16(dev, XSDPS_NORM_INTR_STS_EN_OFFSET,
			  XSDPS_NORM_INTR_ALL_MASK & (~XSDPS_INTR_CARD_MASK));
	zynq_sdhc_write16(dev, XSDPS_ERR_INTR_STS_EN_OFFSET, ZYNQ_SDHC_HOST_ERROR_INTR_MASK);

	zynq_sdhc_write16(dev, XSDPS_NORM_INTR_SIG_EN_OFFSET, 0);
	zynq_sdhc_write16(dev, XSDPS_ERR_INTR_SIG_EN_OFFSET, 0);

	zynq_sdhc_write16(dev, XSDPS_TIMEOUT_CTRL_OFFSET, ZYNQ_SDHC_HOST_MAX_TIMEOUT);
}

static void clear_interrupts(const struct device *dev)
{
#define INT_CLEAR_ALL                                                                              \
	((ZYNQ_SDHC_HOST_ERROR_INTR_MASK << 16u) | ZYNQ_SDHC_HOST_NORMAL_INTR_MASK_CLR)

	zynq_sdhc_write32(dev, XSDPS_NORM_INTR_STS_OFFSET, INT_CLEAR_ALL);
}

static int zynq_sdhc_host_sw_reset(const struct device *dev, enum zynq_sdhc_swrst sr)
{

	/** Using XSDPS_CLK_CTRL_OFFSET(0x2C) in place of XSDPS_SW_RST_OFFSET(0x2F) for 32bit
	 * address aligned reading */
	uint32_t timeout = 100U;
	uint8_t sr_reg_val = 0;
	uint8_t write_val = 0;
	switch (sr) {
	case ZYNQ_SDHC_HOST_SWRST_ALL:
		write_val = ZYNQ_SDHC_HOST_SW_RESET_REG_ALL;
		break;
	case ZYNQ_SDHC_HOST_SWRST_CMD_LINE:
		write_val = ZYNQ_SDHC_HOST_SW_RESET_REG_CMD;
		break;
	case ZYNQ_SDHC_HOST_SWRST_DATA_LINE:
		write_val = ZYNQ_SDHC_HOST_SW_RESET_REG_DATA;
		break;
	}
	zynq_sdhc_write8(dev, XSDPS_SW_RST_OFFSET, write_val);

	for (; timeout > 0; timeout--) {
		sr_reg_val = zynq_sdhc_read8(dev, XSDPS_SW_RST_OFFSET);
		bool is_reset_done = (sr_reg_val & write_val) == 0;
		if (is_reset_done) {
			break;
		}
		k_sleep(K_MSEC(1U));
	}

	if (timeout == 0) {
		LOG_ERR("Software reset failed");
		return -EIO;
	}
	return 0;
}

// static uint8_t adma_buff[512] __aligned(32);
static int zynq_sdhc_dma_init(const struct device *dev, struct sdhc_data *data,
			      bool read) // NOLINT
{
	struct zynq_sdhc_data *sdhc_data = dev->data;
	volatile struct zynq_sdhc_reg *regs = DEV_REG(dev);

	// if (IS_ENABLED(CONFIG_DCACHE) && !read) {
	sys_cache_data_flush_and_invd_range(data->data, (data->blocks * data->block_size));
	// }
	// uintptr_t buff_base = 0;

	if (IS_ENABLED(CONFIG_XLNX_ZYNQ_SDHC_HOST_ADMA)) {
		// uint8_t *buff = data->data;
		uintptr_t buff_base = (uintptr_t)data->data;

		memset(sdhc_data->adma_desc_tbl, 0, sizeof(adma_desc_t) * (data->blocks + 1));
		for (int i = 0; i < data->blocks; i++) {
			uint8_t is_last = (i == (data->blocks - 1)) ? 1 : 0;
			adma_desc_t *desc = &sdhc_data->adma_desc_tbl[i];
#if defined(CONFIG_64BIT)
			desc->address = buff_base + (i * data->block_size);
#else
			desc->address = buff_base + (i * data->block_size);
#endif
			desc->len = data->block_size;

			desc->attr.attr_bits.valid = 1;
			desc->attr.attr_bits.end = is_last;
			desc->attr.attr_bits.int_en = 1;

			// always tran
			desc->attr.attr_bits.act = 2; // 0b10;

			LOG_DBG("adma_tbl entry: addr: 0x%04x, attr: 0x%02x, len: 0x%02x",
				desc->address, desc->attr.val, desc->len);
		}
		LOG_DBG("adma_desc_tbl: 0x%04lx", (uintptr_t)sdhc_data->adma_desc_tbl);
		regs->adma_sys_addr1 = (uintptr_t)sdhc_data->adma_desc_tbl;
#if defined(CONFIG_64BIT)
		bool is_hc_v3 = (regs->host_ctrl_version == ZYNQ_SDHC_HC_SPEC_V3);
		if (is_hc_v3) {
			regs->adma_sys_addr2 =
				(uint32_t)(((uint64_t)sdhc_data->adma_desc_tbl) >> 32U);
		}
#endif

		sys_cache_data_flush_range(sdhc_data->adma_desc_tbl,
					   sizeof(sdhc_data->adma_desc_tbl));

	} else {
		/* Setup DMA transfer using SDMA */
		// regs->sdma_sysaddr = (uint32_t)((uint64_t)data->data);
#if defined(CONFIG_64BIT)
		regs->sdma_sysaddr = (uint32_t)(((uint64_t)data->data));
#else
		regs->sdma_sysaddr = (uint32_t)(data->data);
#endif
	}
	return 0;
}

uint16_t zynq_sdhc_generate_transfer_mode(const struct device *dev, struct sdhc_data *data,
					  bool read)
{
	struct zynq_sdhc_data *sdhc_data = dev->data;
	uint16_t multi_block = 0u;
	if (data->blocks > 1) {
		multi_block = 1u;
	}
	uint16_t transfer_mode = 0;

	// bit 2
	if (IS_ENABLED(CONFIG_XLNX_ZYNQ_SDHC_HOST_AUTO_STOP)) {
		if ((IS_ENABLED(CONFIG_XLNX_ZYNQ_SDHC_HOST_ADMA) &&
		     sdhc_data->host_io.timing == SDHC_TIMING_SDR104)) { // NOLINT

			/* Auto cmd23 only applicable for ADMA */
			SET_BITS(transfer_mode, ZYNQ_SDHC_HOST_XFER_AUTO_CMD_EN_LOC,
				 ZYNQ_SDHC_HOST_XFER_AUTO_CMD_EN_MASK, multi_block ? 2 : 0);
		} else {
			SET_BITS(transfer_mode, ZYNQ_SDHC_HOST_XFER_AUTO_CMD_EN_LOC,
				 ZYNQ_SDHC_HOST_XFER_AUTO_CMD_EN_MASK, multi_block ? 1 : 0);
		}
	} else {
		SET_BITS(transfer_mode, ZYNQ_SDHC_HOST_XFER_AUTO_CMD_EN_LOC,
			 ZYNQ_SDHC_HOST_XFER_AUTO_CMD_EN_MASK, 0);
	}

	// bit 1
	if (IS_ENABLED(CONFIG_XLNX_ZYNQ_SDHC_HOST_AUTO_STOP)) {
		SET_BITS(transfer_mode, ZYNQ_SDHC_HOST_XFER_BLOCK_CNT_EN_LOC,
			 ZYNQ_SDHC_HOST_XFER_BLOCK_CNT_EN_MASK, multi_block ? 1 : 0);
		/* Enable block count in transfer register */
	} else {
		/* Set block count register to 0 for infinite transfer mode */
		SET_BITS(transfer_mode, ZYNQ_SDHC_HOST_XFER_BLOCK_CNT_EN_LOC,
			 ZYNQ_SDHC_HOST_XFER_BLOCK_CNT_EN_MASK, 0);
	}

	// bit 5
	SET_BITS(transfer_mode, ZYNQ_SDHC_HOST_XFER_MULTI_BLOCK_SEL_LOC,
		 ZYNQ_SDHC_HOST_XFER_MULTI_BLOCK_SEL_MASK, multi_block);

	// bit 4
	/* Set data transfer direction, Read = 1, Write = 0 */
	SET_BITS(transfer_mode, ZYNQ_SDHC_HOST_XFER_DATA_DIR_LOC, ZYNQ_SDHC_HOST_XFER_DATA_DIR_MASK,
		 read ? 1u : 0u);

	// bit 0
	if (IS_ENABLED(CONFIG_XLNX_ZYNQ_SDHC_HOST_DMA)) {
		/* Enable DMA or not */
		SET_BITS(transfer_mode, ZYNQ_SDHC_HOST_XFER_DMA_EN_LOC,
			 ZYNQ_SDHC_HOST_XFER_DMA_EN_MASK, 1u);
	} else {
		SET_BITS(transfer_mode, ZYNQ_SDHC_HOST_XFER_DMA_EN_LOC,
			 ZYNQ_SDHC_HOST_XFER_DMA_EN_MASK, 0u);
	}

	return transfer_mode;
}

static int zynq_sdhc_init_xfr(const struct device *dev, struct sdhc_data *data, bool read)
{
	volatile struct zynq_sdhc_reg *regs = DEV_REG(dev);
	uint16_t transfer_mode = 0;
	struct zynq_sdhc_data *sdhc_data = dev->data;

	if (IS_ENABLED(CONFIG_XLNX_ZYNQ_SDHC_HOST_DMA)) {
		zynq_sdhc_dma_init(dev, data, read);
		sdhc_data->xfer_flag = 1;
	}

	if (IS_ENABLED(CONFIG_XLNX_ZYNQ_SDHC_HOST_ADMA)) {
		SET_BITS(regs->host_ctrl1, ZYNQ_SDHC_HOST_CTRL1_DMA_SEL_LOC,
			 ZYNQ_SDHC_HOST_CTRL1_DMA_SEL_MASK, 2U);
	} else {
		SET_BITS(regs->host_ctrl1, ZYNQ_SDHC_HOST_CTRL1_DMA_SEL_LOC,
			 ZYNQ_SDHC_HOST_CTRL1_DMA_SEL_MASK, 0U);
	}
	/* set block size register */
	zynq_sdhc_write16(dev, XSDPS_BLK_SIZE_OFFSET, (data->block_size & XSDPS_BLK_SIZE_MASK));

	transfer_mode = zynq_sdhc_generate_transfer_mode(dev, data, read);
	zynq_sdhc_write16(dev, XSDPS_XFER_MODE_OFFSET, transfer_mode);

	if (!IS_ENABLED(CONFIG_XLNX_ZYNQ_SDHC_HOST_AUTO_STOP)) {
		/* Set block count register to 0 for infinite transfer mode */
		zynq_sdhc_write16(dev, XSDPS_BLK_CNT_OFFSET, 0);
	} else {
		zynq_sdhc_write16(dev, XSDPS_BLK_CNT_OFFSET, (data->blocks & XSDPS_BLK_CNT_MASK));
	}

	if (IS_ENABLED(CONFIG_XLNX_ZYNQ_SDHC_HOST_BLOCK_GAP)) {
		/* Set an interrupt at the block gap */
		zynq_sdhc_write8(dev, XSDPS_BLK_GAP_CTRL_OFFSET, 1u);
	} else {
		zynq_sdhc_write8(dev, XSDPS_BLK_GAP_CTRL_OFFSET, 0u);
	}

	/* Set data timeout time */
	uint16_t timeout_val = zynq_sdhc_calc_timeout(dev, data->timeout_ms);
	zynq_sdhc_write16(dev, XSDPS_TIMEOUT_CTRL_OFFSET, timeout_val);

	return 0;
}

static int wait_xfr_intr_complete(const struct device *dev, uint32_t time_out)
{
	struct zynq_sdhc_data *emmc = dev->data;
	uint32_t events = 0;
	int ret;
	k_timeout_t wait_time;

	LOG_DBG("");

	if (time_out == SDHC_TIMEOUT_FOREVER) {
		wait_time = K_FOREVER;
	} else {
		wait_time = K_MSEC(time_out);
	}

	if (IS_ENABLED(CONFIG_XLNX_ZYNQ_SDHC_HOST_INTR)) {
		events = k_event_wait(&emmc->irq_event,
				      ZYNQ_SDHC_HOST_XFER_COMPLETE |
					      ERR_INTR_STATUS_EVENT(ZYNQ_SDHC_HOST_DMA_TXFR_ERR),
				      false, wait_time);
	}

	if (events & ZYNQ_SDHC_HOST_XFER_COMPLETE) {
		ret = 0;
	} else if (events & ERR_INTR_STATUS_EVENT(0xFFFF)) {
		LOG_ERR("wait for xfer complete error: %x", events);
		ret = -EIO;
	} else {
		LOG_ERR("wait for xfer complete timeout");
		ret = -EAGAIN;
	}

	return ret;
}

static int wait_xfr_poll_complete(const struct device *dev, uint32_t time_out)
{
	volatile struct zynq_sdhc_reg *regs = DEV_REG(dev);
	int ret = -EAGAIN;
	int32_t retry = (int32_t)time_out;

	LOG_DBG("");

	while (retry > 0) {
		if (regs->normal_int_stat & ZYNQ_SDHC_HOST_XFER_COMPLETE) {
			regs->normal_int_stat |= ZYNQ_SDHC_HOST_XFER_COMPLETE;
			ret = 0;
			break;
		}

		k_busy_wait(ZYNQ_SDHC_HOST_MSEC_DELAY);
		retry--;
	}

	return ret;
}

static int wait_xfr_complete(const struct device *dev, uint32_t time_out)
{
	int ret;

	if (IS_ENABLED(CONFIG_XLNX_ZYNQ_SDHC_HOST_INTR)) {
		ret = wait_xfr_intr_complete(dev, time_out);
	} else {
		ret = wait_xfr_poll_complete(dev, time_out);
	}
	return ret;
}

static enum zynq_sdhc_resp_type zynq_sdhc_decode_resp_type(enum sd_rsp_type type)
{
	enum zynq_sdhc_resp_type resp_type = ZYNQ_SDHC_HOST_RESP_NONE;
	// we only take the lower 4 bits, the upper 4 bits is used for spi mode
	uint8_t resp_type_sel = type & 0xF;

	switch (resp_type_sel) {
	case SD_RSP_TYPE_NONE:
		resp_type = ZYNQ_SDHC_HOST_RESP_NONE;
		break;
	case SD_RSP_TYPE_R1:
	case SD_RSP_TYPE_R3:
	case SD_RSP_TYPE_R4:
	case SD_RSP_TYPE_R5:
		resp_type = ZYNQ_SDHC_HOST_RESP_LEN_48;
		break;
	case SD_RSP_TYPE_R1b:
		resp_type = ZYNQ_SDHC_HOST_RESP_LEN_48_BUSY;
		break;
	case SD_RSP_TYPE_R2:
		resp_type = ZYNQ_SDHC_HOST_RESP_LEN_136;
		break;

	case SD_RSP_TYPE_R5b:
	case SD_RSP_TYPE_R6:
	case SD_RSP_TYPE_R7:
	default:
		resp_type = ZYNQ_SDHC_HOST_INVAL_HOST_RESP;
	}

	return resp_type;
}

#if defined(CONFIG_XLNX_ZYNQ_SDHC_HOST_INTR)
static int wait_for_cmd_complete(struct zynq_sdhc_data *sdhc_data, uint32_t time_out)
{
	int ret;
	k_timeout_t wait_time;
	uint32_t events;

	if (time_out == SDHC_TIMEOUT_FOREVER) {
		wait_time = K_FOREVER;
	} else {
		wait_time = K_MSEC(time_out);
	}

	events = k_event_wait(&sdhc_data->irq_event,
			      ZYNQ_SDHC_HOST_CMD_COMPLETE |
				      ERR_INTR_STATUS_EVENT(ZYNQ_SDHC_HOST_ERR_STATUS),
			      false, wait_time);

	if (events & ZYNQ_SDHC_HOST_CMD_COMPLETE) {
		ret = 0;
	} else if (events & ERR_INTR_STATUS_EVENT(ZYNQ_SDHC_HOST_ERR_STATUS)) {
		LOG_ERR("wait for cmd complete error: %x", events);
		ret = -EIO;
	} else {
		LOG_ERR("wait for cmd complete timeout");
		ret = -EAGAIN;
	}

	return ret;
}
#else
static int poll_cmd_complete(const struct device *dev, uint32_t time_out)
{
	int ret = -EAGAIN;
	uint32_t norm_and_err_int_stat = 0;
	struct zynq_sdhc_data *sdhc_data = dev->data;
	uint32_t t0 = time_out;
#if defined(CONFIG_XLNX_ZYNQ_SDHC_HOST_ADMA)
	uint32_t t1 = time_out;
	uint8_t adma_err_stat = 0;
#endif

	while (t0 > 0) {
		// FIXME(pan), every adma desc xfer would gen a tc signal, we shall add a flag to
		// avoid missmatch
		norm_and_err_int_stat = zynq_sdhc_read32(dev, XSDPS_NORM_INTR_STS_OFFSET);
		uint16_t norm_int_stat = norm_and_err_int_stat & 0xFFFF;
		if (norm_int_stat & ZYNQ_SDHC_HOST_CMD_COMPLETE) { // TC
			zynq_sdhc_write16(dev, XSDPS_NORM_INTR_STS_OFFSET,
					  ZYNQ_SDHC_HOST_CMD_COMPLETE);
			ret = 0;
			break;
		}
		k_busy_wait(1000u);
		t0--;
	}

	if (time_out == 0) {
		LOG_ERR("nom_err: tc timeout");
	}

	bool err_irq = (norm_and_err_int_stat & BIT(15)) != 0;
	if (err_irq) {
		uint16_t err_int_stat = (norm_and_err_int_stat >> 16u) & 0xFFu;
		LOG_ERR("err_int_stat: 0x%02x", err_int_stat);
		// clear error status
		zynq_sdhc_write16(dev, XSDPS_ERR_INTR_STS_OFFSET, err_int_stat);
		ret = -EIO;
	}

	if (IS_ENABLED(CONFIG_XLNX_ZYNQ_SDHC_HOST_ADMA)) {
		if (sdhc_data->xfer_flag) {
			while (t1 > 0) {
				adma_err_stat = zynq_sdhc_read8(dev, XSDPS_ADMA_ERR_STS_OFFSET);
				if ((adma_err_stat & ZYNQ_SDHC_HOST_ADMA_ERR_MASK) == 0) {
					break;
				}

				k_busy_wait(1000u);
				t1--;
			}
			if (adma_err_stat) {
				LOG_ERR("adma error: %x", adma_err_stat);
				ret = -EIO;
			}
		}
		sdhc_data->xfer_flag = 0;
	}

	return ret;
}
#endif

static void update_cmd_response(const struct device *dev, struct sdhc_command *sdhc_cmd) // NOLINT
{
	if (sdhc_cmd->response_type == SD_RSP_TYPE_NONE) {
		return;
	}
	uint32_t resp0 = zynq_sdhc_read32(dev, XSDPS_RESP0_OFFSET);

	if (sdhc_cmd->response_type == SD_RSP_TYPE_R2) {
		uint32_t resp1 = zynq_sdhc_read32(dev, XSDPS_RESP1_OFFSET);
		uint32_t resp2 = zynq_sdhc_read32(dev, XSDPS_RESP2_OFFSET);
		uint32_t resp3 = zynq_sdhc_read32(dev, XSDPS_RESP3_OFFSET);

		// LOG_DBG("cmd resp: %x %x %x %x", resp0, resp1, resp2, resp3);
		// memcpy((((uint8_t *)sdhc_cmd->response) + 1),
		//        (const void *)(DEVICE_MMIO_GET(dev) + XSDPS_RESP0_OFFSET),
		//        (sizeof(sdhc_cmd->response) - 1));
		sdhc_cmd->response[3u] = (resp3 << 8) | (resp2 >> 24);
		sdhc_cmd->response[2u] = (resp2 << 8) | (resp1 >> 24);
		sdhc_cmd->response[1u] = (resp1 << 8) | (resp0 >> 24);
		sdhc_cmd->response[0u] = (resp0 << 8);

	} else {
		// uint32_t resp0 = zynq_sdhc_read32(dev, XSDPS_RESP0_OFFSET);
		LOG_DBG("cmd resp: %x", resp0);
		sdhc_cmd->response[0u] = resp0;
	}
}

static int zynq_sdhc_host_send_cmd(const struct device *dev,
				   const struct zynq_sdhc_cmd_config *config)
{

	uintptr_t reg_base = DEVICE_MMIO_GET(dev);
	volatile struct zynq_sdhc_reg *regs = (volatile struct zynq_sdhc_reg *)reg_base;
	struct zynq_sdhc_data *sdhc = dev->data;
	struct sdhc_command *sdhc_cmd = config->sdhc_cmd;
	enum zynq_sdhc_resp_type resp_type_select =
		zynq_sdhc_decode_resp_type(sdhc_cmd->response_type);
	uint32_t cmd_reg;
	int ret;

	LOG_DBG("");

	/* Check if CMD line is available */
	if (regs->present_state & ZYNQ_SDHC_HOST_PSTATE_CMD_INHIBIT) {
		LOG_ERR("CMD line is not available");
		return -EBUSY;
	}

	if (config->data_present && (regs->present_state & ZYNQ_SDHC_HOST_PSTATE_DAT_INHIBIT)) {
		LOG_ERR("Data line is not available");
		return -EBUSY;
	}

	if (resp_type_select == ZYNQ_SDHC_HOST_INVAL_HOST_RESP) {
		LOG_ERR("Invalid eMMC resp type:%d", resp_type_select);
		return -EINVAL;
	}

	if (IS_ENABLED(CONFIG_XLNX_ZYNQ_SDHC_HOST_INTR)) {
		k_event_clear(&sdhc->irq_event, ZYNQ_SDHC_HOST_CMD_COMPLETE);
	}
	zynq_sdhc_write32(dev, XSDPS_ARGMT_OFFSET, sdhc_cmd->arg);

	cmd_reg = (config->cmd_idx << ZYNQ_SDHC_HOST_CMD_INDEX_LOC) |
		  (config->cmd_type << ZYNQ_SDHC_HOST_CMD_TYPE_LOC) |
		  (config->data_present << ZYNQ_SDHC_HOST_CMD_DATA_PRESENT_LOC) |
		  (config->idx_check_en << ZYNQ_SDHC_HOST_CMD_IDX_CHECK_EN_LOC) |
		  (config->crc_check_en << ZYNQ_SDHC_HOST_CMD_CRC_CHECK_EN_LOC) |
		  (resp_type_select << ZYNQ_SDHC_HOST_CMD_RESP_TYPE_LOC);
	zynq_sdhc_write16(dev, XSDPS_CMD_OFFSET, cmd_reg);

	LOG_DBG("CMD REG:%x %x", cmd_reg, regs->cmd);

#if defined(CONFIG_XLNX_ZYNQ_SDHC_HOST_INTR)
	ret = wait_for_cmd_complete(sdhc, sdhc_cmd->timeout_ms);
#else
	ret = poll_cmd_complete(dev, sdhc_cmd->timeout_ms);
#endif
	if (ret) {
		LOG_ERR("Error on send cmd: %d, status:%d, cmd_raw: 0x%02x", config->cmd_idx, ret,
			cmd_reg);
		return ret;
	}

	update_cmd_response(dev, sdhc_cmd);

	return 0;
}

static int zynq_sdhc_send_cmd_no_data(const struct device *dev, struct sdhc_command *cmd)
{
	struct zynq_sdhc_cmd_config sdhc_cmd = {0};

	sdhc_cmd.sdhc_cmd = cmd;
	sdhc_cmd.cmd_idx = cmd->opcode;
	sdhc_cmd.cmd_type = ZYNQ_SDHC_HOST_CMD_NORMAL;
	sdhc_cmd.data_present = false;
	sdhc_cmd.idx_check_en = false;
	sdhc_cmd.crc_check_en = false;

	return zynq_sdhc_host_send_cmd(dev, &sdhc_cmd);
}

static int zynq_sdhc_send_cmd_data(const struct device *dev, struct sdhc_command *cmd,
				   struct sdhc_data *data, bool read)
{
	struct zynq_sdhc_cmd_config cmd_config;
	int ret;

	cmd_config.sdhc_cmd = cmd;
	cmd_config.cmd_idx = cmd->opcode;
	cmd_config.cmd_type = ZYNQ_SDHC_HOST_CMD_NORMAL;
	cmd_config.data_present = true;
	cmd_config.idx_check_en = true;
	cmd_config.crc_check_en = true;

	ret = zynq_sdhc_init_xfr(dev, data, read);
	if (ret) {
		LOG_ERR("Error on init xfr");
		return ret;
	}

	ret = zynq_sdhc_host_send_cmd(dev, &cmd_config);
	if (ret) {
		return ret;
	}

	if (IS_ENABLED(CONFIG_XLNX_ZYNQ_SDHC_HOST_DMA)) {
		ret = wait_xfr_complete(dev, data->timeout_ms);
	} else {
		if (read) {
			ret = read_data_port(dev, data);
		} else {
			ret = write_data_port(dev, data);
		}
	}

	return ret;
}

static int read_data_port(const struct device *dev, struct sdhc_data *sdhc)
{
	struct zynq_sdhc_data *emmc = dev->data;
	volatile struct zynq_sdhc_reg *regs = DEV_REG(dev);
	uint32_t block_size = sdhc->block_size;
	uint32_t i, block_cnt = sdhc->blocks;
	uint32_t *data = (uint32_t *)sdhc->data;
	k_timeout_t wait_time;

	if (sdhc->timeout_ms == SDHC_TIMEOUT_FOREVER) {
		wait_time = K_FOREVER;
	} else {
		wait_time = K_MSEC(sdhc->timeout_ms);
	}

	LOG_DBG("");

	while (block_cnt--) {
		if (IS_ENABLED(CONFIG_XLNX_ZYNQ_SDHC_HOST_INTR)) {
			uint32_t events;

			events = k_event_wait(&emmc->irq_event, ZYNQ_SDHC_HOST_BUF_RD_READY, false,
					      wait_time);
			k_event_clear(&emmc->irq_event, ZYNQ_SDHC_HOST_BUF_RD_READY);
			if (!(events & ZYNQ_SDHC_HOST_BUF_RD_READY)) {
				LOG_ERR("time out on ZYNQ_SDHC_HOST_BUF_RD_READY:%d",
					(sdhc->blocks - block_cnt));
				return -EIO;
			}
		} else {
			while ((regs->present_state & ZYNQ_SDHC_HOST_PSTATE_BUF_READ_EN) == 0) {
				;
			}
		}

		if (regs->present_state & ZYNQ_SDHC_HOST_PSTATE_DAT_INHIBIT) {
			for (i = block_size >> 2u; i != 0u; i--) {
				*data = regs->data_port;
				data++;
			}
		}
	}

	return wait_xfr_complete(dev, sdhc->timeout_ms);
}

static int write_data_port(const struct device *dev, struct sdhc_data *sdhc)
{
	struct zynq_sdhc_data *emmc = dev->data;
	volatile struct zynq_sdhc_reg *regs = DEV_REG(dev);
	uint32_t block_size = sdhc->block_size;
	uint32_t i, block_cnt = sdhc->blocks;
	uint32_t *data = (uint32_t *)sdhc->data;
	k_timeout_t wait_time;

	if (sdhc->timeout_ms == SDHC_TIMEOUT_FOREVER) {
		wait_time = K_FOREVER;
	} else {
		wait_time = K_MSEC(sdhc->timeout_ms);
	}

	LOG_DBG("");

	while ((regs->present_state & ZYNQ_SDHC_HOST_PSTATE_BUF_WRITE_EN) == 0) {
		;
	}

	while (1) {
		uint32_t events;

		if (IS_ENABLED(CONFIG_XLNX_ZYNQ_SDHC_HOST_INTR)) {
			k_event_clear(&emmc->irq_event, ZYNQ_SDHC_HOST_BUF_WR_READY);
		}

		if (regs->present_state & ZYNQ_SDHC_HOST_PSTATE_DAT_INHIBIT) {
			for (i = block_size >> 2u; i != 0u; i--) {
				regs->data_port = *data;
				data++;
			}
		}

		LOG_DBG("ZYNQ_SDHC_HOST_BUF_WR_READY");

		if (!(--block_cnt)) {
			break;
		}
		if (IS_ENABLED(CONFIG_XLNX_ZYNQ_SDHC_HOST_INTR)) {
			events = k_event_wait(&emmc->irq_event, ZYNQ_SDHC_HOST_BUF_WR_READY, false,
					      wait_time);
			k_event_clear(&emmc->irq_event, ZYNQ_SDHC_HOST_BUF_WR_READY);

			if (!(events & ZYNQ_SDHC_HOST_BUF_WR_READY)) {
				LOG_ERR("time out on ZYNQ_SDHC_HOST_BUF_WR_READY");
				return -EIO;
			}
		} else {
			while ((regs->present_state & ZYNQ_SDHC_HOST_PSTATE_BUF_WRITE_EN) == 0) {
				;
			}
		}
	}

	return wait_xfr_complete(dev, sdhc->timeout_ms);
}

static int zynq_sdhc_stop_transfer(const struct device *dev)
{
	struct zynq_sdhc_data *emmc = dev->data;
	struct sdhc_command hdc_cmd = {0};
	struct zynq_sdhc_cmd_config cmd;

	hdc_cmd.arg = emmc->rca << ZYNQ_SDHC_HOST_RCA_SHIFT;
	hdc_cmd.response_type = SD_RSP_TYPE_R1;
	hdc_cmd.timeout_ms = 1000;

	cmd.sdhc_cmd = &hdc_cmd;
	cmd.cmd_idx = SD_STOP_TRANSMISSION;
	cmd.cmd_type = ZYNQ_SDHC_HOST_CMD_NORMAL;
	cmd.data_present = false;
	cmd.idx_check_en = false;
	cmd.crc_check_en = false;

	return zynq_sdhc_host_send_cmd(dev, &cmd);
}

static int zynq_sdhc_xfr(const struct device *dev, struct sdhc_command *cmd, struct sdhc_data *data,
			 bool read)
{
	struct zynq_sdhc_data *sdhc_data = dev->data;
	int ret = 0;
	struct zynq_sdhc_cmd_config zynq_sdhc_cmd;
	// struct
	ret = zynq_sdhc_init_xfr(dev, data, read);
	if (ret) {
		LOG_ERR("error emmc init xfr");
		return ret;
	}

	zynq_sdhc_cmd.sdhc_cmd = cmd;
	zynq_sdhc_cmd.cmd_type = ZYNQ_SDHC_HOST_CMD_NORMAL;
	zynq_sdhc_cmd.data_present = true;
	zynq_sdhc_cmd.idx_check_en = true;
	zynq_sdhc_cmd.crc_check_en = true;

	if (IS_ENABLED(CONFIG_XLNX_ZYNQ_SDHC_HOST_INTR)) {
		k_event_clear(&sdhc_data->irq_event, ZYNQ_SDHC_HOST_XFER_COMPLETE);
		k_event_clear(&sdhc_data->irq_event,
			      read ? ZYNQ_SDHC_HOST_BUF_RD_READY : ZYNQ_SDHC_HOST_BUF_WR_READY);
	}

	if (data->blocks > 1) {
		zynq_sdhc_cmd.cmd_idx = read ? SD_READ_MULTIPLE_BLOCK : SD_WRITE_MULTIPLE_BLOCK;
		ret = zynq_sdhc_host_send_cmd(dev, &zynq_sdhc_cmd);
	} else {
		zynq_sdhc_cmd.cmd_idx = read ? SD_READ_SINGLE_BLOCK : SD_WRITE_SINGLE_BLOCK;
		ret = zynq_sdhc_host_send_cmd(dev, &zynq_sdhc_cmd);
	}

	if (ret) {
		return ret;
	}

	if (IS_ENABLED(CONFIG_XLNX_ZYNQ_SDHC_HOST_DMA)) {
		ret = wait_xfr_complete(dev, data->timeout_ms);
	} else {
		if (read) {
			ret = read_data_port(dev, data);
		} else {
			ret = write_data_port(dev, data);
		}
	}

	if (!IS_ENABLED(CONFIG_XLNX_ZYNQ_SDHC_HOST_AUTO_STOP)) {
		zynq_sdhc_stop_transfer(dev);
	}
	return ret;
}

/**
 * @brief RESET the SDHC controller
 *
 * @param dev
 * @return int
 */
static int zynq_sdhc_reset(const struct device *dev)
{
	volatile struct zynq_sdhc_reg *regs = DEV_REG(dev);
	int ret = 0;

	if (!(regs->present_state & ZYNQ_SDHC_HOST_PSTATE_CARD_INSERTED)) {
		LOG_ERR("No card inserted");
		return -ENODEV;
	}

	ret = zynq_sdhc_host_sw_reset(dev, ZYNQ_SDHC_HOST_SWRST_ALL);
	if (ret != 0) {
		return ret;
	}

	clear_interrupts(dev);

	if (IS_ENABLED(CONFIG_XLNX_ZYNQ_SDHC_HOST_INTR)) {
		configure_status_interrupts_enable_signals(dev);
	} else {
		configure_status_interrupts_disable_signals(dev);
	}

	return 0;
}

static int zynq_sdhc_request(const struct device *dev, struct sdhc_command *cmd,
			     struct sdhc_data *data)
{
	int ret = 0;

	if (data) {
		switch (cmd->opcode) {
		case SD_WRITE_SINGLE_BLOCK:
		case SD_WRITE_MULTIPLE_BLOCK:
			LOG_DBG("SD_WRITE_SINGLE_BLOCK");
			ret = zynq_sdhc_xfr(dev, cmd, data, false);
			break;

		case SD_READ_SINGLE_BLOCK:
		case SD_READ_MULTIPLE_BLOCK:
			LOG_DBG("SD_READ_SINGLE_BLOCK");
			ret = zynq_sdhc_xfr(dev, cmd, data, true);
			break;

		case MMC_SEND_EXT_CSD:
			LOG_DBG("EMMC_HOST_SEND_EXT_CSD");
			ret = zynq_sdhc_send_cmd_data(dev, cmd, data, true);
			break;

		default:
			ret = zynq_sdhc_send_cmd_data(dev, cmd, data, true);
		}
	} else {
		ret = zynq_sdhc_send_cmd_no_data(dev, cmd);
	}

	return ret;
}

static int zynq_sdhc_set_io(const struct device *dev, struct sdhc_io *ios)
{
	struct zynq_sdhc_data *data = dev->data;
	volatile struct zynq_sdhc_reg *regs = DEV_REG(dev);
	struct sdhc_io *host_io = &data->host_io;
	int ret = 0;
	uint32_t tgt_freq = zynq_get_clock_speed(ios->clock);

	LOG_DBG("emmc I/O: DW %d, Clk %d Hz, card power state %s, voltage %s", ios->bus_width,
		ios->clock, ios->power_mode == SDHC_POWER_ON ? "ON" : "OFF",
		ios->signal_voltage == SD_VOL_1_8_V ? "1.8V" : "3.3V");

	if (tgt_freq && (tgt_freq > data->props.f_max || tgt_freq < data->props.f_min)) {
		LOG_ERR("Invalid argument for clock freq: %d Support max:%d and Min:%d", ios->clock,
			data->props.f_max, data->props.f_min);
		return -EINVAL;
	}

	/* Set HC clock */
	if (host_io->clock != ios->clock) {
		LOG_DBG("Clock: %d", host_io->clock);
		if (ios->clock != 0) {
			/* Enable clock */
			LOG_DBG("CLOCK: %d", ios->clock);
			if (!zynq_sdhc_clock_set(dev, ios->clock)) {
				return -ENOTSUP;
			}
		} else {
			zynq_sdhc_disable_clock(dev);
		}
		host_io->clock = ios->clock;
	}

	/* Set data width */
	if (host_io->bus_width != ios->bus_width) {
		LOG_DBG("bus_width: %d", host_io->bus_width);
		bool bus_width_gte = data->bus_width >= ios->bus_width;
		if (bus_width_gte) {
			if (ios->bus_width == SDHC_BUS_WIDTH8BIT) {
				SET_BITS(regs->host_ctrl1, ZYNQ_SDHC_HOST_CTRL1_EXT_DAT_WIDTH_LOC,
					 ZYNQ_SDHC_HOST_CTRL1_EXT_DAT_WIDTH_MASK,
					 ios->bus_width == SDHC_BUS_WIDTH8BIT ? 1 : 0);
			} else {
				SET_BITS(regs->host_ctrl1, ZYNQ_SDHC_HOST_CTRL1_DAT_WIDTH_LOC,
					 ZYNQ_SDHC_HOST_CTRL1_DAT_WIDTH_MASK,
					 ios->bus_width == SDHC_BUS_WIDTH4BIT ? 1 : 0);
			}
			host_io->bus_width = ios->bus_width;
		} else {
			return -ENOTSUP;
		}
	}

	/* Set HC signal voltage */
	if (ios->signal_voltage != host_io->signal_voltage) {
		LOG_DBG("signal_voltage: %d", ios->signal_voltage);
		ret = zynq_sdhc_set_voltage(dev, ios->signal_voltage);
		if (ret) {
			LOG_ERR("Set signal volatge failed:%d", ret);
			return ret;
		}
		host_io->signal_voltage = ios->signal_voltage;
	}

	/* Set card power */
	if (host_io->power_mode != ios->power_mode) {
		LOG_DBG("power_mode: %d", ios->power_mode);

		ret = zynq_sdhc_set_power(dev, ios->power_mode);
		if (ret) {
			LOG_ERR("Set Bus power failed:%d", ret);
			return ret;
		}
		host_io->power_mode = ios->power_mode;
	}

	/* Set I/O timing */
	if (host_io->timing != ios->timing) {
		if (data->hc_ver == SD_SPEC_VER3_0) {
			LOG_DBG("timing: %d", ios->timing);

			ret = set_timing(dev, ios->timing);
			if (ret) {
				LOG_ERR("Set timing failed:%d", ret);
				return ret;
			}
		}
		host_io->timing = ios->timing;
	}

	return ret;
}

static int zynq_sdhc_get_card_present(const struct device *dev)
{
	struct zynq_sdhc_data *sdhc_data = dev->data;
	volatile struct zynq_sdhc_reg *regs = DEV_REG(dev);

	sdhc_data->card_present = (bool)((regs->present_state >> 16U) & 1u);
	if (!sdhc_data->card_present) {
		LOG_ERR("No card inserted");
	}

	return ((int)sdhc_data->card_present);
}

static int zynq_sdhc_execute_tuning(const struct device *dev) // NOLINT
{
	if (IS_ENABLED(CONFIG_XLNX_ZYNQ_SDHC_HOST_TUNING)) {
		volatile struct zynq_sdhc_reg *regs = DEV_REG(dev);

		LOG_DBG("Executing tuning...");
		regs->host_ctrl2 |= ZYNQ_SDHC_HOST_START_TUNING;
		while (!(regs->host_ctrl2 & ZYNQ_SDHC_HOST_START_TUNING)) {
			;
		}

		if (regs->host_ctrl2 & ZYNQ_SDHC_HOST_TUNING_SUCCESS) {
			LOG_DBG("Tuning Completed successful");
		} else {
			LOG_ERR("Tuning Failed");
			return -EIO;
		}
	}

	return 0;
}

static int zynq_sdhc_card_busy(const struct device *dev)
{
	volatile struct zynq_sdhc_reg *regs = DEV_REG(dev);
	if (regs->present_state & 7U) {
		return 1;
	}
	return 0;
}

static int zynq_sdhc_get_host_props(const struct device *dev, struct sdhc_host_props *props)
{
	const struct zynq_sdhc_config *cfg = dev->config;
	struct zynq_sdhc_data *data = dev->data;
	uint64_t cap = zynq_sdhc_read64(dev, XSDPS_CAPS_OFFSET);
	data->host_caps = cap;

	memset(props, 0, sizeof(struct sdhc_host_props));
	props->f_max = cfg->max_bus_freq;
	props->f_min = cfg->min_bus_freq;
	props->power_delay = cfg->power_delay_ms;

	props->host_caps.vol_180_support = (bool)(cap & BIT(26U));
	props->host_caps.vol_300_support = (bool)(cap & BIT(25U));
	props->host_caps.vol_330_support = (bool)(cap & BIT(24U));
	props->host_caps.suspend_res_support = false;
	props->host_caps.sdma_support = (bool)(cap & BIT(22U));
	props->host_caps.high_spd_support = (bool)(cap & BIT(21U));
	props->host_caps.adma_2_support = (bool)(cap & BIT(19U));
	// FIXME(pan): may differ behavior for v3/v2
	props->host_caps.max_blk_len = (cap >> 16U) & (0x3U);
	props->host_caps.ddr50_support = (bool)(cap & BIT64(34U));
	props->host_caps.sdr104_support = (bool)(cap & BIT64(33U));
	props->host_caps.sdr50_support = (bool)(cap & BIT64(32U));
	props->host_caps.bus_8_bit_support = data->bus_width == SDHC_BUS_WIDTH8BIT;
	props->host_caps.bus_4_bit_support = data->bus_width == SDHC_BUS_WIDTH4BIT;
	props->host_caps.hs200_support = (bool)(cfg->hs200_mode);
	props->host_caps.hs400_support = (bool)(cfg->hs400_mode);

	data->props = *props;

	return 0;
}

static void zynq_sdhc_isr(const struct device *dev)
{
	struct zynq_sdhc_data *emmc = dev->data;
	volatile struct zynq_sdhc_reg *regs = DEV_REG(dev);

	if (regs->normal_int_stat & ZYNQ_SDHC_HOST_CMD_COMPLETE) {
		regs->normal_int_stat |= ZYNQ_SDHC_HOST_CMD_COMPLETE;
		if (IS_ENABLED(CONFIG_XLNX_ZYNQ_SDHC_HOST_INTR)) {
			k_event_post(&emmc->irq_event, ZYNQ_SDHC_HOST_CMD_COMPLETE);
		}
	}

	if (regs->normal_int_stat & ZYNQ_SDHC_HOST_XFER_COMPLETE) {
		regs->normal_int_stat |= ZYNQ_SDHC_HOST_XFER_COMPLETE;
		if (IS_ENABLED(CONFIG_XLNX_ZYNQ_SDHC_HOST_INTR)) {
			k_event_post(&emmc->irq_event, ZYNQ_SDHC_HOST_XFER_COMPLETE);
		}
	}

	if (regs->normal_int_stat & ZYNQ_SDHC_HOST_DMA_INTR) {
		regs->normal_int_stat |= ZYNQ_SDHC_HOST_DMA_INTR;
		if (IS_ENABLED(CONFIG_XLNX_ZYNQ_SDHC_HOST_INTR)) {
			k_event_post(&emmc->irq_event, ZYNQ_SDHC_HOST_DMA_INTR);
		}
	}

	if (regs->normal_int_stat & ZYNQ_SDHC_HOST_BUF_WR_READY) {
		regs->normal_int_stat |= ZYNQ_SDHC_HOST_BUF_WR_READY;
		if (IS_ENABLED(CONFIG_XLNX_ZYNQ_SDHC_HOST_INTR)) {
			k_event_post(&emmc->irq_event, ZYNQ_SDHC_HOST_BUF_WR_READY);
		}
	}

	if (regs->normal_int_stat & ZYNQ_SDHC_HOST_BUF_RD_READY) {
		regs->normal_int_stat |= ZYNQ_SDHC_HOST_BUF_RD_READY;
		if (IS_ENABLED(CONFIG_XLNX_ZYNQ_SDHC_HOST_INTR)) {
			k_event_post(&emmc->irq_event, ZYNQ_SDHC_HOST_BUF_RD_READY);
		}
	}

	if (regs->err_int_stat) {
		LOG_ERR("err int:%x", regs->err_int_stat);
		if (IS_ENABLED(CONFIG_XLNX_ZYNQ_SDHC_HOST_INTR)) {
			k_event_post(&emmc->irq_event, ERR_INTR_STATUS_EVENT(regs->err_int_stat));
		}
		if (regs->err_int_stat & ZYNQ_SDHC_HOST_DMA_TXFR_ERR) {
			regs->err_int_stat |= ZYNQ_SDHC_HOST_DMA_TXFR_ERR;
		} else {
			regs->err_int_stat |= regs->err_int_stat;
		}
	}

	if (regs->normal_int_stat) {
		if (IS_ENABLED(CONFIG_XLNX_ZYNQ_SDHC_HOST_INTR)) {
			k_event_post(&emmc->irq_event, regs->normal_int_stat);
		}
		regs->normal_int_stat |= regs->normal_int_stat;
	}

	if (regs->adma_err_stat) {
		LOG_ERR("adma err:%x", regs->adma_err_stat);
	}
}

static int zynq_sdhc_init(const struct device *dev)
{
	const struct zynq_sdhc_config *config = dev->config;
	struct zynq_sdhc_data *data = dev->data;

	DEVICE_MMIO_MAP(dev, K_MEM_CACHE_NONE);

#ifdef CONFIG_PINCTRL
	/** note！
	we only configure the pins but not the PLL/CLK of SDIO [0xF8000150:0xF8001E03]
	(we shall configure these part in u-boot/fsbl) */
	int err = pinctrl_apply_state(config->pincfg, PINCTRL_STATE_DEFAULT);
	if (err < 0) {
		LOG_ERR("Failed to apply pin state %d", err);
		return err;
	}
#endif

	uint16_t hc_ver = zynq_sdhc_read16(dev, XSDPS_HOST_CTRL_VER_OFFSET);

	uint8_t spec_ver = hc_ver & XSDPS_HC_SPEC_VER_MASK;
	switch (spec_ver) {
	case 0:
		data->hc_ver = SD_SPEC_VER1_0;
		break;
	case 1:
		data->hc_ver = SD_SPEC_VER2_0;
		break;
	case 2:
		data->hc_ver = SD_SPEC_VER3_0;
		break;
	default:
		data->hc_ver = SD_SPEC_VER1_0;
		break;
	}
	data->host_caps = zynq_sdhc_read64(dev, XSDPS_CAPS_OFFSET);

	k_sem_init(&data->lock, 1, 1);

	if (IS_ENABLED(CONFIG_XLNX_ZYNQ_SDHC_HOST_INTR)) {
		k_event_init(&data->irq_event);
		config->config_func(dev);
	}
	memset(&data->host_io, 0, sizeof(data->host_io));
	// memset(adma_desc_tbl, 0, sizeof(adma_desc_tbl));

	return zynq_sdhc_reset(dev);
}

static const struct sdhc_driver_api zynq_sdhc_api = {
	.reset = zynq_sdhc_reset,
	.request = zynq_sdhc_request,
	.set_io = zynq_sdhc_set_io,
	.get_card_present = zynq_sdhc_get_card_present,
	.execute_tuning = zynq_sdhc_execute_tuning,
	.card_busy = zynq_sdhc_card_busy,
	.get_host_props = zynq_sdhc_get_host_props,
};

#if CONFIG_PINCTRL
#define XLNX_SDHC_PINCTRL_DEFINE(port) PINCTRL_DT_INST_DEFINE(port);
#define XLNX_SDHC_PINCTRL_INIT(port)   .pincfg = PINCTRL_DT_INST_DEV_CONFIG_GET(port),
#else
#define XLNX_SDHC_PINCTRL_DEFINE(port)
#define XLNX_SDHC_PINCTRL_INIT(port)
#endif /* CONFIG_PINCTRL */

#define ZYNQ_SDHC_INIT(n)                                                                          \
	XLNX_SDHC_PINCTRL_DEFINE(n)                                                                \
	static void zynq_sdhc_##n##_irq_config_func(const struct device *dev)                      \
	{                                                                                          \
		ARG_UNUSED(dev);                                                                   \
		IRQ_CONNECT(DT_INST_IRQN(n), DT_INST_IRQ(n, priority), zynq_sdhc_isr,              \
			    DEVICE_DT_INST_GET(n), 0);                                             \
		irq_enable(DT_INST_IRQN(n));                                                       \
	}                                                                                          \
	static const struct zynq_sdhc_config zynq_sdhc_##n##_config = {                            \
		DEVICE_MMIO_ROM_INIT(DT_DRV_INST(n)),                                              \
		XLNX_SDHC_PINCTRL_INIT(n).config_func = zynq_sdhc_##n##_irq_config_func,           \
		.clock_freq = DT_INST_PROP(n, clock_frequency),                                    \
		.min_bus_freq = DT_INST_PROP(n, min_bus_freq),                                     \
		.max_bus_freq = DT_INST_PROP(n, max_bus_freq),                                     \
		.power_delay_ms = DT_INST_PROP(n, power_delay_ms),                                 \
		.dw_4bit = DT_INST_ENUM_HAS_VALUE(n, bus_width, 4),                                \
		.dw_8bit = DT_INST_ENUM_HAS_VALUE(n, bus_width, 8),                                \
		.hs200_mode = DT_INST_PROP(n, mmc_hs200_1_8v),                                     \
		.hs400_mode = DT_INST_PROP(n, mmc_hs400_1_8v),                                     \
	};                                                                                         \
	static struct zynq_sdhc_data zynq_sdhc_##n##_data = {                                      \
		.card_present = false,                                                             \
		.bus_width = DT_INST_PROP(n, bus_width),                                           \
		.slot_type = DT_INST_PROP(n, slot_type),                                           \
	};                                                                                         \
	DEVICE_DT_INST_DEFINE(n, &zynq_sdhc_init, NULL, &zynq_sdhc_##n##_data,                     \
			      &zynq_sdhc_##n##_config, POST_KERNEL,                                \
			      CONFIG_KERNEL_INIT_PRIORITY_DEVICE, &zynq_sdhc_api);

DT_INST_FOREACH_STATUS_OKAY(ZYNQ_SDHC_INIT)