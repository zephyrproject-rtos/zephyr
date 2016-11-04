MDEF_FILE = prj.mdef
BOARD ?= qemu_x86

soc_arduino_101_sss = soc_quark_se_c1000_ss
soc_quark_se_c1000_ss_devboard = soc_quark_se_c1000_ss

ifeq ($(soc_$(BOARD)),soc_quark_se_c1000_ss)
CONF_FILE = prj_soc_quark_se_c1000_ss.conf
else
CONF_FILE = prj.conf
endif

include ${ZEPHYR_BASE}/Makefile.inc
