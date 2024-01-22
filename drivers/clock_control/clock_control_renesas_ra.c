/*
 * Copyright (c) 2023 TOKITA Hiroshi <tokita.hiroshi@fujitsu.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <string.h>

#define DT_DRV_COMPAT renesas_ra_clock_generation_circuit

#include <zephyr/drivers/clock_control.h>
#include <zephyr/kernel.h>
#include <soc.h>
#include <zephyr/dt-bindings/clock/renesas-ra-cgc.h>

#if DT_SAME_NODE(DT_INST_PROP(0, clock_source), DT_PATH(clocks, pll))
#define SYSCLK_SRC pll
#elif DT_SAME_NODE(DT_INST_PROP(0, clock_source), DT_PATH(clocks, mosc))
#define SYSCLK_SRC mosc
#elif DT_SAME_NODE(DT_INST_PROP(0, clock_source), DT_PATH(clocks, sosc))
#define SYSCLK_SRC sosc
#elif DT_SAME_NODE(DT_INST_PROP(0, clock_source), DT_PATH(clocks, hoco))
#define SYSCLK_SRC hoco
#elif DT_SAME_NODE(DT_INST_PROP(0, clock_source), DT_PATH(clocks, moco))
#define SYSCLK_SRC moco
#elif DT_SAME_NODE(DT_INST_PROP(0, clock_source), DT_PATH(clocks, loco))
#define SYSCLK_SRC loco
#else
#error Unknown clock source
#endif

#define FREQ_iclk  (clock_freqs[_CONCAT(SCRSCK_, SYSCLK_SRC)] / DT_INST_PROP(0, iclk_div))
#define FREQ_pclka (clock_freqs[_CONCAT(SCRSCK_, SYSCLK_SRC)] / DT_INST_PROP(0, pclka_div))
#define FREQ_pclkb (clock_freqs[_CONCAT(SCRSCK_, SYSCLK_SRC)] / DT_INST_PROP(0, pclkb_div))
#define FREQ_pclkc (clock_freqs[_CONCAT(SCRSCK_, SYSCLK_SRC)] / DT_INST_PROP(0, pclkc_div))
#define FREQ_pclkd (clock_freqs[_CONCAT(SCRSCK_, SYSCLK_SRC)] / DT_INST_PROP(0, pclkd_div))
#define FREQ_fclk  (clock_freqs[_CONCAT(SCRSCK_, SYSCLK_SRC)] / DT_INST_PROP(0, fclk_div))

#define CLKSRC_FREQ(clk) DT_PROP(DT_PATH(clocks, clk), clock_frequency)

#define IS_CLKSRC_ENABLED(clk) DT_NODE_HAS_STATUS(DT_PATH(clocks, clk), okay)

#define SCKSCR_INIT_VALUE _CONCAT(CLKSRC_, SYSCLK_SRC)

#define SCKDIV_ENABLED(clk) DT_INST_NODE_HAS_PROP(0, clk##_div)
#define SCKDIV_VAL(clk)     _CONCAT(SCKDIV_, DT_INST_PROP(0, clk##_div))
#define SCKDIV_POS(clk)     _CONCAT(SCKDIV_POS_, clk)

#define SCKDIVCR_BITS(clk)                                                                         \
	COND_CODE_1(SCKDIV_ENABLED(clk), ((SCKDIV_VAL(clk) & 0xFU) << SCKDIV_POS(clk)), (0U))

#define SCKDIVCR_INIT_VALUE                                                                        \
	(SCKDIVCR_BITS(iclk) | SCKDIVCR_BITS(pclka) | SCKDIVCR_BITS(pclkb) |                       \
	 SCKDIVCR_BITS(pclkc) | SCKDIVCR_BITS(pclkd) | SCKDIVCR_BITS(bclk) | SCKDIVCR_BITS(fclk))

#define HOCOWTCR_INIT_VALUE (6)

/*
 * Required cycles for sub-clokc stabilizing.
 */
#define SUBCLK_STABILIZE_CYCLES 5

extern int z_clock_hw_cycles_per_sec;

enum {
	CLKSRC_hoco = 0,
	CLKSRC_moco,
	CLKSRC_loco,
	CLKSRC_mosc,
	CLKSRC_sosc,
	CLKSRC_pll,
};

enum {
	SCKDIV_1 = 0,
	SCKDIV_2,
	SCKDIV_4,
	SCKDIV_8,
	SCKDIV_16,
	SCKDIV_32,
	SCKDIV_64,
	SCKDIV_128,
	SCKDIV_3,
	SCKDIV_6,
	SCKDIV_12
};

enum {
	SCKDIV_POS_pclkd = 0x0U,
	SCKDIV_POS_pclkc = 0x4U,
	SCKDIV_POS_pclkb = 0x8U,
	SCKDIV_POS_pclka = 0xcU,
	SCKDIV_POS_bclk = 0x10U,
	SCKDIV_POS_pclke = 0x14U,
	SCKDIV_POS_iclk = 0x18U,
	SCKDIV_POS_fclk = 0x1cU
};

enum {
	OSCSF_HOCOSF_POS = 0,
	OSCSF_MOSCSF_POS = 3,
	OSCSF_PLLSF_POS = 5,
};

enum {
	OPCCR_OPCMTSF_POS = 4,
};

static const uint32_t PRCR_KEY = 0xA500U;
static const uint32_t PRCR_CLOCKS = 0x1U;
static const uint32_t PRCR_LOW_POWER = 0x2U;

enum {
#if DT_INST_REG_SIZE_BY_NAME(0, mstp) == 16
	MSTPCRA_OFFSET = -0x4,
#else
	MSTPCRA_OFFSET = 0x0,
#endif
	MSTPCRB_OFFSET = (MSTPCRA_OFFSET + 0x4),
	MSTPCRC_OFFSET = (MSTPCRB_OFFSET + 0x4),
	MSTPCRD_OFFSET = (MSTPCRC_OFFSET + 0x4),
	MSTPCRE_OFFSET = (MSTPCRD_OFFSET + 0x4),
};

enum {
	SCKDIVCR_OFFSET = 0x020,
	SCKSCR_OFFSET = 0x026,
	MEMWAIT_OFFSET = 0x031,
	MOSCCR_OFFSET = 0x032,
	HOCOCR_OFFSET = 0x036,
	OSCSF_OFFSET = 0x03C,
	CKOCR_OFFSET = 0x03E,
	OPCCR_OFFSET = 0x0A0,
	HOCOWTCR_OFFSET = 0x0A5,
	PRCR_OFFSET = 0x3FE,
	SOSCCR_OFFSET = 0x480,
};

enum {
	SCRSCK_hoco,
	SCRSCK_moco,
	SCRSCK_loco,
	SCRSCK_mosc,
	SCRSCK_sosc,
	SCRSCK_pll,
};

static const int clock_freqs[] = {
	COND_CODE_1(IS_CLKSRC_ENABLED(hoco), (CLKSRC_FREQ(hoco)), (0)),
	COND_CODE_1(IS_CLKSRC_ENABLED(moco), (CLKSRC_FREQ(moco)), (0)),
	COND_CODE_1(IS_CLKSRC_ENABLED(loco), (CLKSRC_FREQ(loco)), (0)),
	COND_CODE_1(IS_CLKSRC_ENABLED(mosc), (CLKSRC_FREQ(mosc)), (0)),
	COND_CODE_1(IS_CLKSRC_ENABLED(sosc), (CLKSRC_FREQ(sosc)), (0)),
	COND_CODE_1(IS_CLKSRC_ENABLED(pll),
		    (DT_PROP(DT_PHANDLE_BY_IDX(DT_PATH(clocks, pll), clocks, 0), clock_frequency) *
		     DT_PROP(DT_PATH(clocks, pll), clock_mult) /
		     DT_PROP(DT_PATH(clocks, pll), clock_div)),
		    (0)),
};

static uint32_t MSTP_read(size_t offset)
{
	return sys_read32(DT_INST_REG_ADDR_BY_NAME(0, mstp) + offset);
}

static void MSTP_write(size_t offset, uint32_t value)
{
	sys_write32(value, DT_INST_REG_ADDR_BY_NAME(0, mstp) + offset);
}

static uint8_t SYSTEM_read8(size_t offset)
{
	return sys_read8(DT_INST_REG_ADDR_BY_NAME(0, system) + offset);
}

static void SYSTEM_write8(size_t offset, uint8_t value)
{
	sys_write8(value, DT_INST_REG_ADDR_BY_NAME(0, system) + offset);
}

static void SYSTEM_write16(size_t offset, uint16_t value)
{
	sys_write16(value, DT_INST_REG_ADDR_BY_NAME(0, system) + offset);
}

static void SYSTEM_write32(size_t offset, uint32_t value)
{
	sys_write32(value, DT_INST_REG_ADDR_BY_NAME(0, system) + offset);
}

static int clock_control_ra_on(const struct device *dev, clock_control_subsys_t subsys)
{
	uint32_t clkid = (uint32_t)subsys;
	int lock = irq_lock();

	MSTP_write(MSTPCRA_OFFSET + RA_CLOCK_GROUP(clkid),
		   MSTP_read(MSTPCRB_OFFSET) & ~RA_CLOCK_BIT(clkid));
	irq_unlock(lock);

	return 0;
}

static int clock_control_ra_off(const struct device *dev, clock_control_subsys_t subsys)
{
	uint32_t clkid = (uint32_t)subsys;
	int lock = irq_lock();

	MSTP_write(MSTPCRA_OFFSET + RA_CLOCK_GROUP(clkid),
		   MSTP_read(MSTPCRB_OFFSET) | RA_CLOCK_BIT(clkid));
	irq_unlock(lock);

	return 0;
}

static int clock_control_ra_get_rate(const struct device *dev, clock_control_subsys_t subsys,
				     uint32_t *rate)
{
	uint32_t clkid = (uint32_t)subsys;

	switch (clkid & 0xFFFFFF00) {
	case RA_CLOCK_SCI(0):
		*rate = FREQ_pclka;
		break;
	default:
		return -EINVAL;
	}

	return 0;
}

static const struct clock_control_driver_api ra_clock_control_driver_api = {
	.on = clock_control_ra_on,
	.off = clock_control_ra_off,
	.get_rate = clock_control_ra_get_rate,
};

static void crude_busy_loop_impl(uint32_t cycles)
{
	__asm__ volatile(".align 8\n"
			 "busy_loop:\n"
			 "	sub	r0, r0, #1\n"
			 "	cmp	r0, #0\n"
			 "	bne.n	busy_loop\n");
}

static inline void crude_busy_loop(uint32_t wait_us)
{
	static const uint64_t cycles_per_loop = 4;

	crude_busy_loop_impl(sys_clock_hw_cycles_per_sec() * wait_us / USEC_PER_SEC /
			     cycles_per_loop);
}

static int clock_control_ra_init(const struct device *dev)
{
	uint8_t sysclk = SYSTEM_read8(SCKSCR_OFFSET);

	z_clock_hw_cycles_per_sec = clock_freqs[sysclk];

	SYSTEM_write16(PRCR_OFFSET, PRCR_KEY | PRCR_CLOCKS | PRCR_LOW_POWER);

	if (clock_freqs[SCRSCK_hoco] == 64000000) {
		SYSTEM_write8(HOCOWTCR_OFFSET, HOCOWTCR_INIT_VALUE);
	}

	SYSTEM_write8(SOSCCR_OFFSET, !IS_CLKSRC_ENABLED(sosc));
	SYSTEM_write8(MOSCCR_OFFSET, !IS_CLKSRC_ENABLED(mosc));
	SYSTEM_write8(HOCOCR_OFFSET, !IS_CLKSRC_ENABLED(hoco));

	if (IS_CLKSRC_ENABLED(sosc)) {
		crude_busy_loop(z_clock_hw_cycles_per_sec / clock_freqs[CLKSRC_sosc] *
				SUBCLK_STABILIZE_CYCLES);
	}

	if (IS_CLKSRC_ENABLED(mosc)) {
		while ((SYSTEM_read8(OSCSF_OFFSET) & BIT(OSCSF_MOSCSF_POS)) !=
		       BIT(OSCSF_MOSCSF_POS)) {
			;
		}
	}

	if (IS_CLKSRC_ENABLED(hoco)) {
		while ((SYSTEM_read8(OSCSF_OFFSET) & BIT(OSCSF_HOCOSF_POS)) !=
		       BIT(OSCSF_HOCOSF_POS)) {
			;
		}
	}

	SYSTEM_write32(SCKDIVCR_OFFSET, SCKDIVCR_INIT_VALUE);
	SYSTEM_write8(SCKSCR_OFFSET, SCKSCR_INIT_VALUE);

	/* re-read system clock setting and apply to hw_cycles */
	sysclk = SYSTEM_read8(SCKSCR_OFFSET);
	z_clock_hw_cycles_per_sec = clock_freqs[sysclk];

	SYSTEM_write8(OPCCR_OFFSET, 0);
	while ((SYSTEM_read8(OPCCR_OFFSET) & BIT(OPCCR_OPCMTSF_POS)) != 0) {
		;
	}

	SYSTEM_write8(MEMWAIT_OFFSET, 1);
	SYSTEM_write16(PRCR_OFFSET, PRCR_KEY);

	return 0;
}

DEVICE_DT_INST_DEFINE(0, &clock_control_ra_init, NULL, NULL, NULL, PRE_KERNEL_1,
		      CONFIG_CLOCK_CONTROL_INIT_PRIORITY, &ra_clock_control_driver_api);
