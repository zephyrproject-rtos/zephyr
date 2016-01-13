cflags-y += $(call cc-option,-ffunction-sections,) $(call cc-option,-fdata-sections,)
cflags-$(CONFIG_LTO) = $(call cc-option,-flto,)

include $(srctree)/arch/$(ARCH)/soc/$(SOC_NAME)/Makefile

KBUILD_CFLAGS += $(cflags-y)
KBUILD_CXXFLAGS += $(cflags-y)

