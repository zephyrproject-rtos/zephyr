/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT nxp_lpc546xx_eeprom

#include <zephyr/device.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/drivers/eeprom.h>
#include <zephyr/drivers/reset.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/sys_io.h>
#include <zephyr/sys/util.h>

#include <fsl_clock.h>
#include <fsl_eeprom.h>
#include <fsl_power.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(eeprom_mcux_lpc546xx, CONFIG_EEPROM_LOG_LEVEL);

/*
 * The controller registers and the storage array are two separate regions:
 * writes go through the controller, reads are ordinary loads from the array.
 */
BUILD_ASSERT(DT_INST_REG_ADDR_BY_NAME(0, memory) == FSL_FEATURE_EEPROM_BASE_ADDRESS,
	     "EEPROM memory reg does not match the SDK base address");
#define EEPROM_LPC546XX_PAGE_SIZE (FSL_FEATURE_EEPROM_SIZE / FSL_FEATURE_EEPROM_PAGE_COUNT)

/* The last page holds the controller's initialization data and is not writable. */
BUILD_ASSERT(DT_INST_PROP(0, size) <= (FSL_FEATURE_EEPROM_SIZE - EEPROM_LPC546XX_PAGE_SIZE),
	     "EEPROM size must exclude the last page of the array");

/* Internal clock range and wait-state minimums, LPC546xx data sheet, EEPROM characteristics. */
#define EEPROM_LPC546XX_CLK_MIN_HZ 800000U
#define EEPROM_LPC546XX_RPHASE1_NS 70U
#define EEPROM_LPC546XX_RPHASE2_NS 35U
#define EEPROM_LPC546XX_PHASE1_NS  20U
#define EEPROM_LPC546XX_PHASE2_NS  40U
#define EEPROM_LPC546XX_PHASE3_NS  10U

/* A page program takes 1.99 ms at 1.5 MHz and scales with the 0.8 MHz minimum clock. */
#define EEPROM_LPC546XX_PROGRAM_TIMEOUT_US 10000U

/* The SDK delays more than 100 us after powering the array before using it. */
#define EEPROM_LPC546XX_POWER_UP_US 150U

struct eeprom_lpc546xx_config {
	EEPROM_Type *base;
	mem_addr_t memory;
	size_t size;
	const struct device *clock_dev;
	clock_control_subsys_t clock_subsys;
	struct reset_dt_spec reset;
	bool read_only;
};

struct eeprom_lpc546xx_data {
	struct k_mutex lock;
};

/* Program the words staged in the page buffer and wait for the cycle to end. */
static int eeprom_lpc546xx_program(EEPROM_Type *base)
{
	EEPROM_ClearInterruptFlag(base, (uint32_t)kEEPROM_ProgramFinishInterruptEnable);
	base->CMD = FSL_FEATURE_EEPROM_PROGRAM_CMD;

	if (!WAIT_FOR((EEPROM_GetInterruptStatus(base) &
		       (uint32_t)kEEPROM_ProgramFinishInterruptEnable) != 0U,
		      EEPROM_LPC546XX_PROGRAM_TIMEOUT_US, k_busy_wait(10))) {
		LOG_ERR("page program timed out");
		return -EIO;
	}

	return 0;
}

static bool eeprom_lpc546xx_in_bounds(const struct eeprom_lpc546xx_config *config, off_t offset,
				      size_t len)
{
	return (offset >= 0) && ((size_t)offset <= config->size) &&
	       (len <= (config->size - (size_t)offset));
}

static int eeprom_lpc546xx_read(const struct device *dev, off_t offset, void *data, size_t len)
{
	const struct eeprom_lpc546xx_config *config = dev->config;
	struct eeprom_lpc546xx_data *dev_data = dev->data;

	if (len == 0U) {
		return 0;
	}

	if (!eeprom_lpc546xx_in_bounds(config, offset, len)) {
		LOG_ERR("attempt to read past device boundary");
		return -EINVAL;
	}

	k_mutex_lock(&dev_data->lock, K_FOREVER);
	memcpy(data, (const void *)(config->memory + (size_t)offset), len);
	k_mutex_unlock(&dev_data->lock);

	return 0;
}

static int eeprom_lpc546xx_write(const struct device *dev, off_t offset, const void *data,
				 size_t len)
{
	const struct eeprom_lpc546xx_config *config = dev->config;
	struct eeprom_lpc546xx_data *dev_data = dev->data;
	const uint8_t *src = data;
	size_t pos;
	int ret = 0;

	if (config->read_only) {
		return -EACCES;
	}

	if (len == 0U) {
		return 0;
	}

	if (!eeprom_lpc546xx_in_bounds(config, offset, len)) {
		LOG_ERR("attempt to write past device boundary");
		return -EINVAL;
	}

	pos = (size_t)offset;

	k_mutex_lock(&dev_data->lock, K_FOREVER);

	/*
	 * The array only takes word writes, so a byte-granular request becomes
	 * a read-modify-write of the partial words it touches. The hardware
	 * programs a page at a time; within a page, the partial first and last
	 * words are read before the first word is staged, because the array
	 * should not be read between staging writes and the program command.
	 *
	 * EEPROM_Write() does a similar thing but underflows its unsigned length
	 * when a write starts unaligned and is shorter than the rest of that
	 * word, so it is not used here.
	 */
	while (len > 0U) {
		size_t page_end =
			ROUND_DOWN(pos, EEPROM_LPC546XX_PAGE_SIZE) + EEPROM_LPC546XX_PAGE_SIZE;
		size_t seg = MIN(len, page_end - pos);
		size_t first = ROUND_DOWN(pos, sizeof(uint32_t));
		size_t last = ROUND_DOWN(pos + seg - 1U, sizeof(uint32_t));
		uint32_t head = 0U;
		uint32_t tail = 0U;

		if ((pos % sizeof(uint32_t)) != 0U) {
			head = sys_read32(config->memory + first);
		}
		if ((((pos + seg) % sizeof(uint32_t)) != 0U) &&
		    ((last != first) || ((pos % sizeof(uint32_t)) == 0U))) {
			tail = sys_read32(config->memory + last);
		}

		for (size_t word = first; word <= last; word += sizeof(uint32_t)) {
			size_t start = MAX(pos, word);
			size_t end = MIN(pos + seg, word + sizeof(uint32_t));
			/* Partial words start from the old data. */
			uint32_t value =
				(((pos % sizeof(uint32_t)) != 0U) && (word == first)) ? head : tail;

			memcpy((uint8_t *)&value + (start - word), src + (start - pos),
			       end - start);
			sys_write32(value, config->memory + word);
		}

		ret = eeprom_lpc546xx_program(config->base);
		if (ret != 0) {
			break;
		}

		src += seg;
		pos += seg;
		len -= seg;
	}

	k_mutex_unlock(&dev_data->lock);

	return ret;
}

static size_t eeprom_lpc546xx_size(const struct device *dev)
{
	const struct eeprom_lpc546xx_config *config = dev->config;

	return config->size;
}

/* Wait-state register value for a phase that must last at least @p ns: cycles - 1. */
static uint32_t eeprom_lpc546xx_wait_state(uint32_t clock_rate, uint32_t ns)
{
	uint32_t cycles = (uint32_t)DIV_ROUND_UP((uint64_t)clock_rate * ns, NSEC_PER_SEC);

	return MAX(cycles, 1U) - 1U;
}

static int eeprom_lpc546xx_init(const struct device *dev)
{
	const struct eeprom_lpc546xx_config *config = dev->config;
	struct eeprom_lpc546xx_data *data = dev->data;
	EEPROM_Type *base = config->base;
	uint32_t clock_rate;
	uint32_t clock_div;
	int ret;

	k_mutex_init(&data->lock);

	if (!device_is_ready(config->clock_dev) || !device_is_ready(config->reset.dev)) {
		return -ENODEV;
	}

	ret = clock_control_get_rate(config->clock_dev, config->clock_subsys, &clock_rate);
	if (ret != 0) {
		return ret;
	}

	clock_div = DIV_ROUND_UP(clock_rate, FSL_FEATURE_EEPROM_INTERNAL_FREQ);
	if ((clock_div == 0U) || ((clock_rate / clock_div) < EEPROM_LPC546XX_CLK_MIN_HZ) ||
	    ((clock_div - 1U) > EEPROM_CLKDIV_CLKDIV_MASK)) {
		LOG_ERR("unsupported clock rate %u Hz", clock_rate);
		return -EINVAL;
	}

	CLOCK_EnableClock(kCLOCK_Eeprom);

	ret = reset_line_toggle_dt(&config->reset);
	if (ret != 0) {
		return ret;
	}

	POWER_DisablePD(kPDRUNCFG_PD_EEPROM);
	k_busy_wait(EEPROM_LPC546XX_POWER_UP_US);

	base->CLKDIV = EEPROM_CLKDIV_CLKDIV(clock_div - 1U);
	/*
	 * Auto programming is off: it programs the page after every word written
	 * to the array, which collides with the explicit program this driver
	 * issues once per page and faults the bus.
	 */
	EEPROM_SetAutoProgram(base, kEEPROM_AutoProgramDisable);
	base->RWSTATE = EEPROM_RWSTATE_RPHASE1(eeprom_lpc546xx_wait_state(
				clock_rate, EEPROM_LPC546XX_RPHASE1_NS)) |
			EEPROM_RWSTATE_RPHASE2(
				eeprom_lpc546xx_wait_state(clock_rate, EEPROM_LPC546XX_RPHASE2_NS));
	base->WSTATE = EEPROM_WSTATE_PHASE1(
			       eeprom_lpc546xx_wait_state(clock_rate, EEPROM_LPC546XX_PHASE1_NS)) |
		       EEPROM_WSTATE_PHASE2(
			       eeprom_lpc546xx_wait_state(clock_rate, EEPROM_LPC546XX_PHASE2_NS)) |
		       EEPROM_WSTATE_PHASE3(
			       eeprom_lpc546xx_wait_state(clock_rate, EEPROM_LPC546XX_PHASE3_NS));

	return 0;
}

static DEVICE_API(eeprom, eeprom_lpc546xx_api) = {
	.read = eeprom_lpc546xx_read,
	.write = eeprom_lpc546xx_write,
	.size = eeprom_lpc546xx_size,
};

static struct eeprom_lpc546xx_data eeprom_lpc546xx_data_0;

static const struct eeprom_lpc546xx_config eeprom_lpc546xx_config_0 = {
	.base = (EEPROM_Type *)DT_INST_REG_ADDR_BY_NAME(0, control),
	.memory = DT_INST_REG_ADDR_BY_NAME(0, memory),
	.size = DT_INST_PROP(0, size),
	.clock_dev = DEVICE_DT_GET(DT_INST_CLOCKS_CTLR(0)),
	.clock_subsys = (clock_control_subsys_t)DT_INST_CLOCKS_CELL(0, name),
	.reset = RESET_DT_SPEC_INST_GET(0),
	.read_only = DT_INST_PROP(0, read_only),
};

DEVICE_DT_INST_DEFINE(0, eeprom_lpc546xx_init, NULL, &eeprom_lpc546xx_data_0,
		      &eeprom_lpc546xx_config_0, POST_KERNEL, CONFIG_EEPROM_INIT_PRIORITY,
		      &eeprom_lpc546xx_api);
