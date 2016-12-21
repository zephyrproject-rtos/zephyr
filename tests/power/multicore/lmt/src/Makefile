TEST_CASE ?= sleep-success

ifeq ($(TEST_CASE), sleep-success)
ccflags-y += -DTEST_CASE_SLEEP_SUCCESS
endif

ccflags-y +=-I$(ZEPHYR_BASE)/drivers

obj-y = main.o
