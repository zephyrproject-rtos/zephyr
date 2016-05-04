include $(srctree)/arch/$(ARCH)/soc/$(SOC_PATH)/Makefile

# FIXME: Disable use of global pointer for now, this isn't properly set up
# in our linker script.
arch_cflags := $(call cc-option,-G0)

# FIXME I got tired of adding ARG_UNUSED to all the stubs, sanitycheck treats
# warnings as errors. Remove this once the stubs are all implemented
arch_cflags += $(call cc-option,-Wno-unused-parameter)

# Put functions and data in their own binary sections so that ld can
# garbage collect them
arch_cflags += $(call cc-option,-ffunction-sections) \
	       $(call cc-option,-fdata-sections)

# Nios II CPUs are configurable and we need to pull in the generated
# headers system.h and linker.h which specify what is enabled and where
# everything is.
arch_cflags += $(call cc-option,-I$(srctree)/arch/$(ARCH)/soc/$(SOC_PATH)/include)

KBUILD_AFLAGS += $(arch_cflags)
KBUILD_CFLAGS += $(arch_cflags)
KBUILD_CXXFLAGS += $(arch_cflags)
