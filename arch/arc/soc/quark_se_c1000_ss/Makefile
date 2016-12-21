soc-cflags = $(call cc-option,-mcpu=quarkse_em) \
             $(call cc-option,-mno-sdata)

soc-aflags = $(soc-cflags)
soc-cxxflags = $(soc-cflags)
soc-cflags += -DQM_SENSOR=1
soc-cflags += -I$(srctree)/arch/x86/soc/intel_quark

## FIXME
SOC_SERIES = quark_se
