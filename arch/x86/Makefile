PERF_OPT=$(if $(filter -Os,$(KBUILD_CFLAGS)),,y)

cflags-y += $(call cc-option,-ffunction-sections,) $(call cc-option,-fdata-sections,)
ifeq ($(PERF_OPT),y)
cflags-y += -DPERF_OPT
else
PREFERRED_STACK_BOUNDARY = y
endif

ifeq ($(COMPILER),clang)
biarch := $(call cc-option,-m32)
KBUILD_AFLAGS += $(biarch)
KBUILD_CFLAGS += $(biarch)
else
cflags-$(PREFERRED_STACK_BOUNDARY) += $(call cc-option,-mpreferred-stack-boundary=2)
endif
cflags-$(CONFIG_LTO) += $(call cc-option,-flto,)

ifneq ($(CONFIG_SSE_FP_MATH),y)
cflags-y += -mno-sse
endif

include $(srctree)/arch/$(ARCH)/soc/$(SOC_NAME)/Makefile

KBUILD_CXXFLAGS += $(cflags-y)
KBUILD_CFLAGS += $(cflags-y)

QEMU_BIOS ?= /usr/share/qemu
QEMU_CPU_TYPE_x86 = qemu32
QEMU_X86_NO_REBOOT_y =
QEMU_X86_NO_REBOOT_  = -no-reboot
QEMU_FLAGS_x86 = -m 32 -cpu $(QEMU_CPU_TYPE_x86) \
		$(QEMU_X86_NO_REBOOT_$(CONFIG_REBOOT)) \
		-nographic -display none -net none \
		-clock dynticks -no-acpi -balloon none \
		-L $(QEMU_BIOS) -bios bios.bin \
		-machine type=pc-0.14
QEMU_x86 = qemu-system-i386

export QEMU_FLAGS_x86 QEMU_x86 QEMU_CPU_TYPE_x86
