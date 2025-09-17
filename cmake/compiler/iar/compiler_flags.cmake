# Copyright (c) 2025 IAR Systems AB
#
# SPDX-License-Identifier: Apache-2.0

# Compiler options for the IAR C/C++ Compiler for Arm

#####################################################
# This section covers flags related to optimization #
#####################################################
set_compiler_property(PROPERTY no_optimization -On)
set_compiler_property(PROPERTY optimization_debug -Ol)
set_compiler_property(PROPERTY optimization_speed -Ohs)
set_compiler_property(PROPERTY optimization_size -Ohz)
set_compiler_property(PROPERTY optimization_size_aggressive -Ohz)
set_compiler_property(PROPERTY optimization_fast --no_size_constraints)

# IAR uses the GNU assembler so the options differ from the compiler
set_property(TARGET asm PROPERTY no_optimization -O0)
set_property(TARGET asm PROPERTY optimization_debug -Og)
set_property(TARGET asm PROPERTY optimization_speed -O2)
set_property(TARGET asm PROPERTY optimization_size -Os)
set_property(TARGET asm PROPERTY optimization_size_aggressive -Oz)
set_property(TARGET asm PROPERTY optimization_fast -Ofast)

#######################################################
# This section covers flags related to warning levels #
#######################################################

# -DW=... settings
#
# W=1 - warnings that may be relevant and does not occur too often
# W=2 - warnings that occur quite often but may still be relevant
# W=3 - the more obscure warnings, can most likely be ignored

set(IAR_SUPPRESSED_WARNINGS)

# Set basic suppressed warning first

# Pe223: function "xxx" declared implicitly
# Pe054: too few arguments in invocation of macro
# Pe1675: unrecognized GCC pragma
# Pe111: statement is unreachable
# Pe1143: arithmetic on pointer to void or function type
# Pe068: integer conversion resulted in a change of sign
list(APPEND IAR_SUPPRESSED_WARNINGS
     --diag_suppress=Pe068,Pe1143,Pe111,Pe1675,Pe054,Pe223)

# Since IAR does not turn on warnings we do this backwards
# by not suppressing warnings on higher levels.
if(NOT W MATCHES "3")
  # Pe381: extra ";" ignored
  list(APPEND IAR_SUPPRESSED_WARNINGS --diag_suppress=Pe381)
endif()
if(NOT W MATCHES "2" AND NOT W MATCHES "3")
  # Pe1097: unknown attribute
  # Pa082: undefined behavior: the order of volatile accesses is undefined
  # Pa084: pointless integer comparison, the result is always false
  # Pe185: dynamic initialization in unreachable code
  # Pe167: argument of type "onoff_notify_fn" is incompatible with...
  # Pe144: a value of type "void *" cannot be used to initialize...
  # Pe177: function "xxx" was declared but never referenced
  # Pe513: a value of type "void *" cannot be assigned to an
  #        entity of type "int (*)(int)"
  list(APPEND IAR_SUPPRESSED_WARNINGS
       --diag_suppress=Pe513,Pe177,Pe144,Pe167,Pe185,Pa084,Pa082,Pe1097)
endif()
if(NOT W MATCHES "1" AND NOT W MATCHES "2" AND NOT W MATCHES "3")
  # Pe188: enumerated type mixed with another type
  # Pe128: loop is not reachable
  # Pe550: variable "res" was set but never used
  # Pe546: transfer of control bypasses initialization
  # Pe186: pointless comparison of unsigned integer with zero
  # Pe054: too few arguments in invocation of macro xxx
  # Pe144: a value of type "void *" cannot be used to initialize an
  #        entity of type [...] "void (*)(struct onoff_manager *, int)"
  # Pe167: argument of type "void *" is incompatible with [...]
  #        "void (*)(void *, void *, void *)"
  list(APPEND IAR_SUPPRESSED_WARNINGS
       --diag_suppress=Pe054,Pe186,Pe546,Pe550,Pe128,Pe188,Pe144,Pe167)
endif()

set_compiler_property(PROPERTY warning_base ${IAR_SUPPRESSED_WARNINGS})

# Extended warning set supported by the compiler
set_compiler_property(PROPERTY warning_extended)

# Compiler property that will issue error if a declaration does not specify a type
set_compiler_property(PROPERTY warning_error_implicit_int)

# Compiler flags to use when compiling according to MISRA
set_compiler_property(PROPERTY warning_error_misra_sane)

set_property(TARGET compiler PROPERTY warnings_as_errors --warnings_are_errors)

###########################################################################
# This section covers flags related to C or C++ standards / standard libs #
###########################################################################

# Compiler flags for C standard. The specific standard must be appended by user.
# For example, gcc specifies this as: set_compiler_property(PROPERTY cstd -std=)
# TC-WG: the `cstd99` is used regardless of this flag being useful for iccarm
# This flag will make it a symbol. Works for C,CXX,ASM
# Since ICCARM does not use C standard flags, we just make them a defined symbol
# instead
set_compiler_property(PROPERTY cstd -D__IAR_CSTD_)

# Compiler flags for disabling C standard include and instead specify include
# dirs in nostdinc_include to use.
set_compiler_property(PROPERTY nostdinc)
set_compiler_property(PROPERTY nostdinc_include)

# Compiler flags for disabling C++ standard include.
set_compiler_property(TARGET compiler-cpp PROPERTY nostdincxx)

# Required C++ flags when compiling C++ code
set_property(TARGET compiler-cpp PROPERTY required --c++)

# Compiler flags to use for specific C++ dialects
set_property(TARGET compiler-cpp PROPERTY dialect_cpp98)
set_property(TARGET compiler-cpp PROPERTY dialect_cpp11)
set_property(TARGET compiler-cpp PROPERTY dialect_cpp14)
set_property(TARGET compiler-cpp PROPERTY dialect_cpp17 --libc++)
set_property(TARGET compiler-cpp PROPERTY dialect_cpp2a --libc++)
set_property(TARGET compiler-cpp PROPERTY dialect_cpp20 --libc++)
set_property(TARGET compiler-cpp PROPERTY dialect_cpp2b --libc++)
set_property(TARGET compiler-cpp PROPERTY dialect_cpp23 --libc++)

# Flag for disabling strict aliasing rule in C and C++
set_compiler_property(PROPERTY no_strict_aliasing)

# Flag for disabling exceptions in C++
set_property(TARGET compiler-cpp PROPERTY no_exceptions --no_exceptions)

# Flag for disabling rtti in C++
set_property(TARGET compiler-cpp PROPERTY no_rtti --no_rtti)

###################################################
# This section covers all remaining C / C++ flags #
###################################################

# Flags for coverage generation
set_compiler_property(PROPERTY coverage)

# Security canaries flags.
set_compiler_property(PROPERTY security_canaries --stack_protection)
set_compiler_property(PROPERTY security_canaries_strong --stack_protection)
set_compiler_property(PROPERTY security_canaries_all --security_canaries_all_is_not_supported)
set_compiler_property(PROPERTY security_canaries_explicit --security_canaries_explicit_is_not_supported)

if(CONFIG_STACK_CANARIES_TLS)
  check_set_compiler_property(APPEND PROPERTY security_canaries --stack_protector_guard=tls)
  check_set_compiler_property(APPEND PROPERTY security_canaries_strong --stack_protector_guard=tls)
  check_set_compiler_property(APPEND PROPERTY security_canaries_all --stack_protector_guard=tls)
  check_set_compiler_property(APPEND PROPERTY security_canaries_explicit --stack_protector_guard=tls)
endif()

set_compiler_property(PROPERTY security_fortify)

# Flag for a hosted (no-freestanding) application
set_compiler_property(PROPERTY hosted)

# gcc flag for a freestanding application
set_compiler_property(PROPERTY freestanding)

# Flag to include debugging symbol in compilation
set_property(TARGET compiler PROPERTY debug --debug)
set_property(TARGET compiler-cpp PROPERTY debug --debug)

set_compiler_property(PROPERTY no_common)

# Flags for imacros. The specific header must be appended by user.
set_property(TARGET compiler PROPERTY imacros --preinclude)
set_property(TARGET compiler-cpp PROPERTY imacros --preinclude)
set_property(TARGET asm PROPERTY imacros -imacros)

# Compiler flag for turning off thread-safe initialization of local statics
set_property(TARGET compiler-cpp PROPERTY no_threadsafe_statics)

# Required ASM flags when compiling
set_property(TARGET asm PROPERTY required)

# Compiler flag for disabling pointer arithmetic warnings
set_compiler_property(PROPERTY warning_no_pointer_arithmetic)
# Compiler flag for warning about shadow variables
set_compiler_property(PROPERTY warning_shadow_variables)
# Compiler flag for disabling warning about array bounds
set_compiler_property(PROPERTY warning_no_array_bounds)

# Compiler flags for disabling position independent code / executable
set_compiler_property(PROPERTY no_position_independent)

# Compiler flag for defining preinclude files.
set_compiler_property(PROPERTY include_file --preinclude)

set_compiler_property(PROPERTY cmse --cmse)

set_property(TARGET asm PROPERTY cmse -mcmse)
