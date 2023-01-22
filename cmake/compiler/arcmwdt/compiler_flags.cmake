set_property(GLOBAL PROPERTY CSTD gnu99)

# List the warnings that are not supported for C++ compilations
list(APPEND CXX_EXCLUDED_OPTIONS
  -Werror=implicit-int
  -Wold-style-definition
  )

###################################################
# Setting compiler properties for MWDT compilers. #
###################################################

#####################################################
# This section covers flags related to optimization #
#####################################################
set_compiler_property(PROPERTY no_optimization -O0)
set_compiler_property(PROPERTY optimization_debug -O0)
set_compiler_property(PROPERTY optimization_speed -O2)
set_compiler_property(PROPERTY optimization_size  -Os)

#######################################################
# This section covers flags related to warning levels #
#######################################################

# options for warnings
# base options
set_compiler_property(PROPERTY warning_base
                      -Wformat
                      -Wformat-security
                      -Wno-format-zero-length
                      -Wno-main-return-type
                      -Wno-unaligned-pointer-conversion
                      -Wno-incompatible-pointer-types-discards-qualifiers
                      -Wno-typedef-redefinition
)

check_set_compiler_property(APPEND PROPERTY warning_base -Wno-pointer-sign)

# Prohibit void pointer arithmetic. Illegal in C99
check_set_compiler_property(APPEND PROPERTY warning_base -Wpointer-arith)

# level 1 warning options
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

# level 2 warning options
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

# level 3 warning options
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

# extended warning options
check_set_compiler_property(PROPERTY warning_extended
                            -Wno-sometimes-uninitialized
                            -Wno-shift-overflow
                            -Wno-missing-braces
                            -Wno-self-assign
                            -Wno-address-of-packed-member
                            -Wno-unused-function
                            -Wno-initializer-overrides
                            -Wno-section
                            -Wno-unknown-warning-option
                            -Wno-unused-variable
                            -Wno-format-invalid-specifier
                            -Wno-gnu
                            # comparison of unsigned expression < 0 is always false
                            -Wno-tautological-compare
)

check_set_compiler_property(PROPERTY warning_error_implicit_int -Werror=implicit-int)

set_compiler_property(PROPERTY warning_error_misra_sane -Werror=vla)

###########################################################################
# This section covers flags related to C or C++ standards / standard libs #
###########################################################################

set_compiler_property(PROPERTY cstd -std=)

if (NOT CONFIG_ARCMWDT_LIBC)
  set_compiler_property(PROPERTY nostdinc -Hno_default_include -Hnoarcexlib)
  set_compiler_property(APPEND PROPERTY nostdinc_include ${NOSTDINC})
endif()

# C++ std options
set_property(TARGET compiler-cpp PROPERTY dialect_cpp98 "-std=c++98")
set_property(TARGET compiler-cpp PROPERTY dialect_cpp11 "-std=c++11")
set_property(TARGET compiler-cpp PROPERTY dialect_cpp14 "-std=c++14")
set_property(TARGET compiler-cpp PROPERTY dialect_cpp17 "-std=c++17")

#no support of C++2a, C++20, C++2b
set_property(TARGET compiler-cpp PROPERTY dialect_cpp2a "")
set_property(TARGET compiler-cpp PROPERTY dialect_cpp20 "")
set_property(TARGET compiler-cpp PROPERTY dialect_cpp2b "")

# Flag for disabling strict aliasing rule in C and C++
set_compiler_property(PROPERTY no_strict_aliasing -fno-strict-aliasing)

# Disable exceptions flag in C++
set_property(TARGET compiler-cpp PROPERTY no_exceptions "-fno-exceptions")

# Disable rtti in C++
set_property(TARGET compiler-cpp PROPERTY no_rtti "-fno-rtti")

###################################################
# This section covers all remaining C / C++ flags #
###################################################

# mwdt flag for a freestanding application
# do not link in supplied run-time startup files
set_compiler_property(PROPERTY freestanding -Hnocrt)

# Flag to enable debugging
if(CONFIG_THREAD_LOCAL_STORAGE)
  # FIXME: Temporary workaround for ARC MWDT toolchain issue - LLDAC linker produce errors on
  # debugging information (if -g option specified) of thread-local variables.
  set_compiler_property(PROPERTY debug)
else()
  set_compiler_property(PROPERTY debug -g)
endif()

# compile common globals like normal definitions
set_compiler_property(PROPERTY no_common -fno-common)

# mwdt's coverage mechanism is different with gnu
# at present, zephyr only support gnu coverage
set_compiler_property(PROPERTY coverage "")

# mwdt compiler flags for imacros. The specific header must be appended by user.
set_compiler_property(PROPERTY imacros -imacros)

# Security canaries.
#no support of -mstack-protector-guard=global"
set_compiler_property(PROPERTY security_canaries -fstack-protector-all)

#no support of _FORTIFY_SOURCE"
set_compiler_property(PROPERTY security_fortify_compile_time)
set_compiler_property(PROPERTY security_fortify_run_time)

# Required C++ flags when using mwdt
set_property(TARGET compiler-cpp PROPERTY required "-Hcplus" "-Hoff=Stackcheck_alloca")

# Compiler flag for turning off thread-safe initialization of local statics
set_property(TARGET compiler-cpp PROPERTY no_threadsafe_statics "-fno-threadsafe-statics")

# ARC MWDT does not support -fno-pic and -fno-pie flags,
# but it has PIE disabled by default - so no extra flags are required here.
set_compiler_property(PROPERTY no_position_independent "")

set_compiler_property(PROPERTY no_global_merge "")

#################################
# This section covers asm flags #
#################################

# Required ASM flags when using mwdt
set_property(TARGET asm PROPERTY required "-Hasmcpp")

if(CONFIG_ARCMWDT_LIBC)
  # We rely on the default C/C++ include locations which are provided by MWDT if we do build with
  # MW C / C++ libraries. However, for that case we still need to explicitly set header directory
  # to ASM builds (which may use 'stdbool.h').
  set_property(TARGET asm APPEND PROPERTY required "-I${NOSTDINC}")
endif()
