/*
 * Copyright (c) 2019 Intel Corporation
 * Copyright (c) 2020 acontis technologies GmbH
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT pcie_controller

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(pcie, LOG_LEVEL_ERR);

#include <zephyr/init.h>
#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/sys/check.h>
#include <stdbool.h>
#include <zephyr/drivers/pcie/pcie.h>
#include <zephyr/sys/iterable_sections.h>

#include "pcie_priv.h"

#ifdef CONFIG_ACPI
#include <zephyr/acpi/acpi.h>
#endif

#if CONFIG_PCIE_MSI
#include <zephyr/drivers/pcie/msi.h>
#endif

#ifdef CONFIG_PCIE_CONTROLLER
#include <zephyr/drivers/pcie/controller.h>
#endif

struct scan_context {
	const struct device *ctrl;
	const struct pcie_scan_ctrl_opt *opt;
	const struct pcie_scan_opt *legacy_opt;
	uint32_t visited[8];
	int status;
};

struct scan_data {
	size_t found;
	size_t max_dev;
	int status;
};

#ifdef CONFIG_PCIE_PRT
/* platform interrupt are hardwired or can be dynamically allocated. */
static bool prt_en;
#endif

/* functions documented in drivers/pcie/pcie.h */

void pcie_set_cmd(pcie_bdf_t bdf, uint32_t bits, bool on)
{
	uint32_t cmdstat;

	cmdstat = pcie_conf_read(bdf, PCIE_CONF_CMDSTAT);

	if (on) {
		cmdstat |= bits;
	} else {
		cmdstat &= ~bits;
	}

	pcie_conf_write(bdf, PCIE_CONF_CMDSTAT, cmdstat);
}

uint32_t pcie_get_cap(pcie_bdf_t bdf, uint32_t cap_id)
{
#ifdef CONFIG_PCIE_CONTROLLER
	return pcie_ctrl_get_cap(DEVICE_DT_GET(DT_CHOSEN(zephyr_pcie_controller)), bdf, cap_id);
#else
	uint32_t reg = 0U;
	uint32_t data;

	data = pcie_conf_read(bdf, PCIE_CONF_CMDSTAT);
	if ((data & PCIE_CONF_CMDSTAT_CAPS) != 0U) {
		data = pcie_conf_read(bdf, PCIE_CONF_CAPPTR);
		reg = PCIE_CONF_CAPPTR_FIRST(data);
	}

	while (reg != 0U) {
		data = pcie_conf_read(bdf, reg);

		if (PCIE_CONF_CAP_ID(data) == cap_id) {
			break;
		}

		reg = PCIE_CONF_CAP_NEXT(data);
	}

	return reg;
#endif
}

uint32_t pcie_get_ext_cap(pcie_bdf_t bdf, uint32_t cap_id)
{
#ifdef CONFIG_PCIE_CONTROLLER
	const struct pcie_endpoint ep = {
		.ctrl = DEVICE_DT_GET(DT_CHOSEN(zephyr_pcie_controller)), .bdf = bdf,
	};
	uint32_t cap;

	return pcie_endpoint_get_ext_cap(&ep, cap_id, &cap) == 0 ? cap : 0U;
#else
	unsigned int reg = PCIE_CONF_EXT_CAPPTR; /* Start at end of the PCI configuration space */
	uint32_t data;

	while (reg != 0U) {
		data = pcie_conf_read(bdf, reg);
		if (!data || data == 0xffffffffU) {
			return 0;
		}

		if (PCIE_CONF_EXT_CAP_ID(data) == cap_id) {
			break;
		}

		reg = PCIE_CONF_EXT_CAP_NEXT(data) >> 2;

		if (reg < PCIE_CONF_EXT_CAPPTR) {
			return 0;
		}
	}

	return reg;
#endif
}

/**
 * @brief Get the BAR at a specific BAR index
 *
 * @param bdf the PCI(e) endpoint
 * @param bar_index 0-based BAR index
 * @param bar Pointer to struct pcie_bar
 * @param io true for I/O BARs, false otherwise
 * @return true if the BAR was found and is valid, false otherwise
 */
static bool pcie_get_bar(pcie_bdf_t bdf,
			 unsigned int bar_index,
			 struct pcie_bar *bar,
			 bool io)
{
#ifdef CONFIG_PCIE_CONTROLLER
	const struct pcie_endpoint ep = z_pcie_legacy_endpoint(bdf);

	return z_pcie_get_bar(&ep, bar_index, bar, io) == 0;
#else
	uint32_t reg = bar_index + PCIE_CONF_BAR0;
	uint32_t cmd_reg;
	bool ret = false;
	uintptr_t phys_addr;
	size_t size;

	if (reg > PCIE_CONF_BAR5) {
		return false;
	}

	phys_addr = pcie_conf_read(bdf, reg);
	if ((PCIE_CONF_BAR_MEM(phys_addr) && io) || (PCIE_CONF_BAR_IO(phys_addr) && !io)) {
		return false;
	}

	if (PCIE_CONF_BAR_INVAL_FLAGS(phys_addr)) {
		/* Discard on invalid flags */
		return false;
	}

	cmd_reg = pcie_conf_read(bdf, PCIE_CONF_CMDSTAT);

	/* IO/memory decode should be disabled before sizing/update BAR. */
	pcie_conf_write(bdf, PCIE_CONF_CMDSTAT,
			cmd_reg & (~(PCIE_CONF_CMDSTAT_IO | PCIE_CONF_CMDSTAT_MEM)));

	pcie_conf_write(bdf, reg, 0xFFFFFFFFU);
	size = pcie_conf_read(bdf, reg);
	pcie_conf_write(bdf, reg, (uint32_t)phys_addr);

	if (IS_ENABLED(CONFIG_64BIT) && PCIE_CONF_BAR_64(phys_addr)) {
		reg++;
		phys_addr |= ((uint64_t)pcie_conf_read(bdf, reg)) << 32;

		if ((PCIE_CONF_BAR_ADDR(phys_addr) == PCIE_CONF_BAR_INVAL64) ||
		    (PCIE_CONF_BAR_ADDR(phys_addr) == PCIE_CONF_BAR_NONE)) {
			/* Discard on invalid address */
			goto err_exit;
		}

		pcie_conf_write(bdf, reg, 0xFFFFFFFFU);
		size |= ((uint64_t)pcie_conf_read(bdf, reg)) << 32;
		pcie_conf_write(bdf, reg, (uint32_t)((uint64_t)phys_addr >> 32));
	} else if ((PCIE_CONF_BAR_ADDR(phys_addr) == PCIE_CONF_BAR_INVAL) ||
		   (PCIE_CONF_BAR_ADDR(phys_addr) == PCIE_CONF_BAR_NONE)) {
		/* Discard on invalid address */
		goto err_exit;
	}

	if (PCIE_CONF_BAR_IO(phys_addr)) {
		size = PCIE_CONF_BAR_IO_ADDR(size);
		if (size == 0) {
			/* Discard on invalid size */
			goto err_exit;
		}
	} else {
		size = PCIE_CONF_BAR_ADDR(size);
		if (size == 0) {
			/* Discard on invalid size */
			goto err_exit;
		}
	}

	bar->phys_addr = PCIE_CONF_BAR_ADDR(phys_addr);
	bar->size = size & ~(size-1);

	ret = true;
err_exit:
	pcie_conf_write(bdf, PCIE_CONF_CMDSTAT, cmd_reg);

	return ret;
#endif
}

/**
 * @brief Probe the nth BAR assigned to an endpoint.
 *
 * A PCI(e) endpoint has 0 or more BARs. This function
 * allows the caller to enumerate them by calling with index=0..n.
 * Value of n has to be below 6, as there is a maximum of 6 BARs. The indices
 * are order-preserving with respect to the endpoint BARs: e.g., index 0
 * will return the lowest-numbered BAR on the endpoint.
 *
 * @param bdf the PCI(e) endpoint
 * @param index (0-based) index
 * @param bar Pointer to struct pcie_bar
 * @param io true for I/O BARs, false otherwise
 * @return true if the BAR was found and is valid, false otherwise
 */
static bool pcie_probe_bar(pcie_bdf_t bdf,
			   unsigned int index,
			   struct pcie_bar *bar,
			   bool io)
{
	const struct pcie_endpoint ep = z_pcie_legacy_endpoint(bdf);
	uint32_t reg;

	for (reg = PCIE_CONF_BAR0;
	     (index > 0) && (reg <= PCIE_CONF_BAR5); reg++, index--) {
		uint32_t addr;

		if (z_pcie_conf_read(&ep, reg, &addr) != 0) {
			return false;
		}

		if (PCIE_CONF_BAR_MEM(addr) && PCIE_CONF_BAR_64(addr)) {
			reg++;
		}
	}

	if (index != 0) {
		return false;
	}

	return pcie_get_bar(bdf, reg - PCIE_CONF_BAR0, bar, io);
}

bool pcie_get_mbar(pcie_bdf_t bdf,
		   unsigned int bar_index,
		   struct pcie_bar *mbar)
{
#ifdef CONFIG_PCIE_CONTROLLER
	return pcie_ctrl_get_mbar(DEVICE_DT_GET(DT_CHOSEN(zephyr_pcie_controller)), bdf,
				  bar_index, mbar);
#else
	return pcie_get_bar(bdf, bar_index, mbar, false);
#endif
}

bool pcie_probe_mbar(pcie_bdf_t bdf,
		     unsigned int index,
		     struct pcie_bar *mbar)
{
	return pcie_probe_bar(bdf, index, mbar, false);
}

bool pcie_get_iobar(pcie_bdf_t bdf,
		    unsigned int bar_index,
		    struct pcie_bar *iobar)
{
	return pcie_get_bar(bdf, bar_index, iobar, true);
}

bool pcie_probe_iobar(pcie_bdf_t bdf,
		      unsigned int index,
		      struct pcie_bar *iobar)
{
	return pcie_probe_bar(bdf, index, iobar, true);
}

#ifndef CONFIG_PCIE_CONTROLLER

unsigned int pcie_alloc_irq(pcie_bdf_t bdf)
{
	unsigned int irq;
	uint32_t data;

	data = pcie_conf_read(bdf, PCIE_CONF_INTR);
	irq = PCIE_CONF_INTR_IRQ(data);

	if ((irq == PCIE_CONF_INTR_IRQ_NONE) ||
	    (irq >= CONFIG_MAX_IRQ_LINES) ||
	    arch_irq_is_used(irq)) {

		/* In some platforms, PCI interrupts are hardwired to specific interrupt inputs
		 * on the interrupt controller and are not configurable. Hence we need to retrieve
		 * IRQ from acpi. But if it is configurable then we allocate irq dynamically.
		 */
#ifdef CONFIG_PCIE_PRT
		if (prt_en) {
			irq = acpi_legacy_irq_get(bdf);
		} else {
			irq = arch_irq_allocate();
		}
#else
		irq = arch_irq_allocate();
#endif

		if (irq == UINT_MAX) {
			return PCIE_CONF_INTR_IRQ_NONE;
		}

		data &= ~0xffU;
		data |= irq;
		pcie_conf_write(bdf, PCIE_CONF_INTR, data);
	} else {
		arch_irq_set_used(irq);
	}

	return irq;
}
#endif /* CONFIG_PCIE_CONTROLLER */

unsigned int pcie_get_irq(pcie_bdf_t bdf)
{
	uint32_t data = pcie_conf_read(bdf, PCIE_CONF_INTR);

	return PCIE_CONF_INTR_IRQ(data);
}

bool pcie_connect_dynamic_irq(pcie_bdf_t bdf,
			      unsigned int irq,
			      unsigned int priority,
			      void (*routine)(const void *parameter),
			      const void *parameter,
			      uint32_t flags)
{
#if defined(CONFIG_PCIE_MSI) && defined(CONFIG_PCIE_MSI_MULTI_VECTOR)
	if (pcie_is_msi(bdf)) {
		msi_vector_t vector;

		if ((pcie_msi_vectors_allocate(bdf, priority,
					       &vector, 1) == 0) ||
		    !pcie_msi_vector_connect(bdf, &vector,
					     routine, parameter, flags)) {
			return false;
		}
	} else
#endif /* CONFIG_PCIE_MSI && CONFIG_PCIE_MSI_MULTI_VECTOR */
	{
		if (irq_connect_dynamic(irq, priority, routine,
					parameter, flags) < 0) {
			return false;
		}
	}

	return true;
}

void pcie_irq_enable(pcie_bdf_t bdf, unsigned int irq)
{
#if CONFIG_PCIE_MSI
	if (pcie_msi_enable(bdf, NULL, 1, irq)) {
		return;
	}
#endif
	irq_enable(irq);
}

static bool scan_flag(const struct pcie_scan_ctrl_opt *opt, uint32_t flag)
{
	return ((opt->flags & flag) != 0U);
}

static bool scan_bus(struct scan_context *ctx, uint8_t bus);

static uint32_t scan_read(struct scan_context *ctx, pcie_bdf_t bdf, unsigned int reg)
{
	const struct pcie_endpoint ep = {.ctrl = ctx->ctrl, .bdf = bdf};
	uint32_t value = UINT32_MAX;

	if (ctx->status == 0) {
		ctx->status = z_pcie_conf_read(&ep, reg, &value);
	}
	return value;
}

static bool scan_dev(struct scan_context *ctx, uint8_t bus, uint8_t dev)
{
	const struct pcie_scan_ctrl_opt *opt = ctx->opt;
	uint8_t deferred_buses[PCIE_MAX_FUNC + 1];
	uint8_t deferred_count = 0U;

	for (uint8_t func = 0U; func <= PCIE_MAX_FUNC; func++) {
		const struct pcie_endpoint ep = {
			.ctrl = ctx->ctrl, .bdf = PCIE_BDF(bus, dev, func),
		};
		uint32_t id, type;
		bool masked, do_cb;

		masked = opt->scan_masking_fn != NULL &&
			opt->scan_masking_fn(&ep, opt->cb_data);
		if (masked) {
			continue;
		}

		id = scan_read(ctx, ep.bdf, PCIE_CONF_ID);
		if (ctx->status != 0) {
			return false;
		}
		if (!PCIE_ID_IS_VALID(id)) {
			continue;
		}
		type = scan_read(ctx, ep.bdf, PCIE_CONF_TYPE);
		if (ctx->status != 0) {
			return false;
		}
		do_cb = PCIE_CONF_TYPE_GET(type) == PCIE_CONF_TYPE_STANDARD ||
			scan_flag(opt, PCIE_SCAN_CB_ALL);
		if (do_cb) {
			bool keep_scanning = opt->cb(&ep, id, opt->cb_data);

			if (!keep_scanning) {
				return false;
			}
		}

		if (PCIE_CONF_TYPE_GET(type) == PCIE_CONF_TYPE_PCI_BRIDGE &&
		    scan_flag(opt, PCIE_SCAN_RECURSIVE)) {
			uint32_t num = scan_read(ctx, ep.bdf, PCIE_BUS_NUMBER);
			uint8_t secondary = PCIE_BUS_SECONDARY_NUMBER(num);

			if (ctx->status != 0) {
				return false;
			}
			if (secondary != 0U) {
				deferred_buses[deferred_count++] = secondary;
			}
		}
		if (func == 0U && !PCIE_CONF_MULTIFUNCTION(type)) {
			break;
		}
	}
	for (uint8_t idx = 0U; idx < deferred_count; idx++) {
		if (!scan_bus(ctx, deferred_buses[idx])) {
			return false;
		}
	}
	return true;
}

static bool scan_bus(struct scan_context *ctx, uint8_t bus)
{
	if ((ctx->visited[bus / 32U] & BIT(bus % 32U)) != 0U) {
		return true;
	}
	ctx->visited[bus / 32U] |= BIT(bus % 32U);
	for (uint8_t dev = 0U; dev <= PCIE_MAX_DEV; dev++) {
		if (!scan_dev(ctx, bus, dev)) {
			return false;
		}
	}
	return true;
}

static int scan(struct scan_context *ctx)
{
	const struct pcie_scan_ctrl_opt *opt = ctx->opt;
	bool multi = false;

	if (!IS_ENABLED(CONFIG_PCIE_CONTROLLER) && opt->bus == 0U &&
	    scan_flag(opt, PCIE_SCAN_RECURSIVE)) {
		const struct pcie_endpoint ep = {.ctrl = ctx->ctrl, .bdf = PCIE_HOST_CONTROLLER(0)};

		if (opt->scan_masking_fn == NULL || !opt->scan_masking_fn(&ep, opt->cb_data)) {
			multi = PCIE_CONF_MULTIFUNCTION(scan_read(ctx, ep.bdf, PCIE_CONF_TYPE));
		}
	}
	if (multi) {
		for (uint8_t bus = 0U; bus <= PCIE_MAX_FUNC; bus++) {
			const struct pcie_endpoint ep = {
				.ctrl = ctx->ctrl, .bdf = PCIE_HOST_CONTROLLER(bus),
			};

			if (opt->scan_masking_fn != NULL &&
			    opt->scan_masking_fn(&ep, opt->cb_data)) {
				continue;
			}
			if (scan_read(ctx, ep.bdf, PCIE_CONF_ID) == PCIE_ID_NONE) {
				continue;
			}
			if (!scan_bus(ctx, bus)) {
				break;
			}
		}
	} else {
		scan_bus(ctx, opt->bus);
	}
	return ctx->status;
}

static bool legacy_scan_cb(const struct pcie_endpoint *ep, pcie_id_t id, void *cb_data)
{
	const struct scan_context *ctx = cb_data;
	const struct pcie_scan_opt *opt = ctx->legacy_opt;

	return opt->cb(ep->bdf, id, opt->cb_data);
}

static bool legacy_scan_mask(const struct pcie_endpoint *ep, void *cb_data)
{
	const struct scan_context *ctx = cb_data;
	const struct pcie_scan_opt *opt = ctx->legacy_opt;

	return opt->scan_masking_fn != NULL && opt->scan_masking_fn(ep->bdf);
}

int pcie_scan(const struct pcie_scan_opt *opt)
{
	const struct pcie_endpoint ep = z_pcie_legacy_endpoint(0U);
	struct scan_context ctx = {.ctrl = ep.ctrl, .legacy_opt = opt};

	if (opt == NULL || opt->cb == NULL) {
		return -EINVAL;
	}
	const struct pcie_scan_ctrl_opt ctrl_opt = {
		.bus = opt->bus,
		.cb = legacy_scan_cb,
		.cb_data = &ctx,
		.flags = opt->flags,
		.scan_masking_fn = legacy_scan_mask,
	};

	ctx.opt = &ctrl_opt;
	return scan(&ctx);
}

int pcie_scan_ctrl(const struct device *ctrl, const struct pcie_scan_ctrl_opt *opt)
{
	struct scan_context ctx = {.ctrl = ctrl, .opt = opt};

	if (ctrl == NULL || opt == NULL || opt->cb == NULL) {
		return -EINVAL;
	}
	if (!IS_ENABLED(CONFIG_PCIE_CONTROLLER)) {
		return -ENOTSUP;
	}
	return scan(&ctx);
}

static bool pcie_dev_match(const struct pcie_endpoint *ep, pcie_id_t id, void *cb_data)
{
	struct scan_data *data = cb_data;

	STRUCT_SECTION_FOREACH(pcie_dev, dev) {
		if (dev->bdf != PCIE_BDF_NONE || dev->ctrl != ep->ctrl) {
			continue;
		}

		if (dev->id != id) {
			continue;
		}

		uint32_t class_rev;

		if (ep->ctrl != NULL) {
			data->status = pcie_endpoint_conf_read(ep, PCIE_CONF_CLASSREV, &class_rev);
			if (data->status != 0) {
				return false;
			}
		} else {
			class_rev = pcie_conf_read(ep->bdf, PCIE_CONF_CLASSREV);
		}

		if (dev->class_rev == (class_rev & dev->class_rev_mask)) {
			dev->bdf = ep->bdf;
			dev->class_rev = class_rev;
			data->found++;
			break;
		}
	}

	/* Continue if we've not yet found all devices */
	return (data->found != data->max_dev);
}

#ifndef CONFIG_PCIE_CONTROLLER
static bool pcie_dev_cb(pcie_bdf_t bdf, pcie_id_t id, void *cb_data)
{
	const struct pcie_endpoint ep = {.bdf = bdf};

	return pcie_dev_match(&ep, id, cb_data);
}
#endif

#define PCIE_SCAN_EXTRA_BUS(node_id, prop, idx)                                \
	if (data.found != data.max_dev) {                                          \
		opt.bus = DT_PROP_BY_IDX(node_id, prop, idx);                     \
		pcie_scan(&opt);                                                   \
	}

static int pcie_init(void)
{
	struct scan_data data = {0};
#ifdef CONFIG_PCIE_CONTROLLER
	int status = 0;
	struct pcie_scan_ctrl_opt opt = {
		.cb = pcie_dev_match,
#else
	struct pcie_scan_opt opt = {
		.cb = pcie_dev_cb,
#endif
		.cb_data = &data,
		.flags = PCIE_SCAN_RECURSIVE,
	};

#ifdef CONFIG_PCIE_PRT
	const char *hid, *uid = ACPI_DT_UID(DT_DRV_INST(0));
	int ret;

	BUILD_ASSERT(ACPI_DT_HAS_HID(DT_DRV_INST(0)),
		 "No HID property for PCIe devicetree node");
	hid = ACPI_DT_HID(DT_DRV_INST(0));

	ret = acpi_legacy_irq_init(hid, uid);
	if (!ret) {
		prt_en = true;
	} else {
		__ASSERT(ret == -ENOENT, "Error retrieve interrupt routing table!");
	}
#endif

	STRUCT_SECTION_COUNT(pcie_dev, &data.max_dev);
	/* Don't bother calling pcie_scan() if there are no devices to look for */
	if (data.max_dev == 0) {
		return 0;
	}

	data.found = 0;

#ifdef CONFIG_PCIE_CONTROLLER
	STRUCT_SECTION_FOREACH(pcie_dev, dev) {
		bool scanned = false;
		int ret;

		STRUCT_SECTION_FOREACH(pcie_dev, prior) {
			if (prior == dev) {
				break;
			}
			if (prior->ctrl == dev->ctrl) {
				scanned = true;
				break;
			}
		}
		if (scanned) {
			continue;
		}
		if (dev->ctrl == NULL || !DEVICE_API_IS(pcie_ctrl, dev->ctrl)) {
			ret = -EINVAL;
		} else if (!device_is_ready(dev->ctrl)) {
			ret = -ENODEV;
		} else {
			data.status = 0;
			ret = pcie_scan_ctrl(dev->ctrl, &opt);
			if (ret == 0) {
				ret = data.status;
			}
		}
		if (status == 0) {
			status = ret;
		}
	}
	return status;
#else
	/* Scan default bus 0 */
	pcie_scan(&opt);

	/* Scan additional buses defined in devicetree scan-buses property */
#if DT_INST_NODE_HAS_PROP(0, scan_buses)
	DT_INST_FOREACH_PROP_ELEM(0, scan_buses, PCIE_SCAN_EXTRA_BUS)
#endif

#endif

	return 0;
}


/*
 * If a pcie controller is employed, pcie_scan() depends on it for working.
 * Thus, pcie must be bumped to the next level
 */
#ifdef CONFIG_PCIE_CONTROLLER
#define PCIE_SYS_INIT_LEVEL	PRE_KERNEL_2
#else
#define PCIE_SYS_INIT_LEVEL	PRE_KERNEL_1
#endif

SYS_INIT(pcie_init, PCIE_SYS_INIT_LEVEL, CONFIG_PCIE_INIT_PRIORITY);
