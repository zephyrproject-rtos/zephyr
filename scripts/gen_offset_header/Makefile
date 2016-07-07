HOSTCFLAGS_gen_offset_header.o += -DKERNEL_VERSION=0 -Wall -Werror -g
HOSTCFLAGS_gen_idt.o += -Wno-unused-result

hostprogs-y	+= gen_offset_header
gen_offset_header-objs	:= gen_offset_header.o
always		:= $(hostprogs-y)
