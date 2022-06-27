/*
 * Copyright (c) 2021 BayLibre, SAS
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(pcie_core, LOG_LEVEL_INF);

#include <zephyr/kernel.h>
#include <zephyr/drivers/pcie/pcie.h>
#include <zephyr/drivers/pcie/controller.h>

#if CONFIG_PCIE_MSI
#include <zephyr/drivers/pcie/msi.h>
#endif

/* arch agnostic PCIe API implementation */

uint32_t pcie_conf_read(pcie_bdf_t bdf, unsigned int reg)
{
	const struct device *dev;

	dev = DEVICE_DT_GET(DT_CHOSEN(zephyr_pcie_controller));
	if (!dev) {
		LOG_ERR("Failed to get PCIe root complex");
		return 0xffffffff;
	}

	return pcie_ctrl_conf_read(dev, bdf, reg);
}

void pcie_conf_write(pcie_bdf_t bdf, unsigned int reg, uint32_t data)
{
	const struct device *dev;

	dev = DEVICE_DT_GET(DT_CHOSEN(zephyr_pcie_controller));
	if (!dev) {
		LOG_ERR("Failed to get PCIe root complex");
		return;
	}

	pcie_ctrl_conf_write(dev, bdf, reg, data);
}

uint32_t pcie_generic_ctrl_conf_read(mm_reg_t cfg_addr, pcie_bdf_t bdf, unsigned int reg)
{
	volatile uint32_t *bdf_cfg_mem = (volatile uint32_t *)((uintptr_t)cfg_addr + (bdf << 4));

	if (!cfg_addr) {
		return 0xffffffff;
	}

	return bdf_cfg_mem[reg];
}

void pcie_generic_ctrl_conf_write(mm_reg_t cfg_addr, pcie_bdf_t bdf,
				 unsigned int reg, uint32_t data)
{
	volatile uint32_t *bdf_cfg_mem = (volatile uint32_t *)((uintptr_t)cfg_addr + (bdf << 4));

	if (!cfg_addr) {
		return;
	}

	bdf_cfg_mem[reg] = data;
}

static void pcie_generic_ctrl_enumerate_bars(const struct device *ctrl_dev, pcie_bdf_t bdf,
					     unsigned int nbars)
{
	unsigned int bar, reg, data;
	uintptr_t scratch, bar_bus_addr;
	size_t size, bar_size;

	for (bar = 0, reg = PCIE_CONF_BAR0; bar < nbars && reg <= PCIE_CONF_BAR5; reg ++, bar++) {
		bool found_mem64 = false;
		bool found_mem = false;

		data = scratch = pcie_conf_read(bdf, reg);

		if (PCIE_CONF_BAR_INVAL_FLAGS(data)) {
			continue;
		}

		if (PCIE_CONF_BAR_MEM(data)) {
			found_mem = true;
			if (PCIE_CONF_BAR_64(data)) {
				found_mem64 = true;
				scratch |= ((uint64_t)pcie_conf_read(bdf, reg + 1)) << 32;
				if (PCIE_CONF_BAR_ADDR(scratch) == PCIE_CONF_BAR_INVAL64) {
					continue;
				}
			} else {
				if (PCIE_CONF_BAR_ADDR(scratch) == PCIE_CONF_BAR_INVAL) {
					continue;
				}
			}
		}

		pcie_conf_write(bdf, reg, 0xFFFFFFFF);
		size = pcie_conf_read(bdf, reg);
		pcie_conf_write(bdf, reg, scratch & 0xFFFFFFFF);

		if (found_mem64) {
			pcie_conf_write(bdf, reg + 1, 0xFFFFFFFF);
			size |= ((uint64_t)pcie_conf_read(bdf, reg + 1)) << 32;
			pcie_conf_write(bdf, reg + 1, scratch >> 32);
		}

		if (!PCIE_CONF_BAR_ADDR(size)) {
			if (found_mem64) {
				reg++;
			}
			continue;
		}

		if (found_mem) {
			if (found_mem64) {
				bar_size = (uint64_t)~PCIE_CONF_BAR_ADDR(size) + 1;
			} else {
				bar_size = (uint32_t)~PCIE_CONF_BAR_ADDR(size) + 1;
			}
		} else {
			bar_size = (uint32_t)~PCIE_CONF_BAR_IO_ADDR(size) + 1;
		}

		if (pcie_ctrl_region_allocate(ctrl_dev, bdf, found_mem,
					      found_mem64, bar_size, &bar_bus_addr)) {
			uintptr_t bar_phys_addr;

			pcie_ctrl_region_translate(ctrl_dev, bdf, found_mem,
						   found_mem64, bar_bus_addr, &bar_phys_addr);

			LOG_INF("[%02x:%02x.%x] BAR%d size 0x%lx "
				"assigned [%s 0x%lx-0x%lx -> 0x%lx-0x%lx]",
				PCIE_BDF_TO_BUS(bdf), PCIE_BDF_TO_DEV(bdf), PCIE_BDF_TO_FUNC(bdf),
				bar, bar_size,
				found_mem ? (found_mem64 ? "mem64" : "mem") : "io",
				bar_bus_addr, bar_bus_addr + bar_size - 1,
				bar_phys_addr, bar_phys_addr + bar_size - 1);

			pcie_conf_write(bdf, reg, bar_bus_addr & 0xFFFFFFFF);
			if (found_mem64) {
				pcie_conf_write(bdf, reg + 1, bar_bus_addr >> 32);
			}
		} else {
			LOG_INF("[%02x:%02x.%x] BAR%d size 0x%lx Failed memory allocation.",
				PCIE_BDF_TO_BUS(bdf), PCIE_BDF_TO_DEV(bdf), PCIE_BDF_TO_FUNC(bdf),
				bar, bar_size);
		}

		if (found_mem64) {
			reg++;
		}
	}
}

static bool pcie_generic_ctrl_enumerate_type1(const struct device *ctrl_dev, pcie_bdf_t bdf,
					      unsigned int bus_number)
{
	uint32_t class = pcie_conf_read(bdf, PCIE_CONF_CLASSREV);

	/* Handle only PCI-to-PCI bridge for now */
	if (PCIE_CONF_CLASSREV_CLASS(class) == 0x06 &&
	    PCIE_CONF_CLASSREV_SUBCLASS(class) == 0x04) {
		uint32_t number = pcie_conf_read(bdf, PCIE_BUS_NUMBER);
		uintptr_t bar_base_addr;

		pcie_generic_ctrl_enumerate_bars(ctrl_dev, bdf, 2);

		/* Configure bus number registers */
		pcie_conf_write(bdf, PCIE_BUS_NUMBER,
				PCIE_BUS_NUMBER_VAL(PCIE_BDF_TO_BUS(bdf),
						    bus_number,
						    0xff, /* set max until we finished scanning */
						    PCIE_SECONDARY_LATENCY_TIMER(number)));

		/* I/O align on 4k boundary */
		if (pcie_ctrl_region_get_allocate_base(ctrl_dev, bdf, false, false,
						       KB(4), &bar_base_addr)) {
			uint32_t io = pcie_conf_read(bdf, PCIE_IO_SEC_STATUS);
			uint32_t io_upper = pcie_conf_read(bdf, PCIE_IO_BASE_LIMIT_UPPER);

			pcie_conf_write(bdf, PCIE_IO_SEC_STATUS,
					PCIE_IO_SEC_STATUS_VAL(PCIE_IO_BASE(io),
							       PCIE_IO_LIMIT(io),
							       PCIE_SEC_STATUS(io)));

			pcie_conf_write(bdf, PCIE_IO_BASE_LIMIT_UPPER,
				PCIE_IO_BASE_LIMIT_UPPER_VAL(PCIE_IO_BASE_UPPER(io_upper),
							     PCIE_IO_LIMIT_UPPER(io_upper)));

			pcie_set_cmd(bdf, PCIE_CONF_CMDSTAT_IO, true);
		}

		/* MEM align on 1MiB boundary */
		if (pcie_ctrl_region_get_allocate_base(ctrl_dev, bdf, true, false,
						       MB(1), &bar_base_addr)) {
			uint32_t mem = pcie_conf_read(bdf, PCIE_MEM_BASE_LIMIT);

			pcie_conf_write(bdf, PCIE_MEM_BASE_LIMIT,
					PCIE_MEM_BASE_LIMIT_VAL((bar_base_addr & 0xfff00000) >> 16,
								PCIE_MEM_LIMIT(mem)));

			pcie_set_cmd(bdf, PCIE_CONF_CMDSTAT_MEM, true);
		}

		/* TODO: add support for prefetchable */

		pcie_set_cmd(bdf, PCIE_CONF_CMDSTAT_MASTER, true);

		return true;
	}

	return false;
}

static void pcie_generic_ctrl_post_enumerate_type1(const struct device *ctrl_dev, pcie_bdf_t bdf,
						   unsigned int bus_number)
{
	uint32_t number = pcie_conf_read(bdf, PCIE_BUS_NUMBER);
	uintptr_t bar_base_addr;

	/* Configure bus subordinate */
	pcie_conf_write(bdf, PCIE_BUS_NUMBER,
			PCIE_BUS_NUMBER_VAL(PCIE_BUS_PRIMARY_NUMBER(number),
				PCIE_BUS_SECONDARY_NUMBER(number),
				bus_number - 1,
				PCIE_SECONDARY_LATENCY_TIMER(number)));

	/* I/O align on 4k boundary */
	if (pcie_ctrl_region_get_allocate_base(ctrl_dev, bdf, false, false,
					       KB(4), &bar_base_addr)) {
		uint32_t io = pcie_conf_read(bdf, PCIE_IO_SEC_STATUS);
		uint32_t io_upper = pcie_conf_read(bdf, PCIE_IO_BASE_LIMIT_UPPER);

		pcie_conf_write(bdf, PCIE_IO_SEC_STATUS,
				PCIE_IO_SEC_STATUS_VAL(PCIE_IO_BASE(io),
					((bar_base_addr - 1) & 0x0000f000) >> 16,
					PCIE_SEC_STATUS(io)));

		pcie_conf_write(bdf, PCIE_IO_BASE_LIMIT_UPPER,
				PCIE_IO_BASE_LIMIT_UPPER_VAL(PCIE_IO_BASE_UPPER(io_upper),
					((bar_base_addr - 1) & 0xffff0000) >> 16));
	}

	/* MEM align on 1MiB boundary */
	if (pcie_ctrl_region_get_allocate_base(ctrl_dev, bdf, true, false,
					       MB(1), &bar_base_addr)) {
		uint32_t mem = pcie_conf_read(bdf, PCIE_MEM_BASE_LIMIT);

		pcie_conf_write(bdf, PCIE_MEM_BASE_LIMIT,
				PCIE_MEM_BASE_LIMIT_VAL(PCIE_MEM_BASE(mem),
					(bar_base_addr - 1) >> 16));
	}

	/* TODO: add support for prefetchable */
}

static void pcie_generic_ctrl_enumerate_type0(const struct device *ctrl_dev, pcie_bdf_t bdf)
{
	/* Setup Type0 BARs */
	pcie_generic_ctrl_enumerate_bars(ctrl_dev, bdf, 6);
}

static bool pcie_generic_ctrl_enumerate_endpoint(const struct device *ctrl_dev,
						 pcie_bdf_t bdf, unsigned int bus_number,
						 bool *skip_next_func)
{
	bool multifunction_device = false;
	bool layout_type_1 = false;
	uint32_t data, class, id;
	bool is_bridge = false;

	*skip_next_func = false;

	id = pcie_conf_read(bdf, PCIE_CONF_ID);
	if (id == PCIE_ID_NONE) {
		return false;
	}

	class = pcie_conf_read(bdf, PCIE_CONF_CLASSREV);
	data = pcie_conf_read(bdf, PCIE_CONF_TYPE);

	multifunction_device = PCIE_CONF_MULTIFUNCTION(data);
	layout_type_1 = PCIE_CONF_TYPE_BRIDGE(data);

	LOG_INF("[%02x:%02x.%x] %04x:%04x class %x subclass %x progif %x "
		"rev %x Type%x multifunction %s",
		PCIE_BDF_TO_BUS(bdf), PCIE_BDF_TO_DEV(bdf), PCIE_BDF_TO_FUNC(bdf),
		id & 0xffff, id >> 16,
		PCIE_CONF_CLASSREV_CLASS(class),
		PCIE_CONF_CLASSREV_SUBCLASS(class),
		PCIE_CONF_CLASSREV_PROGIF(class),
		PCIE_CONF_CLASSREV_REV(class),
		layout_type_1 ? 1 : 0,
		multifunction_device ? "true" : "false");

	/* Do not enumerate sub-functions if not a multifunction device */
	if (PCIE_BDF_TO_FUNC(bdf) == 0 && !multifunction_device) {
		*skip_next_func = true;
	}

	if (layout_type_1) {
		is_bridge = pcie_generic_ctrl_enumerate_type1(ctrl_dev, bdf, bus_number);
	} else {
		pcie_generic_ctrl_enumerate_type0(ctrl_dev, bdf);
	}

	return is_bridge;
}

/* Return the next BDF or PCIE_BDF_NONE without changing bus number */
static inline unsigned int pcie_bdf_bus_next(unsigned int bdf, bool skip_next_func)
{
	if (skip_next_func) {
		if (PCIE_BDF_TO_DEV(bdf) == PCIE_BDF_DEV_MASK) {
			return PCIE_BDF_NONE;
		}

		return PCIE_BDF(PCIE_BDF_TO_BUS(bdf), PCIE_BDF_TO_DEV(bdf) + 1, 0);
	}

	if (PCIE_BDF_TO_DEV(bdf) == PCIE_BDF_DEV_MASK &&
	    PCIE_BDF_TO_FUNC(bdf) == PCIE_BDF_FUNC_MASK) {
		return PCIE_BDF_NONE;
	}

	return PCIE_BDF(PCIE_BDF_TO_BUS(bdf),
			(PCIE_BDF_TO_DEV(bdf) +
			 ((PCIE_BDF_TO_FUNC(bdf) + 1) / (PCIE_BDF_FUNC_MASK + 1))),
			((PCIE_BDF_TO_FUNC(bdf) + 1) & PCIE_BDF_FUNC_MASK));
}

struct pcie_bus_state {
	/* Current scanned bus BDF, always valid */
	unsigned int bus_bdf;
	/* Current bridge endpoint BDF, either valid or PCIE_BDF_NONE */
	unsigned int bridge_bdf;
	/* Next BDF to scan on bus, either valid or PCIE_BDF_NONE when all EP scanned */
	unsigned int next_bdf;
};

#define MAX_TRAVERSE_STACK 256

/* Non-recursive stack based PCIe bus & bridge enumeration */
void pcie_generic_ctrl_enumerate(const struct device *ctrl_dev, pcie_bdf_t bdf_start)
{
	struct pcie_bus_state stack[MAX_TRAVERSE_STACK], *state;
	unsigned int bus_number = PCIE_BDF_TO_BUS(bdf_start) + 1;
	bool skip_next_func = false;
	bool is_bridge = false;

	int stack_top = 0;

	/* Start with first endpoint of immediate Root Controller bus */
	stack[stack_top].bus_bdf = PCIE_BDF(PCIE_BDF_TO_BUS(bdf_start), 0, 0);
	stack[stack_top].bridge_bdf = PCIE_BDF_NONE;
	stack[stack_top].next_bdf = bdf_start;

	while (stack_top >= 0) {
		/* Top of stack contains the current PCIe bus to traverse */
		state = &stack[stack_top];

		/* Finish current bridge configuration before scanning other endpoints */
		if (state->bridge_bdf != PCIE_BDF_NONE) {
			pcie_generic_ctrl_post_enumerate_type1(ctrl_dev, state->bridge_bdf,
							       bus_number);

			state->bridge_bdf = PCIE_BDF_NONE;
		}

		/* We still have more endpoints to scan */
		if (state->next_bdf != PCIE_BDF_NONE) {
			while (state->next_bdf != PCIE_BDF_NONE) {
				is_bridge = pcie_generic_ctrl_enumerate_endpoint(ctrl_dev,
										 state->next_bdf,
										 bus_number,
										 &skip_next_func);
				if (is_bridge) {
					state->bridge_bdf = state->next_bdf;
					state->next_bdf = pcie_bdf_bus_next(state->next_bdf,
									    skip_next_func);

					/* If we can't handle more bridges, don't go further */
					if (stack_top == (MAX_TRAVERSE_STACK - 1) ||
					    bus_number == PCIE_BDF_BUS_MASK) {
						break;
					}

					/* Push to stack to scan this bus */
					stack_top++;
					stack[stack_top].bus_bdf = PCIE_BDF(bus_number, 0, 0);
					stack[stack_top].bridge_bdf = PCIE_BDF_NONE;
					stack[stack_top].next_bdf = PCIE_BDF(bus_number, 0, 0);

					/* Increase bus number */
					bus_number++;

					break;
				}

				state->next_bdf = pcie_bdf_bus_next(state->next_bdf,
								    skip_next_func);
			}
		} else {
			/* We finished scanning this bus, go back and scan next endpoints */
			stack_top--;
		}
	}
}

#ifdef CONFIG_PCIE_MSI
uint32_t pcie_msi_map(unsigned int irq, msi_vector_t *vector, uint8_t n_vector)
{
	ARG_UNUSED(irq);

	return vector->arch.address;
}

uint16_t pcie_msi_mdr(unsigned int irq, msi_vector_t *vector)
{
	ARG_UNUSED(irq);

	return vector->arch.eventid;
}

uint8_t arch_pcie_msi_vectors_allocate(unsigned int priority,
				       msi_vector_t *vectors,
				       uint8_t n_vector)
{
	const struct device *dev;

	dev = DEVICE_DT_GET(DT_CHOSEN(zephyr_pcie_controller));
	if (!dev) {
		LOG_ERR("Failed to get PCIe root complex");
		return 0;
	}

	return pcie_ctrl_msi_device_setup(dev, priority, vectors, n_vector);
}

bool arch_pcie_msi_vector_connect(msi_vector_t *vector,
				  void (*routine)(const void *parameter),
				  const void *parameter,
				  uint32_t flags)
{
	if (irq_connect_dynamic(vector->arch.irq, vector->arch.priority, routine,
				parameter, flags) != vector->arch.irq) {
		return false;
	}

	irq_enable(vector->arch.irq);

	return true;
}
#endif
