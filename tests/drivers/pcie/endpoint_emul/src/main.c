/*
 * SPDX-FileCopyrightText: Copyright (c) 2026 Process Mission
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/device.h>
#include <zephyr/drivers/dma.h>
#include <zephyr/drivers/pcie/endpoint/pcie_ep.h>
#include <zephyr/drivers/pcie/endpoint/pcie_ep_emul.h>
#include <zephyr/kernel.h>
#include <zephyr/ztest.h>

#include <limits.h>
#include <string.h>

#define CFG_VENDOR_DEVICE_ID 0x00U
#define CFG_COMMAND_STATUS   0x04U
#define CFG_CLASS_REVISION   0x08U
#define CFG_BAR0             0x10U
#define CFG_BAR1             0x14U
#define CFG_BAR2             0x18U
#define CFG_BAR3             0x1cU
#define CFG_BAR5             0x24U
#define CFG_CAPABILITY_PTR   0x34U
#define CFG_MSI_CAP          0x40U
#define CFG_MSIX_CAP         0x50U
#define CFG_SIZE             4096U

/* Status register (upper command/status half) capabilities-list bit. */
#define STATUS_CAPABILITIES_LIST 0x00100000U

/* MSI/MSI-X message control fields (upper half of the first cap dword). */
#define MSI_CTRL_ENABLE     0x00010000U
#define MSI_MME_SHIFT       20U
#define MSIX_CTRL_ENABLE    0x80000000U
#define MSIX_CTRL_FUNC_MASK 0x40000000U

#define EP0_VENDOR_DEVICE_ID 0x00011234U
#define EP0_CLASS_REVISION   0x06040001U
#define EP1_VENDOR_DEVICE_ID 0x00025678U
#define EP1_CLASS_REVISION   0xff000002U

#define APERTURE0_PCIE_BASE 0x80000000ULL
#define APERTURE1_PCIE_BASE 0x90000000ULL
#define APERTURE_LEN        0x2000U

static const struct device *const ep0 = DEVICE_DT_GET(DT_NODELABEL(tst_ep0));
static const struct device *const ep1 = DEVICE_DT_GET(DT_NODELABEL(tst_ep1));
/* Third endpoint without a dmas property. */
static const struct device *const ep2 = DEVICE_DT_GET(DT_NODELABEL(tst_ep2));
static const struct device *const dma_dev = DEVICE_DT_GET(DT_NODELABEL(dma));

static uint8_t host_mem0[APERTURE_LEN];
static uint8_t host_mem1[APERTURE_LEN];

ZTEST(pcie_ep_emul, test_devices_ready)
{
	zassert_true(device_is_ready(ep0), "first emulated endpoint is not ready");
	zassert_true(device_is_ready(ep1), "second emulated endpoint is not ready");
	zassert_true(device_is_ready(ep2), "third emulated endpoint is not ready");
}

ZTEST(pcie_ep_emul, test_identity_per_instance)
{
	uint32_t value;

	zassert_ok(pcie_ep_conf_read(ep0, CFG_VENDOR_DEVICE_ID, &value));
	zassert_equal(value, EP0_VENDOR_DEVICE_ID);
	zassert_ok(pcie_ep_conf_read(ep0, CFG_CLASS_REVISION, &value));
	zassert_equal(value, EP0_CLASS_REVISION);

	zassert_ok(pcie_ep_conf_read(ep1, CFG_VENDOR_DEVICE_ID, &value));
	zassert_equal(value, EP1_VENDOR_DEVICE_ID);
	zassert_ok(pcie_ep_conf_read(ep1, CFG_CLASS_REVISION, &value));
	zassert_equal(value, EP1_CLASS_REVISION);
}

ZTEST(pcie_ep_emul, test_conf_read_invalid_access)
{
	uint32_t value;

	zassert_equal(pcie_ep_conf_read(ep0, 1, &value), -EINVAL);
	zassert_equal(pcie_ep_conf_read(ep0, CFG_SIZE - 2, &value), -EINVAL);
	zassert_equal(pcie_ep_conf_read(ep0, CFG_SIZE, &value), -EINVAL);
	zassert_equal(pcie_ep_conf_read(ep0, UINT32_MAX - 2, &value), -EINVAL);
}

ZTEST(pcie_ep_emul, test_conf_write_readonly_identity)
{
	uint32_t value;

	pcie_ep_conf_write(ep0, CFG_VENDOR_DEVICE_ID, 0xdeadbeefU);
	zassert_ok(pcie_ep_conf_read(ep0, CFG_VENDOR_DEVICE_ID, &value));
	zassert_equal(value, EP0_VENDOR_DEVICE_ID);

	pcie_ep_conf_write(ep0, CFG_CLASS_REVISION, 0xdeadbeefU);
	zassert_ok(pcie_ep_conf_read(ep0, CFG_CLASS_REVISION, &value));
	zassert_equal(value, EP0_CLASS_REVISION);

	/* Invalid writes are a no-op and must not corrupt the image. */
	pcie_ep_conf_write(ep0, 1, 0xdeadbeefU);
	pcie_ep_conf_write(ep0, CFG_SIZE, 0xdeadbeefU);
	zassert_ok(pcie_ep_conf_read(ep0, CFG_VENDOR_DEVICE_ID, &value));
	zassert_equal(value, EP0_VENDOR_DEVICE_ID);
}

ZTEST(pcie_ep_emul, test_conf_write_command_bits)
{
	uint32_t value;

	/*
	 * Writable command bits 0..2; status stays read-only and keeps
	 * the capabilities-list bit set at init (ep0 advertises MSI/MSI-X).
	 */
	pcie_ep_conf_write(ep0, CFG_COMMAND_STATUS, 0xffffffffU);
	zassert_ok(pcie_ep_conf_read(ep0, CFG_COMMAND_STATUS, &value));
	zassert_equal(value, STATUS_CAPABILITIES_LIST | 0x7U);

	pcie_ep_conf_write(ep0, CFG_COMMAND_STATUS, 0x0U);
	zassert_ok(pcie_ep_conf_read(ep0, CFG_COMMAND_STATUS, &value));
	zassert_equal(value, STATUS_CAPABILITIES_LIST);
}

ZTEST(pcie_ep_emul, test_bar_assignment_and_mask)
{
	uint32_t value;

	/* BAR0 of ep0 is 64 KiB: the low 16 bits are size-masked. */
	zassert_ok(pcie_ep_conf_read(ep0, CFG_BAR0, &value));
	zassert_equal(value, 0x0U);

	pcie_ep_conf_write(ep0, CFG_BAR0, 0x1234abcdU);
	zassert_ok(pcie_ep_conf_read(ep0, CFG_BAR0, &value));
	zassert_equal(value, 0x12340000U);

	/* BAR1 of ep0 is 4 KiB: the low 12 bits are size-masked. */
	pcie_ep_conf_write(ep0, CFG_BAR1, 0x80000fffU);
	zassert_ok(pcie_ep_conf_read(ep0, CFG_BAR1, &value));
	zassert_equal(value, 0x80000000U);

	/* BAR2..BAR5 of ep0 are disabled: reads stay zero, writes ignored. */
	pcie_ep_conf_write(ep0, CFG_BAR2, 0x12345000U);
	zassert_ok(pcie_ep_conf_read(ep0, CFG_BAR2, &value));
	zassert_equal(value, 0x0U);
	pcie_ep_conf_write(ep0, CFG_BAR5, 0x12345000U);
	zassert_ok(pcie_ep_conf_read(ep0, CFG_BAR5, &value));
	zassert_equal(value, 0x0U);
}

ZTEST(pcie_ep_emul, test_instance_isolation)
{
	uint32_t value;

	pcie_ep_conf_write(ep0, CFG_BAR0, 0x11110000U);
	pcie_ep_conf_write(ep0, CFG_COMMAND_STATUS, 0x7U);

	zassert_ok(pcie_ep_conf_read(ep1, CFG_BAR0, &value));
	zassert_equal(value, 0x0U);
	/* ep1's status keeps its read-only capabilities-list bit (MSI). */
	zassert_ok(pcie_ep_conf_read(ep1, CFG_COMMAND_STATUS, &value));
	zassert_equal(value, STATUS_CAPABILITIES_LIST);

	/* ep1 BAR0 is 4 KiB: same write as above keeps the low 12 bits. */
	pcie_ep_conf_write(ep1, CFG_BAR0, 0x22222fffU);
	zassert_ok(pcie_ep_conf_read(ep1, CFG_BAR0, &value));
	zassert_equal(value, 0x22222000U);

	zassert_ok(pcie_ep_conf_read(ep0, CFG_BAR0, &value));
	zassert_equal(value, 0x11110000U);
}

ZTEST(pcie_ep_emul, test_msi_msix_capabilities)
{
	uint32_t value;

	/* ep0 advertises both capabilities; make sure both start disabled. */
	pcie_ep_conf_write(ep0, CFG_MSI_CAP, 0x0U);
	pcie_ep_conf_write(ep0, CFG_MSIX_CAP, 0x0U);

	zassert_ok(pcie_ep_conf_read(ep0, CFG_COMMAND_STATUS, &value));
	zassert_true((value & STATUS_CAPABILITIES_LIST) != 0);
	zassert_ok(pcie_ep_conf_read(ep0, CFG_CAPABILITY_PTR, &value));
	zassert_equal(value, CFG_MSI_CAP);

	/* MSI: ID 0x05, next 0x50, Multiple Message Capable = 2 (4 vectors). */
	zassert_ok(pcie_ep_conf_read(ep0, CFG_MSI_CAP, &value));
	zassert_equal(value, 0x00045005U);

	/* MSI-X: ID 0x11, last capability, table size 7 (8 vectors). */
	zassert_ok(pcie_ep_conf_read(ep0, CFG_MSIX_CAP, &value));
	zassert_equal(value, 0x00070011U);

	/* Table at offset 0 of BAR0; PBA right after the 8-entry table. */
	zassert_ok(pcie_ep_conf_read(ep0, CFG_MSIX_CAP + 4, &value));
	zassert_equal(value, 0x0U);
	zassert_ok(pcie_ep_conf_read(ep0, CFG_MSIX_CAP + 8, &value));
	zassert_equal(value, 0x80U);

	/*
	 * The MSI enable bit and MME (clamped to MMC = 2) are writable, as is
	 * the MSI-X function mask bit; the rest of both dwords is read-only.
	 */
	pcie_ep_conf_write(ep0, CFG_MSI_CAP, 0xffffffffU);
	zassert_ok(pcie_ep_conf_read(ep0, CFG_MSI_CAP, &value));
	zassert_equal(value, 0x00255005U);
	pcie_ep_conf_write(ep0, CFG_MSIX_CAP, 0xffffffffU);
	zassert_ok(pcie_ep_conf_read(ep0, CFG_MSIX_CAP, &value));
	zassert_equal(value, 0xc0070011U);

	/* Message address/data read as zero and ignore writes. */
	pcie_ep_conf_write(ep0, CFG_MSI_CAP + 4, 0xdeadbeefU);
	pcie_ep_conf_write(ep0, CFG_MSI_CAP + 8, 0xdeadbeefU);
	zassert_ok(pcie_ep_conf_read(ep0, CFG_MSI_CAP + 4, &value));
	zassert_equal(value, 0x0U);
	zassert_ok(pcie_ep_conf_read(ep0, CFG_MSI_CAP + 8, &value));
	zassert_equal(value, 0x0U);

	/* The host-side accessors follow the same writability rules. */
	zassert_ok(pcie_ep_emul_host_conf_write(ep0, CFG_MSI_CAP, 0x0U));
	zassert_ok(pcie_ep_emul_host_conf_write(ep0, CFG_MSIX_CAP, 0x0U));
	zassert_ok(pcie_ep_emul_host_conf_read(ep0, CFG_MSI_CAP, &value));
	zassert_equal(value, 0x00045005U);

	/* ep1 advertises MSI with 1 vector and no MSI-X. */
	pcie_ep_conf_write(ep1, CFG_MSI_CAP, 0x0U);
	zassert_ok(pcie_ep_conf_read(ep1, CFG_COMMAND_STATUS, &value));
	zassert_true((value & STATUS_CAPABILITIES_LIST) != 0);
	zassert_ok(pcie_ep_conf_read(ep1, CFG_CAPABILITY_PTR, &value));
	zassert_equal(value, CFG_MSI_CAP);
	zassert_ok(pcie_ep_conf_read(ep1, CFG_MSI_CAP, &value));
	zassert_equal(value, 0x00000005U);
	pcie_ep_conf_write(ep1, CFG_MSI_CAP, 0x00010000U);
	zassert_ok(pcie_ep_conf_read(ep1, CFG_MSI_CAP, &value));
	zassert_equal(value, 0x00010005U);
	pcie_ep_conf_write(ep1, CFG_MSI_CAP, 0x0U);

	/* Where ep0 has its MSI-X capability, ep1 reads zero and ignores writes. */
	pcie_ep_conf_write(ep1, CFG_MSIX_CAP, 0xffffffffU);
	zassert_ok(pcie_ep_conf_read(ep1, CFG_MSIX_CAP, &value));
	zassert_equal(value, 0x0U);

	/*
	 * ep2 has no MSI but a 2-vector MSI-X capability: without an MSI
	 * capability the MSI-X one heads the list at 0x40.
	 */
	pcie_ep_conf_write(ep2, CFG_MSI_CAP, 0x0U);
	zassert_ok(pcie_ep_conf_read(ep2, CFG_COMMAND_STATUS, &value));
	zassert_true((value & STATUS_CAPABILITIES_LIST) != 0);
	zassert_ok(pcie_ep_conf_read(ep2, CFG_CAPABILITY_PTR, &value));
	zassert_equal(value, CFG_MSI_CAP);

	/*
	 * ID 0x11 (MSI-X, not MSI), next pointer 0 terminates the list,
	 * table size 1 (2 vectors).
	 */
	zassert_ok(pcie_ep_conf_read(ep2, CFG_MSI_CAP, &value));
	zassert_equal(value, 0x00010011U);

	/* Table at offset 0 of BAR0; PBA right after the 2-entry table. */
	zassert_ok(pcie_ep_conf_read(ep2, CFG_MSI_CAP + 4, &value));
	zassert_equal(value, 0x0U);
	zassert_ok(pcie_ep_conf_read(ep2, CFG_MSI_CAP + 8, &value));
	zassert_equal(value, 0x20U);

	/* The MSI-X enable and function mask bits are writable; the rest is not. */
	pcie_ep_conf_write(ep2, CFG_MSI_CAP, 0xffffffffU);
	zassert_ok(pcie_ep_conf_read(ep2, CFG_MSI_CAP, &value));
	zassert_equal(value, 0xc0010011U);
	pcie_ep_conf_write(ep2, CFG_MSI_CAP, 0x0U);

	/* Where ep0 has its MSI-X capability, ep2 reads zero (its own is at 0x40). */
	pcie_ep_conf_write(ep2, CFG_MSIX_CAP, 0xffffffffU);
	zassert_ok(pcie_ep_conf_read(ep2, CFG_MSIX_CAP, &value));
	zassert_equal(value, 0x0U);
}

ZTEST(pcie_ep_emul, test_dma_channels_claimed_exclusively)
{
	/*
	 * ep0 and ep1 claimed all four channels of the controller at init
	 * and hold them for their lifetime, so a late channel request on
	 * the same controller finds nothing free.
	 */
	zassert_equal(dma_request_channel(dma_dev, NULL), -EINVAL);

	/* Requesting any of the claimed channels by exact number fails, too. */
	for (uint32_t ch = 0; ch < 4U; ch++) {
		zassert_equal(dma_request_channel(dma_dev, &ch), -EINVAL,
			      "exact request for claimed channel %u succeeded", ch);
	}
}

ZTEST(pcie_ep_emul, test_aperture_registration)
{
	/* Overlap with the fixture aperture is rejected. */
	zassert_equal(pcie_ep_emul_register_aperture(ep0, APERTURE0_PCIE_BASE + 0x1000, host_mem0,
						     APERTURE_LEN),
		      -EALREADY);

	/* Zero length and overflowing ranges are rejected. */
	zassert_equal(pcie_ep_emul_register_aperture(ep0, 0xa0000000ULL, host_mem0, 0), -EINVAL);
	zassert_equal(pcie_ep_emul_register_aperture(ep0, UINT64_MAX - 0x10ULL, host_mem0, 0x20U),
		      -EINVAL);
	zassert_equal(pcie_ep_emul_register_aperture(ep0, 0xa0000000ULL, NULL, 0x1000U), -EINVAL);

	/* Unregistering an unknown aperture fails; the registered one is
	 * removed by the fixture teardown.
	 */
	zassert_equal(pcie_ep_emul_unregister_aperture(ep0, 0xa0000000ULL), -ENOENT);
}

ZTEST(pcie_ep_emul, test_map_addr_negative_paths)
{
	uint64_t mapped;

	/* Not covered by any registered aperture. */
	zassert_equal(pcie_ep_map_addr(ep0, 0x1000ULL, &mapped, 0x100U, PCIE_OB_ANYMEM), -ENOTSUP);

	/* Crossing the aperture end is rejected. */
	zassert_equal(pcie_ep_map_addr(ep0, APERTURE0_PCIE_BASE + APERTURE_LEN - 0x100U, &mapped,
				       0x200U, PCIE_OB_ANYMEM),
		      -ENOTSUP);

	/* Zero-length and overflowing requests are rejected. */
	zassert_equal(pcie_ep_map_addr(ep0, APERTURE0_PCIE_BASE, &mapped, 0, PCIE_OB_ANYMEM),
		      -EINVAL);
	zassert_equal(pcie_ep_map_addr(ep0, UINT64_MAX - 0x10ULL, &mapped, 0x20U, PCIE_OB_ANYMEM),
		      -EINVAL);

	/* Apertures are per instance: ep0's aperture is invisible to ep1. */
	zassert_equal(pcie_ep_map_addr(ep1, APERTURE0_PCIE_BASE, &mapped, 0x100U, PCIE_OB_ANYMEM),
		      -ENOTSUP);
}

ZTEST(pcie_ep_emul, test_map_unmap_lifecycle)
{
	uint64_t mapped[CONFIG_PCIE_EP_EMUL_MAX_MAPS];
	uint64_t extra;
	int ret;

	/* Fill the per-instance mapping table. */
	for (int i = 0; i < CONFIG_PCIE_EP_EMUL_MAX_MAPS; i++) {
		ret = pcie_ep_map_addr(ep0, APERTURE0_PCIE_BASE + i * 0x100U, &mapped[i], 0x100U,
				       PCIE_OB_ANYMEM);
		zassert_equal(ret, 0x100);
		zassert_equal(mapped[i], (uint64_t)(uintptr_t)&host_mem0[i * 0x100U]);
	}

	/* The table is bounded: one more mapping of a new address fails. */
	zassert_equal(pcie_ep_map_addr(ep0,
				       APERTURE0_PCIE_BASE + CONFIG_PCIE_EP_EMUL_MAX_MAPS * 0x100U,
				       &extra, 0x100U, PCIE_OB_ANYMEM),
		      -ENOMEM);

	/* Unknown and duplicate unmaps are ignored and disturb nothing. */
	pcie_ep_unmap_addr(ep0, (uint64_t)(uintptr_t)&host_mem1[0]);
	pcie_ep_unmap_addr(ep0, mapped[0]);
	pcie_ep_unmap_addr(ep0, mapped[0]);

	/* A freed slot is reusable, the remaining records stay valid. */
	ret = pcie_ep_map_addr(ep0, APERTURE0_PCIE_BASE, &mapped[0], 0x100U, PCIE_OB_ANYMEM);
	zassert_equal(ret, 0x100);

	/* Unregistering an aperture with active mappings is rejected. */
	zassert_equal(pcie_ep_emul_unregister_aperture(ep0, APERTURE0_PCIE_BASE), -EBUSY);

	for (int i = 0; i < CONFIG_PCIE_EP_EMUL_MAX_MAPS; i++) {
		pcie_ep_unmap_addr(ep0, mapped[i]);
	}

	zassert_ok(pcie_ep_emul_unregister_aperture(ep0, APERTURE0_PCIE_BASE));
	zassert_ok(
		pcie_ep_emul_register_aperture(ep0, APERTURE0_PCIE_BASE, host_mem0, APERTURE_LEN));
}

ZTEST(pcie_ep_emul, test_map_addr_duplicate_and_oversize)
{
	uint64_t mapped;
	uint64_t dup;

	/* Sizes that do not fit the int return value are rejected up front. */
	zassert_equal(pcie_ep_map_addr(ep0, APERTURE0_PCIE_BASE, &mapped, (uint32_t)INT_MAX + 1U,
				       PCIE_OB_ANYMEM),
		      -EINVAL);

	/* A second mapping of a live address is rejected instead of aliased. */
	zassert_equal(pcie_ep_map_addr(ep0, APERTURE0_PCIE_BASE, &mapped, 0x100U, PCIE_OB_ANYMEM),
		      0x100);
	zassert_equal(pcie_ep_map_addr(ep0, APERTURE0_PCIE_BASE, &dup, 0x100U, PCIE_OB_ANYMEM),
		      -EALREADY);

	/* Overlapping sub/superranges of a live mapping are rejected too. */
	zassert_equal(
		pcie_ep_map_addr(ep0, APERTURE0_PCIE_BASE + 0x40U, &dup, 0x40U, PCIE_OB_ANYMEM),
		-EALREADY);
	zassert_equal(
		pcie_ep_map_addr(ep0, APERTURE0_PCIE_BASE + 0x80U, &dup, 0x100U, PCIE_OB_ANYMEM),
		-EALREADY);
	pcie_ep_unmap_addr(ep0, mapped);

	/* A repeat unmap frees nothing; the record is owned once and reusable. */
	pcie_ep_unmap_addr(ep0, mapped);
	pcie_ep_unmap_addr(ep0, mapped);
	zassert_equal(pcie_ep_map_addr(ep0, APERTURE0_PCIE_BASE, &mapped, 0x100U, PCIE_OB_ANYMEM),
		      0x100);
	pcie_ep_unmap_addr(ep0, mapped);
}

ZTEST(pcie_ep_emul, test_xfer_data_memcpy_both_directions)
{
	uint8_t local_buf[0x100];
	uint8_t expected[0x100];
	uint8_t zeros[sizeof(local_buf)] = {0};

	for (int i = 0; i < sizeof(expected); i++) {
		host_mem0[i] = (uint8_t)(i ^ 0x5a);
		expected[i] = (uint8_t)(i + 1);
	}

	/* Host to device: host aperture contents land in local memory. */
	memset(local_buf, 0, sizeof(local_buf));
	zassert_ok(pcie_ep_xfer_data_memcpy(ep0, APERTURE0_PCIE_BASE, (uintptr_t *)local_buf,
					    sizeof(local_buf), PCIE_OB_ANYMEM, HOST_TO_DEVICE));
	zassert_mem_equal(local_buf, host_mem0, sizeof(local_buf));

	/* Device to host: local contents land in the host aperture. */
	memcpy(local_buf, expected, sizeof(local_buf));
	memset(host_mem0, 0, sizeof(local_buf));
	zassert_ok(pcie_ep_xfer_data_memcpy(ep0, APERTURE0_PCIE_BASE, (uintptr_t *)local_buf,
					    sizeof(local_buf), PCIE_OB_ANYMEM, DEVICE_TO_HOST));
	zassert_mem_equal(host_mem0, expected, sizeof(expected));

	/* Unregistered host addresses fail without touching local memory. */
	memset(local_buf, 0, sizeof(local_buf));
	zassert_equal(pcie_ep_xfer_data_memcpy(ep0, 0x1000ULL, (uintptr_t *)local_buf,
					       sizeof(local_buf), PCIE_OB_ANYMEM, HOST_TO_DEVICE),
		      -ENOTSUP);
	zassert_mem_equal(local_buf, zeros, sizeof(local_buf));

	/*
	 * Device to host to an unregistered address fails, too, and the
	 * registered aperture backing is left unchanged.
	 */
	zassert_equal(pcie_ep_xfer_data_memcpy(ep0, 0x1000ULL, (uintptr_t *)local_buf,
					       sizeof(local_buf), PCIE_OB_ANYMEM, DEVICE_TO_HOST),
		      -ENOTSUP);
	zassert_mem_equal(host_mem0, expected, sizeof(expected));

	/* The mapping used by the helper is released again. */
	zassert_equal(pcie_ep_emul_unregister_aperture(ep0, APERTURE0_PCIE_BASE), 0);
	zassert_ok(
		pcie_ep_emul_register_aperture(ep0, APERTURE0_PCIE_BASE, host_mem0, APERTURE_LEN));
}

ZTEST(pcie_ep_emul, test_ep1_aperture_backing_and_isolation)
{
	uint8_t local_buf[0x100];
	uint8_t expected[sizeof(local_buf)];
	uint8_t zeros[sizeof(local_buf)] = {0};
	uint64_t mapped;

	for (int i = 0; i < sizeof(expected); i++) {
		expected[i] = (uint8_t)(i ^ 0x3c);
	}

	/* ep1's fixture aperture maps into ep1's own backing buffer. */
	zassert_equal(pcie_ep_map_addr(ep1, APERTURE1_PCIE_BASE, &mapped, sizeof(local_buf),
				       PCIE_OB_ANYMEM),
		      sizeof(local_buf));
	zassert_equal(mapped, (uint64_t)(uintptr_t)host_mem1);
	pcie_ep_unmap_addr(ep1, mapped);

	/* Host to device: data lands from ep1's backing buffer. */
	memcpy(host_mem1, expected, sizeof(expected));
	memset(local_buf, 0, sizeof(local_buf));
	zassert_ok(pcie_ep_xfer_data_memcpy(ep1, APERTURE1_PCIE_BASE, (uintptr_t *)local_buf,
					    sizeof(local_buf), PCIE_OB_ANYMEM, HOST_TO_DEVICE));
	zassert_mem_equal(local_buf, expected, sizeof(local_buf));

	/*
	 * Device to host: ep1's backing buffer receives the data while
	 * ep0's backing buffer (zeroed by the fixture) stays untouched.
	 */
	memset(host_mem1, 0, sizeof(local_buf));
	memcpy(local_buf, expected, sizeof(local_buf));
	zassert_ok(pcie_ep_xfer_data_memcpy(ep1, APERTURE1_PCIE_BASE, (uintptr_t *)local_buf,
					    sizeof(local_buf), PCIE_OB_ANYMEM, DEVICE_TO_HOST));
	zassert_mem_equal(host_mem1, expected, sizeof(expected));
	zassert_mem_equal(host_mem0, zeros, sizeof(local_buf));

	/* The mapping used by the helper is released again. */
	zassert_ok(pcie_ep_emul_unregister_aperture(ep1, APERTURE1_PCIE_BASE));
	zassert_ok(
		pcie_ep_emul_register_aperture(ep1, APERTURE1_PCIE_BASE, host_mem1, APERTURE_LEN));
}

/* Backing buffers used only to fill the aperture table. */
static uint8_t aperture_fill_mem[CONFIG_PCIE_EP_EMUL_MAX_APERTURES - 1][0x100];

ZTEST(pcie_ep_emul, test_aperture_table_full)
{
	/* The fixture aperture occupies one slot; fill the remaining ones. */
	for (int i = 0; i < CONFIG_PCIE_EP_EMUL_MAX_APERTURES - 1; i++) {
		zassert_ok(pcie_ep_emul_register_aperture(ep0, 0xa0000000ULL + i * 0x1000ULL,
							  aperture_fill_mem[i],
							  sizeof(aperture_fill_mem[i])));
	}

	/* The per-instance table is bounded: one more aperture fails. */
	zassert_equal(pcie_ep_emul_register_aperture(ep0, 0xb0000000ULL, host_mem0, 0x100U),
		      -ENOMEM);

	for (int i = 0; i < CONFIG_PCIE_EP_EMUL_MAX_APERTURES - 1; i++) {
		zassert_ok(pcie_ep_emul_unregister_aperture(ep0, 0xa0000000ULL + i * 0x1000ULL));
	}
}

/* ep0: legacy IRQ, 4 MSI vectors, 8 MSI-X vectors. */
/* ep1: no legacy IRQ, 1 MSI vector, no MSI-X vectors. */

ZTEST(pcie_ep_emul, test_raise_irq_events)
{
	struct pcie_ep_emul_irq_event event;

	/* Enable the capabilities like a real host: MSI with all 4 vectors
	 * allocated (MME = log2(4) = 2), MSI-X unmasked.
	 */
	pcie_ep_conf_write(ep0, CFG_MSI_CAP, MSI_CTRL_ENABLE | (2U << MSI_MME_SHIFT));
	pcie_ep_conf_write(ep0, CFG_MSIX_CAP, MSIX_CTRL_ENABLE);

	zassert_ok(pcie_ep_raise_irq(ep0, PCIE_EP_IRQ_LEGACY, 0));
	zassert_ok(pcie_ep_raise_irq(ep0, PCIE_EP_IRQ_MSI, 2));
	zassert_ok(pcie_ep_raise_irq(ep0, PCIE_EP_IRQ_MSIX, 7));

	/* Events are consumed in raise order with type and vector intact. */
	zassert_ok(pcie_ep_emul_wait_irq_event(ep0, &event, K_NO_WAIT));
	zassert_equal(event.type, PCIE_EP_IRQ_LEGACY);
	zassert_equal(event.vector, 0);
	zassert_ok(pcie_ep_emul_wait_irq_event(ep0, &event, K_NO_WAIT));
	zassert_equal(event.type, PCIE_EP_IRQ_MSI);
	zassert_equal(event.vector, 2);
	zassert_ok(pcie_ep_emul_wait_irq_event(ep0, &event, K_NO_WAIT));
	zassert_equal(event.type, PCIE_EP_IRQ_MSIX);
	zassert_equal(event.vector, 7);

	/* The queue is empty: neither polling nor waiting yields an event. */
	zassert_equal(pcie_ep_emul_wait_irq_event(ep0, &event, K_NO_WAIT), -EAGAIN);
	zassert_equal(pcie_ep_emul_wait_irq_event(ep0, &event, K_MSEC(10)), -EAGAIN);

	/* NULL destination is rejected. */
	zassert_equal(pcie_ep_emul_wait_irq_event(ep0, NULL, K_NO_WAIT), -EINVAL);
}

ZTEST(pcie_ep_emul, test_raise_irq_negative)
{
	struct pcie_ep_emul_irq_event event;

	/* IRQ types disabled in devicetree are rejected. */
	zassert_equal(pcie_ep_raise_irq(ep1, PCIE_EP_IRQ_LEGACY, 0), -ENOTSUP);
	zassert_equal(pcie_ep_raise_irq(ep1, PCIE_EP_IRQ_MSIX, 0), -ENOTSUP);

	/* Capabilities the host has not enabled reject raises. */
	zassert_equal(pcie_ep_raise_irq(ep0, PCIE_EP_IRQ_MSI, 0), -ENOTSUP);
	zassert_equal(pcie_ep_raise_irq(ep0, PCIE_EP_IRQ_MSIX, 0), -ENOTSUP);

	/* Out-of-range vectors are rejected regardless of enable state. */
	zassert_equal(pcie_ep_raise_irq(ep0, PCIE_EP_IRQ_MSI, 4), -EINVAL);
	zassert_equal(pcie_ep_raise_irq(ep0, PCIE_EP_IRQ_MSIX, 8), -EINVAL);
	zassert_equal(pcie_ep_raise_irq(ep1, PCIE_EP_IRQ_MSI, 1), -EINVAL);

	/*
	 * The host allocated fewer vectors than advertised: with MME = 0
	 * (one vector, the default) only vector 0 could be raised.
	 */
	pcie_ep_conf_write(ep0, CFG_MSI_CAP, MSI_CTRL_ENABLE);
	zassert_equal(pcie_ep_raise_irq(ep0, PCIE_EP_IRQ_MSI, 1), -EINVAL);
	pcie_ep_conf_write(ep0, CFG_MSI_CAP, MSI_CTRL_ENABLE | (1U << MSI_MME_SHIFT));
	zassert_equal(pcie_ep_raise_irq(ep0, PCIE_EP_IRQ_MSI, 2), -EINVAL);
	pcie_ep_conf_write(ep0, CFG_MSI_CAP, 0x0U);
	zassert_equal(pcie_ep_raise_irq(ep0, PCIE_EP_IRQ_MSI, 0), -ENOTSUP);

	/* A function-masked MSI-X capability suppresses raises. */
	pcie_ep_conf_write(ep0, CFG_MSIX_CAP, MSIX_CTRL_ENABLE | MSIX_CTRL_FUNC_MASK);
	zassert_equal(pcie_ep_raise_irq(ep0, PCIE_EP_IRQ_MSIX, 0), -ENOTSUP);
	pcie_ep_conf_write(ep0, CFG_MSIX_CAP, 0x0U);

	/* An invalid interrupt type is rejected. */
	zassert_equal(pcie_ep_raise_irq(ep0, (enum pci_ep_irq_type)3, 0), -EINVAL);

	/* Failed raises must not record events on either instance. */
	zassert_equal(pcie_ep_emul_wait_irq_event(ep0, &event, K_NO_WAIT), -EAGAIN);
	zassert_equal(pcie_ep_emul_wait_irq_event(ep1, &event, K_NO_WAIT), -EAGAIN);
}

ZTEST(pcie_ep_emul, test_raise_irq_msix_enable_state_ep2)
{
	struct pcie_ep_emul_irq_event event;

	/* ep2's MSI-X capability heads its list at 0x40; it starts disabled. */
	zassert_equal(pcie_ep_raise_irq(ep2, PCIE_EP_IRQ_MSIX, 0), -ENOTSUP);

	/* Enabled and unmasked, both vectors raise and record events. */
	pcie_ep_conf_write(ep2, CFG_MSI_CAP, MSIX_CTRL_ENABLE);
	zassert_ok(pcie_ep_raise_irq(ep2, PCIE_EP_IRQ_MSIX, 0));
	zassert_ok(pcie_ep_raise_irq(ep2, PCIE_EP_IRQ_MSIX, 1));
	zassert_ok(pcie_ep_emul_wait_irq_event(ep2, &event, K_NO_WAIT));
	zassert_equal(event.type, PCIE_EP_IRQ_MSIX);
	zassert_equal(event.vector, 0);
	zassert_ok(pcie_ep_emul_wait_irq_event(ep2, &event, K_NO_WAIT));
	zassert_equal(event.type, PCIE_EP_IRQ_MSIX);
	zassert_equal(event.vector, 1);

	/* Function-masked: raises are suppressed and record nothing. */
	pcie_ep_conf_write(ep2, CFG_MSI_CAP, MSIX_CTRL_ENABLE | MSIX_CTRL_FUNC_MASK);
	zassert_equal(pcie_ep_raise_irq(ep2, PCIE_EP_IRQ_MSIX, 0), -ENOTSUP);
	zassert_equal(pcie_ep_emul_wait_irq_event(ep2, &event, K_NO_WAIT), -EAGAIN);

	/* Disabled again: raises fail and record nothing. */
	pcie_ep_conf_write(ep2, CFG_MSI_CAP, 0x0U);
	zassert_equal(pcie_ep_raise_irq(ep2, PCIE_EP_IRQ_MSIX, 0), -ENOTSUP);
	zassert_equal(pcie_ep_emul_wait_irq_event(ep2, &event, K_NO_WAIT), -EAGAIN);
}

ZTEST(pcie_ep_emul, test_irq_event_queue_bounded)
{
	struct pcie_ep_emul_irq_event event;

	/* Enable MSI like a real host, with all 4 vectors allocated. */
	pcie_ep_conf_write(ep0, CFG_MSI_CAP, MSI_CTRL_ENABLE | (2U << MSI_MME_SHIFT));

	/* The per-instance queue holds exactly MAX_IRQ_EVENTS events. */
	for (int i = 0; i < CONFIG_PCIE_EP_EMUL_MAX_IRQ_EVENTS; i++) {
		/* ep0 has 4 MSI vectors; cycle through the valid range. */
		zassert_ok(pcie_ep_raise_irq(ep0, PCIE_EP_IRQ_MSI, (uint32_t)(i % 4)));
	}
	zassert_equal(pcie_ep_raise_irq(ep0, PCIE_EP_IRQ_MSI, 0), -ENOSPC);

	/* Draining frees capacity; all events arrive in order. */
	for (int i = 0; i < CONFIG_PCIE_EP_EMUL_MAX_IRQ_EVENTS; i++) {
		zassert_ok(pcie_ep_emul_wait_irq_event(ep0, &event, K_NO_WAIT));
		zassert_equal(event.type, PCIE_EP_IRQ_MSI);
		zassert_equal(event.vector, (uint32_t)(i % 4));
	}
	zassert_equal(pcie_ep_emul_wait_irq_event(ep0, &event, K_NO_WAIT), -EAGAIN);
	zassert_ok(pcie_ep_raise_irq(ep0, PCIE_EP_IRQ_LEGACY, 0));
}

struct reset_cb_record {
	int calls;
	bool in_isr;
	void *arg;
};

static struct reset_cb_record ep0_reset_records[PCIE_RESET_MAX];
static struct reset_cb_record ep1_reset_records[PCIE_RESET_MAX];

static void reset_cb(void *arg)
{
	struct reset_cb_record *record = arg;

	record->calls++;
	record->in_isr = k_is_in_isr();
	record->arg = arg;
}

ZTEST(pcie_ep_emul, test_reset_callbacks)
{
	/* Register one callback per reset type, each with its own record. */
	for (int i = 0; i < PCIE_RESET_MAX; i++) {
		zassert_ok(pcie_ep_register_reset_cb(ep0, (enum pcie_reset)i, reset_cb,
						     &ep0_reset_records[i]));
	}

	/* Injection runs the callback in interrupt context with its arg. */
	zassert_ok(pcie_ep_emul_inject_reset(ep0, PCIE_PERST));
	zassert_ok(pcie_ep_emul_inject_reset(ep0, PCIE_PERST_INB));
	zassert_ok(pcie_ep_emul_inject_reset(ep0, PCIE_FLR));

	for (int i = 0; i < PCIE_RESET_MAX; i++) {
		zassert_equal(ep0_reset_records[i].calls, 1, "reset %d callback did not run", i);
		zassert_true(ep0_reset_records[i].in_isr,
			     "reset %d callback did not run in interrupt context", i);
		zassert_equal(ep0_reset_records[i].arg, &ep0_reset_records[i]);
	}

	/* A second PERST injection reaches the same callback again. */
	zassert_ok(pcie_ep_emul_inject_reset(ep0, PCIE_PERST));
	zassert_equal(ep0_reset_records[PCIE_PERST].calls, 2);
}

ZTEST(pcie_ep_emul, test_reset_negative)
{
	/* Invalid reset types and NULL callbacks are rejected. */
	zassert_equal(
		pcie_ep_register_reset_cb(ep0, PCIE_RESET_MAX, reset_cb, &ep0_reset_records[0]),
		-EINVAL);
	zassert_equal(pcie_ep_register_reset_cb(ep0, (enum pcie_reset)(-1), reset_cb,
						&ep0_reset_records[0]),
		      -EINVAL);
	zassert_equal(pcie_ep_register_reset_cb(ep0, PCIE_PERST, NULL, NULL), -EINVAL);
	zassert_equal(pcie_ep_emul_inject_reset(ep0, PCIE_RESET_MAX), -EINVAL);

	/* Injecting a reset with no registered callback is a no-op. */
	zassert_ok(pcie_ep_emul_inject_reset(ep1, PCIE_FLR));
	zassert_equal(ep1_reset_records[PCIE_FLR].calls, 0);
}

ZTEST(pcie_ep_emul, test_irq_reset_instance_isolation)
{
	struct pcie_ep_emul_irq_event event;

	zassert_ok(pcie_ep_register_reset_cb(ep0, PCIE_PERST, reset_cb,
					     &ep0_reset_records[PCIE_PERST]));
	zassert_ok(pcie_ep_register_reset_cb(ep1, PCIE_PERST, reset_cb,
					     &ep1_reset_records[PCIE_PERST]));

	/* Events raised on ep0 are only consumable on ep0. */
	pcie_ep_conf_write(ep0, CFG_MSI_CAP, MSI_CTRL_ENABLE | (2U << MSI_MME_SHIFT));
	zassert_ok(pcie_ep_raise_irq(ep0, PCIE_EP_IRQ_MSI, 1));
	zassert_equal(pcie_ep_emul_wait_irq_event(ep1, &event, K_NO_WAIT), -EAGAIN);
	zassert_ok(pcie_ep_emul_wait_irq_event(ep0, &event, K_NO_WAIT));
	zassert_equal(event.type, PCIE_EP_IRQ_MSI);
	zassert_equal(event.vector, 1);

	/* A reset injected on ep1 never fires the callback of ep0. */
	zassert_ok(pcie_ep_emul_inject_reset(ep1, PCIE_PERST));
	zassert_equal(ep0_reset_records[PCIE_PERST].calls, 0);
	zassert_equal(ep1_reset_records[PCIE_PERST].calls, 1);

	zassert_ok(pcie_ep_emul_inject_reset(ep0, PCIE_PERST));
	zassert_equal(ep0_reset_records[PCIE_PERST].calls, 1);
	zassert_equal(ep1_reset_records[PCIE_PERST].calls, 1);
}

ZTEST(pcie_ep_emul, test_host_conf_access)
{
	uint32_t value;

	/* Host reads see the same image as the endpoint API. */
	zassert_ok(pcie_ep_emul_host_conf_read(ep0, CFG_VENDOR_DEVICE_ID, &value));
	zassert_equal(value, EP0_VENDOR_DEVICE_ID);

	/* Host writes follow the same writability rules. */
	zassert_ok(pcie_ep_emul_host_conf_write(ep0, CFG_BAR0, 0x4567abcdU));
	zassert_ok(pcie_ep_emul_host_conf_read(ep0, CFG_BAR0, &value));
	zassert_equal(value, 0x45670000U);
	/* Writes to read-only registers are reported and change nothing. */
	zassert_equal(pcie_ep_emul_host_conf_write(ep0, CFG_VENDOR_DEVICE_ID, 0xdeadbeefU), -EPERM);
	zassert_ok(pcie_ep_emul_host_conf_read(ep0, CFG_VENDOR_DEVICE_ID, &value));
	zassert_equal(value, EP0_VENDOR_DEVICE_ID);
	zassert_equal(pcie_ep_emul_host_conf_write(ep0, CFG_BAR2, 0xdeadbeefU), -EPERM);

	/* Invalid host accesses fail instead of being silently ignored. */
	zassert_equal(pcie_ep_emul_host_conf_read(ep0, 1, &value), -EINVAL);
	zassert_equal(pcie_ep_emul_host_conf_read(ep0, CFG_SIZE, &value), -EINVAL);
	zassert_equal(pcie_ep_emul_host_conf_write(ep0, CFG_SIZE, 0), -EINVAL);
}

ZTEST(pcie_ep_emul, test_host_bar_access)
{
	uint8_t written[0x40];
	uint8_t read_back[sizeof(written)];
	uint8_t other[sizeof(written)];
	uint8_t zeros[sizeof(written)] = {0};

	for (int i = 0; i < sizeof(written); i++) {
		written[i] = (uint8_t)(i * 3 + 1);
	}

	/* Data written by the host reads back through the same BAR. */
	zassert_ok(pcie_ep_emul_host_bar_write(ep0, 0, 0x100, written, sizeof(written)));
	memset(read_back, 0, sizeof(read_back));
	zassert_ok(pcie_ep_emul_host_bar_read(ep0, 0, 0x100, read_back, sizeof(read_back)));
	zassert_mem_equal(read_back, written, sizeof(written));

	/* BAR backing is per instance: ep1's BAR0 is unaffected. */
	zassert_ok(pcie_ep_emul_host_bar_read(ep1, 0, 0x100, other, sizeof(other)));
	zassert_mem_equal(other, zeros, sizeof(other));

	/* Out-of-window, disabled-BAR, and malformed accesses fail. */
	zassert_equal(pcie_ep_emul_host_bar_write(ep0, 6, 0, written, sizeof(written)), -EINVAL);
	zassert_equal(pcie_ep_emul_host_bar_write(ep0, 2, 0, written, sizeof(written)), -EINVAL);
	zassert_equal(pcie_ep_emul_host_bar_write(ep0, 0, 0x10000 - 0x10, written, 0x20), -EINVAL);
	zassert_equal(pcie_ep_emul_host_bar_read(ep0, 0, 0, NULL, 0x10), -EINVAL);
	zassert_equal(pcie_ep_emul_host_bar_read(ep0, 0, 0, read_back, 0), -EINVAL);
	/* ep1 BAR0 is 4 KiB: a larger read is out of window. */
	zassert_equal(pcie_ep_emul_host_bar_read(ep1, 0, 0, other, 0x2000), -EINVAL);
}

static void fill_pattern(uint8_t *buf, size_t len, uint8_t seed)
{
	for (size_t i = 0; i < len; i++) {
		buf[i] = (uint8_t)(seed + i * 7U);
	}
}

ZTEST(pcie_ep_emul, test_dma_xfer_both_directions)
{
	uint8_t local_buf[0x100];
	uint8_t expected[sizeof(local_buf)];
	uint8_t zeros[sizeof(local_buf)] = {0};

	fill_pattern(host_mem0, sizeof(local_buf), 0x11);
	fill_pattern(expected, sizeof(expected), 0x42);

	/* Host to device: host aperture contents land in local memory. */
	memset(local_buf, 0, sizeof(local_buf));
	zassert_ok(pcie_ep_xfer_data_dma(ep0, APERTURE0_PCIE_BASE, (uintptr_t *)local_buf,
					 sizeof(local_buf), PCIE_OB_ANYMEM, HOST_TO_DEVICE));
	zassert_mem_equal(local_buf, host_mem0, sizeof(local_buf));

	/* Device to host: local contents land in the host aperture. */
	memcpy(local_buf, expected, sizeof(local_buf));
	memset(host_mem0, 0, sizeof(local_buf));
	zassert_ok(pcie_ep_xfer_data_dma(ep0, APERTURE0_PCIE_BASE, (uintptr_t *)local_buf,
					 sizeof(local_buf), PCIE_OB_ANYMEM, DEVICE_TO_HOST));
	zassert_mem_equal(host_mem0, expected, sizeof(expected));

	/* Unregistered host addresses fail without touching local memory. */
	memset(local_buf, 0, sizeof(local_buf));
	zassert_equal(pcie_ep_xfer_data_dma(ep0, 0x1000ULL, (uintptr_t *)local_buf,
					    sizeof(local_buf), PCIE_OB_ANYMEM, HOST_TO_DEVICE),
		      -ENOTSUP);
	zassert_mem_equal(local_buf, zeros, sizeof(local_buf));

	/* The mapping used by the helper is released after completion. */
	zassert_ok(pcie_ep_emul_unregister_aperture(ep0, APERTURE0_PCIE_BASE));
	zassert_ok(
		pcie_ep_emul_register_aperture(ep0, APERTURE0_PCIE_BASE, host_mem0, APERTURE_LEN));
}

ZTEST(pcie_ep_emul, test_dma_xfer_negative)
{
	uint8_t local_buf[0x40];
	uint64_t host_base = (uint64_t)(uintptr_t)host_mem0;
	struct dma_block_config steal_block = {
		.source_address = host_base,
		.dest_address = host_base + 0x100,
		.block_size = 0x10,
	};
	struct dma_config steal_cfg = {
		.dma_slot = 0,
		.channel_direction = MEMORY_TO_MEMORY,
		.source_data_size = 1,
		.dest_data_size = 1,
		.source_burst_length = 1,
		.dest_burst_length = 1,
		.block_count = 1,
		.head_block = &steal_block,
	};
	uint8_t zeros[sizeof(local_buf)] = {0};
	uint64_t mapped;

	memset(local_buf, 0, sizeof(local_buf));

	/* An invalid direction is rejected before anything else. */
	zassert_equal(pcie_ep_dma_xfer(ep0, host_base, (uintptr_t)local_buf, sizeof(local_buf),
				       (enum xfer_direction)2),
		      -EINVAL);

	/* Zero size and a NULL local pointer are rejected. */
	zassert_equal(pcie_ep_dma_xfer(ep0, host_base, (uintptr_t)local_buf, 0, HOST_TO_DEVICE),
		      -EINVAL);
	zassert_equal(pcie_ep_dma_xfer(ep0, host_base, 0, sizeof(local_buf), HOST_TO_DEVICE),
		      -EINVAL);

	/*
	 * A raw address inside the aperture without an active mapping
	 * fails, as does an address outside any aperture; neither touches
	 * local memory.
	 */
	zassert_equal(pcie_ep_dma_xfer(ep0, host_base, (uintptr_t)local_buf, sizeof(local_buf),
				       HOST_TO_DEVICE),
		      -ENOTSUP);
	zassert_equal(pcie_ep_dma_xfer(ep0, host_base - 0x1000, (uintptr_t)local_buf,
				       sizeof(local_buf), HOST_TO_DEVICE),
		      -ENOTSUP);
	zassert_mem_equal(local_buf, zeros, sizeof(local_buf));

	/* The helper rejects transfers crossing the aperture end, too. */
	zassert_equal(pcie_ep_xfer_data_dma(ep0, APERTURE0_PCIE_BASE + APERTURE_LEN - 0x20,
					    (uintptr_t *)local_buf, sizeof(local_buf),
					    PCIE_OB_ANYMEM, HOST_TO_DEVICE),
		      -ENOTSUP);

	/*
	 * Once unmapped, the stale address fails even though the aperture
	 * is still registered; a range crossing the mapping end fails, too.
	 */
	zassert_equal(pcie_ep_map_addr(ep0, APERTURE0_PCIE_BASE, &mapped, sizeof(local_buf),
				       PCIE_OB_ANYMEM),
		      sizeof(local_buf));
	zassert_equal(pcie_ep_dma_xfer(ep0, mapped + 0x20, (uintptr_t)local_buf, sizeof(local_buf),
				       HOST_TO_DEVICE),
		      -ENOTSUP);
	pcie_ep_unmap_addr(ep0, mapped);
	zassert_equal(pcie_ep_dma_xfer(ep0, mapped, (uintptr_t)local_buf, sizeof(local_buf),
				       HOST_TO_DEVICE),
		      -ENOTSUP);
	zassert_mem_equal(local_buf, zeros, sizeof(local_buf));

	/*
	 * A channel grabbed by another user is reported busy: occupy the
	 * host-to-device channel of ep0, leaving it configured, while a
	 * mapping is active.
	 */
	zassert_equal(pcie_ep_map_addr(ep0, APERTURE0_PCIE_BASE, &mapped, sizeof(local_buf),
				       PCIE_OB_ANYMEM),
		      sizeof(local_buf));
	zassert_ok(dma_config(dma_dev, 0, &steal_cfg));
	zassert_equal(pcie_ep_dma_xfer(ep0, mapped, (uintptr_t)local_buf, sizeof(local_buf),
				       HOST_TO_DEVICE),
		      -EBUSY);
	zassert_mem_equal(local_buf, zeros, sizeof(local_buf));

	/* Release the channel; the mapped transfer succeeds afterwards. */
	zassert_ok(dma_stop(dma_dev, 0));
	fill_pattern(host_mem0, sizeof(local_buf), 0x77);
	zassert_ok(pcie_ep_dma_xfer(ep0, mapped, (uintptr_t)local_buf, sizeof(local_buf),
				    HOST_TO_DEVICE));
	zassert_mem_equal(local_buf, host_mem0, sizeof(local_buf));
	pcie_ep_unmap_addr(ep0, mapped);
}

ZTEST(pcie_ep_emul, test_dma_xfer_no_dmas_instance)
{
	uint8_t local_buf[0x40] = {0};
	uint8_t zeros[sizeof(local_buf)] = {0};
	uint64_t host_base = (uint64_t)(uintptr_t)host_mem0;

	/* An invalid direction is rejected with -EINVAL even without dmas. */
	zassert_equal(pcie_ep_dma_xfer(ep2, host_base, (uintptr_t)local_buf, sizeof(local_buf),
				       (enum xfer_direction)2),
		      -EINVAL);

	/*
	 * ep2 has no dmas property: valid transfers in both directions
	 * report -ENODEV and leave local memory untouched.
	 */
	zassert_equal(pcie_ep_dma_xfer(ep2, host_base, (uintptr_t)local_buf, sizeof(local_buf),
				       HOST_TO_DEVICE),
		      -ENODEV);
	zassert_equal(pcie_ep_dma_xfer(ep2, host_base, (uintptr_t)local_buf, sizeof(local_buf),
				       DEVICE_TO_HOST),
		      -ENODEV);
	zassert_mem_equal(local_buf, zeros, sizeof(local_buf));
}

#define DMA_CONCURRENT_ITERS 32U

struct dma_xfer_worker {
	const struct device *ep;
	uint64_t pcie_base;
	uint8_t *host_mem;
	uint8_t seed;
	int failures;
};

static void dma_xfer_worker_body(void *p0, void *p1, void *p2)
{
	struct dma_xfer_worker *worker = p0;
	uint8_t local_buf[0x1c0];
	uint8_t expected[sizeof(local_buf)];

	ARG_UNUSED(p1);
	ARG_UNUSED(p2);

	for (int i = 0; i < DMA_CONCURRENT_ITERS; i++) {
		uint32_t off = (uint32_t)(i * 0x40U) % 0x800U;
		uint32_t len = 0x100U + (uint32_t)(i % 4U) * 0x40U;

		/* Host to device through the worker's own endpoint. */
		fill_pattern(&worker->host_mem[off], len, (uint8_t)(worker->seed + i));
		fill_pattern(expected, len, (uint8_t)(worker->seed + i));
		memset(local_buf, 0, len);
		if (pcie_ep_xfer_data_dma(worker->ep, worker->pcie_base + off,
					  (uintptr_t *)local_buf, len, PCIE_OB_ANYMEM,
					  HOST_TO_DEVICE) != 0 ||
		    memcmp(local_buf, expected, len) != 0) {
			worker->failures++;
			continue;
		}

		/* Device to host into a disjoint host region. */
		fill_pattern(local_buf, len, (uint8_t)(worker->seed ^ 0x5aU ^ i));
		memset(&worker->host_mem[off + 0x1000U], 0, len);
		if (pcie_ep_xfer_data_dma(worker->ep, worker->pcie_base + off + 0x1000U,
					  (uintptr_t *)local_buf, len, PCIE_OB_ANYMEM,
					  DEVICE_TO_HOST) != 0 ||
		    memcmp(&worker->host_mem[off + 0x1000U], local_buf, len) != 0) {
			worker->failures++;
		}
	}
}

static K_THREAD_STACK_DEFINE(dma_xfer_worker_stack, 4096);

ZTEST(pcie_ep_emul, test_dma_xfer_concurrent_two_endpoints)
{
	struct dma_xfer_worker worker0 = {
		.ep = ep0,
		.pcie_base = APERTURE0_PCIE_BASE,
		.host_mem = host_mem0,
		.seed = 0x21,
	};
	struct dma_xfer_worker worker1 = {
		.ep = ep1,
		.pcie_base = APERTURE1_PCIE_BASE,
		.host_mem = host_mem1,
		.seed = 0xa7,
	};
	struct k_thread thread;

	/*
	 * ep0 (channels 0/1) transfers from a worker thread while the test
	 * thread drives ep1 (channels 2/3). Every transfer blocks on its
	 * own completion callback, so both directions of both endpoints
	 * interleave on the controller workqueue. Distinct apertures and
	 * per-iteration patterns surface any cross-instance data or
	 * callback delivery as a corruption failure.
	 */
	k_thread_create(&thread, dma_xfer_worker_stack,
			K_THREAD_STACK_SIZEOF(dma_xfer_worker_stack), dma_xfer_worker_body,
			&worker0, NULL, NULL, K_PRIO_PREEMPT(1), 0, K_NO_WAIT);

	dma_xfer_worker_body(&worker1, NULL, NULL);

	zassert_ok(k_thread_join(&thread, K_FOREVER));
	zassert_equal(worker0.failures, 0, "concurrent ep0 transfers corrupted");
	zassert_equal(worker1.failures, 0, "concurrent ep1 transfers corrupted");
}

#define PIN_TEST_PCIE_BASE 0xa0000000ULL
#define PIN_TEST_XFER_LEN  0x400U

static uint8_t pin_test_mem[0x1000];

static struct {
	uint64_t mapped;
	int xfer_ret;
	bool data_ok;
} pin_xfer;

static K_THREAD_STACK_DEFINE(pin_xfer_stack, 4096);

static void pin_xfer_body(void *p0, void *p1, void *p2)
{
	uint8_t local_buf[PIN_TEST_XFER_LEN];
	uint8_t expected[sizeof(local_buf)];
	uint64_t mapped = 0;

	ARG_UNUSED(p0);
	ARG_UNUSED(p1);
	ARG_UNUSED(p2);

	fill_pattern(pin_test_mem, sizeof(local_buf), 0x33);
	fill_pattern(expected, sizeof(expected), 0x33);
	memset(local_buf, 0, sizeof(local_buf));

	pin_xfer.xfer_ret = pcie_ep_map_addr(ep0, PIN_TEST_PCIE_BASE, &mapped, sizeof(local_buf),
					     PCIE_OB_ANYMEM);
	if (pin_xfer.xfer_ret > 0) {
		pin_xfer.mapped = mapped;
		pin_xfer.xfer_ret = pcie_ep_dma_xfer(ep0, mapped, (uintptr_t)local_buf,
						     sizeof(local_buf), HOST_TO_DEVICE);
		pin_xfer.data_ok = pin_xfer.xfer_ret == 0 &&
				   memcmp(local_buf, expected, sizeof(local_buf)) == 0;
	}
}

ZTEST(pcie_ep_emul, test_dma_xfer_pins_mapping)
{
	uint64_t filler[CONFIG_PCIE_EP_EMUL_MAX_MAPS - 1];
	uint64_t free_addr;
	uint64_t mapped;
	struct k_thread xfer_thread;
	int unregister_active;
	int unregister_draining;
	int map_full;

	zassert_ok(pcie_ep_emul_register_aperture(ep0, PIN_TEST_PCIE_BASE, pin_test_mem,
						  sizeof(pin_test_mem)));

	/* Fill all map slots but the one the in-flight transfer will use. */
	for (int i = 0; i < CONFIG_PCIE_EP_EMUL_MAX_MAPS - 1; i++) {
		zassert_equal(pcie_ep_map_addr(ep0, APERTURE0_PCIE_BASE + i * 0x100U, &filler[i],
					       0x100U, PCIE_OB_ANYMEM),
			      0x100);
	}

	memset(&pin_xfer, 0, sizeof(pin_xfer));

	/*
	 * Both this test thread and the worker are cooperative, so the
	 * preemptible controller workqueue cannot run the transfer to
	 * completion while either of them is current. The worker starts
	 * when this thread yields and runs until it blocks inside the
	 * transfer on its completion semaphore, with the map pin held.
	 * The probes below then run while the transfer is deterministically
	 * in flight, and joining the worker blocks this thread again, which
	 * lets the workqueue complete the transfer.
	 */
	k_thread_create(&xfer_thread, pin_xfer_stack, K_THREAD_STACK_SIZEOF(pin_xfer_stack),
			pin_xfer_body, NULL, NULL, NULL, K_PRIO_COOP(1), 0, K_NO_WAIT);
	k_yield();

	/* The worker's mapping is active: unregistration is rejected. */
	unregister_active = pcie_ep_emul_unregister_aperture(ep0, PIN_TEST_PCIE_BASE);

	/* Unmap mid-transfer: the record drains but stays pinned. */
	pcie_ep_unmap_addr(ep0, pin_xfer.mapped);
	unregister_draining = pcie_ep_emul_unregister_aperture(ep0, PIN_TEST_PCIE_BASE);

	/* All other slots are filled; the draining slot must not be reused. */
	free_addr = APERTURE0_PCIE_BASE + (CONFIG_PCIE_EP_EMUL_MAX_MAPS - 1) * 0x100U;
	map_full = pcie_ep_map_addr(ep0, free_addr, &mapped, 0x100U, PCIE_OB_ANYMEM);

	/* Blocking on the join lets the workqueue complete the transfer. */
	zassert_ok(k_thread_join(&xfer_thread, K_FOREVER));

	/* Active and draining pins both block aperture unregistration. */
	zassert_equal(unregister_active, -EBUSY);
	zassert_equal(unregister_draining, -EBUSY);
	/* The draining slot is reserved: it is not handed out by map_addr. */
	zassert_equal(map_full, -ENOMEM);

	/* The transfer itself completed with its data intact throughout. */
	zassert_equal(pin_xfer.xfer_ret, 0);
	zassert_true(pin_xfer.data_ok, "in-flight transfer data corrupted");

	/* Once the transfer completed, unregistration succeeds. */
	zassert_ok(pcie_ep_emul_unregister_aperture(ep0, PIN_TEST_PCIE_BASE));

	/* The drained record is reusable after the transfer completed. */
	zassert_ok(pcie_ep_emul_register_aperture(ep0, PIN_TEST_PCIE_BASE, pin_test_mem,
						  sizeof(pin_test_mem)));
	zassert_equal(pcie_ep_map_addr(ep0, PIN_TEST_PCIE_BASE, &mapped, 0x100U, PCIE_OB_ANYMEM),
		      0x100);
	pcie_ep_unmap_addr(ep0, mapped);
	zassert_ok(pcie_ep_emul_unregister_aperture(ep0, PIN_TEST_PCIE_BASE));

	for (int i = 0; i < CONFIG_PCIE_EP_EMUL_MAX_MAPS - 1; i++) {
		pcie_ep_unmap_addr(ep0, filler[i]);
	}
}

static void *pcie_ep_emul_setup(void)
{
	zassert_ok(
		pcie_ep_emul_register_aperture(ep0, APERTURE0_PCIE_BASE, host_mem0, APERTURE_LEN));
	zassert_ok(
		pcie_ep_emul_register_aperture(ep1, APERTURE1_PCIE_BASE, host_mem1, APERTURE_LEN));
	return NULL;
}

static void pcie_ep_emul_before(void *fixture)
{
	struct pcie_ep_emul_irq_event event;

	ARG_UNUSED(fixture);

	memset(host_mem0, 0, sizeof(host_mem0));
	memset(host_mem1, 0, sizeof(host_mem1));

	/* Disable interrupt capabilities configured by earlier tests. */
	pcie_ep_conf_write(ep0, CFG_MSI_CAP, 0x0U);
	pcie_ep_conf_write(ep0, CFG_MSIX_CAP, 0x0U);
	pcie_ep_conf_write(ep1, CFG_MSI_CAP, 0x0U);

	/* Drop queued interrupt events and reset bookkeeping from earlier tests. */
	while (pcie_ep_emul_wait_irq_event(ep0, &event, K_NO_WAIT) == 0) {
	}
	while (pcie_ep_emul_wait_irq_event(ep1, &event, K_NO_WAIT) == 0) {
	}
	pcie_ep_conf_write(ep2, CFG_MSI_CAP, 0x0U);
	while (pcie_ep_emul_wait_irq_event(ep2, &event, K_NO_WAIT) == 0) {
	}
	memset(ep0_reset_records, 0, sizeof(ep0_reset_records));
	memset(ep1_reset_records, 0, sizeof(ep1_reset_records));
}

ZTEST_SUITE(pcie_ep_emul, NULL, pcie_ep_emul_setup, pcie_ep_emul_before, NULL, NULL);
