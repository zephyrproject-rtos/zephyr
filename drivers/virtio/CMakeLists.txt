# Copyright (c) 2024 Antmicro <www.antmicro.com>
# SPDX-License-Identifier: Apache-2.0

zephyr_library()

zephyr_library_sources_ifdef(CONFIG_VIRTIO virtqueue.c virtio_common.c)
zephyr_library_sources_ifdef(CONFIG_VIRTIO_PCI virtio_pci.c)
zephyr_library_sources_ifdef(CONFIG_VIRTIO_MMIO virtio_mmio.c)
