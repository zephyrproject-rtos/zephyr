set(EMU_PLATFORM qemu)

set(QEMU_CPU_TYPE_${ARCH} arc)

if(${CONFIG_SOC_QEMU_ARC_EM})
set(QEMU_CPU_TYPE_${ARCH} arcem)
set(QEMU_FLAGS_${ARCH} -cpu arcem)
elseif(${CONFIG_SOC_QEMU_ARC_HS})
set(QEMU_CPU_TYPE_${ARCH} archs)
set(QEMU_FLAGS_${ARCH} -cpu archs)
endif()

list(APPEND QEMU_FLAGS_${ARCH}
  -M simhs
  -m 8M
  -nographic
  -no-reboot
  -monitor none
  -global cpu.firq=false
  -global cpu.num-irqlevels=15
  -global cpu.num-irq=25
  -global cpu.ext-irq=20
  -global cpu.freq_hz=1000000
  -global cpu.timer0=true
  -global cpu.timer1=true
  )

set(BOARD_DEBUG_RUNNER qemu)
