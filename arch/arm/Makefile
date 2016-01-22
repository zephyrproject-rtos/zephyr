KBUILD_CFLAGS += $(call cc-option,-ffunction-sections,) $(call cc-option,-fdata-sections,)

arch-$(CONFIG_CPU_CORTEX_M3) += $(call cc-option,-mthumb -mcpu=cortex-m3) \
		$(call cc-option,-mthumb -march=armv7-m)
arch-$(CONFIG_CPU_CORTEX_M4) += $(call cc-option,-mthumb -mcpu=cortex-m4) \
		$(call cc-option,-mthumb -march=armv7e-m)

cflags-$(CONFIG_LTO) = $(call cc-option,-flto,)

KBUILD_CFLAGS += $(cflags-y) $(arch-y)
KBUILD_AFLAGS += $(arch-y)

QEMU_CPU_TYPE_arm = cortex-m3
QEMU_FLAGS_arm = -cpu  $(QEMU_CPU_TYPE_arm) \
		-machine lm3s6965evb -nographic
QEMU_arm = qemu-system-arm

export QEMU_FLAGS_arm QEMU_arm
