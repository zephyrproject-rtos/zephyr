HOSTCFLAGS_gen_idt.o += -I$(srctree)/include/arch/x86
HOSTCFLAGS_gen_idt.o += -DKERNEL_VERSION=0 -Wall -Werror -g
HOSTCFLAGS_version.o += -DKERNEL_VERSION=0 -Wall -Werror -g
HOSTCFLAGS_gen_idt.o += -Wno-unused-result

hostprogs-y	+= gen_idt
gen_idt-objs	:= version.o gen_idt.o
always		:= $(hostprogs-y)
