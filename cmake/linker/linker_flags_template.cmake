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

# Linker flag to tell the linker not to merge globals.
# When globals are merged, the address in ELF is a base address plus
# an offset. However, when userspace is enabled, our handling of
# kobject does not work with this (address + offset) when generating
# the necessary kobject data. This step requires each kobject having
# its own address. Hence the need to tell linker not to merge globals.
# This is a LLVM/Clang only option.
check_set_linker_property(TARGET linker PROPERTY no_global_merge)
