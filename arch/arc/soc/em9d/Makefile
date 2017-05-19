#  -mcpu=em4_dmips is added to KBUILD_CFLAGS to make cc-option check the options correctly
KBUILD_CFLAGS += -mcpu=em4_fpus

soc-cflags += $(call cc-option,-mcpu=em4_fpus -mno-sdata) \
		$(call cc-option,-mdiv-rem -mswap -mnorm) \
		$(call cc-option,-mmpy-option=6 -mbarrel-shifter) \

ifeq ($(CONFIG_CODE_DENSITY), y)
soc-cflags += $(call cc-option,-mcode-density)
endif

ifeq ($(CONFIG_FLOAT), y)
soc-cflags += 	$(call cc-option,-mfpu=fpus_all)
endif
