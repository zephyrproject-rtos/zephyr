# Copyright (c) 2024 Nordic Semiconductor
#
# SPDX-License-Identifier: Apache-2.0

# Base properties.
# Basic linker properties which should always be applied for a Zephyr build.
set_property(TARGET linker PROPERTY base)

# Base properties when C++ support is enabled.
# Basic linker properties which should always be applied for a Zephyr build
# using C++.
set_property(TARGET linker PROPERTY cpp_base)

# Base properties when building Zephyr for an embedded target (baremetal).
set_property(TARGET linker PROPERTY baremetal)

# Property for controlling linker reporting / handling when placing orphaned sections.
set_property(TARGET linker PROPERTY orphan_warning)
set_property(TARGET linker PROPERTY orphan_error)

# coverage is a property holding the linker flag required for coverage support on the toolchain.
# For example, on ld/gcc this would be: -lgcov
# Set the property for the corresponding flags of the given toolchain.
set_property(TARGET linker PROPERTY coverage)

# Linker flag for printing of memusage.
# Set this flag if the linker supports reporting of memusage as part of link,
# such as ls --print-memory-usage flag.
# If memory reporting is a post build command, please use
# cmake/bintools/bintools.cmake instead.
check_set_linker_property(TARGET linker PROPERTY memusage)

# Extra warnings options for twister run
set_property(TARGET linker PROPERTY warnings_as_errors)

# Linker flag for disabling position independent binaries,
# such as, "-no-pie" for LD, and "--no-pie" for LLD.
set_property(TARGET linker PROPERTY no_position_independent)

# Linker flag for doing partial linking
# such as, "-r" or "--relocatable" for LD and LLD.
set_property(TARGET linker PROPERTY partial_linking)

# Linker flag for section sorting by alignment
set_property(TARGET linker PROPERTY sort_alignment)

# Linker flag for disabling relaxation of address optimization for jump calls.
set_property(TARGET linker PROPERTY no_relax)

# Linker flag for enabling relaxation of address optimization for jump calls.
set_property(TARGET linker PROPERTY relax)
