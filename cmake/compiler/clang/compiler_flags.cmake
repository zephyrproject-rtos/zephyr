# First step is to inherit all properties from gcc, as clang is compatible with most flags.
include(${ZEPHYR_BASE}/cmake/compiler/gcc/compiler_flags.cmake)

# Now, let's overwrite the flags that are different in clang.

########################################################
# Setting compiler properties for gcc / g++ compilers. #
########################################################

#####################################################
# This section covers flags related to optimization #
#####################################################

set_compiler_property(PROPERTY optimization_fast -O3 -ffast-math)

#######################################################
# This section covers flags related to warning levels #
#######################################################

# clang option standard warning base in Zephyr
check_set_compiler_property(PROPERTY warning_base
                            -Wall
                            -Wformat
                            -Wformat-security
                            -Wno-format-zero-length
                            -Wno-unused-but-set-variable
                            -Wno-typedef-redefinition
                            -Wno-deprecated-non-prototype
)

# C implicit promotion rules will want to make floats into doubles very easily
check_set_compiler_property(APPEND PROPERTY warning_base -Wdouble-promotion)

check_set_compiler_property(APPEND PROPERTY warning_base -Wno-pointer-sign)

# Prohibit void pointer arithmetic. Illegal in C99
check_set_compiler_property(APPEND PROPERTY warning_base -Wpointer-arith)

# clang options for warning levels 1, 2, 3, when using `-DW=[1|2|3]`
set_compiler_property(PROPERTY warning_dw_1
                      -Wextra
                      -Wunused
                      -Wno-unused-parameter
                      -Wmissing-declarations
                      -Wmissing-format-attribute
)
check_set_compiler_property(APPEND PROPERTY warning_dw_1
                            -Wold-style-definition
                            -Wmissing-prototypes
                            -Wmissing-include-dirs
                            -Wunused-but-set-variable
                            -Wno-missing-field-initializers
)

set_compiler_property(PROPERTY warning_dw_2
                      -Waggregate-return
                      -Wcast-align
                      -Wdisabled-optimization
                      -Wnested-externs
                      -Wshadow
)

check_set_compiler_property(APPEND PROPERTY warning_dw_2
                            -Wlogical-op
                            -Wmissing-field-initializers
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


check_set_compiler_property(PROPERTY warning_extended
                            #FIXME: need to fix all of those
                            -Wno-self-assign
                            -Wno-initializer-overrides
                            -Wno-section
                            -Wno-gnu
)

set_compiler_property(PROPERTY warning_error_coding_guideline
                      -Werror=vla
                      -Wimplicit-fallthrough
                      -Wconversion
                      -Woverride-init
)

###########################################################################
# This section covers flags related to C or C++ standards / standard libs #
###########################################################################

# No printf-return-value optimizations in clang
set_compiler_property(PROPERTY no_printf_return_value)

# Clang does not support "-Wno-volatile"
set_property(TARGET compiler-cpp PROPERTY dialect_cpp2a "-std=c++2a" "-Wno-register")
set_property(TARGET compiler-cpp PROPERTY dialect_cpp20 "-std=c++20" "-Wno-register")
set_property(TARGET compiler-cpp PROPERTY dialect_cpp2b "-std=c++2b" "-Wno-register")

###################################################
# This section covers all remaining C / C++ flags #
###################################################

# clang flags for coverage generation
if (CONFIG_COVERAGE_NATIVE_SOURCE)
  set_compiler_property(PROPERTY coverage -fprofile-instr-generate -fcoverage-mapping)
else()
  set_compiler_property(PROPERTY coverage --coverage -fno-inline)
endif()

# No property flag, clang doesn't understand fortify at all
set_compiler_property(PROPERTY security_fortify_compile_time)
set_compiler_property(PROPERTY security_fortify_run_time)

# No property flag, this is used by the POSIX arch based targets when building with the host libC,
# But clang has problems compiling these with -fno-freestanding.
check_set_compiler_property(PROPERTY hosted)

# clang flag to save temporary object files
set_compiler_property(PROPERTY save_temps -save-temps)

# clang doesn't handle the -T flag
set_compiler_property(PROPERTY linker_script -Wl,-T)

# clang flag for colourful diagnostic messages
set_compiler_property(PROPERTY diagnostic -fcolor-diagnostics)

# clang flag to disable macro backtrace in diagnostics (can't fully disable it, so limit to 1)
set_compiler_property(PROPERTY no_track_macro_expansion "-fmacro-backtrace-limit=1")

set_compiler_property(PROPERTY no_global_merge "-mno-global-merge")

set_compiler_property(PROPERTY specs)
