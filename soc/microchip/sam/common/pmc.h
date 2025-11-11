/*
 * Copyright (C) 2025 Microchip Technology Inc. and its subsidiaries
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 */

#ifndef __SAM_COMMON_PMC_H_
#define __SAM_COMMON_PMC_H_

#include <zephyr/drivers/clock_control/mchp_sam_pmc.h>
#include <zephyr/device.h>
#include <zephyr/spinlock.h>

#define		AT91_PMC_PLL_ACR_DEFAULT_UPLL	0x12020010	/* Default PLL ACR value for UPLL */
#define		AT91_PMC_PLL_ACR_DEFAULT_PLLA	0x00020010	/* Default PLL ACR value for PLLA */

#define		AT91_PMC_OSCBYPASS	(1    <<  1)		/* Oscillator Bypass */

struct pmc_data {
	unsigned int ncore;
	struct device **chws;
	unsigned int nsystem;
	struct device **shws;
	unsigned int nperiph;
	struct device **phws;
	unsigned int ngck;
	struct device **ghws;
	unsigned int npck;
	struct device **pchws;
};

struct clk_range {
	unsigned long min;
	unsigned long max;
};

struct clk_master_layout {
	uint32_t offset;
	uint32_t mask;
	uint8_t pres_shift;
};

struct clk_master_characteristics {
	struct clk_range output;
	uint32_t divisors[5];
	uint8_t have_div3_pres;
};

struct clk_pll_layout {
	uint32_t pllr_mask;
	uint32_t mul_mask;
	uint32_t frac_mask;
	uint32_t div_mask;
	uint32_t endiv_mask;
	uint32_t mul_shift;
	uint8_t frac_shift;
	uint8_t div_shift;
	uint8_t endiv_shift;
	uint8_t div2;
};

struct clk_pll_characteristics {
	struct clk_range input;
	int num_output;
	const struct clk_range *output;
	const struct clk_range *core_output;
	uint16_t *icpll;
	uint8_t *out;
	uint8_t upll : 1;
	uint32_t acr;
};

struct clk_programmable_layout {
	uint8_t pres_mask;
	uint8_t pres_shift;
	uint8_t css_mask;
	uint8_t have_slck_mck;
	uint8_t is_pres_direct;
};

struct clk_pcr_layout {
	uint32_t offset;
	uint32_t cmd;
	uint32_t gckcss_mask;
	uint32_t pid_mask;
};


static inline void regmap_read(const volatile uint32_t *map,
			       uint32_t reg, uint32_t *val)
{
	*val = *(volatile uint32_t *)((uint32_t)map + reg);
}

static inline void regmap_write(const volatile uint32_t *map,
				uint32_t reg, uint32_t val)
{
	*(volatile uint32_t *)((uint32_t)map + reg) = val;
}

static inline void regmap_update_bits(const volatile uint32_t *map,
				      uint32_t reg,
				      uint32_t mask, uint32_t val)
{
	uint32_t r = *(volatile uint32_t *)((uint32_t)map + reg);
	*(volatile uint32_t *)((uint32_t)map + reg) = (r & (~mask)) | val;
}

#define reg_update_bits(reg, mask, val) reg = (reg & (~mask)) | (val)

struct device *sam_pmc_get_clock(const struct sam_clk_cfg *cfg,
				 struct pmc_data *pmc_data);

struct pmc_data *pmc_data_allocate(unsigned int ncore, unsigned int nsystem,
				   unsigned int nperiph, unsigned int ngck,
				   unsigned int npck, struct device **table);


void sam_pmc_setup(const struct device *dev);

int clk_register_generated(pmc_registers_t *const pmc, struct k_spinlock *lock,
			   const struct clk_pcr_layout *layout,
			   const char *name,
			   const struct device **parents, uint32_t *mux_table,
			   uint8_t num_parents, uint8_t id,
			   const struct clk_range *range, int chg_pid, struct device **clk);

int clk_register_main_rc_osc(pmc_registers_t *const pmc, const char *name,
			     uint32_t frequency, struct device **clk);

int clk_register_main_osc(pmc_registers_t *const pmc, const char *name, uint32_t bypass,
			  uint32_t frequency, struct device **clk);

int clk_register_main(pmc_registers_t *const pmc, const char *name,
		      const struct device **parents, int num_parents, struct device **clk);

int clk_register_master_div(pmc_registers_t *const pmc, const char *name,
			    const struct device *parent, const struct clk_master_layout *layout,
			    const struct clk_master_characteristics *characteristics,
			    struct k_spinlock *lock, uint32_t safe_div, struct device **clk);

int clk_register_master(pmc_registers_t *const pmc,
			const char *name, int num_parents,
			const struct device **parents,
			uint32_t *mux_table,
			struct k_spinlock *lock, uint8_t id,
			struct device **clk);

int clk_register_peripheral(pmc_registers_t *const pmc,
			    struct k_spinlock *lock,
			    const struct clk_pcr_layout *layout,
			    const char *name,
			    struct device *parent,
			    uint32_t id,
			    const struct clk_range *range,
			    struct device **clk);

int sam9x60_clk_register_div_pll(pmc_registers_t *const pmc, struct k_spinlock *lock,
				 const char *name,
				 const struct device *parent, uint8_t id,
				 const struct clk_pll_characteristics *characteristics,
				 const struct clk_pll_layout *layout,
				 struct device **clk);

int sam9x60_clk_register_frac_pll(pmc_registers_t *const pmc, struct k_spinlock *lock,
				  const char *name,
				  const struct device *parent, uint8_t id,
				  const struct clk_pll_characteristics *characteristics,
				  const struct clk_pll_layout *layout, struct device **clk);

int clk_register_programmable(pmc_registers_t *const pmc,
			      const struct device **parents,
			      uint8_t num_parents, uint8_t id,
			      const struct clk_programmable_layout *layout,
			      uint32_t *mux_table, struct device **clk);

int clk_register_system(pmc_registers_t *const pmc, const char *name,
			const struct device *parent,
			uint8_t id, struct device **clk);

int clk_register_utmi(pmc_registers_t *const pmc, const char *name,
		      const struct device *parent, struct device **clk);

#endif /* __SAM_COMMON_PMC_H_ */
