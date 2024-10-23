# Copyright (c) 2024 Intel Corporation
#
# SPDX-License-Identifier: Apache-2.0

# A script that outputs an estimation on maximum stack usage
# (on the fixed-size privilege elevation stack in kernel space)
# for each syscall found through the DWARF information in the ELF file.

# A path to the output file and the Zephyr ELF file are required.

# Optional flags:
# -v: Include full call tree with individual memory usage and informational flags
# -h: Get help message

# Output is a text file that contains file names,
# syscalls that each file call to and the maximum stack usage
# used by that syscall given information generated from
# CONFIG_STACK_USAGE being enabled.

# Requires CONFIG_USERSPACE and CONFIG_STACK_USAGE to be enabled.

from elftools.elf.elffile import ELFFile
import os
import argparse


# This function uses pyelftools to extract the DWARF information from the ELF file.

# Input is the path to the ELF file.

# Output is a structure organized the same way as the output of:
# readelf -w <elffile> > <outputfile.txt>
# Documentation on the structure and available functions can be found at:
# https://github.com/eliben/pyelftools

def getElf(elffile):
    with open(elffile, 'rb') as f:
        elffile = ELFFile(f)

        if not elffile.has_dwarf_info():
            print('no DWARF info')
            return

        dwarf_info = elffile.get_dwarf_info()

        return dwarf_info


# This function extracts all syscalls specified by the prefixes
# z_mrsh, z_vrfy, and z_impl, from the DWARF information
# (organized by object file).

# Input is the DWARF information returned from getElf(),
# a dictionary to store the compilation unit offsets,
# and a variable to store the privilege elevation stack size if found.

# Output is a dictionary organized by object file that contains a list of syscalls.

def get_syscalls(dwarf_info, cuOffsets, stack):
    syscall_table = {}

    for CU in dwarf_info.iter_CUs():

        cuOffset = CU.cu_offset

        # The first debugging entry in a compilation unit contains
        # the object file the consequent entries belong to.
        top_DIE = CU.get_top_DIE()
        file_name = top_DIE.attributes['DW_AT_name'].value.decode('utf-8')
        syscall_table[file_name] = []
        cuOffsets[file_name] = cuOffset

        for DIE in CU.iter_DIEs():

            # When CONFIG_USERSPACE is enabled, the stack size is found
            # in a structure named z_<arch_name>_thread_stack_header under the
            # userspace.c file.
            if DIE.tag == 'DW_TAG_structure_type' and 'userspace.c' in file_name:
                try:
                    func_name = DIE.attributes['DW_AT_name'].value.decode('utf-8')
                except KeyError:
                    continue

                if 'thread_stack_header' in func_name:
                    stack.append(DIE.attributes['DW_AT_byte_size'].value)

            elif DIE.tag == 'DW_TAG_subprogram':

                try:
                    func_name = DIE.attributes['DW_AT_name'].value.decode('utf-8')
                except KeyError:
                    continue

                # Addresses are relative to their space in the ELF file.
                address = DIE.offset

                if func_name.startswith(('z_mrsh', 'z_vrfy', 'z_impl')):

                    if func_name not in syscall_table[file_name]:
                        syscall_table[file_name].append([func_name, address])

    return syscall_table


# This function generates a call tree for each syscall
# defined in the syscall_table.

# Input is the DWARF information, the syscall_table dictionary,
# a dictionary to store informational flags, and the dictionary of
# compilation unit offsets.

# Output is a dictionary organized by object file that contains
# a list of syscalls and their call tree.

def get_call_tree(dwarf_info, syscall_table, flags, cuOffsets):

    # call_tree dictionary structure:
    # Key = object file
    # Value = [[function_address, depth_of_function_call], [function_address, depth_of_function_call], [...], ...]
    call_tree = {}

    for key in syscall_table:

        call_tree[key] = []

        cuOffset = cuOffsets[key]

        for i in range(0, len(syscall_table[key])):
            address = syscall_table[key][i][1]

            DIE = dwarf_info.get_DIE_from_refaddr(address)

            if address in [entry[0] for entry in call_tree[key]]:
                # If the syscall is already in the call tree, continue.
                continue

            # Add the syscall to the call tree as the root.
            call_tree[key].append([address, 0])

            if DIE.has_children:
                check_call_sites(dwarf_info, DIE, cuOffset, call_tree, key, 1, flags)

    return call_tree


# This function recursively checks a function for
# call sites, inlined subroutines and function pointers.

# Input is the DWARF information, the current DIE (debugging entry),
# the compilation unit offset dictionary, the call_tree dictionary,
# the key (object file), the current function call depth,
# and the informational flags dictionary.

# No output but the call_tree dictionary is updated with the call tree.

# Note: 'DW_AT_abstract_origin' + compilation unit offset of object file = function address in ELF file

def check_call_sites(dwarf_info, DIE, cuOffset, call_tree, key, depth, flags):
    for child in DIE.iter_children():

        # GNU_call_sites are 'regularly' called functions. Not inlined or a pointer.
        if child.tag == 'DW_TAG_GNU_call_site':
            try:
                target_address = child.attributes['DW_AT_abstract_origin'].value + cuOffset
                target_DIE = dwarf_info.get_DIE_from_refaddr(target_address)
                call_tree[key].append([target_address, depth])
                check_call_sites(dwarf_info, target_DIE, cuOffset, call_tree, key, depth + 1, flags)
            except KeyError:
                target_address = ""

        # Inlined_subroutines are inlined functions.
        elif child.tag == 'DW_TAG_inlined_subroutine':
            try:
                target_address = child.attributes['DW_AT_abstract_origin'].value + cuOffset
                target_DIE = dwarf_info.get_DIE_from_refaddr(target_address)
                call_tree[key].append([target_address, depth])
                if target_address not in flags:
                    flags[target_address] = 'inlined'
                check_call_sites(dwarf_info, target_DIE, cuOffset, call_tree, key, depth + 1, flags)
            except KeyError:
                target_address = ""

        # Function pointers can be found in structures, variables and pointer types.
        # This block checks if a tag not specified as inlined or a call site might trace
        # to a pointer type pointing to a subroutine type.
        else:
            ptr_addresses = []
            tags(dwarf_info, call_tree, flags, child, cuOffset, ptr_addresses, 0)
            for ptr in ptr_addresses:
                if ptr != 0:
                    call_tree[key].append([ptr, depth])
                if ptr not in flags:
                    flags[ptr] = 'pointer'

        # Debugging entries can nest other debugging entries.
        if child.has_children:
            check_call_sites(dwarf_info, child, cuOffset, call_tree, key, depth, flags)


# This function recursively traces through debugging entries with specific tags
# that could lead to an entry tagged 'DW_TAG_subroutine_type'. In which case,
# means the first debugging entry that was traced through is a function pointer.

# Input is the DWARF information, the call_tree dictionary, the informational flags dictionary,
# the current debugging entry, the compilation unit offset dictionary,
# a list to store function pointer addresses, and the address of the original debugging entry
# that was traced through if the ending debugging entry is tagged with 'DW_AT_subroutine_type'.

# No output but the pointer addresses list is updated
# with the found function pointer address, if any.

def tags(dwarf_info, call_tree, flags, DIE, cuOffset, ptr_addresses, at_name_addr):
    try:
        try:
            # If getting the at_name attribute fails
            # then I do not want to store the address.
            at_name = DIE.attributes['DW_AT_name'].value.decode('utf-8')
        except KeyError:
            at_name = ""
        if at_name != "":
            at_name_addr = DIE.offset
        at_type = DIE.attributes['DW_AT_type'].value + cuOffset
        target_DIE = dwarf_info.get_DIE_from_refaddr(at_type)
        if target_DIE.tag == 'DW_TAG_pointer_type':
            tags(dwarf_info, call_tree, flags, target_DIE, cuOffset, ptr_addresses, at_name_addr)
        elif target_DIE.tag == 'DW_TAG_const_type':
            tags(dwarf_info, call_tree, flags, target_DIE, cuOffset, ptr_addresses, at_name_addr)
        elif target_DIE.tag == 'DW_TAG_variable':
            tags(dwarf_info, call_tree, flags, target_DIE, cuOffset, ptr_addresses, at_name_addr)
        elif target_DIE.tag == 'DW_TAG_structure_type':
            for c in target_DIE.iter_children():
                if c.tag == 'DW_TAG_member':
                    tags(dwarf_info, call_tree, flags, c, cuOffset, ptr_addresses, at_name_addr)
        elif target_DIE.tag == 'DW_TAG_subroutine_type':
            ptr_addresses.append(at_name_addr)
    except KeyError:
        pass


# This function extracts the maximum stack usage for each function
# using the .su files generated with CONFIG_STACK_USAGE enabled.

# Input is the DWARF information, the call_tree dictionary and
# the informational flags dictionary.

# Output is a dictionary organized by function address that contains
# the maximum stack usage for that function if found.
# If not found, the function's max memory usage is set to 0
# and is flagged with '*'.

def get_memory_usage(dwarf_info, call_tree, flags, build_location):
    mem_usage_dict = {} # Key = function address, Value = memory usage found in .su file

    for key in call_tree:
        mem_usage = 0
        for i in range(0, len(call_tree[key])):
            address = call_tree[key][i][0]

            DIE = dwarf_info.get_DIE_from_refaddr(address)

            try:
                name = DIE.attributes['DW_AT_name'].value.decode('utf-8')
            except KeyError:
                continue

            try:
                decl_line = DIE.attributes['DW_AT_decl_line'].value
                decl_column = DIE.attributes['DW_AT_decl_column'].value
            except KeyError:
                continue

            if address not in mem_usage_dict:
                mem_usage = su_files(key, decl_line, decl_column, name, address, flags, build_location)
                mem_usage_dict[address] = mem_usage

            else:
                continue

    return mem_usage_dict


# This function rewrites the memory usage dictionary to account
# for multiple instances of a function.
# It will go through the dictionary of memory usage and change the
# value of a function's usage to the maximum usage found across all instances.

# Input is the DWARF information, the memory usage dictionary generated
# from get_memory_usage() and the dictionary of memory usage by function name.

# No output but the memory usage dictionary is updated with the new values.

# The structures of both memory dictionaries are as follows:
# original_mem: Key = function address, Value = memory usage found in .su file
# mem_by_name: Key = function name, Value = list of memory usages found in
# all .su files that included that function name

def redo_memory(dwarf_info, original_mem, mem_by_name):
    new_mem = {} # Key = function name, Value = maximum memory usage found in .su file

    for key in original_mem:
        address = key
        DIE = dwarf_info.get_DIE_from_refaddr(address)
        name = DIE.attributes['DW_AT_name'].value.decode('utf-8')

        if name not in new_mem:
            new_mem[name] = original_mem[key]
        else:
            new_mem[name] = max(int(new_mem[name]), int(original_mem[key]))

        # mem_by_name structure is to keep track of functions that have multiple instances
        if name not in mem_by_name:
            mem_by_name[name] = 1
        else:
            mem_by_name[name] += 1

    for key in original_mem:
        address = key
        DIE = dwarf_info.get_DIE_from_refaddr(address)
        name = DIE.attributes['DW_AT_name'].value.decode('utf-8')

        new_memory = new_mem[name]
        original_mem[key] = new_memory


# This function searches for the .su file that
# corresponds to the .c or object file.

# Input is the object file name, the line number of the function call,
# the column number of the function call, the name of the function,
# the address of the function and the informational flags dictionary.

# Output is the maximum stack usage of the function
# if found in the specific .su file.

# Note: if the function is not found in the .su file,
# the function's max memory usage is set to 0 and is flagged with '*'.

def su_files(c_file, line, column, name, address, flags, build_location):
    c_file = c_file.split('/')[-1]
    su_file = c_file + '.su' # .su files will be the name of the object file with .su appended to it
    mem = None

    for root, _, files in os.walk(build_location):
        if su_file in files:

            target_line = str(line) + ':' + str(column) + ':' + name

            found_path = os.path.join(root, su_file)
            mem = confirm_su_file(found_path, target_line, address, flags)

            break

        else:
            continue

    if mem is None:
        mem = 0

    return mem


# This function is a helper function to see if the function is
# in the specified .su file and extract the stack usage.

# Input is the path to the specified .su file,
# the line number of the function call, the address of the function,
# and the informational flags dictionary.

# Output is the stack usage of the function if found in the .su file.

def confirm_su_file(file_name, find_line, address, flags):
    with open(file_name, 'r') as file:

        for line in file:

            if find_line in line:
                split_line = line.split()

                if address not in flags:
                    flags[address] = split_line[2] # flags are 'dynamic', 'bounded', 'static'
                else:
                    if split_line[2] not in flags[address]:
                        flags[address] = flags[address] + ',' + ' ' + split_line[2]
                return split_line[1]

        # Following lines mean that the function was not found in the .su file. Noted by '*'.
        if address not in flags:
            flags[address] = '*'
        else:
            if '*' not in flags[address]:
                flags[address] = flags[address] + ',' + ' ' + '*'

        return 0


# This function converts the addresses of functions
# to their respective names for printing output and
# easier readability.

# Input is the call_tree dictionary, the DWARF information
# and the memory usage dictionary.

# No output but the call_tree dictionary is updated to this format:
# Key = object file
# Value = [[function_name, depth_of_function_call, function_address, memory_usage], [...], ...]

def get_names(call_tree, dwarf_info, mem_usage):
    for key in call_tree:

        for i in range(0, len(call_tree[key])):
            address = call_tree[key][i][0]
            DIE = dwarf_info.get_DIE_from_refaddr(address)

            try:
                name = DIE.attributes['DW_AT_name'].value.decode('utf-8')
            except KeyError:
                name = "NA"

            call_tree[key][i][0] = name

            call_tree[key][i].append(address)

            if address in mem_usage:
                call_tree[key][i].append(mem_usage[address])
            else:
                call_tree[key][i].append(0)


# This function calculates the maximum memory usage path of a syscall.

# Input is the call_tree dictionary and an empty max_usage dictionary.

# No output but the max_usage dictionary is updated with the
# maximum memory usage amount for the specific syscall.

# Note: max_usage dictionary structure:
# Key = object file
# Value = [[root_address, max_memory_usage], [root_address, max_memory_usage], [...], ...]
# with the root_addresses being the root syscall addresses.

def max_mem_usage(call_tree, max_usage):
    for key in call_tree:

        curr_stack = [] # Current call stack
        max_stack = [] # Maximum memory usage path found so far
        root = 0 # Root syscall address
        max_mem_used = 0
        max_usage[key] = []

        for i in range(0, len(call_tree[key])):

            curr_name = call_tree[key][i][0]
            curr_depth = call_tree[key][i][1]
            curr_address = call_tree[key][i][2]
            curr_usage = int(call_tree[key][i][3])

            if i+1 < len(call_tree[key]):
                next_depth = call_tree[key][i+1][1]
            else:
                next_depth = -1

            if curr_depth == 0:
                root = curr_address

            if curr_depth == 0 and next_depth in (-1, 0):
                max_usage[key].append([root, curr_usage])
                continue

            curr_stack.append([curr_name, curr_depth, curr_address, curr_usage])

            if curr_depth >= next_depth:
                max_stack = check_stacks(curr_stack, max_stack)

                if next_depth in (-1, 0):

                    for max in max_stack:
                        max_mem_used += max[3]
                    max_usage[key].append([root, max_mem_used])

                    curr_stack = []
                    max_stack = []
                    max_mem_used = 0

                else:
                    for func in curr_stack:
                        if func[1] >= next_depth:
                            curr_stack.remove(func)


# This function is a helper function to max_mem_usage that
# checks the current stack against the maximum stack to see
# if the current stack has a higher memory usage.

# Input is the current call stack and the maximum call stack
# found so far.

# Output is the maximum call stack of the two stacks.

def check_stacks(curr_s, max_s):
    max_mem = 0
    curr_mem = 0

    for i in curr_s:
        curr_mem += i[3]

    for i in max_s:
        max_mem += i[3]

    if curr_mem >= max_mem:
        max_s = curr_s.copy()
    return max_s


# This function prints the final result of the script
# to a text file specified by the user.

# Input is the call_tree dictionary, the informational flags dictionary,
# the maximum memory usage dictionary, the privileged elevation stack size,
# the memory dictionary (organized by function name) that indicates if there
# were 'copies' of the same function across different object files,
# the output file path and a boolean flag inidicating if the user wants
# the verbose option to be printed or not.

# No direct return to main but the output
# is a text file with the script's results.

def print_tree(call_tree, flags, max_usage, stack_size, mem_by_name, output_file, verbose):
    f = open(output_file, 'w')

    for key in call_tree:
        f.write(f'File: {key}\n')

        prev_depth = -1

        for function, depth, address, mem in call_tree[key]:
            if depth == 0:
                f.write('\n')

            if not verbose:
                if depth != 0:
                    continue

            print_string = '   ' * depth

            if depth > prev_depth or depth == 0:
                print_string += '+'

            if verbose:
                print_string += function + '   ' + str(mem)

                if address in flags:
                    if '*' in flags[address]:
                        if mem_by_name[function] > 1:
                            flags[address] = flags[address].replace('*', 'external')
                    print_string += '   ' + flags[address]
            else:
                print_string += function

            if depth == 0:
                for entry in max_usage[key]:
                    if address == entry[0]:
                        print_string += '   ' + 'MAX MEMORY USAGE: ' + str(entry[1])
                        if stack_size != 0:
                            if entry[1] > (.9 * stack_size):
                                print_string += '   ' + 'WARNING: STACK USAGE EXCEEDS 90% OF STACK SIZE'

            f.write(print_string + '\n')
            prev_depth = depth

        f.write('\n')
        f.write('-' * 50 + '\n')

    f.close()


# This function prints a help message

# No inputs. Outputs text to the console with information on
# how to use the script and what the generated output means.

def print_help():
    print("""
          Usage: stack_usage.py [-h] --elf-file ELF_FILE --output-file OUTPUT_FILE --build-dir BUILD_DIRECTORY_PATH --input-flags=INPUT_FLAGS\n
          \tOUTPUT_FILE: Specify the output file name as a .txt file. Ex. output.txt\n
          Optional flags:\n
          \t-v: Include full call tree with individual memory usage and informational flags
          \t-h: Get help message\n
          Interpreting output:\n
          \tOrganized by file, each line below the file name represents a function call (if any)\n
          \tIf verbose option is turned off, output will be the top-most syscall (usually a z_mrsh function) followed by its maximum memory usage according to files generated with CONFIG_STACK_USAGE enabled.
          \tIf verbose option is turned on, output will include the full call tree on a per-syscall basis with individual memory usage and informational flags.\n
          \tIndentation and + symbols indicate depth of function calls. Depth is determined by how the function is linked in the ELF file.
          \tA function may be called within an inlined function but because the parent function was inlined, the function call will be linked to the root function who inlined the parent function.
          \tAs such, the depth of the function call will be under the root function, not its parent function in source code.\n
          \tIf there are no function calls under a file, it means that the file does not contain any syscalls.\n
          \tMemory usage is in bytes. If a function has multiple instances across several object files, the memory usage will be the maximum usage found across all instances.\n
          Informational flags:\n
          \t1) inlined: Function is inlined\n
          \t2) pointer: Function is called through a function pointer\n
          \t3) external: Function is not defined in the same object file\n
          \t4) static/dynamic/bounded: Flags determined by the .su file generated with -fstack-usage flag when CONFIG_STACK_USAGE is enabled.
          \t\t\t\tSee https://gcc.gnu.org/onlinedocs/gnat_ugn/Static-Stack-Usage-Analysis.html for more information.
          \t5) *: Function was not found in any .su file and has no data on maximum stack usage\n
        """)


def main():
    class CustomArgumentParser(argparse.ArgumentParser):
        def print_help(self):
            print_help()

    parser = CustomArgumentParser(description='Analyze syscall stack usage.')
    parser.add_argument('--elf-file', type=str, required=True, default='', help='ELF file to process')
    parser.add_argument('--output-file', type=str, required=True, default='', help='Output file name')
    parser.add_argument('--build-dir', type=str, required=True, default='', help='Build directory')
    parser.add_argument('--input-flags', type=str, required=False, default='', help='Include full call tree with individual memory usage and informational flags')

    args = parser.parse_args()

    elf_file_path = args.elf_file
    output_file_path = args.output_file
    build_location = args.build_dir
    input_flags = args.input_flags

    try:
        with open(elf_file_path, 'r'):
            pass
    except FileNotFoundError:
        print('ELF file not found. Exiting...')
        return -1

    if output_file_path == '' or not output_file_path.endswith('.txt'):
        output_file_path = build_location + '/syscall_stack_usage.txt'
        print('Invalid output file. Storing in ' + output_file_path)

    if input_flags == '':
        verbose = False
    elif input_flags == '-v':
        verbose = True
    elif input_flags == '-h':
        print_help()
        return 0
    else:
        print('Invalid input flag. Exiting...')
        return -1

    # flags structure:
    # Key = function address
    # Value = [flag1, flag2, ...]
    # Note: values can be 'inlined', 'pointer', 'external', 'dyanamic,bounded', 'static' or '*'
    flags = {}

    # cuOffsets structure:
    # Key = file name
    # Value = compilation unit offset for that file
    cuOffsets = {}

    max_usage = {}

    mem_by_name = {}

    stack_size = [0]

    info = getElf(elf_file_path)
    table = get_syscalls(info, cuOffsets, stack_size)
    full_call_tree = get_call_tree(info, table, flags, cuOffsets)
    memory = get_memory_usage(info, full_call_tree, flags, build_location)

    redo_memory(info, memory, mem_by_name)

    get_names(full_call_tree, info, memory)
    max_mem_usage(full_call_tree, max_usage)

    stack_size = stack_size[-1]

    print_tree(full_call_tree, flags, max_usage, stack_size, mem_by_name, output_file_path, verbose)

    return 0

if __name__ == '__main__':
    main()
