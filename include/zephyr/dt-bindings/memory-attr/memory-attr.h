/*
 * Copyright (c) 2023 Carlo Caione <ccaione@baylibre.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef ZEPHYR_INCLUDE_DT_BINDINGS_MEM_ATTR_H_
#define ZEPHYR_INCLUDE_DT_BINDINGS_MEM_ATTR_H_

/* Ideally we'd generate this enum to match what's coming out of the YAML,
 * however, we dont have a good way to know how to name such an enum from the
 * generation point of view, so for now we just hand code the enum.  This
 * enum is expected to match the order in the yaml (dts/bindings/base/zephyr,memory-attr.yaml)
 */

enum dt_memory_attr {
	DT_MEMORY_ATTR_RAM,
	DT_MEMORY_ATTR_RAM_NOCACHE,
	DT_MEMORY_ATTR_FLASH,
	DT_MEMORY_ATTR_PPB,
	DT_MEMORY_ATTR_IO,
	DT_MEMORY_ATTR_EXTMEM,
	DT_MEMORY_ATTR_UNKNOWN, /* must be last */
};

#endif /* ZEPHYR_INCLUDE_DT_BINDINGS_MEM_ATTR_H_ */
