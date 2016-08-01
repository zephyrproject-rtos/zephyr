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

KERNEL_TYPE = nano
BOARD ?= qemu_x86
CONF_FILE = prj.conf

MYLIB = mylib/lib/libmylib.a

all:
	$(MAKE) clean
	$(MAKE) mylib
	$(MAKE) hello_world

hello_world: $(MYLIB)
	$(MAKE) -C hello_world

$(MYLIB):
	$(MAKE) -C mylib

run:
	$(MAKE) -C hello_world qemu

pristine:
	$(MAKE) -C mylib clean
	$(MAKE) -C hello_world pristine

clean:
	$(MAKE) pristine
