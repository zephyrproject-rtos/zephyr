/*
 * SPDX-FileCopyrightText: Copyright (c) 2026 Process Mission
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 *
 * @brief Process-local test-host interface for the emulated PCIe EP.
 *
 * The functions declared here exist only for the zephyr,pcie-ep-emul
 * driver. They model host-side resources (memory apertures, interrupt
 * event consumption, reset injection, host configuration/BAR access)
 * inside the same process as the emulated endpoint and are intended for
 * tests on POSIX/native targets. They are not part of the generic PCIe
 * EP driver API and have no meaning for real endpoint hardware.
 *
 * Note on the driver's pcie_ep_driver_api dma_xfer implementation: it
 * blocks the caller until the DMA completion callback runs on the
 * controller's workqueue, so it may only be called from thread context,
 * and never from a DMA completion callback or the dma_emul workqueue
 * thread itself — there the callback it waits for could never run and
 * the call would deadlock.
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_PCIE_ENDPOINT_PCIE_EP_EMUL_H_
#define ZEPHYR_INCLUDE_DRIVERS_PCIE_ENDPOINT_PCIE_EP_EMUL_H_

#include <stdint.h>

#include <zephyr/device.h>
#include <zephyr/drivers/pcie/endpoint/pcie_ep.h>
#include <zephyr/kernel.h>

/**
 * @brief PCIe EP Emulator Test-Host Interface
 * @defgroup pcie_ep_emul_interface PCIe EP Emulator Test-Host Interface
 * @ingroup io_emulators
 * @ingroup pcie_interface
 * @{
 */

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Interrupt event recorded by an emulated endpoint
 *
 * One event is recorded per successful pcie_ep_raise_irq() call and
 * consumed by the test host through pcie_ep_emul_wait_irq_event().
 * All fields are process-local test-host state, not wire data.
 */
struct pcie_ep_emul_irq_event {
	/** Interrupt type passed to pcie_ep_raise_irq(). */
	enum pci_ep_irq_type type;
	/** MSI/MSI-X vector passed to pcie_ep_raise_irq(), zero for legacy. */
	uint32_t vector;
};

/**
 * @brief Register a host memory aperture for an emulated endpoint
 *
 * Models a chunk of host memory: the PCIe address range
 * [pcie_base, pcie_base + len) becomes backed by the process-local buffer
 * @a local_buf. pcie_ep_map_addr() only succeeds for ranges wholly inside
 * a registered aperture. The caller retains ownership of @a local_buf,
 * which must stay valid until the aperture is unregistered.
 *
 * @param dev       Emulated endpoint device
 * @param pcie_base PCIe (host) base address of the aperture
 * @param local_buf Process-local backing buffer
 * @param len       Aperture length in bytes
 *
 * @retval 0 on success
 * @retval -EINVAL on NULL buffer, zero length or address overflow
 * @retval -EALREADY if the range overlaps a registered aperture
 * @retval -ENOMEM if the per-instance aperture table is full
 */
int pcie_ep_emul_register_aperture(const struct device *dev, uint64_t pcie_base, void *local_buf,
				   uint32_t len);

/**
 * @brief Unregister a host memory aperture
 *
 * Removes the aperture starting at @a pcie_base. Apertures with mappings
 * still in use (pcie_ep_map_addr() without matching pcie_ep_unmap_addr())
 * cannot be unregistered.
 *
 * @param dev       Emulated endpoint device
 * @param pcie_base PCIe (host) base address given at registration
 *
 * @retval 0 on success
 * @retval -ENOENT if no registered aperture starts at pcie_base
 * @retval -EBUSY if mappings inside the aperture are still active
 */
int pcie_ep_emul_unregister_aperture(const struct device *dev, uint64_t pcie_base);

/**
 * @brief Consume the oldest queued interrupt event (test-host API)
 *
 * Waits up to @a timeout for an interrupt event recorded by a successful
 * pcie_ep_raise_irq() on @a dev, then removes and returns it. Events are
 * consumed in raise order. Pass K_NO_WAIT for a non-blocking poll.
 *
 * @param dev     Emulated endpoint device
 * @param event   Event destination
 * @param timeout Maximum time to wait for an event
 *
 * @retval 0 on success
 * @retval -EINVAL on NULL @a event
 * @retval -EAGAIN if no event arrived within @a timeout
 */
int pcie_ep_emul_wait_irq_event(const struct device *dev, struct pcie_ep_emul_irq_event *event,
				k_timeout_t timeout);

/**
 * @brief Inject a PCIe reset into an emulated endpoint (test-host API)
 *
 * Models the host asserting a reset: the callback registered through
 * pcie_ep_register_reset_cb() for @a reset is invoked with its
 * registration argument in interrupt context (via irq_offload()), as
 * the PCIe EP driver API contract requires. Injecting a reset with no
 * registered callback succeeds without any callback running.
 *
 * @param dev   Emulated endpoint device
 * @param reset Reset type to inject
 *
 * @retval 0 on success
 * @retval -EINVAL on an invalid reset type
 */
int pcie_ep_emul_inject_reset(const struct device *dev, enum pcie_reset reset);

/**
 * @brief Read endpoint configuration space as the host (test-host API)
 *
 * Models a host configuration read: same layout and access rules as
 * pcie_ep_conf_read().
 *
 * @param dev    Emulated endpoint device
 * @param offset Offset within configuration space
 * @param value  Value destination
 *
 * @retval 0 on success
 * @retval -EINVAL on a misaligned or out-of-range offset
 */
int pcie_ep_emul_host_conf_read(const struct device *dev, uint32_t offset, uint32_t *value);

/**
 * @brief Write endpoint configuration space as the host (test-host API)
 *
 * Models a host configuration write: same writability rules as
 * pcie_ep_conf_write() (only the low command bits, enabled BARs as
 * size-masked base assignment, and the MSI/MSI-X control bits described
 * for the device are writable). Writes to registers without writable
 * bits fail with -EPERM and leave the image unchanged.
 *
 * @param dev    Emulated endpoint device
 * @param offset Offset within configuration space
 * @param value  Value to write
 *
 * @retval 0 on success
 * @retval -EINVAL on a misaligned or out-of-range offset
 * @retval -EPERM on a write to a register without writable bits
 *         (identity, class/revision, capability pointer, disabled BARs,
 *         unimplemented space)
 */
int pcie_ep_emul_host_conf_write(const struct device *dev, uint32_t offset, uint32_t value);

/**
 * @brief Read BAR memory as the host (test-host API)
 *
 * Models host reads of the memory window behind BAR @a bar: copies
 * @a len bytes from the BAR backing storage at @a offset into @a buf.
 * The assigned base is not consulted; the test host is expected to
 * access only BARs it has assigned through configuration writes.
 *
 * @param dev    Emulated endpoint device
 * @param bar    BAR index (0..5)
 * @param offset Byte offset within the BAR window
 * @param buf    Destination buffer
 * @param len    Number of bytes to read
 *
 * @retval 0 on success
 * @retval -EINVAL on an invalid BAR index, NULL buffer, zero length,
 *         an access exceeding the BAR size, or a disabled BAR
 */
int pcie_ep_emul_host_bar_read(const struct device *dev, uint32_t bar, uint32_t offset, void *buf,
			       uint32_t len);

/**
 * @brief Write BAR memory as the host (test-host API)
 *
 * Models host writes of the memory window behind BAR @a bar: copies
 * @a len bytes from @a buf into the BAR backing storage at @a offset.
 * See pcie_ep_emul_host_bar_read() for the access semantics.
 *
 * @param dev    Emulated endpoint device
 * @param bar    BAR index (0..5)
 * @param offset Byte offset within the BAR window
 * @param buf    Source buffer
 * @param len    Number of bytes to write
 *
 * @retval 0 on success
 * @retval -EINVAL on an invalid BAR index, NULL buffer, zero length,
 *         an access exceeding the BAR size, or a disabled BAR
 */
int pcie_ep_emul_host_bar_write(const struct device *dev, uint32_t bar, uint32_t offset,
				const void *buf, uint32_t len);

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif /* ZEPHYR_INCLUDE_DRIVERS_PCIE_ENDPOINT_PCIE_EP_EMUL_H_ */
