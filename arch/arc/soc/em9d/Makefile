
arch-cflags = $(call cc-option,-mARCv2EM) \
                   $(call cc-option,-mav2em,) $(call cc-option,-mno-sdata)

KBUILD_AFLAGS += $(arch-cflags)
KBUILD_CFLAGS += $(arch-cflags)
KBUILD_CXXFLAGS += $(arch-cflags)
