PERF_OPT=$(if $(filter -Os,$(KBUILD_CFLAGS)),,y)

cflags-y += $(call cc-option,-ffunction-sections,) $(call cc-option,-fdata-sections,)
ifeq ($(PERF_OPT),y)
cflags-y += -DPERF_OPT
else
PREFERRED_STACK_BOUNDARY = y
endif

ifeq ($(COMPILER),clang)
ifeq ($(CONFIG_X86_IAMCU),)
KBUILD_CPPFLAGS += $(call cc-option,-Qunused-arguments,)
else
CC_M32 = -m32 -mmcu
KBUILD_CFLAGS   += -mmcu
KBUILD_CPPFLAGS += -mmcu
endif
biarch := $(call cc-option,-m32)
dwarf2 := $(call cc-option,-gdwarf-2)
KBUILD_AFLAGS += $(biarch)
KBUILD_CFLAGS += $(biarch)
KBUILD_CFLAGS += $(dwarf2)
else
cflags-$(PREFERRED_STACK_BOUNDARY) += $(call cc-option,-mpreferred-stack-boundary=2)
endif
cflags-$(CONFIG_LTO) += $(call cc-option,-flto,)

ifneq ($(CONFIG_SSE_FP_MATH),y)
cflags-y += -mno-sse
endif

include $(srctree)/arch/$(ARCH)/soc/$(SOC_PATH)/Makefile

KBUILD_CXXFLAGS += $(cflags-y)
KBUILD_CFLAGS += $(cflags-y)

soc-cxxflags ?= $(soc-cflags)
soc-aflags ?= $(soc-cflags)
KBUILD_CFLAGS += $(soc-cflags)
KBUILD_CXXFLAGS += $(soc-cxxflags)
KBUILD_AFLAGS += $(soc-aflags)

QEMU_BIOS ?= /usr/share/qemu
QEMU_CPU_TYPE_x86 = qemu32
QEMU_X86_NO_REBOOT_y =
QEMU_X86_NO_REBOOT_  = -no-reboot
QEMU_FLAGS_x86 = -m 32 -cpu $(QEMU_CPU_TYPE_x86) \
		$(QEMU_X86_NO_REBOOT_$(CONFIG_REBOOT)) \
		-nographic -vga none -display none -net none \
		-clock dynticks -no-acpi -balloon none \
		-L $(QEMU_BIOS) -bios bios.bin \
		-machine type=pc-0.14
QEMU_x86 = qemu-system-i386

export QEMU_FLAGS_x86 QEMU_x86 QEMU_CPU_TYPE_x86
