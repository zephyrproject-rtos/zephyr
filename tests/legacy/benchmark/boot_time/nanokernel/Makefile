CONF_quark_d2000_crb 	= quark_d2000/quark_d2000_prj.conf
CONF_quark_se_c1000_devboard	= quark_se/quark_se_prj.conf
CONF_qemu_x86       	= ia32/prj_expert_test.conf
CONF_qemu_x86_iamcu    	= ia32/prj_expert_test.conf
CONF_galileo        	= quark_x1000/prj.conf
CONF_minnowboard     	= ia32/prj_expert_test.conf
CONF_arduino_101    	= quark_se/quark_se_prj.conf
CONF_tinytile    	= quark_se/quark_se_prj.conf
CONF_panther    	= quark_se/quark_se_prj.conf

BOARD ?= qemu_x86
CONF_FILE = $(CONF_$(BOARD))

include $(ZEPHYR_BASE)/Makefile.test
