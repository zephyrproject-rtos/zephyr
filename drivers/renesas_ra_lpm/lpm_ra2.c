/**
 * Copyright (c) 2023-2024 MUNIC SA
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * @brief Driver for the Low Power Module (LPM) of RA2 family processor.
 *
 */
#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/lpm/lpm_ra2.h>
#include <zephyr/dt-bindings/lpm/lpm_ra2.h>
#include <zephyr/logging/log.h>
#include <soc.h>

#define DT_DRV_COMPAT	renesas_ra2_lpm
LOG_MODULE_REGISTER(lpm_ra2, CONFIG_RENESAS_RA_LPM_LOG_LEVEL);

#define LPM_NODE	DT_DRV_INST(0)

#define MSTP_BASE	DT_REG_ADDR_BY_NAME(LPM_NODE, mstp)

#define LPM_SBYCR	(SYSC_BASE + 0x00c)
#define LPM_SBYCR_SSBY	BIT(15)

#define SLEEP_MODE	(0 << 15)
#define STANDBY_MODE	LPM_SBYCR_SSBY

#define LPM_OPCCR	(SYSC_BASE + 0x0a0)
#define LPM_OPCCR_OPCM_Msk	GENMASK(1, 0)
#define LPM_OPCCR_OPCM(x)	((x) & LPM_OPCCR_OPCM_Msk)
#define LPM_OPCCR_OPCMTSF_Msk	BIT(4)

#define LPM_LPOPT	(SYSC_BASE + 0x04c)
#define LPM_LPOPT_LPOPTEN	BIT(7)

#define LPM_PSMCR	(SYSC_BASE + 0x09f)
#define SAVE_ALL_RAM	0
#define SAVE_HALF_RAM	1

#define LPM_LSMRWDIS	(MSTP_BASE + 0x00c)
#define LPM_LSMRWDIS_RTCRWDIS	BIT(0)
#define LPM_LSMRWDIS_WDTDIS	BIT(1)
#define LPM_LSMRWDIS_IWDTIDS	BIT(2)
#define LPM_LSMRWDIS_WREN	BIT(7)
#define LPM_LSMRWDIS_PRKEY_Pos	8
#define LPM_LSMRWDIS_PRKEY_Msk	GENMASK(15, LPM_LSMRWDIS_PRKEY_Pos)
#define LPM_LSMRWDIS_PRKEY(x)	\
			(((x)<<LPM_LSMRWDIS_PRKEY_Pos)&LPM_LSMRWDIS_PRKEY_Msk)

#define LPM_SYOCDCR	(SYSC_BASE + 0x40e)
#define LPM_DCDCCTL	(SYSC_BASE + 0x440)
#define LPM_VCCSEL	(SYSC_BASE + 0x441)

#define LPM_MSTPCRA	(SYSC_BASE + 0x1c)
#define LPM_MSTPCRB	(MSTP_BASE)
#define LPM_MSTPCRC	(MSTP_BASE + 0x4)
#define LPM_MSTPCRD	(MSTP_BASE + 0x8)

/** Also noted as: module-stop state entered and
 *	cancelled respectively
 */
#define LPM_MODULE_STATE_STOPPED	true
#define LPM_MODULE_STATE_RUN		false

static const struct {
	mm_reg_t base;
	uint32_t check_msk;
} mstpcr_map[] =  {
	{
		.base = LPM_MSTPCRA,
		.check_msk = BIT(22),
	},
	{
		.base = LPM_MSTPCRB,
		.check_msk = BIT(2)  |  BIT(8) |  BIT(9) | BIT(18) | BIT(19) |
			     BIT(22) | BIT(28) | BIT(29) | BIT(30) | BIT(31),
	},
	{
		.base = LPM_MSTPCRC,
		.check_msk = BIT(0)  |  BIT(1) |  BIT(3) | BIT(13) |
			     BIT(14) |  BIT(28) | BIT(31),
	},
	{
		.base = LPM_MSTPCRD,
		.check_msk = BIT(2)  |  BIT(3) |  BIT(5) | BIT(6)  |
			     BIT(14) | BIT(16) | BIT(20) | BIT(29),
	},
};

static struct k_spinlock lock;

static int lpm_ra_set_module_state(uint8_t mod, bool value)
{
	uint32_t id  = BIT((mod - 1) & LPM_RA_MSTPCR_Msk);
	uint32_t reg = (mod - 1) >> LPM_RA_MSTPCR_Pos;

	if (reg >= ARRAY_SIZE(mstpcr_map) ||
		(mstpcr_map[reg].check_msk & id) != id) {
		return -ENODEV;
	}

	K_SPINLOCK(&lock) {
		uint32_t val = sys_read32(mstpcr_map[reg].base);

		if (value) {
			val |= id;
		} else {
			val &= ~id;
		}

		sys_write32(val, mstpcr_map[reg].base);
	}

	return 0;
}

int lpm_ra_activate_module(unsigned int mod)
{
	return lpm_ra_set_module_state(mod, LPM_MODULE_STATE_RUN);
}

int lpm_ra_deactivate_module(unsigned int mod)
{
	return lpm_ra_set_module_state(mod, LPM_MODULE_STATE_STOPPED);
}

int lpm_ra_get_module_state(unsigned int mod)
{
	uint32_t id = (mod - 1) & LPM_RA_MSTPCR_Msk;
	uint32_t reg = (mod - 1) >> LPM_RA_MSTPCR_Pos;

	if (reg >= ARRAY_SIZE(mstpcr_map) ||
		(mstpcr_map[reg].check_msk & id) != id) {
		return -ENODEV;
	}

	return (sys_read32(mstpcr_map[reg].base) & id) ? 1 : 0;
}

#define OM_PROHIBITED_SPEED	2

int lpm_ra_set_op_mode(enum lpm_operating_modes mode)
{
	if ((mode & LPM_OPCCR_OPCM_Msk) == mode &&
			mode != OM_PROHIBITED_SPEED) {
		unsigned int key = irq_lock();
		uint16_t old_prcr = get_register_protection();

		set_register_protection(old_prcr | SYSC_PRCR_LP_PROT);

		sys_write8(LPM_OPCCR_OPCM(mode), LPM_OPCCR);

		set_register_protection(old_prcr);

		while (sys_read8(LPM_OPCCR) & LPM_OPCCR_OPCMTSF_Msk)
			;

		irq_unlock(key);
		return 0;
	}

	return -EINVAL;
}

enum lpm_operating_modes lpm_ra_get_op_mode(void)
{
	return (enum lpm_operating_modes)
			(sys_read8(LPM_OPCCR) & LPM_OPCCR_OPCM_Msk);
}

void lpm_enter_sleep(void)
{
	uint16_t old_prcr = get_register_protection();

	set_register_protection(old_prcr | SYSC_PRCR_LP_PROT);

	sys_write16(SLEEP_MODE, LPM_SBYCR);

	set_register_protection(old_prcr);

	k_cpu_idle();
}

void lpm_enter_standby(void)
{
	uint16_t old_prcr = get_register_protection();

	set_register_protection(old_prcr | SYSC_PRCR_LP_PROT);

	/* Activate low power mode */
	sys_write8(LPM_LPOPT_LPOPTEN, LPM_LPOPT);

	/* We wanna go to software standby mode */
	sys_write16(STANDBY_MODE, LPM_SBYCR);

	set_register_protection(old_prcr);
}

void lpm_leave_standby(void)
{
	uint16_t old_prcr = get_register_protection();

	set_register_protection(old_prcr | SYSC_PRCR_LP_PROT);

	sys_write16(SLEEP_MODE, LPM_SBYCR);

	/* Now that we've woken up, we deactivate low power mode */
	sys_write8(sys_read8(LPM_LPOPT) & ~LPM_LPOPT_LPOPTEN, LPM_LPOPT);

	set_register_protection(old_prcr);
}


int lpm_init(void)
{
	uint16_t old_prcr = get_register_protection();

	set_register_protection(old_prcr |
			SYSC_PRCR_CLK_PROT | SYSC_PRCR_LP_PROT);

	/* Settings for standby mode */
	/* ------------------------------------------------------------------ */
	/* We're suspending to RAM, so all the RAM must be maintained */
	sys_write8(SAVE_ALL_RAM, LPM_PSMCR);

	/* Magic word | Write bit on. */
	sys_write16(LPM_LSMRWDIS_PRKEY(0xA5) | LPM_LSMRWDIS_WREN, LPM_PSMCR);
	/* Magic word | Write bit off | Deactivate RTC in low power . */
	sys_write16(LPM_LSMRWDIS_PRKEY(0xA5) | LPM_LSMRWDIS_RTCRWDIS, LPM_PSMCR);

	sys_write8(0, LPM_LPOPT);

	/* ------------------------------------------------------------------ */
	/* Make sleep mode the default */
	sys_write16(SLEEP_MODE, LPM_SBYCR);

	set_register_protection(old_prcr);

	return 0;
}

SYS_INIT(lpm_init, PRE_KERNEL_1, 0);
