# Those are flags not to test for CXX compiler.
list(APPEND CXX_EXCLUDED_OPTIONS
  -Werror=implicit-int
  -Wold-style-definition
  -Wno-pointer-sign
)

########################################################
# Setting compiler properties for gcc / g++ compilers. #
########################################################

#####################################################
# This section covers flags related to optimization #
#####################################################
set_compiler_property(PROPERTY no_optimization -O0)
if(CMAKE_C_COMPILER_VERSION VERSION_LESS "4.8.0")
  set_compiler_property(PROPERTY optimization_debug -O0)
else()
  set_compiler_property(PROPERTY optimization_debug -Og)
endif()
set_compiler_property(PROPERTY optimization_speed -O2)
set_compiler_property(PROPERTY optimization_size  -Os)

#######################################################
# This section covers flags related to warning levels #
#######################################################

# GCC Option standard warning base in Zephyr
set_compiler_property(PROPERTY warning_base
    -Wall
    -Wformat
    -Wformat-security
    -Wno-format-zero-length
    -Wno-main
)

check_set_compiler_property(APPEND PROPERTY warning_base -Wno-pointer-sign)

# Prohibit void pointer arithmetic. Illegal in C99
check_set_compiler_property(APPEND PROPERTY warning_base -Wpointer-arith)

# not portable
check_set_compiler_property(APPEND PROPERTY warning_base -Wexpansion-to-defined)

# GCC options for warning levels 1, 2, 3, when using `-DW=[1|2|3]`
set_compiler_property(PROPERTY warning_dw_1
                      -Waggregate-return
                      -Wcast-align
                      -Wdisabled-optimization
                      -Wnested-externs
                      -Wshadow
)
check_set_compiler_property(APPEND PROPERTY warning_dw_1
                            -Wlogical-op
                            -Wmissing-field-initializers
)

set_compiler_property(PROPERTY warning_dw_2
                      -Wbad-function-cast
                      -Wcast-qual
                      -Wconversion
                      -Wpacked
                      -Wpadded
                      -Wpointer-arith
                      -Wredundant-decls
                      -Wswitch-default
)
check_set_compiler_property(APPEND PROPERTY warning_dw_2
                            -Wpacked-bitfield-compat
                            -Wvla
)
set_compiler_property(PROPERTY warning_dw_3
                      -Wbad-function-cast
                      -Wcast-qual
                      -Wconversion
                      -Wpacked
                      -Wpadded
                      -Wpointer-arith
                      -Wredundant-decls
                      -Wswitch-default
)
check_set_compiler_property(APPEND PROPERTY warning_dw_3
                            -Wpacked-bitfield-compat
                            -Wvla
)

check_set_compiler_property(PROPERTY warning_extended -Wno-unused-but-set-variable)

check_set_compiler_property(PROPERTY warning_error_implicit_int -Werror=implicit-int)

set_compiler_property(PROPERTY warning_error_misra_sane -Werror=vla)

set_compiler_property(PROPERTY warning_error_coding_guideline
                      -Werror=vla
                      -Wimplicit-fallthrough=2
                      -Wconversion
                      -Woverride-init
)

###########################################################################
# This section covers flags related to C or C++ standards / standard libs #
###########################################################################

# GCC compiler flags for C standard. The specific standard must be appended by user.
set_compiler_property(PROPERTY cstd -std=)

if (NOT CONFIG_NEWLIB_LIBC AND
    NOT COMPILER STREQUAL "xcc" AND
    NOT CONFIG_HAS_ESPRESSIF_HAL AND
    NOT CONFIG_NATIVE_APPLICATION)
  set_compiler_property(PROPERTY nostdinc -nostdinc)
  set_compiler_property(APPEND PROPERTY nostdinc_include ${NOSTDINC})
endif()

set_compiler_property(TARGET compiler-cpp PROPERTY nostdincxx "-nostdinc++")

# Required C++ flags when using gcc
set_property(TARGET compiler-cpp PROPERTY required "-fcheck-new")

# GCC compiler flags for C++ dialects
set_property(TARGET compiler-cpp PROPERTY dialect_cpp98 "-std=c++98")
set_property(TARGET compiler-cpp PROPERTY dialect_cpp11 "-std=c++11" "-Wno-register")
set_property(TARGET compiler-cpp PROPERTY dialect_cpp14 "-std=c++14" "-Wno-register")
set_property(TARGET compiler-cpp PROPERTY dialect_cpp17 "-std=c++17" "-Wno-register")
set_property(TARGET compiler-cpp PROPERTY dialect_cpp2a "-std=c++2a"
  "-Wno-register" "-Wno-volatile")
set_property(TARGET compiler-cpp PROPERTY dialect_cpp20 "-std=c++20"
  "-Wno-register" "-Wno-volatile")
set_property(TARGET compiler-cpp PROPERTY dialect_cpp2b "-std=c++2b"
  "-Wno-register" "-Wno-volatile")

# Disable exceptions flag in C++
set_property(TARGET compiler-cpp PROPERTY no_exceptions "-fno-exceptions")

# Disable rtti in C++
set_property(TARGET compiler-cpp PROPERTY no_rtti "-fno-rtti")


###################################################
# This section covers all remaining C / C++ flags #
###################################################

# gcc flags for coverage generation
set_compiler_property(PROPERTY coverage -fprofile-arcs -ftest-coverage -fno-inline)

# Security canaries.
set_compiler_property(PROPERTY security_canaries -fstack-protector-all)

# Only a valid option with GCC 7.x and above, so let's do check and set.
check_set_compiler_property(APPEND PROPERTY security_canaries -mstack-protector-guard=global)

if(NOT CONFIG_NO_OPTIMIZATIONS)
  # _FORTIFY_SOURCE: Detect common-case buffer overflows for certain functions
  # _FORTIFY_SOURCE=1 : Compile-time checks (requires -O1 at least)
  # _FORTIFY_SOURCE=2 : Additional lightweight run-time checks
  set_compiler_property(PROPERTY security_fortify _FORTIFY_SOURCE=2)
endif()

# gcc flag for a hosted (no-freestanding) application
check_set_compiler_property(APPEND PROPERTY hosted -fno-freestanding)

# gcc flag for a freestanding application
set_compiler_property(PROPERTY freestanding -ffreestanding)

# Flag to enable debugging
set_compiler_property(PROPERTY debug -g)

# GCC 11 by default emits DWARF version 5 which cannot be parsed by
# pyelftools. Can be removed once pyelftools supports v5.
check_set_compiler_property(APPEND PROPERTY debug -gdwarf-4)

set_compiler_property(PROPERTY no_common -fno-common)

# GCC compiler flags for imacros. The specific header must be appended by user.
set_compiler_property(PROPERTY imacros -imacros)

# GCC compiler flags for sanitizing.
set_compiler_property(PROPERTY sanitize_address -fsanitize=address)

set_compiler_property(PROPERTY gprof -pg)

set_compiler_property(PROPERTY sanitize_undefined -fsanitize=undefined)

# GCC compiler flag for turning off thread-safe initialization of local statics
set_property(TARGET compiler-cpp PROPERTY no_threadsafe_statics "-fno-threadsafe-statics")

# Required ASM flags when using gcc
set_property(TARGET asm PROPERTY required "-xassembler-with-cpp")

# gcc flag for colourful diagnostic messages
if (NOT COMPILER STREQUAL "xcc")
set_compiler_property(PROPERTY diagnostic -fdiagnostics-color=always)
endif()

# Compiler flag for disabling pointer arithmetic warnings
set_compiler_property(PROPERTY warning_no_pointer_arithmetic "-Wno-pointer-arith")
