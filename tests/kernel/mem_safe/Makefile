BOARD ?= qemu_x86
CONF_FILE = prj_$(BOARD).conf

# This testcase requires a special linker script, from the testcase itself.
# The only boards that have one are the ones listed below.
valid_board_qemu_x86 = y
valid_board_qemu_cortex_m3 = y

ifneq ($(valid_board_$(BOARD)),y)
$(error not a supported board for this testcase)
endif

include $(ZEPHYR_BASE)/Makefile.test
