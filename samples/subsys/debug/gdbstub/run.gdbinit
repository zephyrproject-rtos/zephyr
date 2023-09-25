set pagination off
#symbol-file build/zephyr/zephyr.elf
target remote :5678
b test
b main.c:33
c

s
set var a = 2
c
if ret == 6
	printf "PASSED\n"
	quit 0
else
	printf "FAILED\n"
	quit 1
end
