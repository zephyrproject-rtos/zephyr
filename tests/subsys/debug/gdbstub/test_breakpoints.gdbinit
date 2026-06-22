b test
b main.c:29
c

# break at test()
s
set var a = 2
c
# break at main()

# set the last breakpoint before quit
b k_thread_abort

if ret == 6
	printf "GDB:PASSED\n"
	# exit main() and continue with code coverage dump, if configured
	c
	# remove some of the breakpoints to check.
	clear test
	clear k_thread_abort
	info break
	quit 0
else
	printf "GDB:FAILED\n"
	c
	quit 1
end
