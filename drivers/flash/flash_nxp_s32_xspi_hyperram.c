/*
 * Copyright 2025 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT	nxp_s32_xspi_hyperram

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(nxp_s32_xspi_hyperram, CONFIG_FLASH_LOG_LEVEL);

#include <zephyr/kernel.h>
#include <zephyr/drivers/flash.h>
#include <zephyr/sys/util.h>

#include <Xspi_Ip.h>

#include "memc_nxp_s32_xspi.h"

#define XSPI_TIMEOUT_CYCLES		0xFFFFFF
#define XSPI_ERASE_VALUE		0xFF

#define XSPI_IS_ALIGNED(addr, bits)	(((addr) & BIT_MASK(bits)) == 0)


/* Use the fixed command sets from Xspi_Ip_HyperRam.c */
extern Xspi_Ip_InstrOpType Xspi_Ip_HyperRamLutTable[XSPI_IP_HR_LUT_SIZE];

struct nxp_s32_xspi_config {
	const struct device *controller;
	struct flash_parameters flash_parameters;
#if defined(CONFIG_FLASH_PAGE_LAYOUT)
	struct flash_pages_layout layout;
#endif
	const Xspi_Ip_MemoryConfigType memory_cfg;
	Xspi_Ip_StateType *state;
};

struct nxp_s32_xspi_data {
	uint8_t instance;
	Xspi_Ip_MemoryConnectionType memory_conn_cfg;
	uint8_t read_sfdp_lut_idx;
#if defined(CONFIG_MULTITHREADING)
	struct k_sem sem;
#endif
};

static ALWAYS_INLINE Xspi_Ip_MemoryConfigType *get_memory_config(const struct device *dev)
{
	return ((Xspi_Ip_MemoryConfigType *)&((const struct nxp_s32_xspi_config *)dev->config)
			->memory_cfg);
}

static inline void nxp_s32_xspi_lock(const struct device *dev)
{
#ifdef CONFIG_MULTITHREADING
	struct nxp_s32_xspi_data *data = dev->data;

	k_sem_take(&data->sem, K_FOREVER);
#else
	ARG_UNUSED(dev);
#endif
}

static inline void nxp_s32_xspi_unlock(const struct device *dev)
{
#ifdef CONFIG_MULTITHREADING
	struct nxp_s32_xspi_data *data = dev->data;

	k_sem_give(&data->sem);
#else
	ARG_UNUSED(dev);
#endif
}

static ALWAYS_INLINE bool area_is_subregion(const struct device *dev, off_t offset, size_t size)
{
	Xspi_Ip_MemoryConfigType *memory_cfg = get_memory_config(dev);

	return ((offset >= 0) && (offset < memory_cfg->memSize) &&
		((memory_cfg->memSize - offset) >= size));
}

uint8_t nxp_s32_xspi_register_device(void)
{
	static uint8_t instance_cnt;

	return instance_cnt++;
}

/* Must be called with lock */
int nxp_s32_xspi_wait_until_ready(const struct device *dev)
{
	struct nxp_s32_xspi_data *data = dev->data;
	Xspi_Ip_StatusType status;
	uint32_t timeout = XSPI_TIMEOUT_CYCLES;
	int ret = 0;

	do {
		status = Xspi_Ip_GetJobStatus(data->instance);
		timeout--;
	} while ((status == STATUS_XSPI_IP_BUSY) && (timeout > 0));

	if (status != STATUS_XSPI_IP_SUCCESS) {
		LOG_ERR("Failed to read memory status (%d)", status);
		ret = -EIO;
	} else if (timeout == 0) {
		LOG_ERR("Timeout, memory is busy");
		ret = -ETIMEDOUT;
	}

	return ret;
}

int nxp_s32_xspi_read(const struct device *dev, off_t offset, void *dest, size_t size)
{
	const struct nxp_s32_xspi_config *config = dev->config;
	struct nxp_s32_xspi_data *data = dev->data;
	Xspi_Ip_StatusType status;
	int ret = 0;

	if (size == 0) {
		return 0;
	}

	if (!dest) {
		return -EINVAL;
	}

	if (!area_is_subregion(dev, offset, size)) {
		return -EINVAL;
	}

	if (size) {
		nxp_s32_xspi_lock(dev);

		status = Xspi_Ip_Read(data->instance,
				      (uint32_t)(offset + config->state->baseAddress),
				      (uint8_t *)dest, (uint32_t)size);
		if (status != STATUS_XSPI_IP_SUCCESS) {
			LOG_ERR("Failed to read %zu bytes at 0x%lx (%d)", size, (long)offset,
				status);
			ret = -EIO;
		}

		ret = nxp_s32_xspi_wait_until_ready(dev);
		if (ret != 0) {
			ret = -EIO;
		}

		nxp_s32_xspi_unlock(dev);
	}

	return ret;
}

int nxp_s32_xspi_write(const struct device *dev, off_t offset, const void *src, size_t size)
{
	const struct nxp_s32_xspi_config *config = dev->config;
	struct nxp_s32_xspi_data *data = dev->data;
	Xspi_Ip_MemoryConfigType *memory_cfg = get_memory_config(dev);
	Xspi_Ip_StatusType status;
	size_t max_write = (size_t)MIN(FEATURE_XSPI_TX_BUF_SIZE, memory_cfg->pageSize);
	size_t len;
	int ret = 0;

	if (!size) {
		return 0;
	}

	if (!src) {
		return -EINVAL;
	}

	if (!area_is_subregion(dev, offset, size) ||
	    (offset % config->flash_parameters.write_block_size) ||
	    (size % config->flash_parameters.write_block_size)) {
		return -EINVAL;
	}

	nxp_s32_xspi_lock(dev);

	while (size) {
		len = MIN(max_write - (offset % max_write), size);
		status = Xspi_Ip_Program(data->instance,
					 (uint32_t)(offset + config->state->baseAddress),
					 (const uint8_t *)src, (uint32_t)len);
		if (status != STATUS_XSPI_IP_SUCCESS) {
			LOG_ERR("Failed to write %zu bytes at 0x%lx (%d)", len, (long)offset,
				status);
			ret = -EIO;
			break;
		}

		ret = nxp_s32_xspi_wait_until_ready(dev);
		if (ret != 0) {
			break;
		}

		size -= len;
		src = (const uint8_t *)src + len;
		offset += len;
	}

	nxp_s32_xspi_unlock(dev);

	return ret;
}

static int nxp_s32_xspi_erase(const struct device *dev, off_t offset, size_t size)
{
	const struct nxp_s32_xspi_config *config = dev->config;
	struct nxp_s32_xspi_data *data = dev->data;
	int ret = 0;
	size_t erase_size = MIN(FEATURE_XSPI_TX_BUF_SIZE, size);
	Xspi_Ip_StatusType status;

	if (!size) {
		return 0;
	}

	if (!area_is_subregion(dev, offset, size) ||
	   (offset % erase_size) || (size % erase_size)) {
		return -EINVAL;
	}

	nxp_s32_xspi_lock(dev);

	while (size > 0) {
		status = Xspi_Ip_EraseBlock(data->instance,
					    (uint32_t)(offset + config->state->baseAddress),
					    erase_size);
		if (status != STATUS_XSPI_IP_SUCCESS) {
			LOG_ERR("Failed to erase %zu bytes at 0x%lx (%d)", erase_size, (long)offset,
				status);
			ret = -EIO;
			break;
		}

		ret = nxp_s32_xspi_wait_until_ready(dev);
		if (ret != 0) {
			ret = -EIO;
			break;
		}

		offset += erase_size;
		size -= erase_size;
	}

	nxp_s32_xspi_unlock(dev);

	return ret;
}

const struct flash_parameters *nxp_s32_xspi_get_parameters(const struct device *dev)
{
	const struct nxp_s32_xspi_config *config = dev->config;

	return &config->flash_parameters;
}

#if defined(CONFIG_FLASH_PAGE_LAYOUT)
void nxp_s32_xspi_pages_layout(const struct device *dev, const struct flash_pages_layout **layout,
			       size_t *layout_size)
{
	const struct nxp_s32_xspi_config *config = dev->config;

	*layout = &config->layout;
	*layout_size = 1;
}
#endif /* CONFIG_FLASH_PAGE_LAYOUT */

int nxp_s32_xspi_read_id(const struct device *dev, uint8_t *id)
{
	struct nxp_s32_xspi_data *data = dev->data;
	Xspi_Ip_StatusType status;
	int ret = 0;

	nxp_s32_xspi_lock(dev);

	status = Xspi_Ip_ReadId(data->instance, id);
	if (status != STATUS_XSPI_IP_SUCCESS) {
		LOG_ERR("Failed to read device ID (%d)", status);
		ret = -EIO;
	}

	nxp_s32_xspi_unlock(dev);

	return ret;
}

static int nxp_s32_xspi_init(const struct device *dev)
{
	struct nxp_s32_xspi_data *data = dev->data;
	const struct nxp_s32_xspi_config *config = dev->config;
	Xspi_Ip_MemoryConfigType *memory_cfg = get_memory_config(dev);
	uint8_t dev_id[memory_cfg->readIdSettings.readIdSize];
	Xspi_Ip_StatusType status;
	int ret = 0;

	/* Used by the HAL to retrieve the internal driver state */
	data->instance = nxp_s32_xspi_register_device();
	__ASSERT_NO_MSG(data->instance < XSPI_IP_MEM_INSTANCE_COUNT);
	data->memory_conn_cfg.xspiInstance = memc_nxp_s32_xspi_get_instance(config->controller);

#if defined(CONFIG_MULTITHREADING)
	k_sem_init(&data->sem, 1, 1);
#endif

	if (!device_is_ready(config->controller)) {
		LOG_ERR("Memory control device not ready");
		return -ENODEV;
	}

	status = Xspi_Ip_Init(data->instance, (const Xspi_Ip_MemoryConfigType *)memory_cfg,
			      (const Xspi_Ip_MemoryConnectionType *)&data->memory_conn_cfg);
	if (status != STATUS_XSPI_IP_SUCCESS) {
		LOG_ERR("Fail to init memory device %d (%d)", data->instance, status);
		return -EIO;
	}

	/* Verify connectivity by reading the device ID */
	ret = nxp_s32_xspi_read_id(dev, dev_id);
	if (ret != 0) {
		LOG_ERR("Device ID read failed (%d)", ret);
		return -ENODEV;
	}

	if (memcmp(dev_id, memory_cfg->readIdSettings.readIdExpected, sizeof(dev_id))) {
		LOG_ERR("Device id does not match config");
		return -EINVAL;
	}

	return ret;
}

static DEVICE_API(flash, nxp_s32_xspi_api) = {
	.erase = nxp_s32_xspi_erase,
	.write = nxp_s32_xspi_write,
	.read = nxp_s32_xspi_read,
	.get_parameters = nxp_s32_xspi_get_parameters,
#if defined(CONFIG_FLASH_PAGE_LAYOUT)
	.page_layout = nxp_s32_xspi_pages_layout,
#endif /* CONFIG_FLASH_PAGE_LAYOUT */
};

#define XSPI_PAGE_LAYOUT(n)							\
	.layout = {								\
		.pages_count = (DT_INST_PROP(n, size) / BITS_PER_BYTE)		\
			/ CONFIG_FLASH_NXP_S32_XSPI_LAYOUT_PAGE_SIZE,		\
		.pages_size = CONFIG_FLASH_NXP_S32_XSPI_LAYOUT_PAGE_SIZE,	\
	}

#define XSPI_READ_ID_CFG(n)							\
	{									\
		.readIdLut = XSPI_IP_HR_LUT_READ_REG,				\
		.readIdSize = DT_INST_PROP_LEN(n, jedec_id),			\
		.readIdExpected = DT_INST_PROP(n, jedec_id),			\
	}

#define XSPI_MEMORY_CONN_CFG(n)							\
	{									\
		.connectionType = (Xspi_Ip_ConnectionType)DT_INST_REG_ADDR(n),	\
		.memAlignment = DT_INST_PROP(n, write_block_size),		\
		.initDevice = TRUE,						\
	}

#define XSPI_ERASE_CFG(n)							\
	{									\
		.eraseTypes = {							\
			{							\
				.eraseLut = XSPI_IP_LUT_INVALID,		\
				.size = 0,					\
			},							\
			{							\
				.eraseLut = XSPI_IP_LUT_INVALID,		\
				.size = 0,					\
			},							\
			{							\
				.eraseLut = XSPI_IP_LUT_INVALID,		\
				.size = 0,					\
			},							\
			{							\
				.eraseLut = XSPI_IP_LUT_INVALID,		\
				.size = 0,					\
			},							\
		},								\
		.chipEraseLut = XSPI_IP_LUT_INVALID,				\
	}

#define XSPI_RESET_CFG(n)							\
	{									\
		.resetCmdLut = XSPI_IP_LUT_INVALID,				\
		.resetCmdCount = 0U,						\
	}

#define XSPI_STATUS_REG_CFG(n)							\
	{									\
		.statusRegReadLut = XSPI_IP_HR_LUT_READ_REG,			\
		.statusRegWriteLut = XSPI_IP_HR_LUT_WRITE_REG,			\
		.writeEnableLut = XSPI_IP_LUT_INVALID,				\
		.regSize = 2U,							\
		.busyMask = 1U,							\
		.busyValue = 1U,						\
		.idleValue = 0U,						\
		.clearErrLut = XSPI_IP_LUT_INVALID,				\
		.writeEnableOffset = 0U,					\
		.blockProtectionOffset = 0U,					\
		.blockProtectionWidth = 0U,					\
		.blockProtectionValue = 0U,					\
	}

#define XSPI_INIT_CFG(n)							\
	{									\
		.opCount = 0U,							\
		.operations = NULL,						\
	}

#define XSPI_LUT_CFG(n)								\
	{									\
		.opCount = XSPI_IP_HR_LUT_SIZE,					\
		.lutOps = (Xspi_Ip_InstrOpType *)Xspi_Ip_HyperRamLutTable,	\
	}

#define XSPI_SUSPEND_CFG(n)							\
	{									\
		.eraseSuspendLut = XSPI_IP_LUT_INVALID,				\
		.eraseResumeLut = XSPI_IP_LUT_INVALID,				\
		.programSuspendLut = XSPI_IP_LUT_INVALID,			\
		.programResumeLut = XSPI_IP_LUT_INVALID,			\
	}

#define XSPI_MEMORY_CFG(n)							\
	{									\
		.memType = XSPI_IP_HYPER_RAM,					\
		.hrConfig = &hyperflash_config_##n,				\
		.memSize = DT_INST_PROP(n, size) / BITS_PER_BYTE,		\
		.pageSize = DT_INST_PROP(n, max_program_buffer_size),		\
		.readLut = XSPI_IP_HR_LUT_READ,					\
		.writeLut = XSPI_IP_HR_LUT_WRITE,				\
		.readIdSettings = XSPI_READ_ID_CFG(n),				\
		.eraseSettings = XSPI_ERASE_CFG(n),				\
		.statusConfig = XSPI_STATUS_REG_CFG(n),				\
		.suspendSettings = XSPI_SUSPEND_CFG(n),				\
		.initResetSettings = XSPI_RESET_CFG(n),				\
		.optionalLuts = {XSPI_IP_LUT_INVALID, XSPI_IP_LUT_INVALID},	\
		.initConfiguration = XSPI_INIT_CFG(n),				\
		.lutSequences = XSPI_LUT_CFG(n),				\
		.initCallout = NULL,						\
		.resetCallout = NULL,						\
		.errorCheckCallout = NULL,					\
		.eccCheckCallout = NULL,					\
		.ctrlAutoCfgPtr = NULL,						\
	}

#define FLASH_NXP_S32_XSPI_MASTER_CLOCK_TYPE(n)					\
	_CONCAT(XSPI_IP_HR_MASTER_CLOCK_TYPE_,					\
		DT_INST_STRING_UPPER_TOKEN(n, master_clock_type))

#define FLASH_NXP_S32_XSPI_DRV_STRENGTH(n)							\
	(DT_INST_PROP(n, drive_strength_ohm) == 19 ? XSPI_IP_HR_DRV_STRENGTH_007 :		\
	(DT_INST_PROP(n, drive_strength_ohm) == 22 ? XSPI_IP_HR_DRV_STRENGTH_006 :		\
	(DT_INST_PROP(n, drive_strength_ohm) == 27 ? XSPI_IP_HR_DRV_STRENGTH_005 :		\
	(DT_INST_PROP(n, drive_strength_ohm) == 34 ? XSPI_IP_HR_DRV_STRENGTH_004 :		\
	(DT_INST_PROP(n, drive_strength_ohm) == 46 ? XSPI_IP_HR_DRV_STRENGTH_003 :		\
	(DT_INST_PROP(n, drive_strength_ohm) == 67 ? XSPI_IP_HR_DRV_STRENGTH_002 :		\
	(DT_INST_PROP(n, drive_strength_ohm) == 115 ? XSPI_IP_HR_DRV_STRENGTH_001 :		\
	XSPI_IP_HR_DRV_STRENGTH_000)))))))

#define FLASH_NXP_S32_XSPI_INITIAL_LATENCY(n)							\
	(DT_INST_PROP(n, initial_latency_cycles) == 5 ? XSPI_IP_HR_INITIAL_LATENCY_5_CLOCKS :	\
	(DT_INST_PROP(n, initial_latency_cycles) == 6 ? XSPI_IP_HR_INITIAL_LATENCY_6_CLOCKS :	\
	(DT_INST_PROP(n, initial_latency_cycles) == 7 ? XSPI_IP_HR_INITIAL_LATENCY_7_CLOCKS :	\
	(DT_INST_PROP(n, initial_latency_cycles) == 3 ? XSPI_IP_HR_INITIAL_LATENCY_3_CLOCKS :	\
	(DT_INST_PROP(n, initial_latency_cycles) == 4 ? XSPI_IP_HR_INITIAL_LATENCY_4_CLOCKS :	\
	XSPI_IP_HR_INITIAL_LATENCY_5_CLOCKS)))))

#define FLASH_NXP_S32_XSPI_REFRESH_INTERVAL(n)							\
	(DT_INST_ENUM_IDX(n, refresh_interval_multiplier) == 1 ? XSPI_IP_HR_ARRAY_REFRESH_001 :	\
	(DT_INST_ENUM_IDX(n, refresh_interval_multiplier) == 2 ? XSPI_IP_HR_ARRAY_REFRESH_002 :	\
	(DT_INST_ENUM_IDX(n, refresh_interval_multiplier) == 3 ? XSPI_IP_HR_ARRAY_REFRESH_003 :	\
	XSPI_IP_HR_ARRAY_REFRESH_000)))

#define FLASH_NXP_S32_XSPI_INIT_DEVICE(n)					\
	static Xspi_Ip_HyperRamConfigType hyperflash_config_##n =		\
	{									\
		.driveStrength = FLASH_NXP_S32_XSPI_DRV_STRENGTH(n),		\
		.initialLatency = FLASH_NXP_S32_XSPI_INITIAL_LATENCY(n),	\
		.masterClockType = FLASH_NXP_S32_XSPI_MASTER_CLOCK_TYPE(n),	\
		.arrayRefresh = FLASH_NXP_S32_XSPI_REFRESH_INTERVAL(n),		\
		.deviceIdWordAddress = DT_INST_PROP(n, device_id_word_addr),	\
	};									\
	static const struct nxp_s32_xspi_config nxp_s32_xspi_config_##n = {	\
		.controller = DEVICE_DT_GET(DT_INST_BUS(n)),			\
		.flash_parameters = {						\
			.write_block_size = DT_INST_PROP(n, write_block_size),	\
			.erase_value = XSPI_ERASE_VALUE,			\
		},								\
		IF_ENABLED(CONFIG_FLASH_PAGE_LAYOUT,				\
			(XSPI_PAGE_LAYOUT(n),))					\
		.memory_cfg = XSPI_MEMORY_CFG(n),				\
		.state = &Xspi_Ip_MemoryStateStructure[n],			\
	};									\
										\
	static struct nxp_s32_xspi_data nxp_s32_xspi_data_##n = {		\
		.memory_conn_cfg = XSPI_MEMORY_CONN_CFG(n),			\
	};									\
										\
	DEVICE_DT_INST_DEFINE(n,						\
			      nxp_s32_xspi_init,				\
			      NULL,						\
			      &nxp_s32_xspi_data_##n,				\
			      &nxp_s32_xspi_config_##n,				\
			      POST_KERNEL,					\
			      CONFIG_FLASH_INIT_PRIORITY,			\
			      &nxp_s32_xspi_api);

DT_INST_FOREACH_STATUS_OKAY(FLASH_NXP_S32_XSPI_INIT_DEVICE)
