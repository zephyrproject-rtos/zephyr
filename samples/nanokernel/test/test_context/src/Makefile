ccflags-y += ${PROJECTINCLUDE} -I${srctree}/samples/include
ccflags-y +=-I$(srctree)/arch/$(ARCH)/platforms/$(subst $(DQUOTE),,$(CONFIG_PLATFORM))
ccflags-$(CONFIG_PLATFORM_QUARK_D2000) += -DFIBER_STACKSIZE=256

obj-y = context.o
