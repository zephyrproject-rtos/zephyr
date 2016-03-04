KBUILD_CFLAGS += $(call cc-option,-ffunction-sections,) $(call cc-option,-fdata-sections,)

include $(srctree)/arch/$(ARCH)/soc/$(SOC_NAME)/Makefile

cflags-$(CONFIG_LTO) += $(call cc-option,-flto,)

KBUILD_CFLAGS += $(cflags-y)
KBUILD_CXXFLAGS += $(cflags-y)

ifeq ($(CONFIG_CPU_CORTEX_M3),y)
arch-cflags += $(call cc-option, -mthumb -mcpu=cortex-m3)
else ifeq ($(CONFIG_CPU_CORTEX_M4),y)
arch-cflags += $(call cc-option, -mthumb -mcpu=cortex-m4)
endif

QEMU_CPU_TYPE_arm = cortex-m3
QEMU_FLAGS_arm = -cpu  $(QEMU_CPU_TYPE_arm) \
		-machine lm3s6965evb -nographic
QEMU_arm = qemu-system-arm

export QEMU_FLAGS_arm QEMU_arm
