#! /usr/bin/python3
#
# Get all the test functions in a given binary given in argv #1
#
# ztest Zephyr images contain a test suite, declared with
# ztest_test_suite() and it's fed a name and list of test functions.
#
# To find them, look for any symbols of type 'struct unit_test'; per
# tests/ztest/include/ztest_test.h, those include in the second member
# (+4) a pointer to a test function.
#
import elf_helper
import sys
eh = elf_helper.ElfHelper(sys.argv[1], False, "unit_test", [],
                          check_bounds = False)
syms = eh.get_symbols()			# map of symbols and their addresses
syms_addrs = {}				# map of address to symbol
for name, addr in syms.items():
    syms_addrs[addr] = name
# Iterate over all objects of type 'unit_test'; for each, dereference
# the value in addr + 4 (->test) and lookup its name; print it.
for addr, obj in eh.find_kobjects(syms).items():
    fn_addr = elf_helper.addr_deref(eh.elf, addr + 4)
    # fn_addr zero is end of table, so we ignore it
    if fn_addr > 0 and fn_addr in syms_addrs:
        print(syms_addrs[fn_addr])
