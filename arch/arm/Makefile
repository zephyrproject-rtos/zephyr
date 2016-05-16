KBUILD_CFLAGS += $(call cc-option,-ffunction-sections,) $(call cc-option,-fdata-sections,)

include $(srctree)/arch/$(ARCH)/soc/$(SOC_PATH)/Makefile

cflags-$(CONFIG_LTO) += $(call cc-option,-flto,)

cflags-cortex-m0 = $(call cc-option,-mabi=aapcs -mthumb -mcpu=cortex-m0) \
			$(call cc-option,-mthumb -march=armv6-m)
cflags-cortex-m0+ = $(call cc-option,-mabi=aapcs -mthumb -mcpu=cortex-m0plus) \
			$(call cc-option,-mthumb -march=armv6-m)
cflags-cortex-m1 = $(call cc-option,-mabi=aapcs -mthumb -mcpu=cortex-m1) \
			$(call cc-option,-mthumb -march=armv6-m)
cflags-cortex-m3 = $(call cc-option,-mabi=aapcs -mthumb -mcpu=cortex-m3) \
			$(call cc-option,-mthumb -march=armv7-m)
cflags-cortex-m4 = $(call cc-option,-mabi=aapcs -mthumb -mcpu=cortex-m4) \
			$(call cc-option,-mthumb -march=armv7e-m)

ifeq ($(CONFIG_FLOAT), y)
cflags-cortex-m4 += $(call cc-option,-mfloat-abi=softfp -mfpu=fpv4-sp-d16)
endif

cflags-cortex-m7 = $(call cc-option,-mabi=aapcs -mthumb -mcpu=cortex-m7) \
			$(call cc-option,-mthumb -march=armv7e-m)

aflags-$(CONFIG_CPU_CORTEX_M3) += $(cflags-cortex-m3)
cflags-$(CONFIG_CPU_CORTEX_M3) += $(cflags-cortex-m3)
cxxflags-$(CONFIG_CPU_CORTEX_M3) += $(cflags-cortex-m3)
_arm_isa-$(CONFIG_CPU_CORTEX_M3) = armv7-m

aflags-$(CONFIG_CPU_CORTEX_M4) += $(cflags-cortex-m4)
cflags-$(CONFIG_CPU_CORTEX_M4) += $(cflags-cortex-m4)
cxxflags-$(CONFIG_CPU_CORTEX_M4) += $(cflags-cortex-m4)
_arm_isa-$(CONFIG_CPU_CORTEX_M4) = armv7e-m

_CROSS_COMPILE_arm_isa = $(_arm_isa-y)

KBUILD_AFLAGS += $(aflags-y)
KBUILD_CFLAGS += $(cflags-y)
KBUILD_CXXFLAGS += $(cflags-y)

soc-cxxflags ?= $(soc-cflags)
soc-aflags ?= $(soc-cflags)
KBUILD_CFLAGS += $(soc-cflags)
KBUILD_CXXFLAGS += $(soc-cxxflags)
KBUILD_AFLAGS += $(soc-aflags)

QEMU_CPU_TYPE_arm = cortex-m3
QEMU_FLAGS_arm = -cpu  $(QEMU_CPU_TYPE_arm) \
		-machine lm3s6965evb -nographic -vga none
QEMU_arm = qemu-system-arm

export QEMU_FLAGS_arm QEMU_arm
