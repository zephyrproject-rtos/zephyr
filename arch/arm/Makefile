KBUILD_CFLAGS += $(call cc-option,-ffunction-sections,) $(call cc-option,-fdata-sections,)

include $(srctree)/arch/$(ARCH)/soc/$(SOC_NAME)/Makefile

cflags-$(CONFIG_LTO) += $(call cc-option,-flto,)

KBUILD_CFLAGS += $(cflags-y)

QEMU_CPU_TYPE_arm = cortex-m3
QEMU_FLAGS_arm = -cpu  $(QEMU_CPU_TYPE_arm) \
		-machine lm3s6965evb -nographic
QEMU_arm = qemu-system-arm

export QEMU_FLAGS_arm QEMU_arm
