#define DT_DRV_COMPAT st_stm32_ospi_psram_v2

#include <errno.h>
#include <soc.h>
#include <zephyr/device.h>
#include <stm32_bitops.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/drivers/clock_control/stm32_clock_control.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/logging/log.h>
#include <zephyr/multi_heap/shared_multi_heap.h>
#include <zephyr/dt-bindings/flash_controller/ospi.h>

#include "zephyr/arch/common/ffs.h"

#define STM32_OSPI_CLOCK_PRESCALER_MIN                1U
#define STM32_OSPI_CLOCK_PRESCALER_MAX                256U
#define STM32_OSPI_CLOCK_COMPUTE(bus_freq, prescaler) ((bus_freq) / (prescaler))

#define PSRAM_MIN_CS_HIGH_NS		50u		/* 50 nanoseconds, typical for PSRAM, time need for internal mem refresh mechanizm */
#define STM32_OSPI_FIFO_THRESHOLD    4

LOG_MODULE_REGISTER(memc_stm32_ospi_psram_v2, CONFIG_MEMC_LOG_LEVEL);

#define STM32_OSPI_NODE DT_INST_PARENT(0)

/* Get the base address of the flash from the DTS st,stm32-ospi node */
#define STM32_OSPI_BASE_ADDRESS DT_REG_ADDR_BY_IDX(STM32_OSPI_NODE, 1)

#define DT_OSPI_PROP_OR(prop, default_value) \
    DT_PROP_OR(STM32_OSPI_NODE, prop, default_value)

#define DT_OSPI_IO_PORT_PROP_OR(prop, default_value)					\
    COND_CODE_1(DT_NODE_HAS_PROP(STM32_OSPI_NODE, prop),				\
        (_CONCAT(HAL_OSPIM_, DT_STRING_TOKEN(STM32_OSPI_NODE, prop))),	\
        ((default_value)))

#define READ_CMD		  0xEB  // Fast Read Quad (standard QSPI command)
#define WRITE_CMD		  0x38  // Quad Write (sometimes 0x02, check device datasheet)
#define ENTER_QPI         0x35
#define DUMMY_CYCLES_READ 6
#define DUMMY_CLOCK_CYCLES_READ 6
#define DUMMY_CLOCK_CYCLES_WRITE  0

struct memc_stm32_ospi_psram_v2_config {
    OCTOSPI_TypeDef *regs;
    const struct pinctrl_dev_config *pcfg;
    const struct stm32_pclken pclken;
#if DT_CLOCKS_HAS_NAME(STM32_OSPI_NODE, ospi_ker)
    const struct stm32_pclken pclken_ker; /* clock subsystem */
#endif
#if DT_CLOCKS_HAS_NAME(STM32_OSPI_NODE, ospi_mgr)
    const struct stm32_pclken pclken_mgr; /* clock subsystem */
#endif
    size_t memory_size;
    uint32_t max_frequency;
    int data_mode; /* SPI or QSPI or OSPI */
    int data_rate; /* DTR or STR */
};

struct memc_stm32_ospi_psram_v2_data {
    OSPI_HandleTypeDef hospi;
};

static int PSRAM_EnableMemoryMapped(OSPI_HandleTypeDef *hospi)
{
	HAL_StatusTypeDef ret;
	HAL_OSPI_DLYB_CfgTypeDef dlyb_cfg, dlyb_cfg_test;

	ret = HAL_OSPI_DLYB_GetClockPeriod(hospi,&dlyb_cfg);
	if ( ret != HAL_OK)
	{
		LOG_DBG("Failed to get DLYB clock period: %d", ret);
		return -EIO;
	}

	/*when DTR, PhaseSel is divided by 4 (emperic value)*/
	dlyb_cfg.PhaseSel /= 4;	//4

	/* save the present configuration for check*/
	dlyb_cfg_test = dlyb_cfg;

	/*set delay block configuration*/
	HAL_OSPI_DLYB_SetConfig(hospi, &dlyb_cfg);

	/*check the set value*/
	HAL_OSPI_DLYB_GetConfig(hospi, &dlyb_cfg);
	if ((dlyb_cfg.PhaseSel != dlyb_cfg_test.PhaseSel) || (dlyb_cfg.Units != dlyb_cfg_test.Units))
	{
		return -EIO;
	}

    OSPI_RegularCmdTypeDef sCommand = {0};
    OSPI_MemoryMappedTypeDef sMemMappedCfg = {0};

	// 1. Command to switch to QSPI mode (1-1-1 -> 4-4-4)
	sCommand.OperationType      = HAL_OSPI_OPTYPE_COMMON_CFG;	//HAL_OSPI_OPTYPE_WRITE_CFG;
	sCommand.FlashId            = HAL_OSPI_FLASH_ID_1;
	sCommand.Instruction        = ENTER_QPI;
	sCommand.InstructionMode    = HAL_OSPI_INSTRUCTION_1_LINE;
	sCommand.InstructionSize    = HAL_OSPI_INSTRUCTION_8_BITS;
	sCommand.InstructionDtrMode = HAL_OSPI_INSTRUCTION_DTR_DISABLE;
	sCommand.AddressMode        = HAL_OSPI_ADDRESS_NONE;
	sCommand.AddressSize        = HAL_OSPI_ADDRESS_32_BITS;
	sCommand.AddressDtrMode     = HAL_OSPI_ADDRESS_DTR_DISABLE;
	sCommand.AlternateBytesMode = HAL_OSPI_ALTERNATE_BYTES_NONE;
	sCommand.DataMode           = HAL_OSPI_DATA_NONE;
	sCommand.DataDtrMode        = HAL_OSPI_DATA_DTR_DISABLE;
	sCommand.DummyCycles        = DUMMY_CLOCK_CYCLES_WRITE;
	sCommand.DQSMode            = HAL_OSPI_DQS_DISABLE;
	sCommand.SIOOMode           = HAL_OSPI_SIOO_INST_EVERY_CMD;

	ret = HAL_OSPI_Command(hospi, &sCommand, HAL_OSPI_TIMEOUT_DEFAULT_VALUE);
	if (ret != HAL_OK) {
		LOG_ERR("%d: Failed to enter QPI mode", ret);
		return -EIO;
	}

	/*Configure Memory Mapped mode (write cmd)*/
	sCommand.OperationType      = HAL_OSPI_OPTYPE_WRITE_CFG;
	sCommand.FlashId            = HAL_OSPI_FLASH_ID_1;
	sCommand.Instruction        = WRITE_CMD;
	sCommand.InstructionMode    = HAL_OSPI_INSTRUCTION_4_LINES;
	sCommand.InstructionSize    = HAL_OSPI_INSTRUCTION_8_BITS;
	sCommand.InstructionDtrMode = HAL_OSPI_INSTRUCTION_DTR_DISABLE;
	sCommand.AddressMode        = HAL_OSPI_ADDRESS_4_LINES;
	sCommand.AddressSize        = HAL_OSPI_ADDRESS_24_BITS;
	sCommand.Address			  = 0x0;
	sCommand.AddressDtrMode     = HAL_OSPI_ADDRESS_DTR_DISABLE;
	sCommand.AlternateBytesMode = HAL_OSPI_ALTERNATE_BYTES_NONE;
	sCommand.DataMode           = HAL_OSPI_DATA_4_LINES;
	sCommand.DataDtrMode        = HAL_OSPI_DATA_DTR_DISABLE;
	sCommand.DummyCycles        = DUMMY_CLOCK_CYCLES_WRITE;
	////sCommand.DQSMode            = HAL_OSPI_DQS_DISABLE;
	/* VERY IMPORTANT! */
	sCommand.DQSMode       	  = HAL_OSPI_DQS_ENABLE;			//ERRATA: ENABLE should fix - but it does not!
	  	  	  	  	  	  	  	  	  	  	  	  	  	  		//the debugger is disconnected and program even do not proceed!
	sCommand.SIOOMode           = HAL_OSPI_SIOO_INST_EVERY_CMD;

	ret = HAL_OSPI_Command(hospi, &sCommand, HAL_OSPI_TIMEOUT_DEFAULT_VALUE);
	if (ret != HAL_OK) {
		LOG_ERR("%d: Failed to set memory map write cmd", ret);
		return -EIO;
	}
	/*Configure Memory Mapped mode (read cmd)*/
	sCommand.OperationType = HAL_OSPI_OPTYPE_READ_CFG;
	sCommand.Instruction   = READ_CMD;
	sCommand.DummyCycles   = DUMMY_CLOCK_CYCLES_READ;
	sCommand.DQSMode       = HAL_OSPI_DQS_DISABLE;
	  /* Remark: if you disable here DQSMode - the debugger will disconnect! */

	ret = HAL_OSPI_Command(hospi, &sCommand, HAL_OSPI_TIMEOUT_DEFAULT_VALUE);
	if (ret != HAL_OK) {
		LOG_ERR("%d: Failed to set memory map read cmd", ret);
		return -EIO;
	}

	sMemMappedCfg.TimeOutActivation = HAL_OSPI_TIMEOUT_COUNTER_ENABLE;
	sMemMappedCfg.TimeOutPeriod     = 0x34;

	ret = HAL_OSPI_MemoryMapped(hospi, &sMemMappedCfg);

	if (ret != HAL_OK) {
		LOG_ERR("%d: Failed to enable memory map", ret);
		return -EIO;
	}
	LOG_DBG("MemoryMap mode enabled");
	return 0;
}

/* Returns the number of clock cycles required to guarantee >= min_ns */
static uint32_t CalcChipSelectHighTimeCycles(uint32_t clk_hz, uint32_t min_ns)
{
	if (clk_hz == 0u) {
		return 0u; /* or ASSERT / error handling */
	}

	/*
	 * cycles = ceil(min_ns * clk_hz / 1e9)
	 * Avoid floating point: ceil(a/b) = (a + b - 1) / b
	 */
	uint64_t numerator = (uint64_t)min_ns * (uint64_t)clk_hz + 1000000000ull - 1ull;
	uint32_t cycles = (uint32_t)(numerator / 1000000000ull);

	/* Usually at least 1 cycle is required to generate a valid high time */
	if (cycles == 0u) {
		cycles = 1u;
	}

	return cycles;
}

/* Function to return true if the PSRAM is in MemoryMapped else false */
static bool stm32_ospi_is_memorymap(const struct device *dev)
{
	struct memc_stm32_ospi_psram_v2_data *dev_data = dev->data;

	return stm32_reg_read_bits(&dev_data->hospi.Instance->CR, OCTOSPI_CR_FMODE) ==
		   OCTOSPI_CR_FMODE;
}

static int memc_stm32_ospi_psram_init(const struct device *dev) {
    const struct memc_stm32_ospi_psram_v2_config *dev_cfg = dev->config;
    struct memc_stm32_ospi_psram_v2_data *dev_data = dev->data;
    uint32_t ahb_clock_freq;
    uint32_t prescaler = STM32_OSPI_CLOCK_PRESCALER_MIN;
    int ret;


    if (!device_is_ready(DEVICE_DT_GET(STM32_CLOCK_CONTROL_NODE))) {
        LOG_ERR("clock control device not ready");
        return -ENODEV;
    }

#ifdef CONFIG_STM32_MEMMAP
	/* If MemoryMapped then configure skip init */
	if (stm32_ospi_is_memorymap(dev)) {
		LOG_DBG("NOR init'd in MemMapped mode");
		/* Force HAL instance in correct state */
		dev_data->hospi.State = HAL_OSPI_STATE_BUSY_MEM_MAPPED;
		return 0;
	}
#endif /* CONFIG_STM32_MEMMAP */

    /* Signals configuration */
    ret = pinctrl_apply_state(dev_cfg->pcfg, PINCTRL_STATE_DEFAULT);
    if (ret < 0) {
        LOG_ERR("OSPI pinctrl setup failed (%d)", ret);
        return ret;
    }

    /* Clock configuration */
    if (clock_control_on(DEVICE_DT_GET(STM32_CLOCK_CONTROL_NODE),
                 (clock_control_subsys_t) &dev_cfg->pclken) != 0) {
        LOG_ERR("Could not enable OSPI clock");
        return -EIO;
                 }
    /* Alternate clock config for peripheral */
#if DT_CLOCKS_HAS_NAME(STM32_OSPI_NODE, ospi_ker)
    if (clock_control_configure(DEVICE_DT_GET(STM32_CLOCK_CONTROL_NODE),
                (clock_control_subsys_t) &dev_cfg->pclken_ker,
                NULL) != 0) {
        LOG_ERR("Could not select OSPI domain clock");
        return -EIO;
                }
    if (clock_control_get_rate(DEVICE_DT_GET(STM32_CLOCK_CONTROL_NODE),
                    (clock_control_subsys_t) &dev_cfg->pclken_ker,
                    &ahb_clock_freq) < 0) {
        LOG_ERR("Failed call clock_control_get_rate(pclken_ker)");
        return -EIO;
                    }
#else
    if (clock_control_get_rate(DEVICE_DT_GET(STM32_CLOCK_CONTROL_NODE),
                (clock_control_subsys_t) &dev_cfg->pclken,
                &ahb_clock_freq) < 0) {
        LOG_ERR("Failed call clock_control_get_rate(pclken)");
        return -EIO;
                }
#endif
#if DT_CLOCKS_HAS_NAME(STM32_OSPI_NODE, ospi_mgr)
    if (clock_control_on(DEVICE_DT_GET(STM32_CLOCK_CONTROL_NODE),
                 (clock_control_subsys_t) &dev_cfg->pclken_mgr) != 0) {
        LOG_ERR("Could not enable OSPI Manager clock");
        return -EIO;
                 }
#endif

    LOG_DBG("AHB clock frequency: %u", ahb_clock_freq);

    for (; prescaler <= STM32_OSPI_CLOCK_PRESCALER_MAX; prescaler++) {
        uint32_t clk = STM32_OSPI_CLOCK_COMPUTE(ahb_clock_freq, prescaler);

        if (clk <= dev_cfg->max_frequency) {
            break;
        }
    }

    __ASSERT_NO_MSG(prescaler >= STM32_OSPI_CLOCK_PRESCALER_MIN &&
        prescaler <= STM32_OSPI_CLOCK_PRESCALER_MAX);

	LOG_DBG("CLK prescaler %u", prescaler);
	LOG_DBG("CLK clock frequency: %u", ahb_clock_freq / prescaler);

    /* Initialize OSPI HAL structure completely */
    dev_data->hospi.Init.ClockPrescaler = prescaler;
    dev_data->hospi.Init.DeviceSize = find_lsb_set(dev_cfg->memory_size) - 1;

		uint8_t cs_high_time_cycles = CalcChipSelectHighTimeCycles(ahb_clock_freq / prescaler, PSRAM_MIN_CS_HIGH_NS);
    dev_data->hospi.Init.ChipSelectHighTime = DT_INST_PROP_OR(0, cs_high_time_cycles, cs_high_time_cycles);
	LOG_DBG("Set ChipSelectHighTime: %u cycles", dev_data->hospi.Init.ChipSelectHighTime);

#if defined(OCTOSPI_DCR2_WRAPSIZE)
    dev_data->hospi.Init.WrapSize = HAL_OSPI_WRAP_NOT_SUPPORTED;
#endif /* OCTOSPI_DCR2_WRAPSIZE */
    /* STR mode else Macronix for DTR mode */
    if (dev_cfg->data_rate == OSPI_DTR_TRANSFER) {
        dev_data->hospi.Init.MemoryType = HAL_OSPI_MEMTYPE_MACRONIX;
        dev_data->hospi.Init.DelayHoldQuarterCycle = HAL_OSPI_DHQC_ENABLE;
    } else {
        dev_data->hospi.Init.MemoryType = HAL_OSPI_MEMTYPE_MICRON;
        dev_data->hospi.Init.DelayHoldQuarterCycle = HAL_OSPI_DHQC_ENABLE;  // HAL_OSPI_DHQC_DISABLE - > HAL_OSPI_DHQC_ENABLE; /* PSRAM needs DHQC enabled in STR mode as well */
    }
    //dev_data->hospi.Init.ChipSelectBoundary = 4;  // DT_INST_PROP(0, st_csbound); //1 > 4 for PSRAM
#if STM32_OSPI_DLYB_BYPASSED
    dev_data->hospi.Init.DelayBlockBypass = HAL_OSPI_DELAY_BLOCK_BYPASSED;
#else
    dev_data->hospi.Init.DelayBlockBypass = HAL_OSPI_DELAY_BLOCK_USED;
#endif /* STM32_OSPI_DLYB_BYPASSED */

    if (HAL_OSPI_Init(&dev_data->hospi) != HAL_OK) {
        LOG_ERR("OSPI Init failed");
        return -EIO;
    }

    LOG_DBG("OSPI Init'd");

#if defined(OCTOSPIM)
    /* OCTOSPI I/O manager init Function */
    OSPIM_CfgTypeDef ospi_mgr_cfg = {0};

    if (dev_data->hospi.Instance == OCTOSPI1) {
        ospi_mgr_cfg.ClkPort = DT_OSPI_PROP_OR(clk_port, 1);
        ospi_mgr_cfg.DQSPort = DT_OSPI_PROP_OR(dqs_port, 0);
        ospi_mgr_cfg.NCSPort = DT_OSPI_PROP_OR(ncs_port, 1);
        ospi_mgr_cfg.IOLowPort = DT_OSPI_IO_PORT_PROP_OR(io_low_port,
                                 HAL_OSPIM_IOPORT_1_LOW);
        ospi_mgr_cfg.IOHighPort = DT_OSPI_IO_PORT_PROP_OR(io_high_port,
                                  HAL_OSPIM_IOPORT_1_HIGH);
    } else if (dev_data->hospi.Instance == OCTOSPI2) {
        ospi_mgr_cfg.ClkPort = DT_OSPI_PROP_OR(clk_port, 2);
        ospi_mgr_cfg.DQSPort = DT_OSPI_PROP_OR(dqs_port, 0);
        ospi_mgr_cfg.NCSPort = DT_OSPI_PROP_OR(ncs_port, 2);
        ospi_mgr_cfg.IOLowPort = DT_OSPI_IO_PORT_PROP_OR(io_low_port,
                                 HAL_OSPIM_IOPORT_2_LOW);
        ospi_mgr_cfg.IOHighPort = DT_OSPI_IO_PORT_PROP_OR(io_high_port,
                                  HAL_OSPIM_IOPORT_2_HIGH);
    }
#if defined(OCTOSPIM_CR_MUXEN)
    ospi_mgr_cfg.Req2AckTime = 1;
#endif /* OCTOSPIM_CR_MUXEN */
    if (HAL_OSPIM_Config(&dev_data->hospi, &ospi_mgr_cfg,
        HAL_OSPI_TIMEOUT_DEFAULT_VALUE) != HAL_OK) {
        LOG_ERR("OSPI M config failed");
        return -EIO;
        }
#if defined(CONFIG_SOC_SERIES_STM32U5X)
    /* OCTOSPI2 delay block init Function */
    HAL_OSPI_DLYB_CfgTypeDef ospi_delay_block_cfg = {0};

    ospi_delay_block_cfg.Units = 56;
    ospi_delay_block_cfg.PhaseSel = 2;
    if (HAL_OSPI_DLYB_SetConfig(&dev_data->hospi, &ospi_delay_block_cfg) != HAL_OK) {
        LOG_ERR("OSPI DelayBlock failed");
        return -EIO;
    }
#endif /* CONFIG_SOC_SERIES_STM32U5X */

#endif /* OCTOSPIM */
#ifdef CONFIG_STM32_MEMMAP
	/* Now configure the octo Flash in MemoryMapped (access by address) */
    ret = PSRAM_EnableMemoryMapped(&dev_data->hospi);
	if (ret != HAL_OK) {
		LOG_ERR("Error (%d): setting PSRAM in MemoryMapped mode", ret);
		return -EINVAL;
	}
	LOG_DBG("PSRAM in MemoryMapped mode at 0x%lx (0x%x bytes)",
		(long)(STM32_OSPI_BASE_ADDRESS),
		dev_cfg->memory_size);
#else

#endif /* CONFIG_STM32_MEMMAP */
	return 0;
}


PINCTRL_DT_DEFINE(STM32_OSPI_NODE);

static struct memc_stm32_ospi_psram_v2_config memc_stm32_ospi_config = {
    .pclken = {
        .bus = DT_CLOCKS_CELL_BY_NAME(STM32_OSPI_NODE, ospix, bus),
        .enr = DT_CLOCKS_CELL_BY_NAME(STM32_OSPI_NODE, ospix, bits)},
    .pcfg = PINCTRL_DT_DEV_CONFIG_GET(STM32_OSPI_NODE),
    .memory_size = DT_INST_PROP(0, size) / 8, /* In Bytes */
    .max_frequency = DT_INST_PROP(0, max_frequency),
    .data_mode = OSPI_QUAD_MODE,
    .data_rate = OSPI_STR_TRANSFER,
#if DT_CLOCKS_HAS_NAME(STM32_OSPI_NODE, ospi_ker)
    .pclken_ker = {
        .bus = DT_CLOCKS_CELL_BY_NAME(STM32_OSPI_NODE, ospi_ker, bus),
        .enr = DT_CLOCKS_CELL_BY_NAME(STM32_OSPI_NODE, ospi_ker, bits)},
#endif
#if DT_CLOCKS_HAS_NAME(STM32_OSPI_NODE, ospi_mgr)
    .pclken_mgr = {
        .bus = DT_CLOCKS_CELL_BY_NAME(STM32_OSPI_NODE, ospi_mgr, bus),
        .enr = DT_CLOCKS_CELL_BY_NAME(STM32_OSPI_NODE, ospi_mgr, bits)},
#endif

};

static struct memc_stm32_ospi_psram_v2_data memc_stm32_ospi_data = {
    .hospi = {
        .Instance = (OCTOSPI_TypeDef *)DT_REG_ADDR(STM32_OSPI_NODE),
        .Init = {
            .FifoThreshold = STM32_OSPI_FIFO_THRESHOLD,
            .SampleShifting = (DT_PROP(STM32_OSPI_NODE, ssht_enable)
                    ? HAL_OSPI_SAMPLE_SHIFTING_HALFCYCLE
                    : HAL_OSPI_SAMPLE_SHIFTING_NONE),
            .ClockMode = HAL_OSPI_CLOCK_MODE_0,
        	.FreeRunningClock = HAL_OSPI_FREERUNCLK_DISABLE,
        	.DualQuad = HAL_OSPI_DUALQUAD_DISABLE,
        	.ChipSelectBoundary = DT_INST_PROP(0, st_csbound),
#if defined(OCTOSPI_DCR4_REFRESH)
        	.Refresh = DT_INST_PROP(0, refresh_cycles),
#endif /* OCTOSPI_DCR4_REFRESH */

        },
    },
};

DEVICE_DT_INST_DEFINE(0, &memc_stm32_ospi_psram_init, NULL,
	&memc_stm32_ospi_data, &memc_stm32_ospi_config,
	POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_DEVICE, NULL);
