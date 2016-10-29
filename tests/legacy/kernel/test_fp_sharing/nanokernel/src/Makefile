ccflags-y += -I${ZEPHYR_BASE}/tests/include

obj-y += main.o

# Some boards are significantly slower than others resulting in the test
# output being in the range of every few seconds to every few minutes. To
# compensate for this, one can control the number of iterations in the PI
# calculation through PI_NUM_ITERATIONS. Lowering this value will increase
# the speed of the test but it will come at the expense of precision.
PI_NUM_ITERATIONS ?= 700000
ccflags-y += "-DPI_NUM_ITERATIONS=${PI_NUM_ITERATIONS}"
