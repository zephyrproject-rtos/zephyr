cflags-y += $(call cc-option,-ffunction-sections,) $(call cc-option,-fdata-sections,)
arch-$(CONFIG_LTO) = $(call cc-option,-flto,)
arch-$(CONFIG_CPU_ARCEM4) = $(call cc-option,-mARCv2EM) \
                         $(call cc-option,-mav2em,) $(call cc-option,-mno-sdata)

KBUILD_CFLAGS += $(arch-y) $(cflags-y)
KBUILD_AFLAGS += $(arch-y)
