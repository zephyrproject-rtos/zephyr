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
