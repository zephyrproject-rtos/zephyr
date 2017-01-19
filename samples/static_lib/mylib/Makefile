#
# Copyright (c) 2016 Intel Corporation
#
# SPDX-License-Identifier: Apache-2.0
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
