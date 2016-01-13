

arch-cflags = $(call cc-option,-mthumb -mcpu=cortex-m4) \
			$(call cc-option,-mthumb -march=armv7e-m)

KBUILD_CFLAGS += $(arch-cflags)
KBUILD_CXXFLAGS += $(arch-cflags)
KBUILD_AFLAGS += $(arch-cflags)
