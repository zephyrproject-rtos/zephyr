/*
 * Copyright (c) 2024 Microchip Technology Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_ESPI_ESPI_MCHP_MEC5_H_
#define ZEPHYR_INCLUDE_DRIVERS_ESPI_ESPI_MCHP_MEC5_H_

#include <errno.h>
#include <zephyr/device.h>
#include <zephyr/sys/slist.h>

enum mec5_espi_sram_bar_id {
	MEC5_ESPI_SRAM_BAR_0 = 0,
	MEC5_ESPI_SRAM_BAR_1,
};

enum mec5_espi_sram_region_size {
	MEC5_ESPI_SRAM_REGION_SIZE_1B = 0,
	MEC5_ESPI_SRAM_REGION_SIZE_2B,
	MEC5_ESPI_SRAM_REGION_SIZE_4B,
	MEC5_ESPI_SRAM_REGION_SIZE_8B,
	MEC5_ESPI_SRAM_REGION_SIZE_16B,
	MEC5_ESPI_SRAM_REGION_SIZE_32B,
	MEC5_ESPI_SRAM_REGION_SIZE_64B,
	MEC5_ESPI_SRAM_REGION_SIZE_128B,
	MEC5_ESPI_SRAM_REGION_SIZE_256B,
	MEC5_ESPI_SRAM_REGION_SIZE_512B,
	MEC5_ESPI_SRAM_REGION_SIZE_1KB,
	MEC5_ESPI_SRAM_REGION_SIZE_2KB,
	MEC5_ESPI_SRAM_REGION_SIZE_4KB,
	MEC5_ESPI_SRAM_REGION_SIZE_8KB,
	MEC5_ESPI_SRAM_REGION_SIZE_16KB,
	MEC5_ESPI_SRAM_REGION_SIZE_32KB,
};

enum mec5_espi_sram_access {
	MEC5_ESPI_SRAM_ACCESS_NONE = 0,
	MEC5_ESPI_SRAM_ACCESS_RO,
	MEC5_ESPI_SRAM_ACCESS_WO,
	MEC5_ESPI_SRAM_ACCESS_RW,
};

struct mec5_espi_sram_bar_cfg {
	void *buf;
	uint8_t szp2;
	uint8_t access;
};

/**
 * @brief Configure Microchip MEC5 eSPI SRAM BAR
 *
 * Configure the specified Microchip MEC5 eSPI SRAM BAR to map a
 * region of EC memory to the eSPI Host's memory address space
 * and set the access mode.
 *
 * @param dev Pointer to the device structure for the driver instance.
 * @param cfg Pointer to the MEC5 eSPI SRAM BAR config structure.
 *
 * @retval 0 If successful.
 * @retval -EWOULDBLOCK if function is called from an ISR.
 * @retval -ERANGE if pin number is out of range.
 * @retval -EIO if fails.
 */
int mec5_espi_sram_bar_configure(const struct device *dev, uint8_t bar_id,
				 struct mec5_espi_sram_bar_cfg *cfg);

/* ---- Generic Host Device API ---- */
#define ESPI_HAE_CFG_LDN_POS	0
#define ESPI_HAE_CFG_LDN_MSK0	0xffu
#define ESPI_HAE_CFG_LDN_MSK	((ESPI_HAE_CFG_LDN_MSK0) << ESPI_HAE_CFG_LDN_POS)

#define ESPI_HAE_CFG_GET_LDN(h)	\
	(((uint32_t)(h) >> ESPI_HAE_CFG_LDN_POS) & ESPI_HAE_CFG_LDN_MSK0)

#define ESPI_HAE_CFG_SET_LDN(ldn)	\
	(((uint32_t)(ldn) & ESPI_HAE_CFG_LDN_MSK0) << ESPI_HAE_CFG_LDN_POS)

/* eSPI peripheral channel host facing devices have different API's with
 * the requirement the first API is "host_access_enable". We can cast
 * each driver's API struct to this one allowing the eSPI driver to invoke
 * host_access_enable after the Host eSPI controller de-asserts PLTRST#.
 */
struct espi_pc_driver_api {
	int (*host_access_enable)(const struct device *dev, uint8_t enable, uint32_t cfg);
	int (*intr_enable)(const struct device *dev, uint8_t enable);
};

static inline int espi_pc_host_access(const struct device *dev, uint8_t en, uint32_t cfg)
{
	const struct espi_pc_driver_api *api = (const struct espi_pc_driver_api *)dev->api;

	if (!api->host_access_enable) {
		return -ENOTSUP;
	}

	return api->host_access_enable(dev, en, cfg);
}

/* !!! TODO - Add uint32_t flags parameter with defines for all enable and all disable */
static inline int espi_pc_intr_enable(const struct device *dev, uint8_t en)
{
	const struct espi_pc_driver_api *api = (const struct espi_pc_driver_api *)dev->api;

	if (!api->intr_enable) {
		return -ENOTSUP;
	}

	return api->intr_enable(dev, en);
}

/* ---- Host Device BDP ---- */
enum mec5_bdp_event {
	MEC5_BDP_EVENT_NONE = 0,
	MEC5_BDP_EVENT_IO,
	MEC5_BDP_EVENT_OVERRUN,
};

struct host_io_data {
	uint32_t data;
	uint8_t start_byte_lane;
	uint8_t size;
};

typedef void (*mchp_espi_pc_bdp_callback_t)(const struct device *dev,
					    struct host_io_data *hiod,
					    void *user_data);

struct mchp_espi_pc_bdp_driver_api {
	int (*host_access_enable)(const struct device *dev, uint8_t enable, uint32_t cfg);
	int (*intr_enable)(const struct device *dev, int intr_en);
	int (*has_data)(const struct device *dev);
	int (*get_data)(const struct device *dev, struct host_io_data *data);
#ifdef CONFIG_ESPI_MEC5_BDP_CALLBACK
	int (*set_callback)(const struct device *dev, mchp_espi_pc_bdp_callback_t cb,
			    void *user_data);
#endif
};

static inline int mchp_espi_ec_bdp_has_data(const struct device *dev)
{
	const struct mchp_espi_pc_bdp_driver_api *api =
		(const struct mchp_espi_pc_bdp_driver_api *)dev->api;

	if (!api->has_data) {
		return -ENOTSUP;
	}

	return api->has_data(dev);
}

static inline int mchp_espi_ec_bdp_get_data(const struct device *dev, struct host_io_data *hiod)
{
	const struct mchp_espi_pc_bdp_driver_api *api =
		(const struct mchp_espi_pc_bdp_driver_api *)dev->api;

	if (!api->get_data) {
		return -ENOTSUP;
	}

	return api->get_data(dev, hiod);
}

#ifdef CONFIG_ESPI_MEC5_BDP_CALLBACK
static inline int mchp_espi_pc_bdp_set_callback(const struct device *dev,
						mchp_espi_pc_bdp_callback_t callback,
						void *user_data)
{
	const struct mchp_espi_pc_bdp_driver_api *api =
		(const struct mchp_espi_pc_bdp_driver_api *)dev->api;

	if (!api->set_callback) {
		return -ENOTSUP;
	}

	return api->set_callback(dev, callback, user_data);
}
#endif

/* Legacy 8042 Keyboard controller on eSPI peripheral channel */

#define ESPI_MCHP_LPC_REQ_FLAG_DIR_POS 0
#define ESPI_MCHP_LPC_REQ_FLAG_RD 0
#define ESPI_MCHP_LPC_REQ_FLAG_WR BIT(ESPI_MCHP_LPC_REQ_FLAG_DIR_POS)

struct mchp_espi_pc_kbc_driver_api {
	int (*host_access_enable)(const struct device *dev, uint8_t enable, uint32_t cfg);
	int (*intr_enable)(const struct device *dev, uint8_t enable);
	int (*lpc_request)(const struct device *dev, enum lpc_peripheral_opcode op,
			   uint32_t *data, uint32_t flags);
};

static inline int mchp_espi_pc_kbc_lpc_request(const struct device *dev,
					       enum lpc_peripheral_opcode op,
					       uint32_t *data, uint32_t flags)
{
	const struct mchp_espi_pc_kbc_driver_api *api =
		(const struct mchp_espi_pc_kbc_driver_api *)dev->api;

	if (!api->lpc_request) {
		return -ENOTSUP;
	}

	return api->lpc_request(dev, op, data, flags);
}

/* -------- OS ACPI EC -------- */
struct mchp_espi_pc_aec_driver_api {
	int (*host_access_enable)(const struct device *dev, uint8_t enable, uint32_t cfg);
	int (*intr_enable)(const struct device *dev, uint8_t enable);
	int (*lpc_request)(const struct device *dev, enum lpc_peripheral_opcode op,
			   uint32_t *data, uint32_t flags);
};

static inline int mchp_espi_pc_aec_lpc_request(const struct device *dev,
					       enum lpc_peripheral_opcode op,
					       uint32_t *data, uint32_t flags)
{
	const struct mchp_espi_pc_aec_driver_api *api =
		(const struct mchp_espi_pc_aec_driver_api *)dev->api;

	if (!api->lpc_request) {
		return -ENOTSUP;
	}

	return api->lpc_request(dev, op, data, flags);
}

/* -------- Host Command ACPI EC -------- */


/* -------- EMI -------- */
enum mchp_emi_region_id {
	MCHP_EMI_MR_0 = 0,
	MCHP_EMI_MR_1,
};

enum mchp_emi_opcode {
	MCHP_EMI_OPC_MR_DIS = 0,	/* disable a previously configured memory window */
	MCHP_EMI_OPC_MR_EN,		/* re-enable a previously configured memory window */
	MCHP_EMI_OPC_MBOX_EC_IRQ_DIS,	/* disable EMI Host-to-EC mailbox interrupt */
	MCHP_EMI_OPC_MBOX_EC_IRQ_EN,	/* enable EMI Host-to-EC mailbox interrupt */
};

/* @brief MCHP EMI memory region description. Address 4-byte aligned. Sizes multiple of 4 bytes */
struct mchp_emi_mem_region {
	void *memptr;
	uint16_t rdsz;
	uint16_t wrsz;
};

typedef void (*mchp_espi_pc_emi_callback_t)(const struct device *dev,
					    uint32_t emi_mbox_data, void *data);

struct mchp_espi_pc_emi_driver_api {
	int (*host_access_enable)(const struct device *dev, uint8_t enable, uint32_t cfg);
	int (*intr_enable)(const struct device *dev, uint8_t enable);
	int (*configure_mem_region)(const struct device *dev, struct mchp_emi_mem_region *mr,
				    uint8_t region_id);
	int (*set_callback)(const struct device *dev, mchp_espi_pc_emi_callback_t callback,
			    void *user_data);
	int (*request)(const struct device *dev, enum mchp_emi_opcode op, uint32_t *data);
};

static inline int mchp_espi_pc_emi_config_mem_region(const struct device *dev,
						     struct mchp_emi_mem_region *mr,
						     int region_id)
{
	const struct mchp_espi_pc_emi_driver_api *api =
		(const struct mchp_espi_pc_emi_driver_api *)dev->api;

	if (!api->configure_mem_region) {
		return -ENOTSUP;
	}

	return api->configure_mem_region(dev, mr, region_id);
}

static inline int mchp_espi_pc_emi_set_callback(const struct device *dev,
						mchp_espi_pc_emi_callback_t callback,
						void *user_data)
{
	const struct mchp_espi_pc_emi_driver_api *api =
		(const struct mchp_espi_pc_emi_driver_api *)dev->api;

	if (!api->set_callback) {
		return -ENOTSUP;
	}

	return api->set_callback(dev, callback, user_data);
}

static inline int mchp_espi_pc_emi_request(const struct device *dev,
					   enum mchp_emi_opcode op, uint32_t *data)
{
	const struct mchp_espi_pc_emi_driver_api *api =
		(const struct mchp_espi_pc_emi_driver_api *)dev->api;

	if (!api->request) {
		return -ENOTSUP;
	}

	return api->request(dev, op, data);
}

/* -------- Mailbox -------- */

struct mchp_espi_pc_mbox_driver_api {
	int (*host_access_enable)(const struct device *dev, uint8_t enable, uint32_t cfg);
	int (*intr_enable)(const struct device *dev, uint8_t enable);
#ifdef ESPI_MEC5_MAILBOX_CALLBACK
	int (*set_callback)(const struct device *dev, mchp_espi_pc_mbox_callback_t callback,
			    void *user_data)
#endif
};

#ifdef ESPI_MEC5_MAILBOX_CALLBACK

struct mchp_mbox_data {
	uint8_t id;
	uint8_t data;
};

typedef void (*mchp_espi_pc_mbox_callback_t)(const struct device *dev,
					     uint32_t mbox_info, void *data);

static inline int mchp_espi_pc_mbox_set_callback(const struct device *dev,
						 mchp_espi_pc_mbox_callback_t callback,
						 void *user_data)
{
	const struct mchp_espi_pc_mbox_driver_api *api =
		(const struct mchp_espi_pc_mbox_driver_api *)dev->api;

	if (!api->set_callback) {
		return -ENOTSUP;
	}

	return api->set_callback(dev, callback, user_data);
}
#endif

#endif /* ZEPHYR_INCLUDE_DRIVERS_ESPI_ESPI_MCHP_MEC5_H_ */
