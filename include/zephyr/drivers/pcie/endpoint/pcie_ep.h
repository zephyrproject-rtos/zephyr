/**
 * @file
 *
 * @brief Public APIs for the PCIe EP drivers.
 */

/*
 * SPDX-License-Identifier: Apache-2.0
 *
 * Copyright 2020 Broadcom
 *
 */
#ifndef ZEPHYR_INCLUDE_DRIVERS_PCIE_EP_H_
#define ZEPHYR_INCLUDE_DRIVERS_PCIE_EP_H_

#include <zephyr/device.h>
#include <zephyr/init.h>
#include <zephyr/kernel.h>
#include <stdint.h>

enum pcie_ob_mem_type {
	PCIE_OB_ANYMEM,  /**< PCIe OB window within any address range */
	PCIE_OB_LOWMEM,  /**< PCIe OB window within 32-bit address range */
	PCIE_OB_HIGHMEM, /**< PCIe OB window above 32-bit address range */
};

enum pci_ep_irq_type {
	PCIE_EP_IRQ_LEGACY,	/**< Raise Legacy interrupt */
	PCIE_EP_IRQ_MSI,	/**< Raise MSI interrupt */
	PCIE_EP_IRQ_MSIX,	/**< Raise MSIX interrupt */
};

enum xfer_direction {
	HOST_TO_DEVICE,		/**< Read from Host */
	DEVICE_TO_HOST,		/**< Write to Host */
};

enum pcie_reset {
	PCIE_PERST,	/**< Cold reset */
	PCIE_PERST_INB,	/**< Inband hot reset */
	PCIE_FLR,	/**< Functional Level Reset */
	PCIE_RESET_MAX
};

/**
 * @typedef pcie_ep_reset_callback_t
 * @brief Callback API for PCIe reset interrupts
 *
 * These callbacks execute in interrupt context. Therefore, use only
 * interrupt-safe APIS. Registration of callbacks is done via
 * @a pcie_ep_register_reset_cb
 *
 * @param arg Pointer provided at registration time, later to be
 *	  passed back as argument to callback function
 */

typedef void (*pcie_ep_reset_callback_t)(void *arg);

struct pcie_ep_driver_api {
	int (*conf_read)(const struct device *dev, uint32_t offset,
			 uint32_t *data);
	void (*conf_write)(const struct device *dev, uint32_t offset,
			   uint32_t data);
	int (*map_addr)(const struct device *dev, uint64_t pcie_addr,
			uint64_t *mapped_addr, uint32_t size,
			enum pcie_ob_mem_type ob_mem_type);
	void (*unmap_addr)(const struct device *dev, uint64_t mapped_addr);
	int (*raise_irq)(const struct device *dev,
			 enum pci_ep_irq_type irq_type,
			 uint32_t irq_num);
	int (*register_reset_cb)(const struct device *dev,
				 enum pcie_reset reset,
				 pcie_ep_reset_callback_t cb, void *arg);
	int (*dma_xfer)(const struct device *dev, uint64_t mapped_addr,
			uintptr_t local_addr, uint32_t size,
			enum xfer_direction dir);
};

/**
 * @brief Read PCIe EP configuration space
 *
 * @details This API reads EP's own configuration space
 *
 * @param dev    Pointer to the device structure for the driver instance
 * @param offset Offset within configuration space
 * @param data   Pointer to data read from the offset
 *
 * @return 0 if successful, negative errno code if failure.
 */

static inline int pcie_ep_conf_read(const struct device *dev,
				    uint32_t offset, uint32_t *data)
{
	const struct pcie_ep_driver_api *api =
			(const struct pcie_ep_driver_api *)dev->api;

	return api->conf_read(dev, offset, data);
}

/**
 * @brief Write PCIe EP configuration space
 *
 * @details This API writes EP's own configuration space
 *
 * @param dev    Pointer to the device structure for the driver instance
 * @param offset Offset within configuration space
 * @param data   Data to be written at the offset
 */

static inline void pcie_ep_conf_write(const struct device *dev,
				      uint32_t offset, uint32_t data)
{
	const struct pcie_ep_driver_api *api =
			(const struct pcie_ep_driver_api *)dev->api;

	api->conf_write(dev, offset, data);
}

/**
 * @brief Map a host memory buffer to PCIe outbound region
 *
 * @details This API maps a host memory buffer to PCIe outbound region,
 *	    It is left to EP driver to manage multiple mappings through
 *	    multiple PCIe outbound regions if supported by SoC
 *
 * @param dev         Pointer to the device structure for the driver instance
 * @param pcie_addr   Host memory buffer address to be mapped
 * @param mapped_addr Mapped PCIe outbound region address
 * @param size        Host memory buffer size (bytes)
 * @param ob_mem_type Hint if lowmem/highmem outbound region has to be used,
 *		      this is useful in cases where bus master cannot generate
 *		      more than 32-bit address; it becomes essential to use
 *		      lowmem outbound region
 *
 * @return Mapped size : If mapped size is less than requested size,
 *	   then requestor has to call the same API again to map
 *	   the unmapped host buffer after data transfer is done with
 *	   mapped size. This situation may arise because of the
 *	   mapping alignment requirements.
 *
 * @return Negative errno code if failure.
 */

static inline int pcie_ep_map_addr(const struct device *dev,
				   uint64_t pcie_addr,
				   uint64_t *mapped_addr, uint32_t size,
				   enum pcie_ob_mem_type ob_mem_type)
{
	const struct pcie_ep_driver_api *api =
			(const struct pcie_ep_driver_api *)dev->api;

	return api->map_addr(dev, pcie_addr, mapped_addr, size, ob_mem_type);
}

/**
 * @brief Remove mapping to PCIe outbound region
 *
 * @details This API removes mapping to PCIe outbound region.
 *	    Mapped PCIe outbound region address is given as argument
 *	    to figure out the outbound region to be unmapped
 *
 * @param dev         Pointer to the device structure for the driver instance
 * @param mapped_addr PCIe outbound region address
 */

static inline void pcie_ep_unmap_addr(const struct device *dev,
				      uint64_t mapped_addr)
{
	const struct pcie_ep_driver_api *api =
			(const struct pcie_ep_driver_api *)dev->api;

	api->unmap_addr(dev, mapped_addr);
}

/**
 * @brief Raise interrupt to Host
 *
 * @details This API raises interrupt to Host
 *
 * @param   dev      Pointer to the device structure for the driver instance
 * @param   irq_type Type of Interrupt be raised (legacy, MSI or MSI-X)
 * @param   irq_num  MSI or MSI-X interrupt number
 *
 * @return 0 if successful, negative errno code if failure.
 */

static inline int pcie_ep_raise_irq(const struct device *dev,
				    enum pci_ep_irq_type irq_type,
				    uint32_t irq_num)
{
	const struct pcie_ep_driver_api *api =
			(const struct pcie_ep_driver_api *)dev->api;
	return api->raise_irq(dev, irq_type, irq_num);
}

/**
 * @brief Register callback function for reset interrupts
 *
 * @details If reset interrupts are handled by device, this API can be
 *	    used to register callback function, which will be
 *	    executed part of corresponding PCIe reset handler
 *
 * @param   dev   Pointer to the device structure for the driver instance
 * @param   reset Reset interrupt type
 * @param   cb    Callback function being registered
 * @param   arg   Argument to be passed back to callback function
 *
 * @return 0 if successful, negative errno code if failure.
 */

static inline int pcie_ep_register_reset_cb(const struct device *dev,
					    enum pcie_reset reset,
					    pcie_ep_reset_callback_t cb,
					    void *arg)
{
	const struct pcie_ep_driver_api *api =
			(const struct pcie_ep_driver_api *)dev->api;

	if (api->register_reset_cb) {
		return api->register_reset_cb(dev, reset, cb, arg);
	}

	return -ENOTSUP;
}

/**
 * @brief Data transfer between mapped Host memory and device memory with
 *	  "System DMA". The term "System DMA" is used to clarify that we
 *	  are not talking about dedicated "PCIe DMA"; rather the one
 *	  which does not understand PCIe address directly, and
 *	  uses the mapped Host memory.
 *
 * @details If DMA controller is available in the EP device, this API can be
 *	    used to achieve data transfer between mapped Host memory,
 *	    i.e. outbound memory and EP device's local memory with DMA
 *
 * @param   dev         Pointer to the device structure for the driver instance
 * @param   mapped_addr Mapped Host memory address
 * @param   local_addr  Device memory address
 * @param   size        DMA transfer length (bytes)
 * @param   dir         Direction of DMA transfer
 *
 * @return 0 if successful, negative errno code if failure.
 */

static inline int pcie_ep_dma_xfer(const struct device *dev,
				   uint64_t mapped_addr,
				   uintptr_t local_addr, uint32_t size,
				   const enum xfer_direction dir)
{
	const struct pcie_ep_driver_api *api =
			(const struct pcie_ep_driver_api *)dev->api;

	if (api->dma_xfer) {
		return api->dma_xfer(dev, mapped_addr, local_addr, size, dir);
	}

	return -ENOTSUP;
}

/**
 * @brief Data transfer using memcpy
 *
 * @details Helper API to achieve data transfer with memcpy
 *          through PCIe outbound memory
 *
 * @param dev         Pointer to the device structure for the driver instance
 * @param pcie_addr   Host memory buffer address
 * @param local_addr  Local memory buffer address
 * @param size        Data transfer size (bytes)
 * @param ob_mem_type Hint if lowmem/highmem outbound region has to be used
 *                    (PCIE_OB_LOWMEM / PCIE_OB_HIGHMEM / PCIE_OB_ANYMEM),
 *                    should be PCIE_OB_LOWMEM if bus master cannot generate
 *                    more than 32-bit address
 * @param dir         Data transfer direction (HOST_TO_DEVICE / DEVICE_TO_HOST)
 *
 * @return 0 if successful, negative errno code if failure.
 */
int pcie_ep_xfer_data_memcpy(const struct device *dev, uint64_t pcie_addr,
			     uintptr_t *local_addr, uint32_t size,
			     enum pcie_ob_mem_type ob_mem_type,
			     enum xfer_direction dir);

/**
 * @brief Data transfer using system DMA
 *
 * @details Helper API to achieve data transfer with system DMA through PCIe
 *          outbound memory, this API is based off pcie_ep_xfer_data_memcpy,
 *          here we use "system dma" instead of memcpy
 *
 * @param dev         Pointer to the device structure for the driver instance
 * @param pcie_addr   Host memory buffer address
 * @param local_addr  Local memory buffer address
 * @param size        Data transfer size (bytes)
 * @param ob_mem_type Hint if lowmem/highmem outbound region has to be used
 *                    (PCIE_OB_LOWMEM / PCIE_OB_HIGHMEM / PCIE_OB_ANYMEM)
 * @param dir         Data transfer direction (HOST_TO_DEVICE / DEVICE_TO_HOST)
 *
 * @return 0 if successful, negative errno code if failure.
 */
int pcie_ep_xfer_data_dma(const struct device *dev, uint64_t pcie_addr,
			  uintptr_t *local_addr, uint32_t size,
			  enum pcie_ob_mem_type ob_mem_type,
			  enum xfer_direction dir);

#endif /* ZEPHYR_INCLUDE_DRIVERS_PCIE_EP_H_ */
