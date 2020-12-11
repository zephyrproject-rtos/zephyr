check_set_linker_property(TARGET linker PROPERTY memusage "${LINKERFLAGPREFIX},--print-memory-usage")

# Some linker flags might not be purely ld specific, but a combination of
# linker and compiler, such as:
# --coverage for clang
# --gcov for gcc
# So load those flags now.
include(${ZEPHYR_BASE}/cmake/linker/${LINKER}/${COMPILER}/linker_flags.cmake OPTIONAL)
