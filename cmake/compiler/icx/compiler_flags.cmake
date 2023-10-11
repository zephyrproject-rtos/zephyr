
include(${ZEPHYR_BASE}/cmake/compiler/clang/compiler_flags.cmake)

# Remove after testing that -Wshadow works
set_compiler_property(PROPERTY warning_shadow_variables)
