ccflags-y +=-I$(srctree)/include/drivers
ccflags-y +=-I$(srctree)/arch/$(ARCH)/$(subst $(DQUOTE),,$(CONFIG_BSP_DIR))
asflags-y +=-I$(srctree)/arch/x86/$(subst $(DQUOTE),,$(CONFIG_BSP_DIR))


obj-${CONFIG_PIC}${CONFIG_SHUTOFF_PIC} = i8259.o

obj-$(CONFIG_PIC) += i8259_boi.o
obj-$(CONFIG_PIC) += system_pic.o
obj-$(CONFIG_LOAPIC) += system_apic.o
obj-$(CONFIG_LOAPIC) += loapic_intr.o

