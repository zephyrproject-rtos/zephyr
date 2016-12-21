MDEF_FILE = prj.mdef
BOARD ?= qemu_x86

small_freq_divider_frdm_k64f=y
small_freq_divider_arduino_due=y
small_freq_divider_qemu_cortex_m3=y

ifeq ($(small_freq_divider_$(BOARD)),y)
	CONF_FILE = prj_small_freq_divider.conf
else
	CONF_FILE = prj.conf
endif

include ${ZEPHYR_BASE}/Makefile.test
