arch-cflags += $(call cc-option,-mthumb -mcpu=cortex-m4) \
		$(call cc-option,-march=armv7e-m) \
		$(call cc-option,-mabi=aapcs) \

_CROSS_COMPILE_arm_isa = armv7e-m
export _CROSS_COMPILE_arm_isa

KBUILD_CFLAGS += $(arch-cflags)
KBUILD_CXXFLAGS += $(arch-cflags)
KBUILD_AFLAGS += $(arch-cflags)
