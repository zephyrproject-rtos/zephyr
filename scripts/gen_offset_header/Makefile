HOSTCFLAGS_gen_offset_header.o += -DKERNEL_VERSION=0 -Wall -Werror -g -m32
HOSTCFLAGS_gen_idt.o += -Wno-unused-result
HOSTLDFLAGS := -m32

hostprogs-y	+= gen_offset_header
gen_offset_header-objs	:= gen_offset_header.o
always		:= $(hostprogs-y)
