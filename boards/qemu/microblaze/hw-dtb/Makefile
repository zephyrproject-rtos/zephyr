# Copyright (c) 2016 Xilinx Inc
# Copyright (c) 2023 Advanced Micro Devices, Inc. (AMD)
# Copyright (c) 2023 Alp Sayin <alpsayin@gmail.com>
# SPDX-License-Identifier: Apache-2.0
#
# Makefile to build the device trees
#

OUTDIR 	?= ./
GCC		?= gcc
DTC		?= dtc

ifeq ($V,)
QUIET=@
.SILENT:
else
QUIET=
endif

SINGLE_ARCH_OUTDIR		:= $(OUTDIR)/LATEST

DTS_FILES			:= $(wildcard *.dts)
DTSI_FILES			:= $(wildcard *.dtsi)
HEADER_FILES			:= $(wildcard *.dtsh)
HEADER_FILES			+= $(wildcard include/*.dtsh)


CPPFLAGS = -I. -Iinclude/

.PHONY:	all source

TARGETS = \
	$(patsubst %.dts,$(SINGLE_ARCH_OUTDIR)/%.$(1),$(DTS_FILES))

COMPILE = \
	$(QUIET)mkdir -p $(1); \
	$(GCC) -E -nostdinc ${CPPFLAGS} -x assembler-with-cpp $(3) -MD -MF $@.cd -o - $< | \
		$(DTC) -q -O $(2) -I dts -o $@ - -b 0; \
	cp $(SINGLE_ARCH_OUTDIR)/board-qemu-microblaze-demo.dtb ..; \
	rm -rf LATEST;

all:	$(call TARGETS,dtb)

-include $(call TARGETS, cd)

source:	all $(call TARGETS,dts)

$(SINGLE_ARCH_OUTDIR)/%.dtb:	%.dts $(DTSI_FILES) $(HEADER_FILES)
	$(call COMPILE,$(SINGLE_ARCH_OUTDIR),dtb)
$(SINGLE_ARCH_OUTDIR)/%.dts:	%.dts $(DTSI_FILES) $(HEADER_FILES)
	$(call COMPILE,$(SINGLE_ARCH_OUTDIR),dts)

clean:
	@rm -rf LATEST;
