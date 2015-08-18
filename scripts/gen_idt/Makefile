HOSTCFLAGS_gen_idt.o += -I$(srctree)/arch/x86/include -I$(srctree)/include/arch/x86
HOSTCFLAGS_gen_idt.o += -DKERNEL_VERSION=0 -Wall -Werror -g -m32
HOSTCFLAGS_version.o += -DKERNEL_VERSION=0 -Wall -Werror -g -m32
HOSTCFLAGS_gen_idt.o += -Wno-unused-result
HOSTLDFLAGS := -m32

hostprogs-y	+= gen_idt
gen_idt-objs	:= version.o gen_idt.o
always		:= $(hostprogs-y)
