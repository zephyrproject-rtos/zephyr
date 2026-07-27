/*
 * Copyright (c) 2026 Microchip Technology Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT microchip_pmecc_g1_flash

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/clock_control/mchp_sam_pmc.h>
#include <zephyr/drivers/flash/mchp_nand_g1.h>
#include <soc.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(flash_mchp_pmecc_g1, CONFIG_FLASH_LOG_LEVEL);

#define PMECC_REG_ID 0
#ifdef CONFIG_MEMC_MCHP_HSMC_G1
#define PMERR_REG_ID 0
#else
#define PMERR_REG_ID 1
#endif

#define GF_TABLE_512_INDEX_OFFSET  0x0
#define GF_TABLE_512_ALPHA_OFFSET  0x4000
#define GF_TABLE_1024_INDEX_OFFSET 0x8000
#define GF_TABLE_1024_ALPHA_OFFSET 0x10000

#define PMECC_TIMEOUT_US 100000

#define BBM_LEN 2
#define TT_MAX  32

struct pmecc_user {
	struct ecc_info *ecc;
	const int16_t *alpha_to;
	const int16_t *index_of;
	int16_t partial_syn[2 * TT_MAX + 1];
	int16_t si[2 * TT_MAX];
	int16_t lmu[TT_MAX + 1];
	int16_t smu[2 * TT_MAX + 1][TT_MAX + 2];
};

struct pmecc_config {
	void *regs[2];
	struct sam_clk_cfg clk_cfg;
	const uintptr_t gf_table;
	const uint8_t *strengths;
	uint8_t num_strgs;
	uint8_t max_strg;

#ifdef CONFIG_MEMC_MCHP_HSMC_G1
	const struct device *smc_dev;
#endif
};

struct pmecc_data {
	struct pmecc_user user;
	struct k_mutex mutex;
};

static const uint8_t pmecc_strengths[] = { 2, 4, 8, 12, 24, 32 };

#ifdef CONFIG_MEMC_MCHP_HSMC_G1
static void pmecc_init(void *regs, struct ecc_info *ecc, uint32_t oobsize)
{
	smc_registers_t *smc = (smc_registers_t *)regs;
	uint8_t sectorsz, bch_err;

	if (ecc->size == 512) {
		sectorsz = HSMC_PMECCFG_SECTORSZ_0;
	} else {
		sectorsz = HSMC_PMECCFG_SECTORSZ_1;
	}

	if (ecc->bits == 2) {
		bch_err = HSMC_PMECCFG_BCH_ERR_BCH_ERR2_Val;
	} else if (ecc->bits == 4) {
		bch_err = HSMC_PMECCFG_BCH_ERR_BCH_ERR4_Val;
	} else if (ecc->bits == 8) {
		bch_err = HSMC_PMECCFG_BCH_ERR_BCH_ERR8_Val;
	} else if (ecc->bits == 12) {
		bch_err = HSMC_PMECCFG_BCH_ERR_BCH_ERR12_Val;
	} else if (ecc->bits == 24) {
		bch_err = HSMC_PMECCFG_BCH_ERR_BCH_ERR24_Val;
	} else {
		bch_err = HSMC_PMECCFG_BCH_ERR_BCH_ERR32_Val;
	}

	smc->HSMC_PMECCTRL = HSMC_PMECCTRL_DISABLE_1;
	smc->HSMC_PMECCTRL = HSMC_PMECCTRL_RST_1;
	smc->HSMC_PMECCIDR = HSMC_PMECCIDR_Msk;

	smc->HSMC_PMECCFG = HSMC_PMECCFG_PAGESIZE(find_msb_set(ecc->steps) - 1) |
			    sectorsz |
			    HSMC_PMECCFG_BCH_ERR(bch_err);
	smc->HSMC_PMECCSAREA = oobsize - 1;
	smc->HSMC_PMECCSADDR = ecc->addr;
	smc->HSMC_PMECCEADDR = ecc->addr + ecc->bytes * ecc->steps - 1;
}

static void pmecc_enable(void *regs, uint32_t is_write)
{
	smc_registers_t *smc = (smc_registers_t *)regs;

	if (is_write) {
		smc->HSMC_PMECCFG &= ~HSMC_PMECCFG_AUTO_Msk;
		smc->HSMC_PMECCFG |= HSMC_PMECCFG_NANDWR_Msk;
	} else {
		smc->HSMC_PMECCFG |= HSMC_PMECCFG_AUTO_Msk;
		smc->HSMC_PMECCFG &= ~HSMC_PMECCFG_NANDWR_Msk;
	}

	smc->HSMC_PMECCTRL = HSMC_PMECCTRL_RST_1;
	smc->HSMC_PMECCTRL = HSMC_PMECCTRL_ENABLE_1;
	smc->HSMC_PMECCTRL = HSMC_PMECCTRL_DATA_1;
}

static inline int pmecc_is_busy(void *regs)
{
	smc_registers_t *smc = (smc_registers_t *)regs;

	return !!(smc->HSMC_PMECCSR & HSMC_PMECCSR_BUSY_Msk);
}

static inline uint8_t *pmecc_redundancy(void *regs, int sector)
{
	smc_registers_t *smc = (smc_registers_t *)regs;

	return (uint8_t *)&smc->SMC_PMECC[sector];
}

static inline int16_t *pmecc_remainder(void *regs, int sector)
{
	smc_registers_t *smc = (smc_registers_t *)regs;

	return (int16_t *)&smc->SMC_REM[sector];
}

static inline uint32_t pmecc_error_status(void *regs)
{
	smc_registers_t *smc = (smc_registers_t *)regs;

	return smc->HSMC_PMECCISR & HSMC_PMECCISR_Msk;
}

static void pmerrloc_enable(void *regs, struct ecc_info *ecc, int errnum)
{
	smc_registers_t *smc = (smc_registers_t *)regs;
	int degree = ecc->size == 512 ? 13 : 14;

	smc->HSMC_ELCFG = HSMC_ELCFG_ERRNUM(errnum) |
			  (ecc->size == 512 ? HSMC_ELCFG_SECTORSZ_0 :
					      HSMC_ELCFG_SECTORSZ_1);
	smc->HSMC_ELEN  = HSMC_ELEN_ENINIT((ecc->size * 8) + (degree * ecc->bits));
}

static void pmerrloc_disable(void *regs)
{
	smc_registers_t *smc = (smc_registers_t *)regs;

	smc->HSMC_ELDIS = HSMC_ELDIS_DIS_1;
}

static inline uint32_t *pmerrloc_sigma(void *regs)
{
	smc_registers_t *smc = (smc_registers_t *)regs;

	return (uint32_t *)&smc->HSMC_SIGMA0;
}

static inline uint32_t pmerrloc_irq_status(void *regs)
{
	smc_registers_t *smc = (smc_registers_t *)regs;

	return smc->HSMC_ELISR;
}

static inline uint32_t pmerrloc_is_done(uint32_t isr)
{
	return !!(isr & HSMC_ELISR_DONE_Msk);
}

static inline uint32_t pmerrloc_error_counter(uint32_t isr)
{
	return (isr & HSMC_ELISR_ERR_CNT_Msk) >> HSMC_ELISR_ERR_CNT_Pos;
}

static inline uint32_t pmerrloc_error_location(void *regs, int32_t id)
{
	smc_registers_t *smc = (smc_registers_t *)regs;

	return smc->HSMC_ERRLOC[id];
}
#endif

static int pmecc_wait_ready(const struct device *dev)
{
	const struct pmecc_config *cfg = dev->config;
	void *regs = cfg->regs[PMECC_REG_ID];
	uint32_t timeout = PMECC_TIMEOUT_US;

	while (pmecc_is_busy(regs) && timeout--) {
		k_usleep(1);
	}

	if (pmecc_is_busy(regs)) {
		return -EBUSY;
	}

	return 0;
}

static void pmecc_gen_syndrome(const struct device *dev, int sector)
{
	const struct pmecc_config *cfg = dev->config;
	struct pmecc_data *data = dev->data;
	struct pmecc_user *user = &data->user;
	int16_t *remainder = pmecc_remainder(cfg->regs[PMECC_REG_ID], sector);

	/* Fill odd syndromes */
	for (int i = 0; i < user->ecc->bits; i++) {
		user->partial_syn[(2 * i) + 1] = remainder[i];
	}
}

static void pmecc_substitute(const struct device *dev)
{
	struct pmecc_data *data = dev->data;
	struct pmecc_user *user = &data->user;
	struct ecc_info *ecc = user->ecc;
	int degree = ecc->size == 512 ? 13 : 14;
	int cw_len = BIT(degree) - 1;
	int strength = ecc->bits;
	const int16_t *alpha_to = user->alpha_to;
	const int16_t *index_of = user->index_of;
	int16_t *partial_syn = user->partial_syn;
	int16_t *si;
	int i, j;

	/*
	 * si[] is a table that holds the current syndrome value,
	 * an element of that table belongs to the field
	 */
	si = user->si;

	memset(&si[1], 0, sizeof(int16_t) * ((2 * strength) - 1));

	/* Computation 2t syndromes based on S(x) */
	/* Odd syndromes */
	for (i = 1; i < 2 * strength; i += 2) {
		for (j = 0; j < degree; j++) {
			if (partial_syn[i] & BIT(j)) {
				si[i] = alpha_to[i * j] ^ si[i];
			}
		}
	}
	/* Even syndrome = (Odd syndrome) ** 2 */
	for (i = 2, j = 1; j <= strength; i = ++j << 1) {
		if (si[j] == 0) {
			si[i] = 0;
		} else {
			int16_t tmp;

			tmp = index_of[si[j]];
			tmp = (tmp * 2) % cw_len;
			si[i] = alpha_to[tmp];
		}
	}
}

static void pmecc_get_sigma(const struct device *dev)
{
	struct pmecc_data *data = dev->data;
	struct pmecc_user *user = &data->user;
	struct ecc_info *ecc = user->ecc;
	int16_t *lmu = user->lmu;
	int16_t *si = user->si;
	int32_t mu[TT_MAX + 2];
	int32_t dmu[TT_MAX + 2] = {0};
	int32_t delta[TT_MAX + 2];
	int degree = ecc->size == 512 ? 13 : 14;
	int cw_len = BIT(degree) - 1;
	int strength = ecc->bits;
	int num = 2 * strength + 1;
	const int16_t *index_of = user->index_of;
	const int16_t *alpha_to = user->alpha_to;
	int j, k;
	uint32_t dmu_0_count, tmp;
	int16_t *smu = &user->smu[0][0];

	/* index of largest delta */
	int ro;
	int largest;
	int diff;

	dmu_0_count = 0;

	/* First Row */

	/* Mu */
	mu[0] = -1;

	memset(smu, 0, sizeof(int16_t) * num);
	smu[0] = 1;

	/* discrepancy set to 1 */
	dmu[0] = 1;
	/* polynom order set to 0 */
	lmu[0] = 0;
	delta[0] = (mu[0] * 2 - lmu[0]) >> 1;

	/* Second Row */

	/* Mu */
	mu[1] = 0;
	/* Sigma(x) set to 1 */
	memset(&smu[num], 0, sizeof(int16_t) * num);
	smu[num] = 1;

	/* discrepancy set to S1 */
	dmu[1] = si[1];

	/* polynom order set to 0 */
	lmu[1] = 0;

	delta[1] = (mu[1] * 2 - lmu[1]) >> 1;

	/* Init the Sigma(x) last row */
	memset(&smu[(strength + 1) * num], 0, sizeof(int16_t) * num);

	for (int i = 1; i <= strength; i++) {
		mu[i + 1] = i << 1;
		/* Begin Computing Sigma (Mu+1) and L(mu) */
		/* check if discrepancy is set to 0 */
		if (dmu[i] == 0) {
			dmu_0_count++;

			tmp = ((strength - (lmu[i] >> 1) - 1) / 2);
			if ((strength - (lmu[i] >> 1) - 1) & 0x1) {
				tmp += 2;
			} else {
				tmp += 1;
			}

			if (dmu_0_count == tmp) {
				for (j = 0; j <= (lmu[i] >> 1) + 1; j++) {
					smu[(strength + 1) * num + j] =
							smu[i * num + j];
				}

				lmu[strength + 1] = lmu[i];
				return;
			}

			/* copy polynom */
			for (j = 0; j <= lmu[i] >> 1; j++) {
				smu[(i + 1) * num + j] = smu[i * num + j];
			}

			/* copy previous polynom order to the next */
			lmu[i + 1] = lmu[i];
		} else {
			ro = 0;
			largest = -1;
			/* find largest delta with dmu != 0 */
			for (j = 0; j < i; j++) {
				if ((dmu[j]) && (delta[j] > largest)) {
					largest = delta[j];
					ro = j;
				}
			}

			/* compute difference */
			diff = (mu[i] - mu[ro]);

			/* Compute degree of the new smu polynomial */
			if ((lmu[i] >> 1) > ((lmu[ro] >> 1) + diff)) {
				lmu[i + 1] = lmu[i];
			} else {
				lmu[i + 1] = ((lmu[ro] >> 1) + diff) * 2;
			}

			/* Init smu[i+1] with 0 */
			for (k = 0; k < num; k++) {
				smu[(i + 1) * num + k] = 0;
			}

			/* Compute smu[i+1] */
			for (k = 0; k <= lmu[ro] >> 1; k++) {
				int16_t a, b, c;

				if (!(smu[ro * num + k] && dmu[i])) {
					continue;
				}

				a = index_of[dmu[i]];
				b = index_of[dmu[ro]];
				c = index_of[smu[ro * num + k]];
				tmp = a + (cw_len - b) + c;
				a = alpha_to[tmp % cw_len];
				smu[(i + 1) * num + (k + diff)] = a;
			}

			for (k = 0; k <= lmu[i] >> 1; k++) {
				smu[(i + 1) * num + k] ^= smu[i * num + k];
			}
		}

		/* End Computing Sigma (Mu+1) and L(mu) */
		/* In either case compute delta */
		delta[i + 1] = (mu[i + 1] * 2 - lmu[i + 1]) >> 1;

		/* Do not compute discrepancy for the last iteration */
		if (i >= strength) {
			continue;
		}

		for (k = 0; k <= (lmu[i + 1] >> 1); k++) {
			tmp = 2 * (i - 1);
			if (k == 0) {
				dmu[i + 1] = si[tmp + 3];
			} else {
				if (smu[(i + 1) * num + k] && si[tmp + 3 - k]) {
					int16_t a, b, c;

					a = index_of[smu[(i + 1) * num + k]];
					b = si[2 * (i - 1) + 3 - k];
					c = index_of[b];
					tmp = a + c;
					tmp %= cw_len;
					dmu[i + 1] = alpha_to[tmp] ^ dmu[i + 1];
				}
			}
		}
	}
}

static int pmecc_err_location(const struct device *dev)
{
	const struct pmecc_config *cfg = dev->config;
	struct pmecc_data *data = dev->data;
	struct pmecc_user *user = &data->user;
	struct ecc_info *ecc = user->ecc;
	void *regs = cfg->regs[PMERR_REG_ID];
	int strength = ecc->bits;
	int roots_nbr, i, err_num = 0;
	int num = (2 * strength) + 1;
	int16_t *smu = &user->smu[0][0];
	uint32_t isr;
	uint32_t *sigma = pmerrloc_sigma(regs);
	uint32_t timeout = PMECC_TIMEOUT_US;

	pmerrloc_disable(regs);

	for (i = 0; i <= user->lmu[strength + 1] >> 1; i++) {
		sigma[i] = smu[(strength + 1) * num + i];
		err_num++;
	}

	pmerrloc_enable(regs, ecc, err_num - 1);
	while (timeout--) {
		isr = pmerrloc_irq_status(regs);

		if (pmerrloc_is_done(isr)) {
			break;
		}

		k_usleep(1);
	}

	if (!pmerrloc_is_done(isr)) {
		LOG_ERR("PMECC: Timeout to calculate error location");
		return -EBADMSG;
	}

	roots_nbr = pmerrloc_error_counter(isr);
	/* Number of roots == degree of smu hence <= cap */
	if (roots_nbr == user->lmu[strength + 1] >> 1) {
		return err_num - 1;
	}

	/*
	 * Number of roots does not match the degree of smu
	 * unable to correct error.
	 */
	return -EBADMSG;
}

static int pmecc_correct_sector(const struct device *dev, int sector,
				void *data, void *ecc)
{
	const struct pmecc_config *cfg = dev->config;
	struct pmecc_user *user = &((struct pmecc_data *)dev->data)->user;
	void *regs = cfg->regs[PMERR_REG_ID];
	int sectorsize = user->ecc->size;
	int eccbytes = user->ecc->bytes;
	int i, nerrors;

	pmecc_gen_syndrome(dev, sector);
	pmecc_substitute(dev);
	pmecc_get_sigma(dev);

	nerrors = pmecc_err_location(dev);
	if (nerrors < 0) {
		LOG_DBG("PMECC: Failed to find number of errors");
		return nerrors;
	}

	for (i = 0; i < nerrors; i++) {
		const char *area;
		int byte, bit;
		uint32_t errpos;
		uint8_t *ptr;

		errpos = pmerrloc_error_location(regs, i);
		errpos--;

		byte = errpos / 8;
		bit = errpos % 8;

		if (byte < sectorsize) {
			ptr = (char *)data + byte;
			area = "data";
		} else if (byte < sectorsize + eccbytes) {
			ptr = (char *)ecc + byte - sectorsize;
			area = "ECC";
		} else {
			LOG_ERR("PMECC: Invalid errpos value (%d, max is %d)",
				errpos, (sectorsize + eccbytes) * 8);
			return -EINVAL;
		}

		LOG_DBG("PMECC: Bit flip in %s area, byte %d: 0x%02x -> 0x%02x",
			area, byte, *ptr, (unsigned int)(*ptr ^ BIT(bit)));

		*ptr ^= BIT(bit);
	}

	return nerrors;
}

int ecc_init_user(const struct device *dev, struct nand_chip *chip)
{
	const struct pmecc_config *cfg = dev->config;
	struct pmecc_data *data = dev->data;
	struct pmecc_user *user = &data->user;
	const struct nand_info *nand = &chip->nand;
	struct ecc_info *ecc = &chip->ecc;
	int i;

	k_mutex_lock(&data->mutex, K_FOREVER);

	ecc->bits = nand->eccbits;
	for (i = 0; i < cfg->num_strgs; i++) {
		if (cfg->strengths[i] >= nand->eccbits) {
			ecc->bits = cfg->strengths[i];
			break;
		}
	}

	ecc->size  = nand->eccsize;
	ecc->steps = nand->pagesize / nand->eccsize;
	if ((ecc->steps > 8) && (ecc->size == 512)) {
		ecc->size  = 1024;
		ecc->steps = nand->pagesize / ecc->size;
	}

	ecc->bytes = ROUND_UP(find_msb_set(ecc->size * 8) * ecc->bits, 8) / 8;
	ecc->addr  = nand->oobsize - (ecc->bytes * ecc->steps);

	if ((ecc->bits > cfg->max_strg) ||
	    (ecc->steps > 8) ||
	    ((ecc->size != 512) && (ecc->size != 1024)) ||
	    ((ecc->bytes * ecc->steps) > (nand->oobsize - BBM_LEN))) {
		LOG_ERR("PMECC: Unsupported ECC settings:");
		LOG_ERR("       bits: %d, size: %d, steps: %d, bytes: %d, addr: %d",
			ecc->bits, ecc->size, ecc->steps, ecc->bytes, ecc->addr);

		k_mutex_unlock(&data->mutex);
		return -ENOTSUP;
	}

	user->ecc = ecc;
	if (ecc->size == 512) {
		user->alpha_to = (const int16_t *)(cfg->gf_table + GF_TABLE_512_ALPHA_OFFSET);
		user->index_of = (const int16_t *)(cfg->gf_table + GF_TABLE_512_INDEX_OFFSET);
	} else {
		user->alpha_to = (const int16_t *)(cfg->gf_table + GF_TABLE_1024_ALPHA_OFFSET);
		user->index_of = (const int16_t *)(cfg->gf_table + GF_TABLE_1024_INDEX_OFFSET);
	}

	pmecc_init(cfg->regs[PMECC_REG_ID], ecc, nand->oobsize);

	LOG_INF("PMECC: ECC bits: %d, size: %d, steps: %d, bytes: %d, addr: %d",
		ecc->bits, ecc->size, ecc->steps, ecc->bytes, ecc->addr);

	k_mutex_unlock(&data->mutex);

	return 0;
}

int ecc_enable(const struct device *dev, uint32_t is_write)
{
	const struct pmecc_config *cfg = dev->config;
	struct pmecc_data *data = dev->data;

	k_mutex_lock(&data->mutex, K_FOREVER);

	pmecc_enable(cfg->regs[PMECC_REG_ID], is_write);

	k_mutex_unlock(&data->mutex);

	return 0;
}

int ecc_get_eccbytes(const struct device *dev, uint8_t *buf)
{
	const struct pmecc_config *cfg = dev->config;
	struct pmecc_data *data = dev->data;
	struct ecc_info *ecc = data->user.ecc;
	void *regs = cfg->regs[PMECC_REG_ID];
	uint8_t *redundancy;

	k_mutex_lock(&data->mutex, K_FOREVER);

	if (pmecc_wait_ready(dev)) {
		k_mutex_unlock(&data->mutex);
		return -EBUSY;
	}

	for (int i = 0; i < ecc->steps; i++) {
		redundancy = pmecc_redundancy(regs, i);
		for (int j = 0; j < ecc->bytes; j++) {
			*buf++ = *redundancy++;
		}
	}

	k_mutex_unlock(&data->mutex);

	return 0;
}

int ecc_process(const struct device *dev, uint8_t *data, uint8_t *oob)
{
	const struct pmecc_config *cfg = dev->config;
	struct pmecc_data *dev_data = dev->data;
	struct pmecc_user *user = &dev_data->user;
	struct ecc_info *ecc = user->ecc;
	uint32_t erris;
	int max_bitflips = 0;
	int ret = 0;

	k_mutex_lock(&dev_data->mutex, K_FOREVER);

	if (pmecc_wait_ready(dev)) {
		LOG_ERR("PMECC: PMECC is not ready");

		ret = -EBUSY;
		goto OUT;
	}

	erris = pmecc_error_status(cfg->regs[PMECC_REG_ID]);
	if (erris == 0) {
		goto OUT;
	}

	for (int i = 0; i < ecc->steps; i++) {
		if (erris & BIT(i)) {
			ret = pmecc_correct_sector(dev, i, data, &oob[ecc->addr]);
			if (ret < 0) {
				goto OUT;
			} else {
				max_bitflips = MAX(max_bitflips, ret);
			}
		}
	}

	ret = max_bitflips;
OUT:
	k_mutex_unlock(&dev_data->mutex);

	return ret;
}

static int flash_pmecc_init(const struct device *dev)
{
	const struct pmecc_config *cfg = dev->config;
	struct pmecc_data *data = dev->data;

#ifdef CONFIG_MEMC_MCHP_HSMC_G1
	if (!device_is_ready(cfg->smc_dev)) {
		LOG_ERR("PMECC: SMC device is not ready");
		return -ENODEV;
	}
#else
	const struct device *const pmc = DEVICE_DT_GET(DT_NODELABEL(pmc));
	int ret;

	if (!device_is_ready(pmc)) {
		LOG_ERR("PMECC: Power Management Controller device not ready");
		return -ENODEV;
	}

	ret = clock_control_on(pmc, (clock_control_subsys_t)&cfg->clk_cfg);
	if (ret) {
		LOG_ERR("PMECC: Clock op failed");
		return ret;
	}
#endif

	k_mutex_init(&data->mutex);

	return 0;
}

#ifdef CONFIG_MEMC_MCHP_HSMC_G1
#define PMECC_DT_INST_REGS0_GET(inst) DT_REG_ADDR(DT_INST_PARENT(inst))
#define PMECC_DT_INST_REGS1_GET(inst) 0
#define PMECC_DT_INST_CLOCK_GET(inst) {0}
#define SMC_DEV_INIT(inst) \
	.smc_dev  = DEVICE_DT_GET(DT_INST_PARENT(inst)),
#else
#define PMECC_DT_INST_REGS0_GET(inst) DT_INST_REG_ADDR_BY_IDX(inst, 0)
#define PMECC_DT_INST_REGS1_GET(inst) DT_INST_REG_ADDR_BY_IDX(inst, 1)
#define PMECC_DT_INST_CLOCK_GET(inst) SAM_DT_INST_CLOCK_PMC_CFG(inst)
#define SMC_DEV_INIT(inst)
#endif

#define PMECC_DT_INST_GF_TABLE_GET(inst) \
	DT_REG_ADDR(DT_INST_PHANDLE(inst, ecc_rom))

#define SAM_PMECC_DEFINE(inst)							\
	static const struct pmecc_config pmecc_config_##inst = {		\
		.regs[0]   = (void *)PMECC_DT_INST_REGS0_GET(inst),		\
		.regs[1]   = (void *)PMECC_DT_INST_REGS1_GET(inst),		\
		.clk_cfg   = PMECC_DT_INST_CLOCK_GET(inst),			\
		.gf_table  = (uintptr_t)PMECC_DT_INST_GF_TABLE_GET(inst),	\
		.strengths = pmecc_strengths,					\
		.num_strgs = ARRAY_SIZE(pmecc_strengths),			\
		.max_strg  = DT_INST_PROP(inst, max_strength),			\
		SMC_DEV_INIT(inst)						\
	};									\
										\
	static struct pmecc_data pmecc_data_##inst = {				\
	};									\
										\
	DEVICE_DT_INST_DEFINE(inst, flash_pmecc_init, NULL,			\
			      &pmecc_data_##inst, &pmecc_config_##inst,		\
			      POST_KERNEL, CONFIG_FLASH_INIT_PRIORITY,		\
			      NULL);

DT_INST_FOREACH_STATUS_OKAY(SAM_PMECC_DEFINE)
