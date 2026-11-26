/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <zephyr/drivers/pcie/controller.h>

#include "pcie_priv.h"

static bool pcie_bdf_is_valid(pcie_bdf_t bdf)
{
	return (bdf & ~PCIE_BDF(PCIE_MAX_BUS, PCIE_MAX_DEV, PCIE_MAX_FUNC)) == 0U;
}

int pcie_endpoint_conf_read(const struct pcie_endpoint *ep, unsigned int reg, uint32_t *value)
{
	if (ep == NULL || ep->ctrl == NULL || !pcie_bdf_is_valid(ep->bdf) ||
	    value == NULL || reg >= 1024U) {
		return -EINVAL;
	}

#ifdef CONFIG_PCIE_CONTROLLER
	if (!DEVICE_API_IS(pcie_ctrl, ep->ctrl)) {
		return -ENOTSUP;
	}
	const struct pcie_ctrl_driver_api *api = DEVICE_API_GET(pcie_ctrl, ep->ctrl);

	if (api == NULL) {
		return -ENOTSUP;
	}
	if (api->conf_read_status != NULL) {
		return api->conf_read_status(ep->ctrl, ep->bdf, reg, value);
	}
	if (api->conf_read != NULL) {
		*value = pcie_ctrl_conf_read(ep->ctrl, ep->bdf, reg);
		return 0;
	}
#endif
	return -ENOTSUP;
}

int pcie_endpoint_conf_write(const struct pcie_endpoint *ep, unsigned int reg, uint32_t value)
{
	if (ep == NULL || ep->ctrl == NULL || !pcie_bdf_is_valid(ep->bdf) || reg >= 1024U) {
		return -EINVAL;
	}

#ifdef CONFIG_PCIE_CONTROLLER
	if (!DEVICE_API_IS(pcie_ctrl, ep->ctrl)) {
		return -ENOTSUP;
	}
	const struct pcie_ctrl_driver_api *api = DEVICE_API_GET(pcie_ctrl, ep->ctrl);

	if (api == NULL) {
		return -ENOTSUP;
	}
	if (api->conf_write_status != NULL) {
		return api->conf_write_status(ep->ctrl, ep->bdf, reg, value);
	}
	if (api->conf_write != NULL) {
		pcie_ctrl_conf_write(ep->ctrl, ep->bdf, reg, value);
		return 0;
	}
#endif
	return -ENOTSUP;
}

int pcie_endpoint_get_cap(const struct pcie_endpoint *ep, uint32_t cap_id, uint32_t *cap)
{
	uint32_t data, reg;
	int ret;

	if (ep == NULL || ep->ctrl == NULL || cap == NULL) {
		return -EINVAL;
	}
	*cap = 0U;
	ret = pcie_endpoint_conf_read(ep, PCIE_CONF_CMDSTAT, &data);
	if (ret != 0 || data == UINT32_MAX || (data & PCIE_CONF_CMDSTAT_CAPS) == 0U) {
		return ret;
	}
	ret = pcie_endpoint_conf_read(ep, PCIE_CONF_CAPPTR, &data);
	if (ret != 0) {
		return ret;
	}
	reg = PCIE_CONF_CAPPTR_FIRST(data);

	/* A bounded walk also terminates a malformed cyclic capability list. */
	for (uint32_t count = 0U; reg != 0U && count < 48U; count++) {
		if (reg < 16U) {
			return -EINVAL;
		}
		ret = pcie_endpoint_conf_read(ep, reg, &data);
		if (ret != 0 || data == UINT32_MAX) {
			return ret;
		}
		if (PCIE_CONF_CAP_ID(data) == cap_id) {
			*cap = reg;
			return 0;
		}
		reg = PCIE_CONF_CAP_NEXT(data);
	}

	return reg == 0U ? 0 : -EINVAL;
}

int pcie_endpoint_get_ext_cap(const struct pcie_endpoint *ep, uint32_t cap_id, uint32_t *cap)
{
	uint32_t reg = PCIE_CONF_EXT_CAPPTR;
	uint32_t data;
	int ret;

	if (ep == NULL || ep->ctrl == NULL || cap == NULL) {
		return -EINVAL;
	}
	*cap = 0U;
	for (uint32_t count = 0U; count < 960U; count++) {
		ret = pcie_endpoint_conf_read(ep, reg, &data);
		if (ret != 0 || data == 0U || data == UINT32_MAX) {
			return ret;
		}
		if (PCIE_CONF_EXT_CAP_ID(data) == cap_id) {
			*cap = reg;
			return 0;
		}
		reg = PCIE_CONF_EXT_CAP_NEXT(data) >> 2;
		if (reg == 0U) {
			return 0;
		}
		if (reg < PCIE_CONF_EXT_CAPPTR) {
			return -EINVAL;
		}
	}

	return -EINVAL;
}

int z_pcie_get_bar(const struct pcie_endpoint *ep, unsigned int bar_index,
		   struct pcie_bar *bar, bool io)
{
	uint32_t reg = PCIE_CONF_BAR0 + bar_index;
	uint32_t low, high = 0U, mask_low, mask_high = 0U, cmd;
	uint32_t n_bars;
	uint64_t address, size;
	int ret, restore;

	if (ep == NULL || ep->ctrl == NULL || bar == NULL || bar_index >= 6U) {
		return -EINVAL;
	}

	ret = pcie_endpoint_conf_read(ep, PCIE_CONF_TYPE, &low);
	if (ret != 0) {
		return ret;
	}

	if (low == UINT32_MAX) {
		return -ENOENT;
	}

	switch (PCIE_CONF_TYPE_GET(low)) {
	case PCIE_CONF_TYPE_STANDARD:
		n_bars = 6U;
		break;
	case PCIE_CONF_TYPE_PCI_BRIDGE:
		n_bars = 2U;
		break;
	default:
		return -ENOTSUP;
	}

	if (bar_index >= n_bars) {
		return -ENOENT;
	}

	/* An upper word of a 64-bit BAR is not another BAR. */
	for (uint32_t prev = PCIE_CONF_BAR0; prev < reg; prev++) {
		ret = pcie_endpoint_conf_read(ep, prev, &low);
		if (ret != 0) {
			return ret;
		}
		if (PCIE_CONF_BAR_MEM(low) && PCIE_CONF_BAR_64(low)) {
			if (++prev == reg) {
				return -ENOENT;
			}
		}
	}
	ret = pcie_endpoint_conf_read(ep, reg, &low);
	if (ret != 0) {
		return ret;
	}
	if (low == UINT32_MAX || PCIE_CONF_BAR_IO(low) != io ||
	    (!io && PCIE_CONF_BAR_INVAL_FLAGS(low))) {
		return -ENOENT;
	}
	if (!io && PCIE_CONF_BAR_64(low)) {
		if (bar_index + 1U >= n_bars) {
			return -ENOENT;
		}
		ret = pcie_endpoint_conf_read(ep, reg + 1U, &high);
		if (ret != 0) {
			return ret;
		}
	}
	address = ((uint64_t)high << 32) | (low & (io ? ~0x3U : ~0xfU));
	if (address == 0U || address > UINTPTR_MAX) {
		return address == 0U ? -ENOENT : -ENOTSUP;
	}
	ret = pcie_endpoint_conf_read(ep, PCIE_CONF_CMDSTAT, &cmd);
	if (ret != 0) {
		return ret;
	}
	/* Status bits are write-one-to-clear, so write only the command half. */
	cmd &= 0xffffU;
	ret = pcie_endpoint_conf_write(ep, PCIE_CONF_CMDSTAT,
				       cmd & ~(PCIE_CONF_CMDSTAT_IO | PCIE_CONF_CMDSTAT_MEM));
	if (ret != 0) {
		return ret;
	}
	ret = pcie_endpoint_conf_write(ep, reg, UINT32_MAX);
	if (ret == 0) {
		ret = pcie_endpoint_conf_read(ep, reg, &mask_low);
	}
	restore = pcie_endpoint_conf_write(ep, reg, low);
	if (restore != 0) {
		/* Keep decoding disabled until every BAR word has been restored. */
		return ret != 0 ? ret : restore;
	}
	if (ret == 0 && !io && PCIE_CONF_BAR_64(low)) {
		ret = pcie_endpoint_conf_write(ep, reg + 1U, UINT32_MAX);
		if (ret == 0) {
			ret = pcie_endpoint_conf_read(ep, reg + 1U, &mask_high);
		}
		restore = pcie_endpoint_conf_write(ep, reg + 1U, high);
		if (restore != 0) {
			return ret != 0 ? ret : restore;
		}
	}
	restore = pcie_endpoint_conf_write(ep, PCIE_CONF_CMDSTAT, cmd);
	if (ret != 0 || restore != 0) {
		return ret != 0 ? ret : restore;
	}
	size = ((uint64_t)mask_high << 32) | (mask_low & (io ? ~0x3U : ~0xfU));
	size &= ~(size - 1U);
	if (size == 0U) {
		return -ENOENT;
	}
	if (size > SIZE_MAX) {
		return -ENOTSUP;
	}

#ifdef CONFIG_PCIE_CONTROLLER
	if (!pcie_ctrl_region_translate(ep->ctrl, ep->bdf, !io, !io && PCIE_CONF_BAR_64(low),
					(uintptr_t)address, &bar->phys_addr)) {
		return -ENOTSUP;
	}
	bar->size = (size_t)size;
	return 0;
#else
	return -ENOTSUP;
#endif
}

int pcie_endpoint_get_mbar(const struct pcie_endpoint *ep, unsigned int bar_index,
			   struct pcie_bar *bar)
{
	return z_pcie_get_bar(ep, bar_index, bar, false);
}

uint32_t pcie_ctrl_get_cap(const struct device *ctrl, pcie_bdf_t bdf, uint32_t cap_id)
{
	const struct pcie_endpoint ep = {.ctrl = ctrl, .bdf = bdf};
	uint32_t cap;

	return pcie_endpoint_get_cap(&ep, cap_id, &cap) == 0 ? cap : 0U;
}

bool pcie_ctrl_get_mbar(const struct device *ctrl, pcie_bdf_t bdf, unsigned int bar_index,
			struct pcie_bar *bar)
{
	const struct pcie_endpoint ep = {.ctrl = ctrl, .bdf = bdf};

	return pcie_endpoint_get_mbar(&ep, bar_index, bar) == 0;
}
