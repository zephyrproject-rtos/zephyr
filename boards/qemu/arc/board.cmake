set(SUPPORTED_EMU_PLATFORMS qemu)

set(QEMU_CPU_TYPE arc)

if(${CONFIG_SOC_QEMU_ARC_EM})
  set(QEMU_CPU_TYPE arcem)
  set(QEMU_BOARD_FLAGS -cpu arcem)
elseif(${CONFIG_SOC_QEMU_ARC_HS})
  set(QEMU_CPU_TYPE archs)
  set(QEMU_BOARD_FLAGS -cpu archs)
elseif(${CONFIG_SOC_QEMU_ARC_HS5X})
  set(QEMU_BINARY_SUFFIX arc)
  set(QEMU_CPU_TYPE hs5x)
  set(QEMU_BOARD_FLAGS -cpu hs5x)
elseif(${CONFIG_SOC_QEMU_ARC_HS6X})
  set(QEMU_BINARY_SUFFIX arc64)
  set(QEMU_CPU_TYPE hs6x)
  set(QEMU_BOARD_FLAGS -cpu hs6x)
endif()

# For old QEMU we had 'simhs' qemu board, however we are going to rename it
# to 'virt' board. It will be renamed in ARC QEMU in the nearest Zephyr SDK
# (where ARCv3 HS6x support will be added to QEMU)
# Let's rely on the QEMU defaults instead of specifying exact board name,
# until the updated Zephyr SDK will be set as default. By that we keep both SDKs
# (old and new) working for ARCv2.
# After that we can specify board explicitly with '-M virt' option.
list(APPEND QEMU_BOARD_FLAGS
  -m 8M
  -global cpu.firq=false
  -global cpu.num-irqlevels=15
  -global cpu.num-irq=25
  -global cpu.ext-irq=20
  -global cpu.freq_hz=10000000
  -global cpu.timer0=true
  -global cpu.timer1=true
  -global cpu.has-mpu=true
  -global cpu.mpu-numreg=16
  )

include(${ZEPHYR_BASE}/boards/common/qemu.board.cmake)
