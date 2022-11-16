/*
 * Copyright (c) 2023 Arm Limited (or its affiliates). All rights reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/drivers/pcie/pcie.h>
#include <zephyr/iommu/iommu.h>

LOG_MODULE_REGISTER(iommu, CONFIG_IOMMU_LOG_LEVEL);

struct iommu_ctx *iommu_get_ctx(const struct device *iommu_dev, const struct device *child)
{
	/* TODO: Unsupported yet. */
	return NULL;
}

struct iommu_ctx *iommu_pci_instantiate_ctx(struct pcie_dev *child, bool bypass)
{
	struct iommu_driver_api *api;
	struct iommu_ctx *ctx;
	struct iommu_domain *iodom;
	uint16_t rid;
	uint32_t sid;
	int ret;

	rid = child->bdf >> PCIE_BDF_FUNC_SHIFT;

	STRUCT_SECTION_FOREACH(iommu_maps_spec, map) {
		if (rid - map->rid_base < map->length) {
			sid = rid + map->iommu_base;
			api = (struct iommu_driver_api *)map->dev->api;
			iodom = api->domain_alloc(map->dev);
			/* TODO: Assert */
			__ASSERT(iodom != NULL, "");
			ctx = api->ctx_alloc(map->dev, iodom, NULL, sid, false);
			__ASSERT(ctx != NULL, "");
			ret = api->ctx_init(map->dev, ctx);
			__ASSERT(ret == 0, "");
			return ctx;
		}
	}

	return NULL;
}
