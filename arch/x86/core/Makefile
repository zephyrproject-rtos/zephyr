ccflags-y += -I$(srctree)/kernel/include
asflags-y += -I$(srctree)/kernel/include

ifeq ($(COMPILER)$(CONFIG_X86_IAMCU),clang)
# We rely on GAS for assembling, so don't use the integrated assembler
KBUILD_AFLAGS += -no-integrated-as
endif

# To create dynamic stubs we need to do a .rept on the total number of
# stubs divided by a constant, this prevents GAS from thinking the '/'
# character starts a comment
KBUILD_AFLAGS += -Wa,--divide

obj-y += cpuhalt.o \
	msr.o irq_manage.o \
	sys_fatal_error_handler.o \
	crt0.o cache_s.o cache.o excstub.o \
	intstub.o swap.o thread.o

obj-$(CONFIG_IRQ_OFFLOAD) += irq_offload.o
obj-$(CONFIG_FP_SHARING) += float.o
obj-$(CONFIG_GDT_DYNAMIC) += gdt.o
obj-$(CONFIG_REBOOT_RST_CNT) += reboot_rst_cnt.o

obj-$(CONFIG_DEBUG_INFO) += debug/
obj-$(CONFIG_REBOOT_RST_CNT) += reboot_rst_cnt.o

# Last since we declare default exception handlers here
obj-y += fatal.o
