# Makefile - LWM2M library

#
# Copyright (c) 2017 Linaro
#
# SPDX-License-Identifier: Apache-2.0
#

ccflags-y += -I$(srctree)/subsys/net/lib/lwm2m

obj-y := \
	lwm2m_engine.o \
	lwm2m_obj_security.o \
	lwm2m_obj_server.o \
	lwm2m_obj_device.o \
	lwm2m_rw_plain_text.o \
	lwm2m_rw_oma_tlv.o

# LWM2M RD Client Support
obj-$(CONFIG_LWM2M_RD_CLIENT_SUPPORT) += lwm2m_rd_client.o

# LWM2M Object Support
obj-$(CONFIG_LWM2M_FIRMWARE_UPDATE_OBJ_SUPPORT) += lwm2m_obj_firmware.o
obj-$(CONFIG_LWM2M_FIRMWARE_UPDATE_PULL_SUPPORT) += lwm2m_obj_firmware_pull.o

# JSON Support
obj-$(CONFIG_LWM2M_RW_JSON_SUPPORT) += lwm2m_rw_json.o

# IPSO Objects
obj-$(CONFIG_LWM2M_IPSO_TEMP_SENSOR) += ipso_temp_sensor.o
obj-$(CONFIG_LWM2M_IPSO_LIGHT_CONTROL) += ipso_light_control.o
