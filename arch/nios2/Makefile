include $(srctree)/arch/$(ARCH)/soc/$(SOC_PATH)/Makefile

# FIXME: Disable use of global pointer for now, this isn't properly set up
# in our linker script.
arch_cflags := $(call cc-option,-G0)

# FIXME I got tired of adding ARG_UNUSED to all the stubs, sanitycheck treats
# warnings as errors. Remove this once the stubs are all implemented
arch_cflags += $(call cc-option,-Wno-unused-parameter)

KBUILD_AFLAGS += $(arch_cflags)
KBUILD_CFLAGS += $(arch_cflags)
KBUILD_CXXFLAGS += $(arch_cflags)
