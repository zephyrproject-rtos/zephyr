set(EMU_PLATFORM qemu)

set(QEMU_binary_suffix x86_64)

# QEMU, in Jailhouse's case, is not emulating Zephyr's hardware
# directly, but the root cell/system, with a mount point reaching
# the kernel itself available
set(QEMU_KERNEL_OPTION ";")

if(NOT CONFIG_REBOOT)
  set(REBOOT_FLAG -no-reboot)
endif()

if (NOT DEFINED JAILHOUSE_QEMU_IMG_FILE)
  message(FATAL_ERROR "You have to provide a valid qcow2 QEMU image to be a Jailhouse root cell (variable JAILHOUSE_QEMU_IMG_FILE)!")
endif()

set(QEMU_CPU_TYPE_${ARCH} kvm64,-kvm_pv_eoi,-kvm_steal_time,-kvm_asyncpf,-kvmclock,+vmx,+x2apic,+arat)
set(QEMU_FLAGS_${ARCH}
  -machine q35,kernel_irqchip=split
  ${REBOOT_FLAG}
  -m 1G
  -enable-kvm
  -smp 4
  -device intel-iommu,intremap=on,x-buggy-eim=on
  -cpu ${QEMU_CPU_TYPE_${ARCH}}
  -drive file=${JAILHOUSE_QEMU_IMG_FILE},format=qcow2,id=disk,if=none
  -device ide-hd,drive=disk
  -netdev user,id=net
  -device e1000e,addr=2.0,netdev=net
  -device intel-hda,addr=1b.0
  -device hda-duplex
  -fsdev local,path=./zephyr,security_model=passthrough,id=vfs
  -device virtio-9p-pci,addr=1f.7,mount_tag=host,fsdev=vfs
  )

# TODO: Support debug
# set(BOARD_DEBUG_RUNNER qemu)
# debugserver: QEMU_EXTRA_FLAGS += -s -S
# debugserver: qemu
