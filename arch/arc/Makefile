

KBUILD_CFLAGS += $(call cc-option,-ffunction-sections,) $(call cc-option,-fdata-sections,)
cflags-$(CONFIG_LTO) = $(call cc-option,-flto,)
cflags-$(CPU_ARCEM4) = $(call cc-option,-mARCv2EM) \
			 $(call cc-option,-mav2em,$(call cc-option,-mno-sdata))

KBUILD_CFLAGS += $(cflags-y)

