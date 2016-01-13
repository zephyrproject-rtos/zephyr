
arch-cflags = -march=atom $(call cc-option,-mtune=atom,$(call cc-option,-mtune=generic))

KBUILD_CFLAGS += $(arch-cflags)
KBUILD_CXXFLAGS += $(arch-cflags)
KBUILD_AFLAGS += $(KBUILD_CFLAGS)
