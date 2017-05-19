#  -mcpu=em4_dmips is added to KBUILD_CFLAGS to make cc-option check the options correctly
KBUILD_CFLAGS += -mcpu=em4_dmips

soc-cflags = $(call cc-option,-mcpu=em4_dmips -mno-sdata) \
		$(call cc-option,-mdiv-rem -mswap -mnorm) \
		$(call cc-option,-mmpy-option=6 -mbarrel-shifter) \
		$(call cc-option,--param l1-cache-size=16384) \
		$(call cc-option,--param l1-cache-line-size=32)

ifeq ($(CONFIG_CODE_DENSITY), y)
soc-cflags += $(call cc-option,-mcode-density)
endif
