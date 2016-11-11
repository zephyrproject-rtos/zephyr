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

O ?= $(error $$(O) is not set.  Try `make all-mylib` from hello_world)

-include $(O)/Makefile.export
CFLAGS = $(KBUILD_CFLAGS) $(NOSTDINC_FLAGS) $(ZEPHYRINCLUDE)
OBJDIR=$(O)/mylib/obj
LIBDIR=$(O)/mylib/lib

all:
	mkdir -p $(OBJDIR) $(LIBDIR)
	$(CC) -c $(CFLAGS) -Iinclude src/mylib.c -o $(OBJDIR)/mylib.o
	$(AR) -rcs $(LIBDIR)/libmylib.a $(OBJDIR)/mylib.o

clean:
	rm -rf $(OBJDIR) $(LIBDIR)
