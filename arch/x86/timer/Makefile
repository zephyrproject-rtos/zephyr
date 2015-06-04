ccflags-y +=-I$(srctree)/include/drivers
ifeq (${CONFIG_BSP_GENERIC_PC},y)
ccflags-y +=-I$(srctree)/arch/x86/generic_pc
endif

ifeq (${CONFIG_BSP_QUARK},y)
ccflags-y +=-I$(srctree)/arch/x86/quark
endif
ccflags-y +=-I$(srctree)/arch/x86

obj-$(CONFIG_HPET_TIMER) += hpet.o
obj-$(CONFIG_LOAPIC_TIMER) += loApicTimer.o
