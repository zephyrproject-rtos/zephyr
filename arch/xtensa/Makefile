# XCC emits an annoying warning if this is used even though the
# $(call cc-option,) test in toplevel Makefile passes.
KBUILD_CFLAGS := $(filter-out -fno-omit-frame-pointer, \
			      ${KBUILD_CFLAGS})

# Put functions and data in their own binary sections so that ld can
# garbage collect them
KBUILD_CFLAGS += $(call cc-option,-ffunction-sections,) \
		 $(call cc-option,-fdata-sections,)

KBUILD_AFLAGS += $(flagBoardType)
KBUILD_CFLAGS += $(flagBoardType) \
		 $(call cc-option,-fms-extensions,)

include $(srctree)/arch/$(ARCH)/core/Makefile
include $(srctree)/arch/$(ARCH)/soc/$(SOC_PATH)/Makefile

cflags-$(CONFIG_LTO) += $(call cc-option,-flto,)

KBUILD_CFLAGS += $(cflags-y)
KBUILD_CXXFLAGS += $(cflags-y)
