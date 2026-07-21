/*
 * Copyright (c) 2022 Intel Corporation
 * SPDX-License-Identifier: Apache-2.0
 *
 * Derived from FreeBSD original driver made by Jim Harris
 * with contributions from Alexander Motin, Wojciech Macek, and Warner Losh
 */

#define DT_DRV_COMPAT nvme_controller

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(nvme, CONFIG_NVME_LOG_LEVEL);

#include <errno.h>

#include <zephyr/kernel.h>

#include <soc.h>
#include <zephyr/device.h>
#include <zephyr/init.h>

#include "nvme_helpers.h"
#include "nvme.h"

static int nvme_controller_wait_for_ready(const struct device *dev,
					  const int desired_val)
{
	struct nvme_controller *nvme_ctrlr = dev->data;
	mm_reg_t regs = DEVICE_MMIO_GET(dev);
	int timeout = sys_clock_tick_get_32() +
		k_ms_to_ticks_ceil32(nvme_ctrlr->ready_timeout_in_ms);
	uint32_t delta_t = USEC_PER_MSEC;
	uint32_t csts;

	while (1) {
		csts = nvme_mmio_read_4(regs, csts);
		if (csts == NVME_GONE) {
			LOG_ERR("Controller is unreachable");
			return -EIO;
		}

		if (((csts >> NVME_CSTS_REG_RDY_SHIFT) &
		     NVME_CSTS_REG_RDY_MASK) == desired_val) {
			break;
		}

		if ((int64_t)timeout - sys_clock_tick_get_32() < 0) {
			LOG_ERR("Timeout error");
			return -EIO;
		}

		k_busy_wait(delta_t);
		delta_t = MIN((MSEC_PER_SEC * USEC_PER_MSEC), delta_t * 3 / 2);
	}

	return 0;
}

static int nvme_controller_disable(const struct device *dev)
{
	mm_reg_t regs = DEVICE_MMIO_GET(dev);
	uint32_t cc, csts;
	uint8_t enabled, ready;
	int err;

	cc = nvme_mmio_read_4(regs, cc);
	csts = nvme_mmio_read_4(regs, csts);

	ready = (csts >> NVME_CSTS_REG_RDY_SHIFT) & NVME_CSTS_REG_RDY_MASK;

	enabled = (cc >> NVME_CC_REG_EN_SHIFT) & NVME_CC_REG_EN_MASK;
	if (enabled == 0) {
		/* Wait for RDY == 0 or timeout & fail */
		if (ready == 0) {
			return 0;
		}

		return nvme_controller_wait_for_ready(dev, 0);
	}

	if (ready == 0) {
		/* EN == 1, wait for  RDY == 1 or timeout & fail */
		err = nvme_controller_wait_for_ready(dev, 1);
		if (err != 0) {
			return err;
		}
	}

	cc &= ~NVME_CC_REG_EN_MASK;
	nvme_mmio_write_4(regs, cc, cc);

	return nvme_controller_wait_for_ready(dev, 0);
}

static int nvme_controller_enable(const struct device *dev)
{
	struct nvme_controller *nvme_ctrlr = dev->data;
	mm_reg_t regs = DEVICE_MMIO_GET(dev);
	uint8_t enabled, ready;
	uint32_t cc, csts;
	int err;

	cc = nvme_mmio_read_4(regs, cc);
	csts = nvme_mmio_read_4(regs, csts);

	ready = (csts >> NVME_CSTS_REG_RDY_SHIFT) & NVME_CSTS_REG_RDY_MASK;

	enabled = (cc >> NVME_CC_REG_EN_SHIFT) & NVME_CC_REG_EN_MASK;
	if (enabled == 1) {
		if (ready == 1) {
			LOG_DBG("Already enabled");
			return 0;
		}

		return nvme_controller_wait_for_ready(dev, 1);
	}

	/* EN == 0 already wait for RDY == 0 or timeout & fail */
	err = nvme_controller_wait_for_ready(dev, 0);
	if (err != 0) {
		return err;
	}

	/* Initialization values for CC */
	cc = 0;
	cc |= 1 << NVME_CC_REG_EN_SHIFT;
	cc |= 0 << NVME_CC_REG_CSS_SHIFT;
	cc |= 0 << NVME_CC_REG_AMS_SHIFT;
	cc |= 0 << NVME_CC_REG_SHN_SHIFT;
	cc |= 6 << NVME_CC_REG_IOSQES_SHIFT; /* SQ entry size == 64 == 2^6 */
	cc |= 4 << NVME_CC_REG_IOCQES_SHIFT; /* CQ entry size == 16 == 2^4 */
	cc |= nvme_ctrlr->mps << NVME_CC_REG_MPS_SHIFT;

	nvme_mmio_write_4(regs, cc, cc);

	return nvme_controller_wait_for_ready(dev, 1);
}

static int nvme_controller_setup_admin_queues(const struct device *dev)
{
	struct nvme_controller *nvme_ctrlr = dev->data;
	mm_reg_t regs = DEVICE_MMIO_GET(dev);
	uint32_t aqa, qsize;

	nvme_cmd_qpair_reset(nvme_ctrlr->adminq);

	/* Admin queue is always id 0 */
	if (nvme_cmd_qpair_setup(nvme_ctrlr->adminq, nvme_ctrlr, 0) != 0) {
		LOG_ERR("Admin cmd qpair setup failed");
		return -EIO;
	}

	nvme_mmio_write_8(regs, asq, nvme_ctrlr->adminq->cmd_bus_addr);
	nvme_mmio_write_8(regs, acq, nvme_ctrlr->adminq->cpl_bus_addr);

	/* acqs and asqs are 0-based. */
	qsize = CONFIG_NVME_ADMIN_ENTRIES - 1;
	aqa = 0;
	aqa = (qsize & NVME_AQA_REG_ACQS_MASK) << NVME_AQA_REG_ACQS_SHIFT;
	aqa |= (qsize & NVME_AQA_REG_ASQS_MASK) << NVME_AQA_REG_ASQS_SHIFT;

	nvme_mmio_write_4(regs, aqa, aqa);

	return 0;
}

static int nvme_controller_setup_io_queues(const struct device *dev)
{
	struct nvme_controller *nvme_ctrlr = dev->data;
	struct nvme_completion_poll_status status;
	struct nvme_cmd_qpair *io_qpair;
	int cq_allocated, sq_allocated;
	int ret, idx;

	nvme_cpl_status_poll_init(&status);

	ret =  nvme_ctrlr_cmd_set_num_queues(nvme_ctrlr,
					     nvme_ctrlr->num_io_queues,
					     nvme_completion_poll_cb, &status);
	if (ret != 0) {
		return ret;
	}

	nvme_completion_poll(&status);
	if (nvme_cpl_status_is_error(&status)) {
		LOG_ERR("Could not set IO num queues to %u",
			nvme_ctrlr->num_io_queues);
		nvme_completion_print(&status.cpl);
		return -EIO;
	}

	/*
	 * Data in cdw0 is 0-based.
	 * Lower 16-bits indicate number of submission queues allocated.
	 * Upper 16-bits indicate number of completion queues allocated.
	 */
	sq_allocated = (status.cpl.cdw0 & 0xFFFF) + 1;
	cq_allocated = (status.cpl.cdw0 >> 16) + 1;

	/*
	 * Controller may allocate more queues than we requested,
	 * so use the minimum of the number requested and what was
	 * actually allocated.
	 */
	nvme_ctrlr->num_io_queues = MIN(nvme_ctrlr->num_io_queues,
					sq_allocated);
	nvme_ctrlr->num_io_queues = MIN(nvme_ctrlr->num_io_queues,
					cq_allocated);

	for (idx = 0; idx < nvme_ctrlr->num_io_queues; idx++) {
		io_qpair = &nvme_ctrlr->ioq[idx];
		if (nvme_cmd_qpair_setup(io_qpair, nvme_ctrlr, idx+1) != 0) {
			LOG_ERR("IO cmd qpair %u setup failed", idx+1);
			return -EIO;
		}

		nvme_cmd_qpair_reset(io_qpair);

		nvme_cpl_status_poll_init(&status);

		ret = nvme_ctrlr_cmd_create_io_cq(nvme_ctrlr, io_qpair,
						  nvme_completion_poll_cb,
						  &status);
		if (ret != 0) {
			return ret;
		}

		nvme_completion_poll(&status);
		if (nvme_cpl_status_is_error(&status)) {
			LOG_ERR("IO CQ creation failed");
			nvme_completion_print(&status.cpl);
			return -EIO;
		}

		nvme_cpl_status_poll_init(&status);

		ret = nvme_ctrlr_cmd_create_io_sq(nvme_ctrlr, io_qpair,
						  nvme_completion_poll_cb,
						  &status);
		if (ret != 0) {
			return ret;
		}

		nvme_completion_poll(&status);
		if (nvme_cpl_status_is_error(&status)) {
			LOG_ERR("IO CQ creation failed");
			nvme_completion_print(&status.cpl);
			return -EIO;
		}
	}

	return 0;
}

static void nvme_controller_gather_info(const struct device *dev)
{
	struct nvme_controller *nvme_ctrlr = dev->data;
	mm_reg_t regs = DEVICE_MMIO_GET(dev);

	uint32_t cap_lo, cap_hi, to, vs, pmrcap;

	nvme_ctrlr->cap_lo = cap_lo = nvme_mmio_read_4(regs, cap_lo);
	LOG_DBG("CapLo: 0x%08x: MQES %u%s%s%s%s, TO %u",
		cap_lo, NVME_CAP_LO_MQES(cap_lo),
		NVME_CAP_LO_CQR(cap_lo) ? ", CQR" : "",
		NVME_CAP_LO_AMS(cap_lo) ? ", AMS" : "",
		(NVME_CAP_LO_AMS(cap_lo) & 0x1) ? " WRRwUPC" : "",
		(NVME_CAP_LO_AMS(cap_lo) & 0x2) ? " VS" : "",
		NVME_CAP_LO_TO(cap_lo));


	nvme_ctrlr->cap_hi = cap_hi = nvme_mmio_read_4(regs, cap_hi);
	LOG_DBG("CapHi: 0x%08x: DSTRD %u%s, CSS %x%s, "
		"MPSMIN %u, MPSMAX %u%s%s", cap_hi,
		NVME_CAP_HI_DSTRD(cap_hi),
		NVME_CAP_HI_NSSRS(cap_hi) ? ", NSSRS" : "",
		NVME_CAP_HI_CSS(cap_hi),
		NVME_CAP_HI_BPS(cap_hi) ? ", BPS" : "",
		NVME_CAP_HI_MPSMIN(cap_hi),
		NVME_CAP_HI_MPSMAX(cap_hi),
		NVME_CAP_HI_PMRS(cap_hi) ? ", PMRS" : "",
		NVME_CAP_HI_CMBS(cap_hi) ? ", CMBS" : "");

	vs = nvme_mmio_read_4(regs, vs);
	LOG_DBG("Version: 0x%08x: %d.%d", vs,
		NVME_MAJOR(vs), NVME_MINOR(vs));

	if (NVME_CAP_HI_PMRS(cap_hi)) {
		pmrcap = nvme_mmio_read_4(regs, pmrcap);
		LOG_DBG("PMRCap: 0x%08x: BIR %u%s%s, PMRTU %u, "
			"PMRWBM %x, PMRTO %u%s", pmrcap,
			NVME_PMRCAP_BIR(pmrcap),
			NVME_PMRCAP_RDS(pmrcap) ? ", RDS" : "",
			NVME_PMRCAP_WDS(pmrcap) ? ", WDS" : "",
			NVME_PMRCAP_PMRTU(pmrcap),
			NVME_PMRCAP_PMRWBM(pmrcap),
			NVME_PMRCAP_PMRTO(pmrcap),
			NVME_PMRCAP_CMSS(pmrcap) ? ", CMSS" : "");
	}

	nvme_ctrlr->dstrd = NVME_CAP_HI_DSTRD(cap_hi) + 2;

	nvme_ctrlr->mps = NVME_CAP_HI_MPSMIN(cap_hi);
	nvme_ctrlr->page_size = 1 << (NVME_MPS_SHIFT + nvme_ctrlr->mps);

	LOG_DBG("MPS: %u - Page Size: %u bytes",
		nvme_ctrlr->mps, nvme_ctrlr->page_size);

	/* Get ready timeout value from controller, in units of 500ms. */
	to = NVME_CAP_LO_TO(cap_lo) + 1;
	nvme_ctrlr->ready_timeout_in_ms = to * 500;

	/* Cap transfers by the maximum addressable by
	 * page-sized PRP (4KB pages -> 2MB).
	 * ToDo: it could be less -> take the minimum.
	 */
	nvme_ctrlr->max_xfer_size = nvme_ctrlr->page_size /
		8 * nvme_ctrlr->page_size;

	LOG_DBG("Max transfer size: %u bytes", nvme_ctrlr->max_xfer_size);
}

static int nvme_controller_pcie_configure(const struct device *dev)
{
	const struct nvme_controller_config *nvme_ctrlr_cfg = dev->config;
	struct nvme_controller *nvme_ctrlr = dev->data;
	struct pcie_bar mbar_regs;
	uint8_t n_vectors;

	if (nvme_ctrlr_cfg->pcie->bdf == PCIE_BDF_NONE) {
		LOG_ERR("Controller not found");
		return -ENODEV;
	}

	LOG_DBG("Configuring NVME controller ID %x:%x at %d:%x.%d",
		PCIE_ID_TO_VEND(nvme_ctrlr_cfg->pcie->id),
		PCIE_ID_TO_DEV(nvme_ctrlr_cfg->pcie->id),
		PCIE_BDF_TO_BUS(nvme_ctrlr_cfg->pcie->bdf),
		PCIE_BDF_TO_DEV(nvme_ctrlr_cfg->pcie->bdf),
		PCIE_BDF_TO_FUNC(nvme_ctrlr_cfg->pcie->bdf));

	if (!pcie_get_mbar(nvme_ctrlr_cfg->pcie->bdf,
			   NVME_PCIE_BAR_IDX, &mbar_regs)) {
		LOG_ERR("Could not get NVME registers");
		return -EIO;
	}

	device_map(DEVICE_MMIO_RAM_PTR(dev), mbar_regs.phys_addr,
		   mbar_regs.size, K_MEM_CACHE_NONE);

	/* Allocating vectors */
	n_vectors = pcie_msi_vectors_allocate(nvme_ctrlr_cfg->pcie->bdf,
					      CONFIG_NVME_INT_PRIORITY,
					      nvme_ctrlr->vectors,
					      NVME_PCIE_MSIX_VECTORS);
	if (n_vectors == 0) {
		LOG_ERR("Could not allocate %u MSI-X vectors",
			NVME_PCIE_MSIX_VECTORS);
		return -EIO;
	}

	/* Enabling MSI-X and the vectors */
	if (!pcie_msi_enable(nvme_ctrlr_cfg->pcie->bdf,
			     nvme_ctrlr->vectors, n_vectors, 0)) {
		LOG_ERR("Could not enable MSI-X");
		return -EIO;
	}

	return 0;
}

static int nvme_controller_identify(struct nvme_controller *nvme_ctrlr)
{
	struct nvme_completion_poll_status status =
		NVME_CPL_STATUS_POLL_INIT(status);

	nvme_ctrlr_cmd_identify_controller(nvme_ctrlr,
					   nvme_completion_poll_cb, &status);
	nvme_completion_poll(&status);
	if (nvme_cpl_status_is_error(&status)) {
		LOG_ERR("Could not identify the controller");
		nvme_completion_print(&status.cpl);
		return -EIO;
	}

	nvme_controller_data_swapbytes(&nvme_ctrlr->cdata);

	/*
	 * Use MDTS to ensure our default max_xfer_size doesn't exceed what the
	 * controller supports.
	 */
	if (nvme_ctrlr->cdata.mdts > 0) {
		nvme_ctrlr->max_xfer_size =
			MIN(nvme_ctrlr->max_xfer_size,
			    1 << (nvme_ctrlr->cdata.mdts + NVME_MPS_SHIFT +
				  NVME_CAP_HI_MPSMIN(nvme_ctrlr->cap_hi)));
	}

	return 0;
}

static void nvme_controller_setup_namespaces(struct nvme_controller *nvme_ctrlr)
{
	uint32_t i;

	for (i = 0;
	     i < MIN(nvme_ctrlr->cdata.nn, CONFIG_NVME_MAX_NAMESPACES); i++) {
		struct nvme_namespace *ns = &nvme_ctrlr->ns[i];

		if (nvme_namespace_construct(ns, i+1, nvme_ctrlr) != 0) {
			break;
		}

		LOG_DBG("Namespace id %u setup and running", i);
	}
}

static int nvme_controller_init(const struct device *dev)
{
	struct nvme_controller *nvme_ctrlr = dev->data;
	int ret;

	k_mutex_init(&nvme_ctrlr->lock);

	nvme_cmd_init();

	nvme_ctrlr->dev = dev;

	ret = nvme_controller_pcie_configure(dev);
	if (ret != 0) {
		return ret;
	}

	nvme_controller_gather_info(dev);

	ret = nvme_controller_disable(dev);
	if (ret != 0) {
		LOG_ERR("Controller cannot be disabled");
		return ret;
	}

	ret = nvme_controller_setup_admin_queues(dev);
	if (ret != 0) {
		return ret;
	}

	ret = nvme_controller_enable(dev);
	if (ret != 0) {
		LOG_ERR("Controller cannot be enabled");
		return ret;
	}

	ret = nvme_controller_setup_io_queues(dev);
	if (ret != 0) {
		return ret;
	}

	ret = nvme_controller_identify(nvme_ctrlr);
	if (ret != 0) {
		return ret;
	}

	nvme_controller_setup_namespaces(nvme_ctrlr);

	return 0;
}

#define NVME_CONTROLLER_DEVICE_INIT(n)					\
	DEVICE_PCIE_INST_DECLARE(n);					\
	NVME_ADMINQ_ALLOCATE(n, CONFIG_NVME_ADMIN_ENTRIES);		\
	NVME_IOQ_ALLOCATE(n, CONFIG_NVME_IO_ENTRIES);			\
									\
	static struct nvme_controller nvme_ctrlr_data_##n = {		\
		.id = n,						\
		.num_io_queues = CONFIG_NVME_IO_QUEUES,			\
		.adminq = &admin_##n,					\
		.ioq = &io_##n,						\
	};								\
									\
	static struct nvme_controller_config nvme_ctrlr_cfg_##n =	\
	{								\
		DEVICE_PCIE_INST_INIT(n, pcie),				\
	};								\
									\
	DEVICE_DT_INST_DEFINE(n, &nvme_controller_init,			\
			      NULL, &nvme_ctrlr_data_##n,		\
			      &nvme_ctrlr_cfg_##n, POST_KERNEL,		\
			      CONFIG_KERNEL_INIT_PRIORITY_DEVICE, NULL);

DT_INST_FOREACH_STATUS_OKAY(NVME_CONTROLLER_DEVICE_INIT)
