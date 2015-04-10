# Makefile - build microkernel footprint project

# Copyright (c) 2013-2014 Wind River Systems, Inc.

# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions are met:
#
# 1) Redistributions of source code must retain the above copyright notice,
# this list of conditions and the following disclaimer.
#
# 2) Redistributions in binary form must reproduce the above copyright notice,
# this list of conditions and the following disclaimer in the documentation
# and/or other materials provided with the distribution.
#
# 3) Neither the name of Wind River Systems nor the names of its contributors
# may be used to endorse or promote products derived from this software without
# specific prior written permission.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
# AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
# IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
# ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
# LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
# CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
# SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
# INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
# CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
# ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
# POSSIBILITY OF SUCH DAMAGE.

include $(subst \,/,${VXMICRO_BASE})/make/env/defs.base

# default BSP
BSP ?= generic_pc

# default configuration
TEST ?= min
PRJ_CONF = prj_${TEST}_${vARCH}.conf
VPFILE = micro_${TEST}.vpf
override EXTRA_CFLAGS += -DTEST_${TEST}

# Conditionally enable unsupported libc functionality for testing purposes only
UNSUPPORTED_STD_LIBS_reg = y
UNSUPPORTED_STD_LIBS_max = y
export UNSUPPORTED_STD_LIBS = ${UNSUPPORTED_STD_LIBS_${TEST}}

PRJ_INCPATH += ${vBASE}/samples/include/

# uncomment this to get a per-arch, per-test output dir
# OUTPUT_DIR ?= ${vARCH}_${TEST}

include ${vMAKE}/target/make.ukernel

ifeq (${vARCH},ARM)
  ifneq (${TEST},min)
    $(error 'reg' and 'max' test do not currently build for ARM)
  endif
endif
