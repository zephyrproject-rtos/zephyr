ccflags-y += ${PROJECTINCLUDE}
ccflags-y += -I${ZEPHYR_BASE}/include/drivers/
ifeq (${PROFILER_NO_SHELL_REGISTER},1)
ccflags-y += -DPROFILER_NO_SHELL_REGISTER
endif

obj-y = profiler.o
