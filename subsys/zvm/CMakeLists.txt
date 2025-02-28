# Copyright 2024-2025 HNU-ESNL: Guoqi Xie, Chenglai Xiong, Xingyu Hu and etc.
# Copyright 2024-2025 openEuler SIG-Zephyr
# SPDX-License-Identifier: Apache-2.0

add_subdirectory_ifdef(CONFIG_ZVM vdev)

zephyr_sources_ifdef(
    CONFIG_ZVM
    vm_cpu.c
    vm_device.c
    vm_irq.c
    vm_manager.c
    vm_mm.c
    vm.c
    os.c
    zvm.c
    zvm_shell.c
)
