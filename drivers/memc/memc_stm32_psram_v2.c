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
#include "psram_spi.h"

#include "zephyr/arch/common/ffs.h"

#define STM32_OSPI_CLOCK_PRESCALER_MIN                1U
#define STM32_OSPI_CLOCK_PRESCALER_MAX                256U
#define STM32_OSPI_CLOCK_COMPUTE(bus_freq, prescaler) ((bus_freq) / (prescaler))

#define SPI_PSRAM_CMD_RESET_EN    0x66    /* Reset Enable */
#define SPI_PSRAM_CMD_RESET_MEM   0x99    /* Reset Memory */
#define SPI_PSRAM_OCMD_RESET_EN   0x6699  /* Octal Reset Enable */
#define SPI_PSRAM_OCMD_RESET_MEM  0x9966  /* Octal Reset Memory */
#define SPI_PSRAM_CMD_RDID        0x9F    /* Read JEDEC ID */
#define JESD216_CMD_READ_ID   SPI_PSRAM_CMD_RDID
#define JESD216_OCMD_READ_ID  0x9F60
#define JESD216_READ_ID_LEN   3

// PSRAM frequency for read ID in spi mode is limited to ± 33MHz
// Set 10 MHz to be on the safe side
#define PSRAM_MAX_FEQUENCY_SPI_MODE                     60000000U

#define STM32_OSPI_RESET_MAX_TIME               100U

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

/* AP Memory registers definition */
#define MR0 0x00000000U
#define MR1 0x00000001U
#define MR2 0x00000002U
#define MR3 0x00000003U
#define MR4 0x00000004U
#define MR8 0x00000008U

/* AP Memory commands */
#define AP_MEM_SYNC_READ_CMD   0x00U
#define AP_MEM_SYNC_WRITE_CMD  0x80U
#define AP_MEM_BURST_READ_CMD  0x20U
#define AP_MEM_BURST_WRITE_CMD 0xA0U
#define AP_MEM_READ_REG_CMD    0x40U
#define AP_MEM_WRITE_REG_CMD   0xC0U
#define AP_MEM_RESET_CMD       0xFFU

/* AP MEM Write Operations */
#define AP_MEM_WRITE_CMD 0x8080

/* AP MEM Read Operations */
#define AP_MEM_READ_CMD 0x0000


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

// ------------------------ QSPI Memory configuration and init functions (START) --------------------------------
static int PSRAM_EnableMemoryMapped(OSPI_HandleTypeDef *hospi)
{
	HAL_StatusTypeDef ret;
	HAL_OSPI_DLYB_CfgTypeDef dlyb_cfg, dlyb_cfg_test;
	//
	// ret = HAL_OSPI_DLYB_GetClockPeriod(hospi,&dlyb_cfg);
	// if ( ret != HAL_OK)
	// {
	// 	LOG_DBG("Failed to get DLYB clock period: %d", ret);
	// 	return -EIO;
	// }
	//
	// /*when DTR, PhaseSel is divided by 4 (emperic value)*/
	// dlyb_cfg.PhaseSel /= 4;	//4
	//
	// /* save the present configuration for check*/
	// dlyb_cfg_test = dlyb_cfg;
	//
	// /*set delay block configuration*/
	// HAL_OSPI_DLYB_SetConfig(hospi, &dlyb_cfg);
	//
	// /*check the set value*/
	// HAL_OSPI_DLYB_GetConfig(hospi, &dlyb_cfg);
	// if ((dlyb_cfg.PhaseSel != dlyb_cfg_test.PhaseSel) || (dlyb_cfg.Units != dlyb_cfg_test.Units))
	// {
	// 	return -EIO;
	// }

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

static int stm32_ospi_mem_reset(const struct device *dev)
{
	struct memc_stm32_ospi_psram_v2_data *dev_data = dev->data;

	/* Reset command sent sucessively for each mode SPI/OPS & STR/DTR */
	OSPI_RegularCmdTypeDef s_command = {
		.OperationType = HAL_OSPI_OPTYPE_COMMON_CFG,
		.FlashId = HAL_OSPI_FLASH_ID_1,
		.AddressMode = HAL_OSPI_ADDRESS_NONE,
		.InstructionMode = HAL_OSPI_INSTRUCTION_1_LINE,
		.InstructionDtrMode = HAL_OSPI_INSTRUCTION_DTR_DISABLE,
		.Instruction = SPI_PSRAM_CMD_RESET_EN,
		.InstructionSize = HAL_OSPI_INSTRUCTION_8_BITS,
		.AlternateBytesMode = HAL_OSPI_ALTERNATE_BYTES_NONE,
		.DataMode = HAL_OSPI_DATA_NONE,
		.DummyCycles = 0U,
		.DQSMode = HAL_OSPI_DQS_DISABLE,
		.SIOOMode = HAL_OSPI_SIOO_INST_EVERY_CMD,
	};

	/* Reset enable in SPI mode and STR transfer mode */
	if (HAL_OSPI_Command(&dev_data->hospi,
		&s_command, HAL_OSPI_TIMEOUT_DEFAULT_VALUE) != HAL_OK) {
		LOG_ERR("OSPI reset enable (SPI/STR) failed");
		return -EIO;
	}

	/* Reset memory in SPI mode and STR transfer mode */
	s_command.Instruction = SPI_PSRAM_CMD_RESET_MEM;
	if (HAL_OSPI_Command(&dev_data->hospi,
		&s_command, HAL_OSPI_TIMEOUT_DEFAULT_VALUE) != HAL_OK) {
		LOG_ERR("OSPI reset memory (SPI/STR) failed");
		return -EIO;
	}

	/* Wait after SWreset CMD, in case SWReset occurred during erase operation */
	k_busy_wait(STM32_OSPI_RESET_MAX_TIME * USEC_PER_MSEC);

	return 0;
}

/*
 * Read the JEDEC ID data from the octoFlash at init or DTS
 * and store in the jedec_id Table of the flash_stm32_ospi_data
 */
static int stm32_ospi_read_jedec_id(const struct device *dev) {
	struct memc_stm32_ospi_psram_v2_data *dev_data = dev->data;

	uint8_t id[20] = {0};


	/* This is a SPI/STR command to issue to the octoFlash device */
	OSPI_RegularCmdTypeDef cmd = {
		.OperationType = HAL_OSPI_OPTYPE_COMMON_CFG,
		.FlashId = HAL_OSPI_FLASH_ID_1,

		.Instruction = JESD216_CMD_READ_ID,
		.InstructionMode = HAL_OSPI_INSTRUCTION_1_LINE,
		.InstructionSize = HAL_OSPI_INSTRUCTION_8_BITS,
		.InstructionDtrMode = HAL_OSPI_INSTRUCTION_DTR_DISABLE,

		.Address = 0x000000,
		.AddressSize = HAL_OSPI_ADDRESS_24_BITS,
		.AddressMode = HAL_OSPI_ADDRESS_1_LINE,
		.AlternateBytesMode = HAL_OSPI_ALTERNATE_BYTES_NONE,

		.DataMode = HAL_OSPI_DATA_1_LINE,
		.DataDtrMode = HAL_OSPI_DATA_DTR_DISABLE,

		.DummyCycles = 0U,
		.NbData = sizeof(id),

		.DQSMode = HAL_OSPI_DQS_DISABLE,
		.SIOOMode = HAL_OSPI_SIOO_INST_EVERY_CMD,

	};

	HAL_StatusTypeDef hal_ret;

	hal_ret = HAL_OSPI_Command(&dev_data->hospi, &cmd,
				   HAL_OSPI_TIMEOUT_DEFAULT_VALUE);

	if (hal_ret != HAL_OK) {
		LOG_ERR("%d: Failed to send OSPI instruction", hal_ret);
		return -EIO;
	}

	/* Place the received data directly into the jedec Table */
	hal_ret = HAL_OSPI_Receive(&dev_data->hospi, id,
				   HAL_OSPI_TIMEOUT_DEFAULT_VALUE);

	if (hal_ret != HAL_OK) {
		LOG_ERR("%d: Failed to read data", hal_ret);
		return -EIO;
	}

	LOG_DBG("Jedec ID = [%02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x]",
		id[0], id[1], id[2], id[3], id[4], id[5], id[6], id[7], id[8], id[9], id[10], id[11], id[12], id[13], id[14], id[15], id[16], id[17], id[18], id[19]);

	if (id[0] == 0 && id[1] == 0 && id[2] == 0) {
		LOG_ERR("Failed to read valid JEDEC ID, all bytes are 0");
		return -EIO;
	}
	else
	{
		const char * manufacturer_name = NULL;
		const char * chip_name = NULL;
		LOG_INF("JEDEC ID read successfully");
		manufacturer_name = psram_get_manufacturer_name(id[0]);
		chip_name = psram_get_device_name(id[0], ((uint16_t)id[1] << 8) | id[2]);
		LOG_INF("PSRAM Manufacturer: %s, Chip: %s", manufacturer_name, chip_name);

	}

	return 0;
}

/* Function to return true if the PSRAM is in MemoryMapped else false */
static bool stm32_ospi_is_memorymap(const struct device *dev)
{
	struct memc_stm32_ospi_psram_v2_data *dev_data = dev->data;

	return stm32_reg_read_bits(&dev_data->hospi.Instance->CR, OCTOSPI_CR_FMODE) ==
		   OCTOSPI_CR_FMODE;
}

static int psram_force_spi_mode(OSPI_HandleTypeDef *hospi)
{
	OSPI_RegularCmdTypeDef cmd = {0};

	cmd.OperationType      = HAL_OSPI_OPTYPE_COMMON_CFG;
	cmd.FlashId            = HAL_OSPI_FLASH_ID_1;
	cmd.InstructionSize    = HAL_OSPI_INSTRUCTION_8_BITS;
	cmd.AddressMode        = HAL_OSPI_ADDRESS_NONE;
	cmd.AlternateBytesMode = HAL_OSPI_ALTERNATE_BYTES_NONE;
	cmd.DataMode           = HAL_OSPI_DATA_NONE;
	cmd.DummyCycles        = 0;
	cmd.DQSMode            = HAL_OSPI_DQS_DISABLE;
	cmd.SIOOMode           = HAL_OSPI_SIOO_INST_EVERY_CMD;

	HAL_OSPI_Abort(hospi);

	/* Exit QPI (CMD=Q) */
	cmd.InstructionMode = HAL_OSPI_INSTRUCTION_4_LINES;
	cmd.Instruction = 0xF5;
	HAL_OSPI_Command(hospi, &cmd, HAL_OSPI_TIMEOUT_DEFAULT_VALUE);

	/* Reset in QPI */
	cmd.Instruction = 0x66;
	HAL_OSPI_Command(hospi, &cmd, HAL_OSPI_TIMEOUT_DEFAULT_VALUE);

	cmd.Instruction = 0x99;
	HAL_OSPI_Command(hospi, &cmd, HAL_OSPI_TIMEOUT_DEFAULT_VALUE);

	/* Reset in SPI */
	cmd.InstructionMode = HAL_OSPI_INSTRUCTION_1_LINE;

	cmd.Instruction = 0x66;
	HAL_OSPI_Command(hospi, &cmd, HAL_OSPI_TIMEOUT_DEFAULT_VALUE);

	cmd.Instruction = 0x99;
	HAL_OSPI_Command(hospi, &cmd, HAL_OSPI_TIMEOUT_DEFAULT_VALUE);

	k_busy_wait(100);

	return 0;
}

static int calculate_clock_prescaler(uint32_t ahb_clock_freq, uint32_t max_freq)
{
	int prescaler = STM32_OSPI_CLOCK_PRESCALER_MIN;

	for (; prescaler <= STM32_OSPI_CLOCK_PRESCALER_MAX; prescaler++) {
		uint32_t clk = STM32_OSPI_CLOCK_COMPUTE(ahb_clock_freq, prescaler);

		if (clk <= max_freq) {
			break;
		}
	}

	__ASSERT_NO_MSG(prescaler >= STM32_OSPI_CLOCK_PRESCALER_MIN &&
		prescaler <= STM32_OSPI_CLOCK_PRESCALER_MAX);

	return prescaler;
}

static int choose_memory_type(struct memc_stm32_ospi_psram_v2_data *dev_data)
{
	//Check configured memory type. If OSPI configured in 8 line mode, we deside that we use AP memory
	//if not, we use QSPI mode
	if (dev_data->hospi.Instance == OCTOSPI1) {
		//check if configured in 8 lines mode for OCTOSPI1
		if (DT_OSPI_IO_PORT_PROP_OR(io_low_port,HAL_OSPIM_IOPORT_1_LOW) &&
			DT_OSPI_IO_PORT_PROP_OR(io_high_port, HAL_OSPIM_IOPORT_1_HIGH))
		{
			dev_data->hospi.Init.MemoryType = HAL_OSPI_MEMTYPE_APMEMORY;
			LOG_DBG("OCTOSPI1 configured in 8 lines mode, use AP memory type");
		} else {
			dev_data->hospi.Init.MemoryType = HAL_OSPI_MEMTYPE_MICRON;
			LOG_DBG("OCTOSPI1 not configured in 8 lines mode, use MICRON memory type");
		}
	}else if (dev_data->hospi.Instance == OCTOSPI2) {
		if (DT_OSPI_IO_PORT_PROP_OR(io_low_port, HAL_OSPIM_IOPORT_2_LOW) &&
			DT_OSPI_IO_PORT_PROP_OR(io_high_port, HAL_OSPIM_IOPORT_2_HIGH))
		{
			dev_data->hospi.Init.MemoryType = HAL_OSPI_MEMTYPE_APMEMORY;
			LOG_DBG("OCTOSPI2 configured in 8 lines mode, use AP memory type");
		} else {
			dev_data->hospi.Init.MemoryType = HAL_OSPI_MEMTYPE_MICRON;
			LOG_DBG("OCTOSPI2 not configured in 8 lines mode, use MICRON memory type");
		}
	}
}


static int stm32_ospi_interface_init(struct memc_stm32_ospi_psram_v2_data *dev_data, const struct memc_stm32_ospi_psram_v2_config *dev_cfg, uint32_t ahb_clock_freq){

	uint16_t prescaler = 0 ;
	HAL_StatusTypeDef ret;

	prescaler = calculate_clock_prescaler(ahb_clock_freq, dev_cfg->max_frequency);

	LOG_DBG("AHB clock frequency: %u Hz", ahb_clock_freq);
	LOG_DBG("CLK prescaler %u", prescaler);
	LOG_DBG("CLK clock frequency: %u", ahb_clock_freq / prescaler);
	LOG_DBG("ChipSelectBoundary: %u", dev_data->hospi.Init.ChipSelectBoundary);
	LOG_DBG("Refresh cycles: %u", dev_data->hospi.Init.Refresh);

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
		LOG_ERR("OSPI config failed");
		return -EIO;
		}
#if defined(CONFIG_SOC_SERIES_STM32U5X)
	/* OCTOSPI2 delay block init Function */
	HAL_OSPI_DLYB_CfgTypeDef ospi_delay_block_cfg = {0};
	HAL_OSPI_DLYB_CfgTypeDef ospi_delay_block_cfg_test = {0};


	HAL_OSPI_DLYB_CfgTypeDef dlyb_cfg, dlyb_cfg_test;

	ret = HAL_OSPI_DLYB_GetClockPeriod(&dev_data->hospi,&dlyb_cfg);
	if ( ret != HAL_OK)
	{
		LOG_ERR("Failed to get DLYB clock period: %d", ret);
		return -EIO;
	}else {
		LOG_DBG("DLYB clock period Units: %u, PhaseSel: %u", dlyb_cfg.Units, dlyb_cfg.PhaseSel);
	}

	/*when DTR, PhaseSel is divided by 4 (emperic value)*/
	dlyb_cfg.PhaseSel /= 4;	//4

	/*set delay block configuration*/
	ret = HAL_OSPI_DLYB_SetConfig(&dev_data->hospi, &dlyb_cfg);
	if ( ret != HAL_OK)
	{
		LOG_ERR("Failed to set DLYB config.");
		return -EIO;
	}

#endif /* CONFIG_SOC_SERIES_STM32U5X */
	return 0;
}
// ------------------------ QSPI Memory configuration and init functions (END) --------------------------------

// ------------------------ AP Memory configuration and init functions (START) --------------------------------
static int ap_memory_write_reg(OSPI_HandleTypeDef *hospi, uint32_t address, uint8_t *value)
{
	OSPI_RegularCmdTypeDef cmd = {0};

	/* Initialize the write register command */
	cmd.OperationType = HAL_OSPI_OPTYPE_COMMON_CFG;
	cmd.InstructionMode = HAL_OSPI_INSTRUCTION_8_LINES;
	cmd.InstructionSize = HAL_OSPI_INSTRUCTION_8_BITS;
	cmd.InstructionDtrMode = HAL_OSPI_INSTRUCTION_DTR_DISABLE;
	cmd.Instruction = AP_MEM_WRITE_REG_CMD;
	cmd.AddressMode = HAL_OSPI_ADDRESS_8_LINES;
	cmd.AddressSize = HAL_OSPI_ADDRESS_32_BITS;
	cmd.AddressDtrMode = HAL_OSPI_ADDRESS_DTR_ENABLE;
	cmd.Address = address;
	cmd.AlternateBytesMode = HAL_OSPI_ALTERNATE_BYTES_NONE;
	cmd.DataMode = HAL_OSPI_DATA_8_LINES;
	cmd.DataDtrMode = HAL_OSPI_DATA_DTR_ENABLE;
	cmd.NbData = 2;
	cmd.DummyCycles = 0;
	cmd.DQSMode = HAL_OSPI_DQS_DISABLE;
	cmd.SIOOMode = HAL_OSPI_SIOO_INST_EVERY_CMD;

	/* Configure the command */
	if (HAL_OSPI_Command(hospi, &cmd, HAL_OSPI_TIMEOUT_DEFAULT_VALUE) != HAL_OK) {
		LOG_ERR("OSPI write command failed");
		return -EIO;
	}

	/* Transmission of the data */
	if (HAL_OSPI_Transmit(hospi, value, HAL_OSPI_TIMEOUT_DEFAULT_VALUE) != HAL_OK) {
		LOG_ERR("OSPI transmit failed");
		return -EIO;
	}

	return 0;
}

static int ap_memory_read_reg(OSPI_HandleTypeDef *hospi, uint32_t address, uint8_t *value,
			      uint32_t latency_cycles)
{
	OSPI_RegularCmdTypeDef cmd = {0};

	/* Initialize the read register command */
	cmd.OperationType = HAL_OSPI_OPTYPE_COMMON_CFG;
	cmd.InstructionMode = HAL_OSPI_INSTRUCTION_8_LINES;
	cmd.InstructionSize = HAL_OSPI_INSTRUCTION_8_BITS;
	cmd.InstructionDtrMode = HAL_OSPI_INSTRUCTION_DTR_DISABLE;
	cmd.Instruction = AP_MEM_READ_REG_CMD;
	cmd.AddressMode = HAL_OSPI_ADDRESS_8_LINES;
	cmd.AddressSize = HAL_OSPI_ADDRESS_32_BITS;
	cmd.AddressDtrMode = HAL_OSPI_ADDRESS_DTR_ENABLE;
	cmd.Address = address;
	cmd.AlternateBytesMode = HAL_OSPI_ALTERNATE_BYTES_NONE;
	cmd.DataMode = HAL_OSPI_DATA_8_LINES;
	cmd.DataDtrMode = HAL_OSPI_DATA_DTR_ENABLE;
	cmd.NbData = 2;
	cmd.DummyCycles = latency_cycles;
	cmd.DQSMode = HAL_OSPI_DQS_ENABLE;
	cmd.SIOOMode = HAL_OSPI_SIOO_INST_EVERY_CMD;

	/* Configure the command */
	if (HAL_OSPI_Command(hospi, &cmd, HAL_OSPI_TIMEOUT_DEFAULT_VALUE) != HAL_OK) {
		LOG_ERR("OSPI read command failed");
		return -EIO;
	}

	/* Reception of the data */
	if (HAL_OSPI_Receive(hospi, value, HAL_OSPI_TIMEOUT_DEFAULT_VALUE) != HAL_OK) {
		LOG_ERR("OSPI receive failed");
		return -EIO;
	}

	return 0;
}

static int ap_memory_configure(OSPI_HandleTypeDef *hospi)
{
	uint8_t read_latency_code = DT_INST_PROP(0, read_latency);
	uint8_t read_latency_cycles = read_latency_code + 3U; /* Code 0 <=> 3 cycles... */

	/* MR0 register for read and write */
	uint8_t regW_MR0[2] = {(DT_INST_PROP(0, fixed_latency) ? 0x20U : 0x00U) |
					   (read_latency_code << 2) | (DT_INST_PROP(0, drive_strength)),
				   0x0D};
	uint8_t regR_MR0[2] = {0};

	/* MR4 register for read and write */
	uint8_t regW_MR4[2] = {(DT_INST_PROP(0, write_latency) << 5) |
					   (DT_INST_PROP(0, adaptive_refresh_rate) << 3) |
					   (DT_INST_PROP(0, pasr)),
				   0x05U};
	uint8_t regR_MR4[2] = {0};

	/* MR8 register for read and write */
	uint8_t regW_MR8[2] = {(DT_INST_PROP(0, rbx) ? 0x08U : 0x00U) |
					   (DT_INST_PROP(0, burst_type_hybrid_wrap) ? 0x04U : 0x00U) |
					   (DT_INST_PROP(0, burst_length)),
				   0x08U};
	uint8_t regR_MR8[2] = {0};

	/* Configure Read Latency and drive Strength */
	if (ap_memory_write_reg(hospi, MR0, regW_MR0) != 0) {
		return -EIO;
	}

	if (ap_memory_read_reg(hospi, MR0, regR_MR0, read_latency_cycles) != 0) {
		return -EIO;
	}

	if (regR_MR0[0] != regW_MR0[0]) {
		return -EIO;
	}

	/* Configure Write Latency and refresh rate */
	if (ap_memory_write_reg(hospi, MR4, regW_MR4) != 0) {
		return -EIO;
	}

	/* Check MR4 configuration */
	if (ap_memory_read_reg(hospi, MR4, regR_MR4, read_latency_cycles) != 0) {
		return -EIO;
	}
	if (regR_MR4[0] != regW_MR4[0]) {
		return -EIO;
	}

	/* Configure Burst Length */
	if (ap_memory_write_reg(hospi, MR8, regW_MR8) != HAL_OK) {
		return -EIO;
	}

	/* Check MR8 configuration */
	if (ap_memory_read_reg(hospi, MR8, regR_MR8, read_latency_cycles) != HAL_OK) {
		return -EIO;
	}

	if (regR_MR8[0] != regW_MR8[0]) {
		return -EIO;
	}

	return 0;
}

static int config_memory_mapped(const struct device *dev)
{
	const struct memc_stm32_ospi_psram_v2_config *dev_cfg = dev->config;
	struct memc_stm32_ospi_psram_v2_data *dev_data = dev->data;
	OSPI_RegularCmdTypeDef cmd = {0};
	OSPI_MemoryMappedTypeDef mem_mapped_cfg = {0};

	/*Configure Memory Mapped mode*/
	cmd.OperationType = HAL_OSPI_OPTYPE_WRITE_CFG;
	cmd.FlashId = HAL_OSPI_FLASH_ID_1;
	cmd.Instruction = AP_MEM_WRITE_CMD;
	cmd.InstructionMode = HAL_OSPI_INSTRUCTION_8_LINES;
	cmd.InstructionSize = HAL_OSPI_INSTRUCTION_16_BITS;
	cmd.InstructionDtrMode = HAL_OSPI_INSTRUCTION_DTR_ENABLE;
	cmd.AddressMode = HAL_OSPI_ADDRESS_8_LINES;
	cmd.AddressSize = HAL_OSPI_ADDRESS_32_BITS;
	cmd.AddressDtrMode = HAL_OSPI_ADDRESS_DTR_ENABLE;
	cmd.AlternateBytesMode = HAL_OSPI_ALTERNATE_BYTES_NONE;
	cmd.DataMode = HAL_OSPI_DATA_8_LINES;
	cmd.DataDtrMode = HAL_OSPI_DATA_DTR_ENABLE;
	cmd.DummyCycles = DUMMY_CLOCK_CYCLES_WRITE;
	cmd.DQSMode = HAL_OSPI_DQS_ENABLE;
	cmd.SIOOMode = HAL_OSPI_SIOO_INST_EVERY_CMD;

	if (HAL_OSPI_Command(&dev_data->hospi, &cmd, HAL_OSPI_TIMEOUT_DEFAULT_VALUE) != HAL_OK) {
		return -EIO;
	}

	cmd.OperationType = HAL_OSPI_OPTYPE_READ_CFG;
	cmd.Instruction = AP_MEM_READ_CMD;
	cmd.DummyCycles = DUMMY_CLOCK_CYCLES_READ;

	if (HAL_OSPI_Command(&dev_data->hospi, &cmd, HAL_OSPI_TIMEOUT_DEFAULT_VALUE) != HAL_OK) {
		return -EIO;
	}

	mem_mapped_cfg.TimeOutActivation = HAL_OSPI_TIMEOUT_COUNTER_ENABLE;
	mem_mapped_cfg.TimeOutPeriod = 0x34;

	if (HAL_OSPI_MemoryMapped(&dev_data->hospi, &mem_mapped_cfg) != HAL_OK) {
		return -EIO;
	}

	return 0;
}


static int memc_ospi_AP_MEMORY_init(const struct device *dev,  uint32_t ahb_clock_freq) {

	const struct memc_stm32_ospi_psram_v2_config *dev_cfg = dev->config;
	struct memc_stm32_ospi_psram_v2_data *dev_data = dev->data;


	dev_data->hospi.Init.ClockPrescaler =  calculate_clock_prescaler(ahb_clock_freq, dev_cfg->max_frequency);
	LOG_DBG("MSB set: %d", find_msb_set(dev_cfg->memory_size) - 1);
	dev_data->hospi.Init.DeviceSize = find_msb_set(dev_cfg->memory_size) - 1;

	/* OSPI Init */
	if (HAL_OSPI_Init(&dev_data->hospi) != HAL_OK) {
		LOG_ERR("HAL_OSPI_Init(): <FAILED>");
		return -EIO;
	}

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

	if (HAL_OSPIM_Config(&dev_data->hospi, &ospi_mgr_cfg, HAL_OSPI_TIMEOUT_DEFAULT_VALUE) != HAL_OK) {
		LOG_ERR("HAL_OSPIM_Config(): <FAILED>");
		return -EIO;
	}

	LL_DLYB_CfgTypeDef dlyb_cfg, ll_dlyb_cfg, ll_dlyb_cfg_test;

	dlyb_cfg.PhaseSel = 0;
	dlyb_cfg.Units = 0;

	if (HAL_OSPI_DLYB_SetConfig(&dev_data->hospi, &dlyb_cfg) != HAL_OK) {
		LOG_ERR("HAL_OSPI_DLYB_SetConfig(): <FAILED>");
		return -EIO;
	}

	if (HAL_OSPI_DLYB_GetClockPeriod(&dev_data->hospi, &ll_dlyb_cfg) != HAL_OK) {
		LOG_ERR("HAL_OSPI_DLYB_GetClockPeriod(): <FAILED>");
		return -EIO;
	}

	/* When DTR, PhaseSel is divided by 4 (emperic value) */
	ll_dlyb_cfg.PhaseSel /= 4;
	ll_dlyb_cfg_test = ll_dlyb_cfg;

	if ((HAL_OSPI_DLYB_SetConfig(&dev_data->hospi, &ll_dlyb_cfg) != HAL_OK) ||
		(HAL_OSPI_DLYB_GetConfig(&dev_data->hospi, &ll_dlyb_cfg) != HAL_OK)) {
		return -EIO;
		}

	if ((ll_dlyb_cfg.PhaseSel != ll_dlyb_cfg_test.PhaseSel) ||
		(ll_dlyb_cfg.Units != ll_dlyb_cfg_test.Units)) {
		LOG_ERR("PhaseSel: <FAILED>");
		return -EIO;
		}

	if (ap_memory_configure(&dev_data->hospi) != 0) {
		LOG_ERR("ap_memory_configure(): <FAILED>");
		return -EIO;
	}
	if (config_memory_mapped(dev) != 0) {
		LOG_ERR("config_memory_mapped(): <FAILED>");
		return -EIO;
	}

	return 0;
}

// ------------------------ AP Memory configuration and init functions (END) --------------------------------

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
		LOG_DBG("PSRAM init'd in MemMapped mode");
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
                 (clock_control_subsys_t) &dev_cfg->pclken) != 0)
		{
        LOG_ERR("Could not enable OSPI clock");
        return -EIO;
		}
    /* Alternate clock config for peripheral */
#if DT_CLOCKS_HAS_NAME(STM32_OSPI_NODE, ospi_ker)
    if (clock_control_configure(DEVICE_DT_GET(STM32_CLOCK_CONTROL_NODE),
                (clock_control_subsys_t) &dev_cfg->pclken_ker,
                NULL) != 0)
    	{
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

	choose_memory_type(dev_data);
	//If AP memory, we skip the rest of the init and configuration and
	if (dev_data->hospi.Init.MemoryType == HAL_OSPI_MEMTYPE_APMEMORY) {
		return memc_ospi_AP_MEMORY_init(dev, ahb_clock_freq);
	}

	stm32_ospi_interface_init(dev_data, dev_cfg, ahb_clock_freq);

	ret = psram_force_spi_mode(&dev_data->hospi);
	if (ret != 0) {
		LOG_ERR("Failed to ENTER QSPI TO SPI mode and RESET");
		return ret;
	}

	/* Reset the PSRAM device */
	ret = stm32_ospi_mem_reset(dev);
	if (ret != 0) {
		LOG_ERR("Failed to reset PSRAM");
		return ret;
	}

	/* Read the PSRAM JEDEC ID and fill the dev_data->jedec_id Table */
	ret = stm32_ospi_read_jedec_id(dev);
	if (ret != 0) {
		LOG_ERR("Failed to read PSRAM JEDEC ID");
		return ret;
	}

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
