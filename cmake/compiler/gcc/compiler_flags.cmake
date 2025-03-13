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
set_compiler_property(PROPERTY optimization_size_aggressive -Oz)
set_compiler_property(PROPERTY optimization_fast -Ofast)

if(CMAKE_C_COMPILER_VERSION GREATER_EQUAL "4.5.0")
  set_compiler_property(PROPERTY optimization_lto -flto=auto)
  set_compiler_property(PROPERTY prohibit_lto -fno-lto)
endif()

#######################################################
# This section covers flags related to warning levels #
#######################################################

# GCC Option standard warning base in Zephyr
check_set_compiler_property(PROPERTY warning_base
    -Wall
    "SHELL:-Wformat -Wformat-security"
    "SHELL:-Wformat -Wno-format-zero-length"
)

# C implicit promotion rules will want to make floats into doubles very easily
check_set_compiler_property(APPEND PROPERTY warning_base -Wdouble-promotion)

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
                            -Wno-missing-field-initializers
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
                      -Wmissing-field-initializers
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
    NOT (CONFIG_PICOLIBC AND NOT CONFIG_PICOLIBC_USE_MODULE) AND
    NOT COMPILER STREQUAL "xcc" AND
    NOT CONFIG_HAS_ESPRESSIF_HAL AND
    NOT CONFIG_NATIVE_BUILD)
  set_compiler_property(PROPERTY nostdinc -nostdinc)
  set_compiler_property(APPEND PROPERTY nostdinc_include ${NOSTDINC})
endif()

check_set_compiler_property(PROPERTY no_printf_return_value -fno-printf-return-value)

set_property(TARGET compiler-cpp PROPERTY nostdincxx "-nostdinc++")

# Required C++ flags when using gcc
set_property(TARGET compiler-cpp PROPERTY required "-fcheck-new")

# GCC compiler flags for C++ dialect: "register" variables and some
# "volatile" usage generates warnings by default in standard versions
# higher than 17 and 20 respectively.  Zephyr uses both, so turn off
# the warnings where needed (but only on the compilers that generate
# them, older toolchains like xcc don't understand the command line
# flags!)
set_property(TARGET compiler-cpp PROPERTY dialect_cpp98 "-std=c++98")
set_property(TARGET compiler-cpp PROPERTY dialect_cpp11 "-std=c++11")
set_property(TARGET compiler-cpp PROPERTY dialect_cpp14 "-std=c++14")
set_property(TARGET compiler-cpp PROPERTY dialect_cpp17 "-std=c++17" "-Wno-register")
set_property(TARGET compiler-cpp PROPERTY dialect_cpp2a "-std=c++2a"
  "-Wno-register" "-Wno-volatile")
set_property(TARGET compiler-cpp PROPERTY dialect_cpp20 "-std=c++20"
  "-Wno-register" "-Wno-volatile")
set_property(TARGET compiler-cpp PROPERTY dialect_cpp2b "-std=c++2b"
  "-Wno-register" "-Wno-volatile")

# Flag for disabling strict aliasing rule in C and C++
set_compiler_property(PROPERTY no_strict_aliasing -fno-strict-aliasing)

# Extra warning options
set_property(TARGET compiler PROPERTY warnings_as_errors -Werror)
set_property(TARGET asm PROPERTY warnings_as_errors -Werror -Wa,--fatal-warnings)

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
set_compiler_property(PROPERTY security_canaries -fstack-protector)
set_compiler_property(PROPERTY security_canaries_strong -fstack-protector-strong)
set_compiler_property(PROPERTY security_canaries_all -fstack-protector-all)
set_compiler_property(PROPERTY security_canaries_explicit -fstack-protector-explicit)

# Only a valid option with GCC 7.x and above, so let's do check and set.
if(CONFIG_STACK_CANARIES_TLS)
  check_set_compiler_property(APPEND PROPERTY security_canaries -mstack-protector-guard=tls)
  check_set_compiler_property(APPEND PROPERTY security_canaries_strong -mstack-protector-guard=tls)
  check_set_compiler_property(APPEND PROPERTY security_canaries_all -mstack-protector-guard=tls)
  check_set_compiler_property(APPEND PROPERTY security_canaries_explicit -mstack-protector-guard=tls)
else()
  check_set_compiler_property(APPEND PROPERTY security_canaries -mstack-protector-guard=global)
  check_set_compiler_property(APPEND PROPERTY security_canaries_global -mstack-protector-guard=global)
  check_set_compiler_property(APPEND PROPERTY security_canaries_all -mstack-protector-guard=global)
  check_set_compiler_property(APPEND PROPERTY security_canaries_explicit -mstack-protector-guard=global)
endif()


if(NOT CONFIG_NO_OPTIMIZATIONS)
  # _FORTIFY_SOURCE: Detect common-case buffer overflows for certain functions
  # _FORTIFY_SOURCE=1 : Loose checking (use wide bounds checks)
  # _FORTIFY_SOURCE=2 : Tight checking (use narrow bounds checks)
  # GCC always does compile-time bounds checking for string/mem functions, so
  # there's no additional value to set here
  set_compiler_property(PROPERTY security_fortify_compile_time)
  set_compiler_property(PROPERTY security_fortify_run_time _FORTIFY_SOURCE=2)
endif()

# gcc flag for a hosted (no-freestanding) application
check_set_compiler_property(APPEND PROPERTY hosted -fno-freestanding)

# gcc flag for a freestanding application
check_set_compiler_property(PROPERTY freestanding -ffreestanding)

# Flag to enable debugging
set_compiler_property(PROPERTY debug -g)

# Flags to save temporary object files
set_compiler_property(PROPERTY save_temps -save-temps=obj)

# Flag to specify linker script
set_compiler_property(PROPERTY linker_script -T)

# Flags to not track macro expansion
set_compiler_property(PROPERTY no_track_macro_expansion -ftrack-macro-expansion=0)

# GCC 11 by default emits DWARF version 5 which cannot be parsed by
# pyelftools. Can be removed once pyelftools supports v5.
check_set_compiler_property(APPEND PROPERTY debug -gdwarf-4)

set_compiler_property(PROPERTY no_common -fno-common)

# GCC compiler flags for imacros. The specific header must be appended by user.
set_compiler_property(PROPERTY imacros -imacros)

set_compiler_property(PROPERTY gprof -pg)

# GCC compiler flag for turning off thread-safe initialization of local statics
set_property(TARGET compiler-cpp PROPERTY no_threadsafe_statics "-fno-threadsafe-statics")

# Required ASM flags when using gcc
set_property(TARGET asm PROPERTY required "-xassembler-with-cpp")

# GCC compiler flags for imacros. The specific header must be appended by user.
set_property(TARGET asm PROPERTY imacros "-imacros")

# gcc flag for colourful diagnostic messages
check_set_compiler_property(PROPERTY diagnostic -fdiagnostics-color=always)

# Compiler flag for disabling pointer arithmetic warnings
set_compiler_property(PROPERTY warning_no_pointer_arithmetic "-Wno-pointer-arith")

#Compiler flags for disabling position independent code / executable
set_compiler_property(PROPERTY no_position_independent
                      -fno-pic
                      -fno-pie
)

set_compiler_property(PROPERTY no_global_merge "")

set_compiler_property(PROPERTY warning_shadow_variables -Wshadow)
set_compiler_property(PROPERTY warning_no_array_bounds -Wno-array-bounds)

set_compiler_property(PROPERTY no_builtin -fno-builtin)
set_compiler_property(PROPERTY no_builtin_malloc -fno-builtin-malloc)

set_compiler_property(PROPERTY specs -specs=)

set_compiler_property(PROPERTY include_file -include)

set_compiler_property(PROPERTY cmse -mcmse)

set_property(TARGET asm PROPERTY cmse -mcmse)
