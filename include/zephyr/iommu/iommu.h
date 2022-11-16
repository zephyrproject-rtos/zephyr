/*
 * Copyright (c) 2023 Arm Limited (or its affiliates). All rights reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Public APIs for IOMMU drivers
 */

#ifndef __ZEPHYR_INCLUDE_IOMMU_IOMMU_H__
#define __ZEPHYR_INCLUDE_IOMMU_IOMMU_H__

#include <errno.h>

#include <zephyr/device.h>
#include <zephyr/drivers/pcie/pcie.h>

#ifdef __cplusplus
extern "C" {
#endif

enum iommu_subdev_type {
	IOMMU_SUBDEV_PCIE = 0,
};

/* Any RID in the interval [rid_base, rid_base + length) is associated with the listed IOMMU,
 * with the IOMMU acceptable ID (RID - rid_base + iommu_base).
 */
struct iommu_maps_spec {
	const struct device *dev; /* iommu device */
	uint32_t rid_base;        /* Request ID */
	uint32_t iommu_base;
	uint32_t length;
};

struct iommu_domain {
	const struct device *dev; /* IOMMU device */
};

struct iommu_ctx {
	struct iommu_domain *domain;
};

/* TODO: Delete clang-format tag @Huifeng */
// clang-format off
#define Z_DEVICE_IOMMU_PCIE_NAME(node_id, spec_idx) \
	_CONCAT(iommu_maps_spec_ ## spec_idx ## _, DEVICE_DT_NAME_GET(node_id))

#define Z_IOMMU_MAPS_SPEC_ITERABLE_DT_INIT(spec_idx, node_id)				  \
	const STRUCT_SECTION_ITERABLE(iommu_maps_spec,					  \
		Z_DEVICE_IOMMU_PCIE_NAME(node_id, spec_idx)) = {			  \
		.dev = DEVICE_DT_GET(DT_PHANDLE_BY_IDX(node_id, iommu_maps, spec_idx)),	  \
		.rid_base = DT_PHA_BY_IDX(node_id, iommu_maps, spec_idx, rid_base),	  \
		.iommu_base = DT_PHA_BY_IDX(node_id, iommu_maps, spec_idx, iommu_base),   \
		.length = DT_PHA_BY_IDX(node_id, iommu_maps, spec_idx, length),		  \
	}

#define Z_IOMMU_MAPS_SPEC_ITERABLE_LIST_DT_INIT(node_id)	  \
	LISTIFY(DT_PROP_LEN(node_id, iommu_maps),		  \
		Z_IOMMU_MAPS_SPEC_ITERABLE_DT_INIT, (;), node_id) \

#define IOMMU_MAPS_SPEC_ITERABLE_LIST_DT_INIT(index) \
	Z_IOMMU_MAPS_SPEC_ITERABLE_LIST_DT_INIT(DT_DRV_INST(index))
// clang-format on

/** @brief IOMMU API structure. */
__subsystem struct iommu_driver_api {
	struct iommu_domain *(*domain_alloc)(const struct device *dev);
	struct iommu_ctx *(*ctx_alloc)(const struct device *dev, struct iommu_domain *iodom,
			const struct device *child, uint32_t sid, bool bypass);
	int (*ctx_init)(const struct device *dev, struct iommu_ctx *ioctx);
	int (*ctx_free)(const struct device *dev, struct iommu_ctx *ioctx);
	int (*dev_map)(const struct device *dev, mem_addr_t va, mem_addr_t pa, int size,
		       uint32_t attrs);
	int (*dev_unmap)(const struct device *dev, mem_addr_t va, int size);
};

struct iommu_ctx *iommu_pci_instantiate_ctx(struct pcie_dev *child, bool bypass);

/* TODO: Temporary usage */
static inline struct iommu_domain *iommu_get_default_domain(struct pcie_dev *child)
{
	struct iommu_driver_api *api;
	uint16_t rid = child->bdf >> PCIE_BDF_FUNC_SHIFT;

	STRUCT_SECTION_FOREACH(iommu_maps_spec, map) {
		if (rid - map->rid_base < map->length) {
			api = (struct iommu_driver_api *)map->dev->api;
			return api->domain_alloc(map->dev);
		}
	}

	__ASSERT(false, "Shouldn't go here");
}

__syscall int iommu_dev_map(struct iommu_domain *iodom, mem_addr_t va, mem_addr_t pa, int size,
			    uint32_t attrs);

static inline int z_impl_iommu_dev_map(struct iommu_domain *iodom, mem_addr_t va, mem_addr_t pa,
				       int size, uint32_t attrs)
{
	const struct device *dev = iodom->dev;
	const struct iommu_driver_api *api = dev->api;

	if (api->dev_map == NULL) {
		return -ENOSYS;
	}

	return api->dev_map(dev, va, pa, size, attrs);
}

__syscall int iommu_dev_unmap(struct iommu_domain *iodom, mem_addr_t va, int size);

static inline int z_impl_iommu_dev_unmap(struct iommu_domain *iodom, mem_addr_t va, int size)
{
	const struct device *dev = iodom->dev;
	const struct iommu_driver_api *api = dev->api;

	if (api->dev_unmap == NULL) {
		return -ENOSYS;
	}

	return api->dev_unmap(dev, va, size);
}

#ifdef __cplusplus
}
#endif

#include <syscalls/iommu.h>

#endif /* __ZEPHYR_INCLUDE_IOMMU_IOMMU_H__ */
