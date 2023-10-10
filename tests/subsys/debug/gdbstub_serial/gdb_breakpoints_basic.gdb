# Test GDB stub over serial connection:
#
# - GDB backend connection establishment,
# - set and delete breakpoints,
# - continue execution,
# - step command,
# - next command,
# - read/write local variables,
# - read/write registers.

# adjust GDB logging parameters
set pagination off
set trace-commands on
set logging overwrite on
set logging redirect off
set logging enabled on

print "expected: connecting to GDBstub from GDB"
target remote :5678
print "expected: break at arch_gdb_init() - GDBstub got connection from GDB"

break main
break  _gdbstub_test_gdb_breakpoints_basic_wrapper
break  gdbstub_test_gdb_breakpoints_basic
break function_1
break function_1_1
print "expected: breakpoints are set"
continue

print "expected: break #1 at ztest main()"
continue

print "expected: break #2 at _gdbstub_test_gdb_cmd_continue_wrapper test case ztest wrapper"
continue

print "expected: break #3 at gdbstub_test_gdb_cmd_continue test case"
continue

print "expected: break #4 at function_1() and continue without changes"
continue

print "expected: break #5 at function_1_1() remove the break and continue"
clear function_1_1
info breakpoints
# do some register read/write
info registers
x/i $pc
p/x $ax
set $ax++
set $ax--
continue

print "expected: break at function_1() again and change values"
step
next
info locals
if res != 0
	print "Unexpected 'res' value"
	# The remote target application will be killed and the test failed on timeout.
	quit 1
end
set var res = val

# detach is not yet implemented at Zephyr's gdbstub
continue

print("failed: Never get here as the remote test is already completed and disconnects")
quit
#
