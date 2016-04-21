# FIXME: Gets rid of bizzarre linker errors in the form:
# "warning: global pointer relative relocation at address 0x100013a8 when _gp
# not defined"
# However disabling global pointer  may be a performance issue...
# May be related to the bogus temporary FLASH and RAM base addresses we are
# using since they are 256MB apart, or we're possibly discarding a Nios2-
# specific section that we need to include in the linker script.
arch_cflags := $(call cc-option,-G0)

# FIXME I got tired of adding ARG_UNUSED to all the stubs, sanitycheck treats
# warnings as errors. Remove this once the stubs are all implemented
arch_cflags += $(call cc-option,-Wno-unused-parameter)

KBUILD_AFLAGS += $(arch_cflags)
KBUILD_CFLAGS += $(arch_cflags)
KBUILD_CXXFLAGS += $(arch_cflags)
