#
# Copyright (c) 2016 Intel Corporation
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#

BOARD = qemu_x86
CONF_FILE = prj.conf

export SOURCE_DIR = $(ZEPHYR_BASE)/samples/static_lib/hello_world
export LDFLAGS_zephyr += -L$(O)/mylib/lib
export ALL_LIBS += mylib

include ${ZEPHYR_BASE}/Makefile.inc

all qemu zephyr : all-mylib
clean pristine mrproper : clean-mylib
all-mylib: outputexports

all-mylib clean-mylib:
	$(MAKE) -C ../mylib O=$(O) $(@:-mylib=)
