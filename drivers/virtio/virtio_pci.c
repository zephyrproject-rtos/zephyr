/*
 * Copyright (c) 2024 Antmicro <www.antmicro.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/device.h>
#include <zephyr/drivers/pcie/pcie.h>
#include <zephyr/kernel/mm.h>
#include <zephyr/logging/log.h>
#include <zephyr/spinlock.h>
#include <zephyr/sys/barrier.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/drivers/virtio.h>
#include <zephyr/drivers/virtio/virtqueue.h>
#include "virtio_common.h"
#include "assert.h"

#define DT_DRV_COMPAT virtio_pci

LOG_MODULE_REGISTER(virtio_pci, CONFIG_VIRTIO_LOG_LEVEL);

/*
 * Based on Virtual I/O Device (VIRTIO) Version 1.3 specification:
 * https://docs.oasis-open.org/virtio/virtio/v1.3/csd01/virtio-v1.3-csd01.pdf
 */

struct virtio_pci_cap {
	uint8_t cap_vndr;
	uint8_t cap_next;
	uint8_t cap_len;
	uint8_t cfg_type;
	uint8_t bar;
	uint8_t id;
	uint8_t pad[2];
	uint32_t offset;
	uint32_t length;
};

struct virtio_pci_notify_cap {
	struct virtio_pci_cap cap;
	uint32_t notify_off_multiplier;
};

struct virtio_pci_common_cfg {
	uint32_t device_feature_select; /* read-write */
	uint32_t device_feature;        /* read-only for driver */
	uint32_t driver_feature_select; /* read-write */
	uint32_t driver_feature;        /* read-write */
	uint16_t config_msix_vector;    /* read-write */
	uint16_t num_queues;            /* read-only for driver */
	uint8_t device_status;          /* read-write */
	uint8_t config_generation;      /* read-only for driver */

	uint16_t queue_select;      /* read-write */
	uint16_t queue_size;        /* read-write */
	uint16_t queue_msix_vector; /* read-write */
	uint16_t queue_enable;      /* read-write */
	uint16_t queue_notify_off;  /* read-only for driver */
	uint64_t queue_desc;        /* read-write */
	uint64_t queue_driver;      /* read-write */
	uint64_t queue_device;      /* read-write */
	uint16_t queue_notify_data; /* read-only for driver */
	uint16_t queue_reset;       /* read-write */

	uint16_t admin_queue_index; /* read-only for driver */
	uint16_t admin_queue_num;   /* read-only for driver */
};

#define VIRTIO_PCI_CAP_COMMON_CFG 1
#define VIRTIO_PCI_CAP_NOTIFY_CFG 2
#define VIRTIO_PCI_CAP_ISR_CFG 3
#define VIRTIO_PCI_CAP_DEVICE_CFG 4
#define VIRTIO_PCI_CAP_PCI_CFG 5
#define VIRTIO_PCI_CAP_SHARED_MEMORY_CFG 8
#define VIRTIO_PCI_CAP_VENDOR_CFG 9

#define CABABILITY_LIST_VALID_BIT 4
#define STATUS_COMMAND_REG 0x1
#define CAPABILITIES_POINTER_REG 0xd
#define CAPABILITIES_POINTER_MASK 0xfc

#define VIRTIO_PCI_MSIX_NO_VECTOR 0xffff

struct virtio_pci_data {
	volatile struct virtio_pci_common_cfg *common_cfg;
	void *device_specific_cfg;
	volatile uint8_t *isr_status;
	volatile uint8_t *notify_cfg;
	uint32_t notify_off_multiplier;

	struct virtq *virtqueues;
	uint16_t virtqueue_count;

	struct k_spinlock isr_lock;
	struct k_spinlock notify_lock;
};

struct virtio_pci_config {
	struct pcie_dev *pcie;
};

/*
 * Even though a virtio device is exposed as a PCI device, it's not a physical
 * one, so we don't have to care about cache flushing/invalidating like we would
 * with a real device that may write to the memory from the outside. Whatever
 * will be written/read to shared memory by the virtio device will be
 * written/read by a hypervisor running on the same cpu as zephyr guest, so the
 * caches will stay coherent
 */

void virtio_pci_isr(const struct device *dev)
{
	struct virtio_pci_data *data = dev->data;
	k_spinlock_key_t key = k_spin_lock(&data->isr_lock);

	virtio_isr(dev, *data->isr_status, data->virtqueue_count);

	k_spin_unlock(&data->isr_lock, key);
}

static bool virtio_pci_read_cap(
	pcie_bdf_t bdf, uint8_t cfg_type, void *cap_struct, size_t cap_struct_size)
{
	struct virtio_pci_cap tmp;
	uint16_t status = (pcie_conf_read(bdf, STATUS_COMMAND_REG) & GENMASK(31, 16)) >> 16;

	if (!(status & BIT(CABABILITY_LIST_VALID_BIT))) {
		LOG_ERR("no capability list for device with bdf 0x%x", bdf);
		return false;
	}

	uint32_t cap_ptr =
		pcie_conf_read(bdf, CAPABILITIES_POINTER_REG) & CAPABILITIES_POINTER_MASK;
	uint32_t cap_off = cap_ptr / sizeof(uint32_t);

	/*
	 * Every capability type struct has size and alignment of multiple of 4 bytes
	 * so pcie_conf_read() can be used directly without aligning
	 */
	do {
		for (int i = 0; i < sizeof(struct virtio_pci_cap) / sizeof(uint32_t); i++) {
			((uint32_t *)&tmp)[i] = pcie_conf_read(bdf, cap_off + i);
		}
		if (tmp.cfg_type == cfg_type) {
			assert(tmp.cap_len == cap_struct_size);
			size_t extra_data_words =
				(tmp.cap_len - sizeof(struct virtio_pci_cap)) / sizeof(uint32_t);
			size_t extra_data_offset =
				cap_off + sizeof(struct virtio_pci_cap) / sizeof(uint32_t);
			uint32_t *extra_data =
				(uint32_t *)((struct virtio_pci_cap *)cap_struct + 1);

			*(struct virtio_pci_cap *)cap_struct = tmp;

			for (int i = 0; i < extra_data_words; i++) {
				extra_data[i] = pcie_conf_read(bdf, extra_data_offset + i);
			}

			return true;
		}

		cap_off = (tmp.cap_next & 0xfc) / sizeof(uint32_t);
	} while (cap_off != 0);

	return false;
}

static void virtio_pci_reset(const struct device *dev)
{
	struct virtio_pci_data *data = dev->data;

	/*
	 * According to spec 4.1.4.3.1 and spec 2.4.2 to reset the device we
	 * must write 0 to the device_status register and wait until we read 0
	 * from it, which means that reset is complete
	 */
	data->common_cfg->device_status = 0;

	while (data->common_cfg->device_status != 0) {
	}
}

static void virtio_pci_notify_queue(const struct device *dev, uint16_t queue_idx)
{
	struct virtio_pci_data *data = dev->data;

	k_spinlock_key_t key = k_spin_lock(&data->notify_lock);

	data->common_cfg->queue_select = sys_cpu_to_le16(queue_idx);
	barrier_dmem_fence_full();
	/*
	 * Because currently we are not negotiating VIRTIO_F_NOTIFICATION_DATA
	 * and VIRTIO_F_NOTIF_CONFIG_DATA, in order to notify the queue we have
	 * to write its index to notify_cfg at the offset
	 * cap.offset + queue_notify_off * notify_off_multiplier,
	 * which here is reduced to queue_notify_off * notify_off_multiplier,
	 * because data->notify_cfg was mapped in virtio_pci_map_cap() to start
	 * at cap.offset. See spec 4.1.4.4 for the offset formula, and spec
	 * 4.1.5.2 and spec 4.1.5.2.1 for the value written
	 */
	size_t notify_off =
		sys_le16_to_cpu(data->common_cfg->queue_notify_off) * data->notify_off_multiplier;
	volatile uint16_t *notify_addr = (uint16_t *)(data->notify_cfg + notify_off);

	*notify_addr = sys_cpu_to_le16(queue_idx);

	k_spin_unlock(&data->notify_lock, key);
}

/*
 * According to the spec 4.1.3.1, PCI virtio driver must use n byte accesses for n byte fields,
 * except for 64 bit fields where 32 bit accesses have to be used, so we are using this
 * function to write 64 bit values to 64 bit fields
 */
static void virtio_pci_write64(uint64_t val, uint64_t *dst)
{
	uint64_t val_le = sys_cpu_to_le64(val);

	((uint32_t *)dst)[0] = val_le & GENMASK64(31, 0);
	((uint32_t *)dst)[1] = (val_le & GENMASK64(63, 32)) >> 32;
}

static int virtio_pci_set_virtqueue(
	const struct device *dev, uint16_t virtqueue_n, struct virtq *virtqueue)
{
	struct virtio_pci_data *data = dev->data;

	data->common_cfg->queue_select = sys_cpu_to_le16(virtqueue_n);
	barrier_dmem_fence_full();

	uint16_t max_queue_size = sys_le16_to_cpu(data->common_cfg->queue_size);

	if (max_queue_size < virtqueue->num) {
		LOG_ERR(
			"virtio pci device doesn't support queue %d bigger than %d, tried to set one with size %d",
			virtqueue_n,
			max_queue_size,
			virtqueue->num
		);
		return -EINVAL;
	}
	data->common_cfg->queue_size = sys_cpu_to_le16(virtqueue->num);
	virtio_pci_write64(
		k_mem_phys_addr(virtqueue->desc), (void *)&data->common_cfg->queue_desc
	);
	virtio_pci_write64(
		k_mem_phys_addr(virtqueue->avail), (void *)&data->common_cfg->queue_driver
	);
	virtio_pci_write64(
		k_mem_phys_addr(virtqueue->used), (void *)&data->common_cfg->queue_device
	);
	data->common_cfg->queue_msix_vector = sys_cpu_to_le16(VIRTIO_PCI_MSIX_NO_VECTOR);
	data->common_cfg->queue_enable = sys_cpu_to_le16(1);

	return 0;
}

static int virtio_pci_init_virtqueues(
	const struct device *dev, uint16_t num_queues, virtio_enumerate_queues cb, void *opaque)
{
	struct virtio_pci_data *data = dev->data;
	uint16_t queue_count = sys_le16_to_cpu(data->common_cfg->num_queues);

	if (num_queues > queue_count) {
		LOG_ERR("requested more virtqueues than available");
		return -EINVAL;
	}

	data->virtqueues = k_malloc(queue_count * sizeof(struct virtq));
	if (!data->virtqueues) {
		LOG_ERR("failed to allocate virtqueue array");
		return -ENOMEM;
	}
	data->virtqueue_count = queue_count;

	int ret = 0;
	int created_queues = 0;
	int activated_queues = 0;

	for (int i = 0; i < queue_count; i++) {
		data->common_cfg->queue_select = sys_cpu_to_le16(i);
		barrier_dmem_fence_full();

		uint16_t queue_size = cb(i, sys_le16_to_cpu(data->common_cfg->queue_size), opaque);

		ret = virtq_create(&data->virtqueues[i], queue_size);
		if (ret != 0) {
			goto fail;
		}
		created_queues++;

		ret = virtio_pci_set_virtqueue(dev, i, &data->virtqueues[i]);
		if (ret != 0) {
			goto fail;
		}
		activated_queues++;
	}

	return 0;

fail:
	for (int j = 0; j < activated_queues; j++) {
		data->common_cfg->queue_select = sys_cpu_to_le16(j);
		barrier_dmem_fence_full();
		data->common_cfg->queue_enable = sys_cpu_to_le16(0);
	}
	for (int j = 0; j < created_queues; j++) {
		virtq_free(&data->virtqueues[j]);
	}
	k_free(data->virtqueues);
	data->virtqueue_count = 0;

	return ret;
}

static bool virtio_pci_map_cap(pcie_bdf_t bdf, struct virtio_pci_cap *cap, void **virt_ptr)
{
	struct pcie_bar mbar;

	if (!pcie_get_mbar(bdf, cap->bar, &mbar)) {
		LOG_ERR("no mbar for capability type %d found", cap->cfg_type);
		return false;
	}
	assert(mbar.phys_addr + cap->offset + cap->length <= mbar.phys_addr + mbar.size);

#ifdef CONFIG_MMU
	k_mem_map_phys_bare(
		(uint8_t **)virt_ptr, mbar.phys_addr + cap->offset, cap->length, K_MEM_PERM_RW
	);
#else
	*virt_ptr = (void *)(mbar.phys_addr + cap->offset);
#endif

	return true;
}

static uint32_t virtio_pci_read_device_feature_word(const struct device *dev, uint32_t word_n)
{
	struct virtio_pci_data *data = dev->data;

	data->common_cfg->device_feature_select = sys_cpu_to_le32(word_n);
	barrier_dmem_fence_full();
	return sys_le32_to_cpu(data->common_cfg->device_feature);
}

static uint32_t virtio_pci_read_driver_feature_word(const struct device *dev, uint32_t word_n)
{
	struct virtio_pci_data *data = dev->data;

	data->common_cfg->driver_feature_select = sys_cpu_to_le32(word_n);
	barrier_dmem_fence_full();
	return sys_le32_to_cpu(data->common_cfg->driver_feature);
}

static void virtio_pci_write_driver_feature_word(
	const struct device *dev, uint32_t word_n, uint32_t val)
{
	struct virtio_pci_data *data = dev->data;

	data->common_cfg->driver_feature_select = sys_cpu_to_le32(word_n);
	barrier_dmem_fence_full();
	data->common_cfg->driver_feature = sys_cpu_to_le32(val);
}

static bool virtio_pci_read_device_feature_bit(const struct device *dev, int bit)
{
	uint32_t word_n = bit / 32;
	uint32_t mask = BIT(bit % 32);

	return virtio_pci_read_device_feature_word(dev, word_n) & mask;
}

static void virtio_pci_write_driver_feature_bit(const struct device *dev, int bit, bool value)
{
	uint32_t word_n = bit / 32;
	uint32_t mask = BIT(bit % 32);
	uint32_t word = virtio_pci_read_driver_feature_word(dev, word_n);

	virtio_pci_write_driver_feature_word(dev, word_n, value ? word | mask : word & ~mask);
}

static int virtio_pci_write_driver_feature_bit_range_check(
	const struct device *dev, int bit, bool value)
{
	if (!IN_RANGE(bit, DEV_TYPE_FEAT_RANGE_0_BEGIN, DEV_TYPE_FEAT_RANGE_0_END) &&
	    !IN_RANGE(bit, DEV_TYPE_FEAT_RANGE_1_BEGIN, DEV_TYPE_FEAT_RANGE_1_END)) {
		return -EINVAL;
	}

	virtio_pci_write_driver_feature_bit(dev, bit, value);

	return 0;
}

static bool virtio_pci_read_status_bit(const struct device *dev, int bit)
{
	struct virtio_pci_data *data = dev->data;
	uint32_t mask = BIT(bit);

	barrier_dmem_fence_full();
	return sys_le32_to_cpu(data->common_cfg->device_status) & mask;
}

static void virtio_pci_write_status_bit(const struct device *dev, int bit)
{
	struct virtio_pci_data *data = dev->data;
	uint32_t mask = BIT(bit);

	barrier_dmem_fence_full();
	data->common_cfg->device_status |= sys_cpu_to_le32(mask);
}

static int virtio_pci_init_common(const struct device *dev)
{
	const struct virtio_pci_config *conf = dev->config;
	struct virtio_pci_data *data = dev->data;
	struct virtio_pci_cap vpc;
	struct virtio_pci_notify_cap vpnc = { .notify_off_multiplier = 0 };

	if (conf->pcie->bdf == PCIE_BDF_NONE) {
		LOG_ERR("no virtio pci device with id 0x%x on the bus", conf->pcie->id);
		return 1;
	}
	LOG_INF(
		"found virtio pci device with id 0x%x and bdf 0x%x", conf->pcie->id, conf->pcie->bdf
	);

	if (virtio_pci_read_cap(conf->pcie->bdf, VIRTIO_PCI_CAP_COMMON_CFG, &vpc, sizeof(vpc))) {
		if (!virtio_pci_map_cap(conf->pcie->bdf, &vpc, (void **)&data->common_cfg)) {
			return 1;
		}
	} else {
		LOG_ERR(
			"no VIRTIO_PCI_CAP_COMMON_CFG for the device with id 0x%x and bdf 0x%x, legacy device?",
			conf->pcie->id,
			conf->pcie->bdf
		);
		return 1;
	}

	if (virtio_pci_read_cap(conf->pcie->bdf, VIRTIO_PCI_CAP_ISR_CFG, &vpc, sizeof(vpc))) {
		if (!virtio_pci_map_cap(conf->pcie->bdf, &vpc, (void **)&data->isr_status)) {
			return 1;
		}
	} else {
		LOG_ERR(
			"no VIRTIO_PCI_CAP_ISR_CFG for the device with id 0x%x and bdf 0x%x",
			conf->pcie->id,
			conf->pcie->bdf
		);
		return 1;
	}

	if (virtio_pci_read_cap(conf->pcie->bdf, VIRTIO_PCI_CAP_NOTIFY_CFG, &vpnc, sizeof(vpnc))) {
		if (!virtio_pci_map_cap(
				conf->pcie->bdf, (struct virtio_pci_cap *)&vpnc,
				(void **)&data->notify_cfg)) {
			return 1;
		}
		data->notify_off_multiplier = sys_le32_to_cpu(vpnc.notify_off_multiplier);
	} else {
		LOG_ERR(
			"no VIRTIO_PCI_CAP_NOTIFY_CFG for the device with id 0x%x and bdf 0x%x",
			conf->pcie->id,
			conf->pcie->bdf
		);
		return 1;
	}

	/*
	 * Some of the device types may present VIRTIO_PCI_CAP_DEVICE_CFG capabilities as per spec
	 * 4.1.4.6. It states that there may be more than one such capability per device, however
	 * none of the devices specified in the Device Types (chapter 5) state that they need more
	 * than one (its always one or zero virtio_devtype_config structs), so we are just trying to
	 * read the first one
	 */
	if (virtio_pci_read_cap(conf->pcie->bdf, VIRTIO_PCI_CAP_DEVICE_CFG, &vpc, sizeof(vpc))) {
		if (!virtio_pci_map_cap(
				conf->pcie->bdf, &vpc, (void **)&data->device_specific_cfg)) {
			return 1;
		}
	} else {
		data->device_specific_cfg = NULL;
		LOG_INF(
			"no VIRTIO_PCI_CAP_DEVICE_CFG for the device with id 0x%x and bdf 0x%x",
			conf->pcie->id,
			conf->pcie->bdf
		);
	}

	/*
	 * The device initialization goes as follows (see 3.1.1):
	 * - first we have to reset the device
	 * - then we have to write ACKNOWLEDGE bit
	 * - then we have to write DRIVER bit
	 * - after that negotiation of feature flags take place, currently this driver only needs
	 *   VIRTIO_F_VERSION_1, the rest of flags is left to negotiate to the specific devices via
	 *   this driver's api that must be finalized with commit_feature_bits() that writes
	 *   FEATURES_OK bit
	 * - next the virtqueues have to be set, again via this driver's api (init_virtqueues())
	 * - initialization is finalized by writing DRIVER_OK bit, which is done by
	 *   finalize_init() from api
	 */

	virtio_pci_reset(dev);

	virtio_pci_write_status_bit(dev, DEVICE_STATUS_ACKNOWLEDGE);
	virtio_pci_write_status_bit(dev, DEVICE_STATUS_DRIVER);

	LOG_INF(
		"virtio pci device with id 0x%x and bdf 0x%x advertised "
		"feature bits: 0x%.8x%.8x%.8x%.8x",
		conf->pcie->id,
		conf->pcie->bdf,
		virtio_pci_read_device_feature_word(dev, 3),
		virtio_pci_read_device_feature_word(dev, 2),
		virtio_pci_read_device_feature_word(dev, 1),
		virtio_pci_read_device_feature_word(dev, 0)
	);

	/*
	 * In case of PCI this should never happen because legacy device would've been caught
	 * earlier in VIRTIO_PCI_CAP_COMMON_CFG check as this capability shouldn't be present
	 * in legacy devices, but we are leaving it here as a sanity check
	 */
	if (!virtio_pci_read_device_feature_bit(dev, VIRTIO_F_VERSION_1)) {
		LOG_ERR(
			"virtio pci device with id 0x%x and bdf 0x%x doesn't advertise "
			"VIRTIO_F_VERSION_1 feature support",
			conf->pcie->id,
			conf->pcie->bdf
		);
		return 1;
	}

	virtio_pci_write_driver_feature_bit(dev, VIRTIO_F_VERSION_1, 1);

	return 0;
};

struct virtq *virtio_pci_get_virtqueue(const struct device *dev, uint16_t queue_idx)
{
	struct virtio_pci_data *data = dev->data;

	return queue_idx < data->virtqueue_count ? &data->virtqueues[queue_idx] : NULL;
}

void *virtio_pci_get_device_specific_config(const struct device *dev)
{
	struct virtio_pci_data *data = dev->data;

	return data->device_specific_cfg;
}

void virtio_pci_finalize_init(const struct device *dev)
{
	virtio_pci_write_status_bit(dev, DEVICE_STATUS_DRIVER_OK);
}

int virtio_pci_commit_feature_bits(const struct device *dev)
{
	const struct virtio_pci_config *conf = dev->config;

	virtio_pci_write_status_bit(dev, DEVICE_STATUS_FEATURES_OK);
	if (!virtio_pci_read_status_bit(dev, DEVICE_STATUS_FEATURES_OK)) {
		LOG_ERR(
			"virtio pci device with id 0x%x and bdf 0x%x doesn't support selected "
			"feature bits: 0x%.8x%.8x%.8x%.8x",
			conf->pcie->id,
			conf->pcie->bdf,
			virtio_pci_read_device_feature_word(dev, 3),
			virtio_pci_read_device_feature_word(dev, 2),
			virtio_pci_read_device_feature_word(dev, 1),
			virtio_pci_read_device_feature_word(dev, 0)
		);
		return -EINVAL;
	}

	return 0;
}

static DEVICE_API(virtio, virtio_pci_driver_api) = {
	.get_virtqueue = virtio_pci_get_virtqueue,
	.notify_virtqueue = virtio_pci_notify_queue,
	.get_device_specific_config = virtio_pci_get_device_specific_config,
	.read_device_feature_bit = virtio_pci_read_device_feature_bit,
	.write_driver_feature_bit = virtio_pci_write_driver_feature_bit_range_check,
	.commit_feature_bits = virtio_pci_commit_feature_bits,
	.init_virtqueues = virtio_pci_init_virtqueues,
	.finalize_init = virtio_pci_finalize_init
};

#define VIRTIO_PCI_DEFINE(inst)                                                     \
	BUILD_ASSERT(DT_NODE_HAS_COMPAT(DT_INST_PARENT(inst), pcie_controller));        \
	DEVICE_PCIE_INST_DECLARE(inst);                                                 \
	static struct virtio_pci_data virtio_pci_data##inst;                            \
	static struct virtio_pci_config virtio_pci_config##inst = {                     \
		DEVICE_PCIE_INST_INIT(inst, pcie)                                           \
	};                                                                              \
	static int virtio_pci_init##inst(const struct device *dev)                      \
	{                                                                               \
		IRQ_CONNECT(                                                                \
			DT_INST_IRQN(inst), DT_INST_IRQ(inst, priority), virtio_pci_isr,        \
			DEVICE_DT_INST_GET(inst), 0                                             \
		);                                                                          \
		int ret = virtio_pci_init_common(dev);                                      \
		irq_enable(DT_INST_IRQN(inst));                                             \
		return ret;                                                                 \
	}                                                                               \
	DEVICE_DT_INST_DEFINE(                                                          \
		inst,                                                                       \
		virtio_pci_init##inst,                                                      \
		NULL,                                                                       \
		&virtio_pci_data##inst,                                                     \
		&virtio_pci_config##inst,                                                   \
		POST_KERNEL,                                                                \
		0,                                                                          \
		&virtio_pci_driver_api                                                      \
	);

DT_INST_FOREACH_STATUS_OKAY(VIRTIO_PCI_DEFINE)
