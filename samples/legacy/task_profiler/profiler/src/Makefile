ccflags-y += ${PROJECTINCLUDE}
ccflags-y += -I${ZEPHYR_BASE}/include/drivers/
ifeq (${PROFILER_NO_SHELL_REGISTER},1)
ccflags-y += -DPROFILER_NO_SHELL_REGISTER
endif
ifeq (${PROFILER_USE_COUNTER},1)
ccflags-y += -DPROFILER_USE_COUNTER
endif

obj-y = profiler.o
