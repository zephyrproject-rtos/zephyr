/*
 * Copyright (c) 2023 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>

#include <zephyr/drivers/pcie/vc.h>
#include <zephyr/drivers/pcie/cap.h>

#include "vc.h"

#define PCIE_VC_NEGOTIATION_RETRIES 100
#define PCIE_VC_NEGOTIATION_DELAY_MS 10

uint32_t pcie_vc_cap_lookup(pcie_bdf_t bdf, struct pcie_vc_regs *regs)
{
	uint32_t base;

	base = pcie_get_ext_cap(bdf, PCIE_EXT_CAP_ID_VC);
	if (base == 0) {
		base = pcie_get_ext_cap(bdf, PCIE_EXT_CAP_ID_MFVC_VC);
		if (base == 0) {
			return 0;
		}
	}

	regs->cap_reg_1 = pcie_conf_read(bdf, base +
					 PCIE_VC_CAP_REG_1_OFFSET);
	regs->cap_reg_2 = pcie_conf_read(bdf, base +
					 PCIE_VC_CAP_REG_2_OFFSET);
	regs->ctrl_reg = pcie_conf_read(bdf, base +
					PCIE_VC_CTRL_STATUS_REG_OFFSET);

	return base;
}

void pcie_vc_load_resources_regs(pcie_bdf_t bdf,
				 uint32_t base,
				 struct pcie_vc_resource_regs *regs,
				 int nb_regs)
{
	int idx;

	for (idx = 0; idx < nb_regs; idx++) {
		regs->cap_reg =
			pcie_conf_read(bdf, base +
				       PCIE_VC_RES_CAP_REG_OFFSET(idx));
		regs->ctrl_reg =
			pcie_conf_read(bdf, base +
				       PCIE_VC_RES_CTRL_REG_OFFSET(idx));
		regs->status_reg =
			pcie_conf_read(bdf, base +
				       PCIE_VC_RES_STATUS_REG_OFFSET(idx));
		regs++;
	}
}

static int pcie_vc_extended_count(const struct pcie_vc_regs *regs)
{
	return FIELD_GET(PCIE_VC_CAP1_EVCC_MASK, regs->cap_reg_1);
}

static int get_vc_registers(pcie_bdf_t bdf,
			    struct pcie_vc_regs *regs,
			    struct pcie_vc_resource_regs *res_regs,
			    uint32_t *base)
{
	int vc_count;

	if (regs == NULL || res_regs == NULL || base == NULL) {
		return -EINVAL;
	}

	*base = pcie_vc_cap_lookup(bdf, regs);
	if (*base == 0) {
		return -ENOTSUP;
	}

	vc_count = pcie_vc_extended_count(regs);
	if (vc_count == 0U) {
		/* Having only VC0 is like having no real VC */
		return -ENOTSUP;
	}

	pcie_vc_load_resources_regs(bdf, *base, res_regs, vc_count + 1);

	return 0;
}

static int wait_for_vc_negotiation(pcie_bdf_t bdf, uint32_t base, int idx)
{
	for (int retry = 0; retry < PCIE_VC_NEGOTIATION_RETRIES; retry++) {
		uint32_t status = pcie_conf_read(bdf, base + PCIE_VC_RES_STATUS_REG_OFFSET(idx));

		if ((status & PCIE_VC_RES_STATUS_NEGOTIATION_PENDING) == 0U) {
			return 0;
		}

		k_msleep(PCIE_VC_NEGOTIATION_DELAY_MS);
	}

	return -ETIMEDOUT;
}

int pcie_vc_enable(pcie_bdf_t bdf)
{
	struct pcie_vc_regs regs;
	struct pcie_vc_resource_regs res_regs[PCIE_VC_MAX_COUNT];
	uint32_t base;
	int vc_count;
	int idx;
	int ret;

	if (get_vc_registers(bdf, &regs, res_regs, &base) != 0) {
		return -ENOTSUP;
	}

	vc_count = pcie_vc_extended_count(&regs);

	/* Check all extended VCs before changing hardware to avoid a partial update. */
	for (idx = 1; idx < vc_count + 1; idx++) {
		if ((res_regs[idx].ctrl_reg & PCIE_VC_RES_CTRL_ENABLE) != 0U) {
			return -EALREADY;
		}
	}

	/* We do not touch VC0: it is always on. */
	for (idx = 1; idx < vc_count + 1; idx++) {
		res_regs[idx].ctrl_reg |= PCIE_VC_RES_CTRL_ENABLE;
		pcie_conf_write(bdf, base + PCIE_VC_RES_CTRL_REG_OFFSET(idx),
				res_regs[idx].ctrl_reg);

		ret = wait_for_vc_negotiation(bdf, base, idx);
		if (ret != 0) {
			return ret;
		}
	}

	return 0;
}

int pcie_vc_disable(pcie_bdf_t bdf)
{
	struct pcie_vc_regs regs;
	struct pcie_vc_resource_regs res_regs[PCIE_VC_MAX_COUNT];
	uint32_t base;
	int vc_count;
	int idx;
	int ret;

	if (get_vc_registers(bdf, &regs, res_regs, &base) != 0) {
		return -ENOTSUP;
	}

	vc_count = pcie_vc_extended_count(&regs);

	/* We do not touch VC0: it is always on. */
	for (idx = 1; idx < vc_count + 1; idx++) {
		res_regs[idx].ctrl_reg &= ~PCIE_VC_RES_CTRL_ENABLE;
		pcie_conf_write(bdf, base + PCIE_VC_RES_CTRL_REG_OFFSET(idx),
				res_regs[idx].ctrl_reg);

		ret = wait_for_vc_negotiation(bdf, base, idx);
		if (ret != 0) {
			return ret;
		}
	}

	return 0;
}

int pcie_vc_map_tc(pcie_bdf_t bdf, struct pcie_vctc_map *map)
{
	struct pcie_vc_regs regs;
	struct pcie_vc_resource_regs res_regs[PCIE_VC_MAX_COUNT];
	uint32_t base;
	int vc_count;
	int idx;
	uint8_t tc_mapped = 0;

	if (map == NULL) {
		return -EINVAL;
	}

	if (get_vc_registers(bdf, &regs, res_regs, &base) != 0) {
		return -ENOTSUP;
	}

	vc_count = pcie_vc_extended_count(&regs);

	/* EVCC excludes the default VC0, while vc_count includes it. */
	if ((vc_count + 1) != map->vc_count) {
		return -EINVAL;
	}

	/* Verifying that map is sane */
	for (idx = 0; idx < map->vc_count; idx++) {
		if (idx == 0 && !(map->vc_tc[idx] & PCIE_VC_SET_TC0)) {
			/* TC0 is on VC0 and cannot be unset */
			return -EINVAL;
		}

		/* Each TC must appear only once in the map */
		if (tc_mapped & map->vc_tc[idx]) {
			return -EINVAL;
		}

		tc_mapped |= map->vc_tc[idx];
	}

	for (idx = 0; idx < map->vc_count; idx++) {
		uint32_t ctrl = res_regs[idx].ctrl_reg;

		ctrl &= ~(PCIE_VC_RES_CTRL_TC_MAP_MASK | PCIE_VC_RES_CTRL_PA_SELECT_MASK);
		ctrl |= FIELD_PREP(PCIE_VC_RES_CTRL_TC_MAP_MASK, map->vc_tc[idx]);
		ctrl |= FIELD_PREP(PCIE_VC_RES_CTRL_PA_SELECT_MASK, PCIE_VC_PA_RR);

		/* Let's just set the VC ID to related index for now */
		if (idx > 0) {
			ctrl &= ~PCIE_VC_RES_CTRL_VC_ID_MASK;
			ctrl |= FIELD_PREP(PCIE_VC_RES_CTRL_VC_ID_MASK, idx);
		}

		res_regs[idx].ctrl_reg = ctrl;
		pcie_conf_write(bdf, base + PCIE_VC_RES_CTRL_REG_OFFSET(idx), ctrl);
	}

	return 0;
}
