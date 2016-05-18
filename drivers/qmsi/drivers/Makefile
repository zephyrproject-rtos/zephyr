#
# Copyright (c) 2016, Intel Corporation
# All rights reserved.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions are met:
#
# 1. Redistributions of source code must retain the above copyright notice,
#    this list of conditions and the following disclaimer.
# 2. Redistributions in binary form must reproduce the above copyright notice,
#    this list of conditions and the following disclaimer in the documentation
#    and/or other materials provided with the distribution.
# 3. Neither the name of the Intel Corporation nor the names of its
#    contributors may be used to endorse or promote products derived from this
#    software without specific prior written permission.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
# AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
# IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
# ARE DISCLAIMED. IN NO EVENT SHALL THE INTEL CORPORATION OR CONTRIBUTORS BE
# LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
# CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
# SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
# INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
# CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
# ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
# POSSIBILITY OF SUCH DAMAGE.
#

SOC_ROOT_DIR = $(SOC)
SOC_MAKEFILE = $(SOC)

SUPPORTED_SOCS = quark_se \
                 quark_d2000

SOC ?= $(DEFAULT_SOC)
TARGET ?= $(DEFAULT_TARGET)

TARGETS_quark_d2000 = x86
TARGETS_quark_se = x86 \
                   sensor

ifeq ($(filter $(SOC),$(SUPPORTED_SOCS)),)
$(error SOC=$(SOC) is not supported. Run 'make help' for help)
endif

SUPPORTED_TARGETS = $(TARGETS_$(SOC))

ifeq ($(filter $(TARGET),$(SUPPORTED_TARGETS)),)
$(error TARGET=$(TARGET) is not supported for $(SOC). Run 'make help' for help)
endif

ifeq ($(TARGET), sensor)
SOC_ROOT_DIR = quark_se
SOC_MAKEFILE = sensor
endif

### Variables
BASE_DIR = ..

HEADERS = $(wildcard $(BASE_DIR)/include/*.h)
HEADERS += $(wildcard $(BASE_DIR)/soc/$(SOC_ROOT_DIR)/include/*.h)
HEADERS += $(wildcard $(BASE_DIR)/drivers/include/*.h)
ifeq ($(TARGET), sensor)
HEADERS += $(wildcard $(BASE_DIR)/drivers/sensor/include/*.h)
endif
EXPORTED_HEADERS += $(addprefix $(LIBQMSI_INCLUDE_DIR)/, $(notdir $(HEADERS)))

### Make includes
include $(BASE_DIR)/base.mk
include $(BASE_DIR)/drivers/drivers.mk
ifeq ($(TARGET), sensor)
include $(BASE_DIR)/drivers/sensor/sensor.mk
endif
include $(BASE_DIR)/soc/$(SOC_ROOT_DIR)/$(SOC_MAKEFILE).mk
include $(BASE_DIR)/soc/$(SOC_ROOT_DIR)/drivers/drivers.mk

SLINK_NAME := $(LIBQMSI_LIB_DIR)/libqmsi.a

$(LIBQMSI_LIB_DIR)/$(LIBQMSI_FILENAME): $(OBJECTS)
	$(call mkdir, $(LIBQMSI_LIB_DIR))
	$(AR) rcs $@ $?
ifeq ($(wildcard $(SLINK_NAME)),)
ifneq ($(OS),Windows_NT)
	$(LN) -s $(LIBQMSI_FILENAME) $(SLINK_NAME)
endif
endif

$(LIBQMSI_INCLUDE_DIR)/%.h: $(HEADERS)
	$(call mkdir, $(LIBQMSI_INCLUDE_DIR))
	$(call copy, $(filter %/$(notdir $@), $(HEADERS)), $@)

.PHONY: all

all: $(LIBQMSI_LIB_DIR)/$(LIBQMSI_FILENAME) $(EXPORTED_HEADERS)
