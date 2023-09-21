/*
 * Copyright (c) 2023 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>

#include <zephyr/drivers/pcie/vc.h>
#include <zephyr/drivers/pcie/cap.h>

#include "vc.h"

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

	regs->cap_reg_1.raw = pcie_conf_read(bdf, base +
					     PCIE_VC_CAP_REG_1_OFFSET);
	regs->cap_reg_2.raw = pcie_conf_read(bdf, base +
					     PCIE_VC_CAP_REG_2_OFFSET);
	regs->ctrl_reg.raw = pcie_conf_read(bdf, base +
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
		regs->cap_reg.raw =
			pcie_conf_read(bdf, base +
				       PCIE_VC_RES_CAP_REG_OFFSET(idx));
		regs->ctrl_reg.raw =
			pcie_conf_read(bdf, base +
				       PCIE_VC_RES_CTRL_REG_OFFSET(idx));
		regs->status_reg.raw =
			pcie_conf_read(bdf, base +
				       PCIE_VC_RES_STATUS_REG_OFFSET(idx));
		regs++;
	}
}

static int get_vc_registers(pcie_bdf_t bdf,
			    struct pcie_vc_regs *regs,
			    struct pcie_vc_resource_regs *res_regs)
{
	uint32_t base;

	base = pcie_vc_cap_lookup(bdf, regs);
	if (base == 0) {
		return -ENOTSUP;
	}

	if (regs->cap_reg_1.vc_count == 0) {
		/* Having only VC0 is like having no real VC */
		return -ENOTSUP;
	}

	pcie_vc_load_resources_regs(bdf, base, res_regs,
			       regs->cap_reg_1.vc_count + 1);

	return 0;
}


int pcie_vc_enable(pcie_bdf_t bdf)
{
	struct pcie_vc_regs regs;
	struct pcie_vc_resource_regs res_regs[PCIE_VC_MAX_COUNT];
	int idx;

	if (get_vc_registers(bdf, &regs, res_regs) != 0) {
		return -ENOTSUP;
	}

	/* We do not touch VC0: it is always on */
	for (idx = 1; idx < regs.cap_reg_1.vc_count + 1; idx++) {
		if (idx > 0 && res_regs[idx].ctrl_reg.vc_enable == 1) {
			/*
			 * VC has not been disabled properly, if at all?
			 */
			return -EALREADY;
		}

		res_regs[idx].ctrl_reg.vc_enable = 1;
	}

	return 0;
}

int pcie_vc_disable(pcie_bdf_t bdf)
{
	struct pcie_vc_regs regs;
	struct pcie_vc_resource_regs res_regs[PCIE_VC_MAX_COUNT];
	int idx;

	if (get_vc_registers(bdf, &regs, res_regs) != 0) {
		return -ENOTSUP;
	}

	/* We do not touch VC0: it is always on */
	for (idx = 1; idx < regs.cap_reg_1.vc_count + 1; idx++) {
		/* Let's wait for the pending negotiation to end */
		while (res_regs[idx].status_reg.vc_negocation_pending == 1) {
			k_msleep(10);
		}

		res_regs[idx].ctrl_reg.vc_enable = 0;
	}

	return 0;
}

int pcie_vc_map_tc(pcie_bdf_t bdf, struct pcie_vctc_map *map)
{
	struct pcie_vc_regs regs;
	struct pcie_vc_resource_regs res_regs[PCIE_VC_MAX_COUNT];
	int idx;
	uint8_t tc_mapped = 0;

	if (get_vc_registers(bdf, &regs, res_regs) != 0) {
		return -ENOTSUP;
	}

	/* Map must relate to the actual VC count */
	if (regs.cap_reg_1.vc_count != map->vc_count) {
		return -EINVAL;
	}

	/* Veryfying that map is sane */
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

	for (idx = 0; idx < regs.cap_reg_1.vc_count + 1; idx++) {
		/* Let's just set the VC ID to related index for now */
		if (idx > 0) {
			res_regs[idx].ctrl_reg.vc_id = idx;
		}

		/* Currently, only HW round robin is used */
		res_regs[idx].ctrl_reg.pa_select = PCIE_VC_PA_RR;

		res_regs[idx].ctrl_reg.tc_vc_map = map->vc_tc[idx];
	}

	return 0;
}
