# Overview of used compiler properties for gcc / g++ compilers.
#
# Define the flags your toolchain support, and keep the unsupported flags empty.

#####################################################
# This section covers flags related to optimization #
#####################################################
set_compiler_property(PROPERTY no_optimization)

set_compiler_property(PROPERTY optimization_debug)

set_compiler_property(PROPERTY optimization_speed)

set_compiler_property(PROPERTY optimization_size)

set_compiler_property(PROPERTY optimization_size_aggressive)

#######################################################
# This section covers flags related to warning levels #
#######################################################

# Property for standard warning base in Zephyr, this will always bet set when compiling.
set_compiler_property(PROPERTY warning_base)

# GCC options for warning levels 1, 2, 3, when using `-DW=[1|2|3]`
# Property for warning levels 1, 2, 3 in Zephyr when using `-DW=[1|2|3]`
set_compiler_property(PROPERTY warning_dw_1)

set_compiler_property(PROPERTY warning_dw_2)

set_compiler_property(PROPERTY warning_dw_3)

# Extended warning set supported by the compiler
set_compiler_property(PROPERTY warning_extended)

# Compiler property that will issue error if a declaration does not specify a type
set_compiler_property(PROPERTY warning_error_implicit_int)

# Compiler flags to use when compiling according to MISRA
set_compiler_property(PROPERTY warning_error_misra_sane)

###########################################################################
# This section covers flags related to C or C++ standards / standard libs #
###########################################################################

# Compiler flags for C standard. The specific standard must be appended by user.
# For example, gcc specifies this as: set_compiler_property(PROPERTY cstd -std=)
set_compiler_property(PROPERTY cstd)

# Compiler flags for disabling C standard include and instead specify include
# dirs in nostdinc_include to use.
set_compiler_property(PROPERTY nostdinc)
set_compiler_property(PROPERTY nostdinc_include)

# Compiler flags for disabling C++ standard include.
set_property(TARGET compiler-cpp PROPERTY nostdincxx)

# Required C++ flags when compiling C++ code
set_property(TARGET compiler-cpp PROPERTY required)

# Compiler flags to use for specific C++ dialects
set_property(TARGET compiler-cpp PROPERTY dialect_cpp98)
set_property(TARGET compiler-cpp PROPERTY dialect_cpp11)
set_property(TARGET compiler-cpp PROPERTY dialect_cpp14)
set_property(TARGET compiler-cpp PROPERTY dialect_cpp17)
set_property(TARGET compiler-cpp PROPERTY dialect_cpp2a)
set_property(TARGET compiler-cpp PROPERTY dialect_cpp20)
set_property(TARGET compiler-cpp PROPERTY dialect_cpp2b)

# Flag for disabling strict aliasing rule in C and C++
set_compiler_property(PROPERTY no_strict_aliasing)

# Extra warnings options for twister run
set_property(TARGET compiler PROPERTY warnings_as_errors)
set_property(TARGET asm PROPERTY warnings_as_errors)

# Flag for disabling exceptions in C++
set_property(TARGET compiler-cpp PROPERTY no_exceptions)

# Flag for disabling rtti in C++
set_property(TARGET compiler-cpp PROPERTY no_rtti)

# Flag for disabling optimizations around printf return value
set_compiler_property(PROPERTY no_printf_return_value)

###################################################
# This section covers all remaining C / C++ flags #
###################################################

# Flags for coverage generation
set_compiler_property(PROPERTY coverage)

# Security canaries flags.
set_compiler_property(PROPERTY security_canaries)
set_compiler_property(PROPERTY security_canaries_strong)
set_compiler_property(PROPERTY security_canaries_all)
set_compiler_property(PROPERTY security_canaries_explicit)

set_compiler_property(PROPERTY security_fortify_compile_time)
set_compiler_property(PROPERTY security_fortify_run_time)

# Flag for a hosted (no-freestanding) application
set_compiler_property(PROPERTY hosted)

# gcc flag for a freestanding application
set_compiler_property(PROPERTY freestanding)

# Flag to include debugging symbol in compilation
set_compiler_property(PROPERTY debug)

# Flags to save temporary object files
set_compiler_property(PROPERTY save_temps)

# Flag to specify linker script
set_compiler_property(PROPERTY linker_script)

set_compiler_property(PROPERTY no_common)

# Flags for imacros. The specific header must be appended by user.
set_compiler_property(PROPERTY imacros)

# Compiler flag for turning off thread-safe initialization of local statics
set_property(TARGET compiler-cpp PROPERTY no_threadsafe_statics)

# Required ASM flags when compiling
set_property(TARGET asm PROPERTY required)

# Compiler flag for disabling pointer arithmetic warnings
set_compiler_property(PROPERTY warning_no_pointer_arithmetic)

# Compiler flags for disabling position independent code / executable
set_compiler_property(PROPERTY no_position_independent)

# Compiler flag to avoid combine more than one global variable into a single aggregate.
# gen_kobject_list.py is does not understand it and end up identifying objects as if
# they had the same address.
set_compiler_property(PROPERTY no_global_merge)

# Compiler flag for warning about shadow variables
set_compiler_property(PROPERTY warning_shadow_variables)

# Compiler flags to avoid recognizing built-in functions
set_compiler_property(PROPERTY no_builtin)
set_compiler_property(PROPERTY no_builtin_malloc)

# Compiler flag for defining specs. Used only by gcc, other compilers may keep
# this undefined.
set_compiler_property(PROPERTY specs)

# Compiler flag for defining preinclude files.
set_compiler_property(PROPERTY include_file)
