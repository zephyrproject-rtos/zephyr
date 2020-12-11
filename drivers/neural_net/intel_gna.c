/*
 * Copyright (c) 2018 Intel Corporation
 *
 * Author: Sathish Kuttan <sathish.k.kuttan@intel.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/** @file
 * @brief Intel GNA device driver
 *
 * Device driver implementation for Intel's
 * Gaussian Mixture Model and Neural Network Accelerator (GNA)
 */

#include <kernel.h>
#include <string.h>
#include <device.h>
#include <drivers/gna.h>

#include "intel_gna.h"

#define LOG_LEVEL CONFIG_NEURAL_NET_LOG_LEVEL
#include <logging/log.h>
LOG_MODULE_REGISTER(neural_net);

#define DEV_NAME(dev) ((dev)->name)
#define DEV_CFG(dev) \
	((const struct intel_gna_config *const)(dev)->config)
#define DEV_DATA(dev) \
	((struct intel_gna_data *const)(dev)->data)

#if LOG_LEVEL >= LOG_LEVEL_DBG
static void intel_gna_regs_dump(const struct device *dev);
static void intel_gna_config_desc_dump(const struct device *dev);
#define INTEL_GNA_REGS_DUMP(dev)	intel_gna_regs_dump((dev))
#define INTEL_GNA_CONFIG_DESC_DUMP(dev)	intel_gna_config_desc_dump((dev))
#else
#define INTEL_GNA_REGS_DUMP(dev)
#define INTEL_GNA_CONFIG_DESC_DUMP(dev)
#endif

#define GNA_MODEL_VIRT_BASE_DEFAULT	0

DEVICE_DECLARE(gna);
static struct intel_gna_config_desc __aligned(GNA_PG_SIZE_IN_BYTES)
	gna_config_desc;
static struct intel_gna_page_table __aligned(GNA_PG_SIZE_IN_BYTES)
	gna_page_table[GNA_NUM_PG_TABLES_NEEDED];

static void intel_gna_interrupt_handler(const struct device *dev)
{
	struct intel_gna_data *const gna = DEV_DATA(dev);

	volatile struct intel_gna_regs *regs = gna->regs;
	struct intel_gna_pending_resp pending_resp;
	struct intel_gna_pending_req pending_req;

	/* check for generic / virtual address out of range error */
	if (regs->gnasts & (GNA_STS_VIRT_ADDR_OOR | GNA_STS_ERROR)) {
		pending_resp.response.result = GNA_RESULT_GENERIC_ERROR;
	}

	/* check for parameter out of range error */
	if (regs->gnasts & GNA_STS_PARAM_OOR) {
		pending_resp.response.result =
			GNA_RESULT_PARAM_OUT_OF_RANGE_ERROR;
	}

	/* check for output buffer full error */
	if (regs->gnasts & GNA_STS_BUFFER_FULL) {
		pending_resp.response.result =
			GNA_RESULT_OUTPUT_BUFFER_FULL_ERROR;
	}

	/* check for scoring completion out of range error */
	if (regs->gnasts & GNA_STS_SCORE_COMPL) {
		pending_resp.response.result = GNA_RESULT_INFERENCE_COMPLETE;
	}

	if (k_msgq_get(&gna->request_queue, &pending_req, K_NO_WAIT) != 0) {
		LOG_ERR("Pending request queue is empty");
	} else {
		SOC_DCACHE_INVALIDATE(pending_req.model->output,
				pending_req.output_len);
		/* copy output from the model buffer to applciation buffer */
		memcpy(pending_req.output, pending_req.model->output,
				pending_req.output_len);
		pending_resp.response.output = pending_req.output;
		pending_resp.response.output_len = pending_req.output_len;
		pending_resp.callback = pending_req.callback;

		pending_resp.response.stats.cycles_per_sec = 200000000U;
		if (regs->gnasts & GNA_STS_STATS_VALID) {
			pending_resp.response.stats.total_cycles = regs->gnaptc;
			pending_resp.response.stats.stall_cycles = regs->gnasc;
		} else {
			pending_resp.response.stats.total_cycles = 0U;
			pending_resp.response.stats.stall_cycles = 0U;
		}

		k_msgq_put(&gna->response_queue, &pending_resp, K_NO_WAIT);

		k_work_submit(&gna->gna_work);
	}

	/* clear GNA operation and disable interrupt */
	regs->gnactrl |= GNA_CTRL_INTR_DISABLE | GNA_CTRL_ABORT_CLEAR;
	gna->state = GNA_STATE_IDLE;
}

static void gna_work_handler(struct k_work *work)
{
	struct intel_gna_data *gna = (struct intel_gna_data *)work;
	struct intel_gna_pending_resp resp;

	while (k_msgq_get(&gna->response_queue, &resp, K_NO_WAIT) == 0) {
		resp.callback(&resp.response);
	}
}

static int intel_gna_setup_page_table(void *physical, size_t size,
		void *virtual)
{
	uint32_t page;
	uint32_t dir_index;
	uint32_t table_index;
	uint32_t virt_addr = (uint32_t)virtual;
	uint32_t phys_addr = (uint32_t)physical;

	LOG_DBG("physical %p size %u virtual %p", physical, size, virtual);

	if (((phys_addr + size - L2_SRAM_BASE) > L2_SRAM_SIZE) ||
			(phys_addr < L2_SRAM_BASE)) {
		LOG_ERR("model at %p of size %u exceeds L2 SRAM space",
				physical, size);
		return -EINVAL;
	}

	dir_index = GNA_VA_PG_DIR(virtual);
	table_index = GNA_VA_PG_TABLE(virtual);

	if (dir_index >= GNA_NUM_PG_TABLES_NEEDED) {
		LOG_ERR("virtual addr %p is in page dir %u (max %u)",
				virtual, dir_index,
				(uint32_t)GNA_NUM_PG_TABLES_NEEDED);
		return -EINVAL;
	}

	for (page = 0U; page < GNA_NUM_PAGES(size); page++) {
		dir_index = GNA_VA_PG_DIR(virt_addr);
		table_index = GNA_VA_PG_TABLE(virt_addr);
		gna_page_table[dir_index].entry[table_index] =
			GNA_PG_TABLE_ENTRY(phys_addr);

		LOG_DBG("di %u tb %u @ %p va %08x pa %08x ent %08x",
				dir_index, table_index,
				&gna_page_table[dir_index].entry[table_index],
				virt_addr, phys_addr,
				gna_page_table[dir_index].entry[table_index]);
		phys_addr += GNA_PG_SIZE_IN_BYTES;
		virt_addr += GNA_PG_SIZE_IN_BYTES;
	}

	return 0;
}

static int intel_gna_initialize(const struct device *dev)
{
	struct intel_gna_data *const gna = DEV_DATA(dev);
	uint32_t page_dir_entry;

	k_msgq_init(&gna->request_queue, (char *)gna->requests,
			sizeof(struct intel_gna_pending_req),
			GNA_REQUEST_QUEUE_LEN);

	k_msgq_init(&gna->response_queue, (char *)gna->responses,
			sizeof(struct intel_gna_pending_resp),
			GNA_REQUEST_QUEUE_LEN);

	k_mem_slab_init(&gna->model_slab, (char *)gna->models,
			sizeof(struct intel_gna_model), GNA_MAX_NUM_MODELS);

	k_work_init(&gna->gna_work, gna_work_handler);

	/* initialize the configuration descriptor's page directory table */
	for (int page = 0; page < GNA_CONFIG_DESC_PG_DIR_SIZE; page++) {
		page_dir_entry = (page < GNA_NUM_PG_TABLES_NEEDED) ?
			GNA_PG_DIR_ENTRY(&gna_page_table[page]) : (uint32_t)-1;
		gna_config_desc.pagedir[page] = page_dir_entry;
		LOG_DBG("%s: page %u pagetable %08x",
			DEV_NAME(dev), page, gna_config_desc.pagedir[page]);
	}
	gna_config_desc.vamaxaddr = GNA_ADDRESSABLE_MEM_SIZE;
	LOG_DBG("%s: max virtual address %08x",
			DEV_NAME(dev), gna_config_desc.vamaxaddr);

	/* flush cache */
	SOC_DCACHE_FLUSH((void *)&gna_config_desc, sizeof(gna_config_desc));

	LOG_INF("%s: initialized (max %u models & max %u pending requests)",
			DEV_NAME(dev), GNA_MAX_NUM_MODELS,
			GNA_REQUEST_QUEUE_LEN);
	LOG_INF("%s: max addressable memory %u MB",
			DEV_NAME(dev), GNA_ADDRESSABLE_MEM_SIZE >> 20);
	LOG_INF("%s: %u page table(s) at %p and %u bytes",
			DEV_NAME(dev), (uint32_t)GNA_NUM_PG_TABLES_NEEDED,
			gna_page_table, sizeof(gna_page_table));
	LOG_INF("%s: configuration descriptor at %p",
			DEV_NAME(dev), &gna_config_desc);

	/* register interrupt handler */
	IRQ_CONNECT(INTEL_GNA_IRQ_ID, INTEL_GNA_IRQ_PRIORITY,
			intel_gna_interrupt_handler, DEVICE_GET(gna), 0);
	/* enable interrupt */
	irq_enable(INTEL_GNA_IRQ_ID);

	gna->state = GNA_STATE_INITIALIZED;
	return 0;
}

static int intel_gna_configure(const struct device *dev,
			       struct gna_config *cfg)
{
	const struct intel_gna_config *const dev_cfg = DEV_CFG(dev);
	struct intel_gna_data *const gna = DEV_DATA(dev);
	volatile struct intel_gna_regs *regs = gna->regs;

	if (gna->state != GNA_STATE_INITIALIZED) {
		LOG_ERR("Configuration attempt in invalid state (%u)",
			gna->state);
		return -EINVAL;
	}

	if (cfg == NULL) {
		LOG_ERR("Config pointer is NULL");
		return -EINVAL;
	}

	dev_cfg->config = *cfg;

	regs->gnactrl |= GNA_CTRL_OPER_MODEL_XNN |
		GNA_CTRL_ERR_INTR_ENABLE | GNA_CTRL_COMPL_INTR_ENABLE;

	switch (CONFIG_INTEL_GNA_POWER_MODE) {
	case GNA_POWER_MODE_ALWAYS_ON:
		regs->gnactrl |= GNA_CTRL_PM_OVRIDE_CLK_ON |
			GNA_CTRL_PM_OVRIDE_PWR_ON;
		break;

	case GNA_POWER_MODE_CLOCK_GATED:
		regs->gnactrl |= GNA_CTRL_PM_OVRIDE_PWR_ON;
		break;

	case GNA_POWER_MODE_POWER_GATED:
	case GNA_POWER_MODE_ALWAYS_OFF:
		break;

	default:
		LOG_ERR("Invalid config CONFIG_INTEL_GNA_POWER_MODE (%u)",
				CONFIG_INTEL_GNA_POWER_MODE);
		break;
	}

	/* assign the configuration descriptor address as the base */
	regs->gnadesbase = GNA_PHYS_ADDR_TO_PAGE(&gna_config_desc);

	INTEL_GNA_CONFIG_DESC_DUMP(dev);

	LOG_INF("Device %s (version %u.%u) configured with power mode %u",
			DEV_NAME(dev), regs->gnaversion >> 1,
			(uint32_t)(regs->gnaversion & BIT(0)),
			CONFIG_INTEL_GNA_POWER_MODE);

	gna->state = GNA_STATE_IDLE;
	return 0;
}

static int intel_gna_register_model(const struct device *dev,
				    struct gna_model_info *model,
				    void **model_handle)
{
	struct intel_gna_data *const gna = DEV_DATA(dev);
	struct intel_gna_model *gna_model;
	struct gna_model_header *header;
	uint32_t ro_size, rw_size;
	void *virtual_base;
	void *ro_region;

	if ((gna->state != GNA_STATE_IDLE) &&
			(gna->state != GNA_STATE_ACTIVE)) {
		LOG_ERR("Invalid state (%u)", gna->state);
		return -EINVAL;
	}

	if ((model_handle == NULL) || (model == NULL)) {
		LOG_ERR("model and/or model_handle is NULL");
		return -EINVAL;
	}

	if ((model->header == NULL) || (model->rw_region == NULL)) {
		LOG_ERR("model header / rw_region is/are NULL");
		return -EINVAL;
	}

	/* check for 64B alignment */
	if (((uint32_t)model->rw_region & BIT_MASK(6)) ||
			((uint32_t)model->ro_region & BIT_MASK(6))) {
		LOG_ERR("rw_region / ro_region not aligned to 64B");
		return -EINVAL;
	}

	if (k_mem_slab_alloc(&gna->model_slab, (void **)&gna_model,
				K_NO_WAIT)) {
		LOG_ERR("No memory to register model");
		return -ENOMEM;
	}

	LOG_INF("model header: %p rw: %p ro: %p", model->header,
			model->rw_region, model->ro_region);

	header = model->header;
	virtual_base = (void *)GNA_MODEL_VIRT_BASE_DEFAULT;

	LOG_INF("model_size: %u rw_region_size: %u", header->model_size,
			header->rw_region_size);

	/* setup page table entries for RW region */
	if (model->rw_region && header->rw_region_size) {
		/* calculate layer descriptor size */
		rw_size = header->layer_count *
			sizeof(struct intel_gna_layer_desc);
		/* round up to page boundary */
		rw_size = GNA_PAGES_TO_BYTES(GNA_NUM_PAGES(rw_size));
		/* add the input rw_region_size to get total rw_region_size */
		rw_size += header->rw_region_size;

		intel_gna_setup_page_table(model->rw_region, rw_size,
				virtual_base);
		SOC_DCACHE_FLUSH(model->rw_region, rw_size);
	}

	if (model->ro_region == NULL) {
		ro_region = (void *)((uint32_t)model->rw_region + rw_size);
	} else {
		ro_region = model->ro_region;
	}

	ro_size = header->model_size - rw_size;

	LOG_INF("rw_region: %p (%u) ro_region: %p (%u)",
			model->rw_region, rw_size, ro_region, ro_size);

	/* setup page table entries for RO region */
	intel_gna_setup_page_table(ro_region, ro_size,
			(void *)((uint32_t)virtual_base + rw_size));

	SOC_DCACHE_FLUSH(ro_region, ro_size);
	SOC_DCACHE_FLUSH(gna_page_table, sizeof(gna_page_table));

	/* copy the model pointers */
	gna_model->model = *model;
	gna_model->vabase = virtual_base;
	gna_model->input = (void *)((uint32_t)model->rw_region +
			*(uint32_t *)((uint32_t)model->rw_region +
				header->input_ptr_offset));
	gna_model->output = (void *)((uint32_t)model->rw_region +
			*(uint32_t *)((uint32_t)model->rw_region +
				header->output_ptr_offset));
	gna_model->registered = true;

	LOG_INF("model->rw_region: %p", model->rw_region);
	LOG_INF("input offset: %u",
		*(uint32_t *)((uint32_t)model->rw_region + header->input_ptr_offset));
	LOG_INF("gna_model->input: %p", gna_model->input);
	LOG_INF("output offset: %u",
		*(uint32_t *)((uint32_t)model->rw_region +
			header->output_ptr_offset));
	LOG_INF("gna_model->output: %p", gna_model->output);
	LOG_DBG("returning model handle: %p", gna_model);
	*model_handle = (void *)gna_model;
	return 0;
}

static int intel_gna_deregister_model(const struct device *dev,
				      void *model_handle)
{
	struct intel_gna_data *const gna = DEV_DATA(dev);
	struct intel_gna_model *gna_model;

	if (model_handle == NULL) {
		LOG_ERR("model_handle is NULL");
		return -EINVAL;
	}

	gna_model = (struct intel_gna_model *)model_handle;
	gna_model->registered = false;
	k_mem_slab_free(&gna->model_slab, &model_handle);

	return 0;
}

static int intel_gna_infer(const struct device *dev,
			   struct gna_inference_req *req,
			   gna_callback callback)
{
	struct intel_gna_data *const gna = DEV_DATA(dev);
	volatile struct intel_gna_regs *regs = gna->regs;
	struct intel_gna_pending_req pending_req;
	struct gna_model_header *header;
	struct intel_gna_model *handle;
	struct gna_model_info *model;
	size_t input_size;
	int ret;

	LOG_DBG("device %p", dev);
	if (req == NULL) {
		LOG_ERR("Invalid request pointer");
		return -EINVAL;
	}

	if (callback == NULL) {
		LOG_ERR("Invalid callback function pointer");
		return -EINVAL;
	}

	handle = (struct intel_gna_model *)req->model_handle;

	if (handle->registered != true) {
		LOG_ERR("Model is not registered. Handle %p", handle);
		return -EINVAL;
	}

	if (req->input == NULL) {
		LOG_ERR("Invalid input buffer");
		return -EINVAL;
	}

	if (req->output == NULL) {
		LOG_ERR("Invalid output buffer");
		return -EINVAL;
	}

	model = &handle->model;
	header = model->header;
	input_size = header->bytes_per_input * header->num_input_nodes;

	pending_req.model = handle;
	pending_req.output = req->output;
	pending_req.output_len = header->bytes_per_output *
		header->num_output_nodes;
	pending_req.callback = callback;

	ret = k_msgq_put(&gna->request_queue, &pending_req, K_NO_WAIT);
	if (ret) {
		LOG_ERR("Unable to queue request (code %d)", ret);
		return ret;
	}

	if (gna->state != GNA_STATE_IDLE) {
		/* multiple pending requests are not yet supported */
		return -EBUSY;
	}

	/* copy input */
	memcpy(handle->input, req->input, input_size);
	SOC_DCACHE_FLUSH(handle->input, input_size);

	/* assign layer descriptor base address to configuration descriptor */
	gna_config_desc.labase = (uint32_t)handle->vabase;
	gna_config_desc.lacnt = (uint16_t)header->layer_count;
	SOC_DCACHE_FLUSH(&gna_config_desc, sizeof(gna_config_desc));

	gna->state = GNA_STATE_ACTIVE;
	regs->gnactrl = (regs->gnactrl & ~GNA_CTRL_INTR_DISABLE) |
		GNA_CTRL_ACCEL_START | GNA_CTRL_STATS_ENABLE_STALL;

	return 0;
}

#if LOG_LEVEL >= LOG_LEVEL_DBG
static void intel_gna_regs_dump(const struct device *dev)
{
	struct intel_gna_data *const gna = DEV_DATA(dev);
	volatile struct intel_gna_regs *regs = gna->regs;

	LOG_DBG("gnasts     :%08x", regs->gnasts);
	LOG_DBG("gnactrl    :%08x", regs->gnactrl);
	LOG_DBG("gnamctl    :%08x", regs->gnamctl);
	LOG_DBG("gnaptc     :%08x", regs->gnaptc);
	LOG_DBG("gnasc      :%08x", regs->gnasc);
	LOG_DBG("gnaisi     :%08x", regs->gnaisi);
	LOG_DBG("gnais_low  :%08x", regs->gnais_low);
	LOG_DBG("gnais_high :%08x", regs->gnais_high);
	LOG_DBG("gnabp_low  :%08x", regs->gnabp_low);
	LOG_DBG("gnabp_high :%08x", regs->gnabp_high);
	LOG_DBG("gnadesbase :%08x", regs->gnadesbase);
	LOG_DBG("gnaibuffs  :%08x", regs->gnaibuffs);
	LOG_DBG("ovrcfgctl  :%08x", regs->gnaibuffs);
	LOG_DBG("gnaversion :%08x", regs->gnaversion);
}

static void intel_gna_config_desc_dump(const struct device *dev)
{
	struct intel_gna_data *const gna = DEV_DATA(dev);
	volatile struct intel_gna_regs *regs = gna->regs;

	LOG_DBG("gnadesbase :%08x", regs->gnadesbase);
	LOG_DBG("labase     :%08x", gna_config_desc.labase);
	LOG_DBG("lacnt      :%u", gna_config_desc.lacnt);
}
#endif

static const struct gna_driver_api gna_driver_api = {
	.configure		= intel_gna_configure,
	.register_model		= intel_gna_register_model,
	.deregister_model	= intel_gna_deregister_model,
	.infer			= intel_gna_infer,
};

static struct intel_gna_config intel_gna_config;
static struct intel_gna_data intel_gna_driver_data = {
	.regs = (volatile struct intel_gna_regs *)INTEL_GNA_BASE_ADDR,
};

DEVICE_AND_API_INIT(gna, CONFIG_INTEL_GNA_NAME, intel_gna_initialize,
		    (void *)&intel_gna_driver_data, &intel_gna_config,
		    POST_KERNEL, CONFIG_INTEL_GNA_INIT_PRIORITY,
		    &gna_driver_api);
