/*
 * Copyright (c) 2026 Jonathan Elliot Peace <jep@alphabetiq.com>
 * SPDX-License-Identifier: Apache-2.0
 *
 * Chip topology, AI core helpers, and initialization state transitions.
 * Mirrors Linux brcmfmac/chip.c (DMP/PL-368 descriptor spec for the
 * EROM scan; brcmf_chip_ai_{iscoreup,coredisable,resetcore} for AI
 * core control; brcmf_chip_cm3_set_{passive,active} for CM3 chips).
 */

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/sd/sdio.h>

#include "brcmfmac_priv.h"

LOG_MODULE_DECLARE(brcmfmac, CONFIG_WIFI_LOG_LEVEL);

/* === EROM (DMP/PL-368) descriptor format =================================== */

#define DMP_DESC_TYPE_MSK       0x0000000F
#define DMP_DESC_EMPTY          0x00000000
#define DMP_DESC_VALID          0x00000001
#define DMP_DESC_COMPONENT      0x00000001
#define DMP_DESC_MASTER_PORT    0x00000003
#define DMP_DESC_ADDRESS        0x00000005
#define DMP_DESC_ADDRSIZE_GT32  0x00000008
#define DMP_DESC_EOT            0x0000000F

#define DMP_COMP_PARTNUM        0x000FFF00
#define DMP_COMP_PARTNUM_S      8

#define DMP_COMP_NUM_SWRAP      0x00F80000
#define DMP_COMP_NUM_SWRAP_S    19
#define DMP_COMP_NUM_MWRAP      0x0007C000
#define DMP_COMP_NUM_MWRAP_S    14

#define DMP_SLAVE_ADDR_BASE     0xFFFFF000
#define DMP_SLAVE_TYPE          0x000000C0
#define DMP_SLAVE_TYPE_S        6
#define DMP_SLAVE_TYPE_SLAVE    0
#define DMP_SLAVE_TYPE_SWRAP    2
#define DMP_SLAVE_TYPE_MWRAP    3
#define DMP_SLAVE_SIZE_TYPE     0x00000030
#define DMP_SLAVE_SIZE_TYPE_S   4
#define DMP_SLAVE_SIZE_4K       0
#define DMP_SLAVE_SIZE_8K       1
#define DMP_SLAVE_SIZE_DESC     3

#define CC_EROMPTR_OFFSET       0xFC

static int dmp_get_desc(struct brcmfmac_data *data, uint32_t *erom_addr,
			uint32_t *val_out, uint8_t *type_out)
{
	int ret = brcmfmac_sdio_backplane_read32(data, *erom_addr, val_out);

	if (ret != 0) {
		return ret;
	}
	*erom_addr += 4;

	if (type_out != NULL) {
		uint8_t t = (uint8_t)(*val_out & DMP_DESC_TYPE_MSK);

		if ((t & ~DMP_DESC_ADDRSIZE_GT32) == DMP_DESC_ADDRESS) {
			t = DMP_DESC_ADDRESS;
		}
		*type_out = t;
	}
	return 0;
}

static int dmp_get_regaddr(struct brcmfmac_data *data, uint32_t *erom_addr,
			   uint32_t *regbase, uint32_t *wrapbase)
{
	uint8_t desc;
	uint32_t val;
	int ret;
	uint8_t wraptype;

	*regbase = 0;
	*wrapbase = 0;

	ret = dmp_get_desc(data, erom_addr, &val, &desc);
	if (ret != 0) {
		return ret;
	}

	if (desc == DMP_DESC_MASTER_PORT) {
		wraptype = DMP_SLAVE_TYPE_MWRAP;
	} else if (desc == DMP_DESC_ADDRESS) {
		*erom_addr -= 4;
		wraptype = DMP_SLAVE_TYPE_SWRAP;
	} else {
		*erom_addr -= 4;
		return -EILSEQ;
	}

	do {
		do {
			ret = dmp_get_desc(data, erom_addr, &val, &desc);
			if (ret != 0) {
				return ret;
			}
			if (desc == DMP_DESC_EOT) {
				*erom_addr -= 4;
				return -EFAULT;
			}
		} while (desc != DMP_DESC_ADDRESS && desc != DMP_DESC_COMPONENT);

		if (desc == DMP_DESC_COMPONENT) {
			*erom_addr -= 4;
			return 0;
		}

		if (val & DMP_DESC_ADDRSIZE_GT32) {
			uint32_t tmp;

			ret = dmp_get_desc(data, erom_addr, &tmp, NULL);
			if (ret != 0) {
				return ret;
			}
		}

		uint8_t sztype = (val & DMP_SLAVE_SIZE_TYPE) >> DMP_SLAVE_SIZE_TYPE_S;

		if (sztype == DMP_SLAVE_SIZE_DESC) {
			uint32_t szdesc;

			ret = dmp_get_desc(data, erom_addr, &szdesc, NULL);
			if (ret != 0) {
				return ret;
			}
			if (szdesc & DMP_DESC_ADDRSIZE_GT32) {
				uint32_t tmp;

				ret = dmp_get_desc(data, erom_addr, &tmp, NULL);
				if (ret != 0) {
					return ret;
				}
			}
		}

		if (sztype != DMP_SLAVE_SIZE_4K && sztype != DMP_SLAVE_SIZE_8K) {
			continue;
		}

		uint8_t stype = (val & DMP_SLAVE_TYPE) >> DMP_SLAVE_TYPE_S;

		if (*regbase == 0 && stype == DMP_SLAVE_TYPE_SLAVE) {
			*regbase = val & DMP_SLAVE_ADDR_BASE;
		}
		if (*wrapbase == 0 && stype == wraptype) {
			*wrapbase = val & DMP_SLAVE_ADDR_BASE;
		}
	} while (*regbase == 0 || *wrapbase == 0);

	return 0;
}

int brcmfmac_chip_erom_scan(struct brcmfmac_data *data)
{
	uint32_t erom_addr;
	int ret = brcmfmac_sdio_backplane_read32(data,
						 BRCMF_SI_ENUM_BASE + CC_EROMPTR_OFFSET,
						 &erom_addr);
	if (ret != 0) {
		LOG_ERR("erom_scan: read eromptr failed: %d", ret);
		return ret;
	}
	LOG_DBG("erom_scan: eromptr=0x%08x", erom_addr);

	data->num_cores = 0;
	uint8_t desc_type = 0;

	while (desc_type != DMP_DESC_EOT) {
		uint32_t val;

		ret = dmp_get_desc(data, &erom_addr, &val, &desc_type);
		if (ret != 0) {
			LOG_ERR("erom_scan: desc read failed: %d", ret);
			return ret;
		}
		if (!(val & DMP_DESC_VALID)) {
			continue;
		}
		if (desc_type == DMP_DESC_EMPTY) {
			continue;
		}
		if (desc_type != DMP_DESC_COMPONENT) {
			continue;
		}

		uint16_t id = (val & DMP_COMP_PARTNUM) >> DMP_COMP_PARTNUM_S;

		uint32_t ident_b;

		ret = dmp_get_desc(data, &erom_addr, &ident_b, &desc_type);
		if (ret != 0) {
			return ret;
		}
		if ((ident_b & DMP_DESC_TYPE_MSK) != DMP_DESC_COMPONENT) {
			LOG_ERR("erom_scan: expected CompIdentB, got 0x%08x", ident_b);
			return -EILSEQ;
		}

		uint8_t nmw = (ident_b & DMP_COMP_NUM_MWRAP) >> DMP_COMP_NUM_MWRAP_S;
		uint8_t nsw = (ident_b & DMP_COMP_NUM_SWRAP) >> DMP_COMP_NUM_SWRAP_S;

		if (nmw + nsw == 0 && id != BCMA_CORE_PMU && id != BCMA_CORE_GCI) {
			continue;
		}

		uint32_t base = 0, wrap = 0;

		ret = dmp_get_regaddr(data, &erom_addr, &base, &wrap);
		if (ret != 0) {
			continue;
		}

		if (data->num_cores >= BRCMFMAC_MAX_CORES) {
			LOG_WRN("erom_scan: MAX_CORES overflow at id=0x%03x", id);
			continue;
		}
		data->cores[data->num_cores].id = id;
		data->cores[data->num_cores].base = base;
		data->cores[data->num_cores].wrapbase = wrap;
		LOG_DBG("erom_scan: core[%u] id=0x%03x base=0x%08x wrap=0x%08x",
			data->num_cores, id, base, wrap);
		data->num_cores++;
	}

	LOG_DBG("erom_scan: discovered %u cores", data->num_cores);
	return 0;
}

const struct bcm_core *brcmfmac_chip_core_find(const struct brcmfmac_data *data,
					       uint16_t id)
{
	for (unsigned int i = 0; i < data->num_cores; i++) {
		if (data->cores[i].id == id) {
			return &data->cores[i];
		}
	}
	return NULL;
}

/* === AI core reset/enable helpers ==========================================
 *
 * Each core has a "wrapper" address space (separate from its data base)
 * where IOCTL and RESET_CTL live.
 */

static bool ai_iscoreup(struct brcmfmac_data *data, uint32_t wrap)
{
	uint32_t v;

	if (brcmfmac_sdio_backplane_read32(data, wrap + BCMA_IOCTL, &v) != 0) {
		return false;
	}
	bool clk_ok = (v & (BCMA_IOCTL_FGC | BCMA_IOCTL_CLK)) == BCMA_IOCTL_CLK;

	if (brcmfmac_sdio_backplane_read32(data, wrap + BCMA_RESET_CTL, &v) != 0) {
		return false;
	}
	return clk_ok && ((v & BCMA_RESET_CTL_RESET) == 0);
}

static int ai_coredisable(struct brcmfmac_data *data, uint32_t wrap,
			  uint32_t prereset, uint32_t reset)
{
	uint32_t v;
	int ret = brcmfmac_sdio_backplane_read32(data, wrap + BCMA_RESET_CTL, &v);

	if (ret != 0) {
		return ret;
	}

	if ((v & BCMA_RESET_CTL_RESET) == 0) {
		(void)brcmfmac_sdio_backplane_write32(data, wrap + BCMA_IOCTL,
						      prereset | BCMA_IOCTL_FGC | BCMA_IOCTL_CLK);
		(void)brcmfmac_sdio_backplane_read32(data, wrap + BCMA_IOCTL, &v);

		(void)brcmfmac_sdio_backplane_write32(data, wrap + BCMA_RESET_CTL,
						      BCMA_RESET_CTL_RESET);
		k_busy_wait(20);

		for (int i = 0; i < 300; i++) {
			(void)brcmfmac_sdio_backplane_read32(data, wrap + BCMA_RESET_CTL, &v);
			if (v == BCMA_RESET_CTL_RESET) {
				break;
			}
			k_busy_wait(1);
		}
	}

	(void)brcmfmac_sdio_backplane_write32(data, wrap + BCMA_IOCTL,
					      reset | BCMA_IOCTL_FGC | BCMA_IOCTL_CLK);
	(void)brcmfmac_sdio_backplane_read32(data, wrap + BCMA_IOCTL, &v);
	return 0;
}

static int ai_resetcore(struct brcmfmac_data *data, uint32_t wrap,
			uint32_t prereset, uint32_t reset, uint32_t postreset)
{
	uint32_t v;
	int ret = ai_coredisable(data, wrap, prereset, reset);

	if (ret != 0) {
		return ret;
	}

	for (int count = 0; count < 50; count++) {
		(void)brcmfmac_sdio_backplane_read32(data, wrap + BCMA_RESET_CTL, &v);
		if ((v & BCMA_RESET_CTL_RESET) == 0) {
			break;
		}
		(void)brcmfmac_sdio_backplane_write32(data, wrap + BCMA_RESET_CTL, 0);
		k_busy_wait(50);
	}

	(void)brcmfmac_sdio_backplane_write32(data, wrap + BCMA_IOCTL,
					      postreset | BCMA_IOCTL_CLK);
	(void)brcmfmac_sdio_backplane_read32(data, wrap + BCMA_IOCTL, &v);
	return 0;
}

/* === chipid + PMU/CHIPCLKCSR setup ========================================= */

int brcmfmac_chip_read_id(struct brcmfmac_data *data)
{
	int ret = brcmfmac_sdio_backplane_read32(data, BRCMF_SI_ENUM_BASE,
						 &data->chipid_reg);
	if (ret != 0) {
		LOG_ERR("chipid read failed: %d", ret);
		return ret;
	}
	data->chip_id   = data->chipid_reg & CID_ID_MASK;
	data->chip_rev  = (data->chipid_reg & CID_REV_MASK) >> CID_REV_SHIFT;
	data->chip_type = (data->chipid_reg & CID_TYPE_MASK) >> CID_TYPE_SHIFT;

	LOG_INF("chipid = 0x%08x  chip=%u (0x%04x) rev=%u type=%u (%s)",
		data->chipid_reg, data->chip_id, data->chip_id,
		data->chip_rev, data->chip_type,
		data->chip_type == 0 ? "SB" :
		(data->chip_type == 1 ? "AXI" : "?"));
	return 0;
}

int brcmfmac_chip_ram_data(struct brcmfmac_data *data)
{
	/* TODO: Read ram_size from core registers like in Linux. */
	if (data->chip_id == BRCM_CC_43430_CHIP_ID ||
	    data->chip_id == BRCM_CC_43439_CHIP_ID) {
		data->ram_base = 0u;
		data->ram_size = 0x80000u;
	} else if (data->chip_id == BRCM_CC_4345_CHIP_ID) {
		data->ram_base = 0x198000u;
		data->ram_size = 0xC8000u;
	} else {
		LOG_ERR("ram_data: don't know RAM addresses for this chip");
		return -1;
	}

	return 0;
}

int brcmfmac_chip_pmu_setup(struct brcmfmac_data *data)
{
	int ret = sdio_write_byte(&data->backplane, SBSDIO_FUNC1_CHIPCLKCSR,
				  BRCMF_INIT_CLKCTL1);
	if (ret != 0) {
		LOG_ERR("CHIPCLKCSR write(0x%02x) failed: %d", BRCMF_INIT_CLKCTL1, ret);
		return ret;
	}

	uint8_t clkval = 0;

	for (int i = 0; i < 100; i++) {
		ret = sdio_read_byte(&data->backplane, SBSDIO_FUNC1_CHIPCLKCSR, &clkval);
		if (ret != 0) {
			LOG_ERR("CHIPCLKCSR read failed: %d", ret);
			return ret;
		}
		if (clkval & SBSDIO_AVBITS) {
			break;
		}
		k_msleep(1);
	}
	if (!(clkval & SBSDIO_AVBITS)) {
		LOG_ERR("ALP/HT never available (CHIPCLKCSR=0x%02x)", clkval);
		return -ETIMEDOUT;
	}
	LOG_DBG("CHIPCLKCSR=0x%02x (%s%s)", clkval,
		(clkval & SBSDIO_ALP_AVAIL) ? "ALP " : "",
		(clkval & SBSDIO_HT_AVAIL) ? "HT" : "");

	ret = sdio_write_byte(&data->backplane, SBSDIO_FUNC1_CHIPCLKCSR,
			      SBSDIO_FORCE_HW_CLKREQ_OFF | SBSDIO_FORCE_ALP);
	if (ret != 0) {
		LOG_ERR("CHIPCLKCSR force-ALP write failed: %d", ret);
		return ret;
	}
	k_busy_wait(65);

	ret = sdio_write_byte(&data->backplane, SBSDIO_FUNC1_SDIOPULLUP, 0);
	if (ret != 0) {
		LOG_ERR("SDIOPULLUP=0 write failed: %d", ret);
		return ret;
	}
	return 0;
}

static int brcmfmac_chip_ht_bringup(struct brcmfmac_data *data)
{
	int ret;
	uint8_t clkcsr = 0;
	int64_t t0 = k_uptime_get();

	for (int i = 0; i < 500; i++) {
		ret = sdio_read_byte(&data->backplane, SBSDIO_FUNC1_CHIPCLKCSR, &clkcsr);
		if (ret != 0) {
			LOG_ERR("ht_bringup: CHIPCLKCSR read failed: %d", ret);
			return ret;
		}
		if (clkcsr & SBSDIO_HT_AVAIL) {
			break;
		}
		k_msleep(1);
	}
	int64_t t_wait = k_uptime_get() - t0;

	if (!(clkcsr & SBSDIO_HT_AVAIL)) {
		LOG_ERR("ht_bringup: HT_AVAIL timeout (CHIPCLKCSR=0x%02x after %lld ms)",
			clkcsr, (long long)t_wait);
		return -ETIMEDOUT;
	}
	LOG_DBG("ht_bringup: HT_AVAIL after %lld ms (CHIPCLKCSR=0x%02x)",
		(long long)t_wait, clkcsr);

	uint32_t chipid;

	ret = brcmfmac_sdio_backplane_read32(data, BRCMF_SI_ENUM_BASE, &chipid);
	if (ret != 0) {
		LOG_ERR("ht_bringup: post-boot chipid read failed: %d", ret);
		return ret;
	}
	LOG_DBG("ht_bringup: post-boot chipid = 0x%08x (chip alive, fw running)",
		chipid);
	return 0;
}

/* === set_passive: prepare for firmware upload ==============================
 *
 * Mirrors Linux brcmf_chip_cm3_set_passive:
 *   1. halt ARM CM3 (so it doesn't fight us during the upload)
 *   2. reset D11 MAC with PHYRESET + PHYCLOCKEN active
 *   3. reset SOCRAM (brings it out of POR to operational)
 *   4. disable bank-3 remap (BCM43430-specific quirk)
 */
static int brcmfmac_chip_cm3_set_passive(struct brcmfmac_data *data)
{
	const struct bcm_core *arm = brcmfmac_chip_core_find(data, BCMA_CORE_ARM_CM3);
	const struct bcm_core *d11 = brcmfmac_chip_core_find(data, BCMA_CORE_80211);
	const struct bcm_core *sr  = brcmfmac_chip_core_find(data, BCMA_CORE_INTERNAL_MEM);

	if (arm == NULL || d11 == NULL || sr == NULL) {
		LOG_ERR("set_passive: missing core(s) arm=%p d11=%p sr=%p",
			(const void *)arm, (const void *)d11, (const void *)sr);
		return -ENODEV;
	}

	LOG_DBG("set_passive: halt ARM CM3 (wrap=0x%08x)", arm->wrapbase);
	int ret = ai_coredisable(data, arm->wrapbase, 0, 0);

	if (ret != 0) {
		LOG_ERR("set_passive: CM3 disable failed: %d", ret);
		return ret;
	}

	LOG_DBG("set_passive: reset D11 PHY (wrap=0x%08x)", d11->wrapbase);
	ret = ai_resetcore(data, d11->wrapbase,
			   D11_BCMA_IOCTL_PHYRESET | D11_BCMA_IOCTL_PHYCLOCKEN,
			   D11_BCMA_IOCTL_PHYCLOCKEN,
			   D11_BCMA_IOCTL_PHYCLOCKEN);
	if (ret != 0) {
		LOG_ERR("set_passive: D11 reset failed: %d", ret);
		return ret;
	}

	LOG_DBG("set_passive: reset SOCRAM (wrap=0x%08x base=0x%08x)",
		sr->wrapbase, sr->base);
	ret = ai_resetcore(data, sr->wrapbase, 0, 0, 0);
	if (ret != 0) {
		LOG_ERR("set_passive: SOCRAM reset failed: %d", ret);
		return ret;
	}

	/*
	 * SOCRAM bank-3 remap disable. Linux brcmfmac applies this only to
	 * BCM43430 and CYW43439 -- on other chips bank 3 is either not
	 * present (BCM4329, BCM43236) or has a different PDA layout
	 * (BCM43340, BCM4334) and the write would misconfigure RAM.
	 */
	if (data->chip_id == BRCM_CC_43430_CHIP_ID ||
	    data->chip_id == BRCM_CC_43439_CHIP_ID) {
		(void)brcmfmac_sdio_backplane_write32(data,
				sr->base + SOCRAM_BANKIDX_OFFSET, 3);
		(void)brcmfmac_sdio_backplane_write32(data,
				sr->base + SOCRAM_BANKPDA_OFFSET, 0);
		LOG_DBG("set_passive: bank-3 remap disabled");
	}

	if (!ai_iscoreup(data, sr->wrapbase)) {
		LOG_ERR("set_passive: SOCRAM did not come up");
		return -EIO;
	}
	LOG_DBG("set_passive: SOCRAM is up");
	return 0;
}

/* === set_active: release ARM CM3 + confirm firmware boot =================== */

static int brcmfmac_chip_cm3_set_active(struct brcmfmac_data *data)
{
	const struct bcm_core *sdio_dev = brcmfmac_chip_core_find(data, BCMA_CORE_SDIO_DEV);
	const struct bcm_core *arm     = brcmfmac_chip_core_find(data, BCMA_CORE_ARM_CM3);

	if (sdio_dev == NULL || arm == NULL) {
		LOG_ERR("set_active: missing core(s) sdio=%p arm=%p",
			(const void *)sdio_dev, (const void *)arm);
		return -ENODEV;
	}

	LOG_DBG("set_active: clear SDIO core intstatus");
	int ret = brcmfmac_sdio_backplane_write32(data,
						  sdio_dev->base + SDPCMD_INTSTATUS,
						  0xFFFFFFFFu);
	if (ret != 0) {
		LOG_ERR("set_active: intstatus clear failed: %d", ret);
		return ret;
	}

	LOG_DBG("set_active: release ARM CM3");
	ret = ai_resetcore(data, arm->wrapbase, 0, 0, 0);
	if (ret != 0) {
		LOG_ERR("set_active: ARM resetcore failed: %d", ret);
		return ret;
	}

	LOG_DBG("set_active: request HT clock");
	ret = sdio_write_byte(&data->backplane, SBSDIO_FUNC1_CHIPCLKCSR,
			      SBSDIO_HT_AVAIL_REQ);
	if (ret != 0) {
		LOG_ERR("set_active: CHIPCLKCSR write failed: %d", ret);
		return ret;
	}

	return brcmfmac_chip_ht_bringup(data);
}

static int brcmfmac_chip_cr4_set_passive(struct brcmfmac_data *data)
{
	/* TODO: All D11 cores */
	const struct bcm_core *arm = brcmfmac_chip_core_find(data, BCMA_CORE_ARM_CR4);
	const struct bcm_core *d11 = brcmfmac_chip_core_find(data, BCMA_CORE_80211);
	uint32_t v;
	int ret;

	if (arm == NULL || d11 == NULL) {
		LOG_ERR("set_passive: missing core(s) arm=%p d11=%p", arm, d11);
		return -ENODEV;
	}

	(void)brcmfmac_sdio_backplane_read32(data, arm->wrapbase + BCMA_IOCTL, &v);
	v &= ARMCR4_BCMA_IOCTL_CPUHALT;
	ret = ai_resetcore(data, arm->wrapbase, v,
			   ARMCR4_BCMA_IOCTL_CPUHALT, ARMCR4_BCMA_IOCTL_CPUHALT);
	if (ret != 0) {
		LOG_ERR("set_passive: CR4 reset failed: %d", ret);
		return ret;
	}

	ret = ai_coredisable(data, d11->wrapbase, 0, 0);
	if (ret != 0) {
		LOG_ERR("set_passive: D11 disable failed: %d", ret);
		return ret;
	}

	return 0;
}

static int brcmfmac_chip_cr4_set_active(struct brcmfmac_data *data)
{
	const struct bcm_core *sdio_dev = brcmfmac_chip_core_find(data, BCMA_CORE_SDIO_DEV);
	const struct bcm_core *arm = brcmfmac_chip_core_find(data, BCMA_CORE_ARM_CR4);

	if (sdio_dev == NULL || arm == NULL) {
		LOG_ERR("set_passive: missing core(s) sdio=%p arm=%p", sdio_dev, arm);
		return -ENODEV;
	}

	LOG_DBG("set_active: clear SDIO core intstatus");
	int ret = brcmfmac_sdio_backplane_write32(data,
						  sdio_dev->base + SDPCMD_INTSTATUS,
						  0xFFFFFFFFu);
	if (ret != 0) {
		LOG_ERR("set_active: intstatus clear failed: %d", ret);
		return ret;
	}

	ret = ai_resetcore(data, arm->wrapbase, ARMCR4_BCMA_IOCTL_CPUHALT, 0, 0);
	if (ret != 0) {
		LOG_ERR("set_active: CR4 reset failed: %d", ret);
		return ret;
	}

	return brcmfmac_chip_ht_bringup(data);
}

int brcmfmac_chip_set_passive(struct brcmfmac_data *data)
{
	const struct bcm_core *arm;

	arm = brcmfmac_chip_core_find(data, BCMA_CORE_ARM_CM3);
	if (arm != NULL) {
		return brcmfmac_chip_cm3_set_passive(data);
	}

	arm = brcmfmac_chip_core_find(data, BCMA_CORE_ARM_CR4);
	if (arm != NULL) {
		return brcmfmac_chip_cr4_set_passive(data);
	}

	return 0;
}

int brcmfmac_chip_set_active(struct brcmfmac_data *data)
{
	const struct bcm_core *arm;

	arm = brcmfmac_chip_core_find(data, BCMA_CORE_ARM_CM3);
	if (arm != NULL) {
		return brcmfmac_chip_cm3_set_active(data);
	}

	arm = brcmfmac_chip_core_find(data, BCMA_CORE_ARM_CR4);
	if (arm != NULL) {
		return brcmfmac_chip_cr4_set_active(data);
	}

	return 0;
}
