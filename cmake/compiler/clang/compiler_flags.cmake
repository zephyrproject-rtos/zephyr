# First step is to inherit all properties from gcc, as clang is compatible with most flags.
include(${ZEPHYR_BASE}/cmake/compiler/gcc/compiler_flags.cmake)

# Now, let's overwrite the flags that are different in clang.

# No property flag, clang doesn't understand fortify at all
set_compiler_property(PROPERTY security_fortify)

# No property flag, this is used by the native_posix, clang has problems
# compiling the native_posix with -fno-freestanding.
check_set_compiler_property(PROPERTY hosted)

# clang flags for coverage generation
set_property(TARGET compiler PROPERTY coverage --coverage -fno-inline)

# clang flag for colourful diagnostic messages
set_compiler_property(PROPERTY diagnostic -fcolor-diagnostics)

#######################################################
# This section covers flags related to warning levels #
#######################################################

# clang option standard warning base in Zephyr
check_set_compiler_property(PROPERTY warning_base
                            -Wall
                            -Wformat
                            -Wformat-security
                            -Wno-format-zero-length
                            -Wno-main
                            -Wno-unused-but-set-variable
                            -Wno-typedef-redefinition
                            -Wno-deprecated-non-prototype
)

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

set_compiler_property(PROPERTY warning_error_coding_guideline
                      -Werror=vla
                      -Wimplicit-fallthrough
                      -Wconversion
                      -Woverride-init
)
