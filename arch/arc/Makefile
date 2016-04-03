cflags-y += $(call cc-option,-ffunction-sections,) $(call cc-option,-fdata-sections,)
cflags-$(CONFIG_ARC_STACK_CHECKING) = $(call cc-option,-fomit-frame-pointer)
cflags-$(CONFIG_LTO) = $(call cc-option,-flto,)

include $(srctree)/arch/$(ARCH)/soc/$(SOC_PATH)/Makefile

KBUILD_CFLAGS += $(cflags-y)
KBUILD_CXXFLAGS += $(cflags-y)

