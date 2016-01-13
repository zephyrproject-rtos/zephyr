

arch-cflags = $(call cc-option,-march=lakemont -mtune=lakemont -miamcu -msoft-float, -march=pentium)

KBUILD_CFLAGS += $(arch-cflags)
KBUILD_CXXFLAGS += $(arch-cflags)
KBUILD_AFLAGS += $(KBUILD_CFLAGS)
