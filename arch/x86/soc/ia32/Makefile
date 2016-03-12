ifeq ($(CONFIG_X86_IAMCU),y)
KBUILD_CFLAGS += $(call cc-option,-msoft-float)
endif
KBUILD_CFLAGS += $(call cc-option,-march=pentium)
KBUILD_CXXFLAGS += $(call cc-option,-march=pentium)
KBUILD_AFLAGS += $(KBUILD_CFLAGS)
