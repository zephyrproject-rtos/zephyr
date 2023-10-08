set pagination off
set trace-commands on
set logging enabled on

target remote :5678

b test
b main.c:29
c

# break at test()
s
set var a = 2
c

# break at main()
if ret == 6
	printf "GDB:PASSED\n"
	quit 0
else
	printf "GDB:FAILED\n"
	quit 1
end
