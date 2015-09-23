

PERF_OPT=$(if $(filter -Os,$(KBUILD_CFLAGS)),,y)

cflags-y += $(call cc-option,-ffunction-sections,) $(call cc-option,-fdata-sections,)
cflags-$(PERF_OPT) += $(call cc-option,-mpreferred-stack-boundary=2)
cflags-$(PERF_OPT) += -DPERF_OPT
cflags-$(CONFIG_LTO) += $(call cc-option,-flto,)


ifneq ($(CONFIG_SSE_FP_MATH),y)
cflags-y += -mno-sse
endif

arch-$(CONFIG_CPU_ATOM) += $(call cc-option,-march=atom) \
	$(call cc-option,-mtune=atom,$(call cc-option,-mtune=generic))
arch-$(CONFIG_CPU_MINUTEIA) += $(call cc-option,-march=pentium)

KBUILD_CFLAGS += $(cflags-y) $(arch-y)
KBUILD_AFLAGS += $(arch-y)


QEMU_BIOS ?= /usr/share/qemu
QEMU_CPU_TYPE_x86 = qemu32
QEMU_FLAGS_x86 = -m 32 -cpu $(QEMU_CPU_TYPE_x86) \
		-no-reboot -nographic -display none -net none \
		-clock dynticks -no-acpi -balloon none \
		-L $(QEMU_BIOS) -bios bios.bin \
		-machine type=pc-0.14
QEMU_x86 = qemu-system-i386

export QEMU_FLAGS_x86 QEMU_x86 QEMU_CPU_TYPE_x86
