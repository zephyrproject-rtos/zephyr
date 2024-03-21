check_set_linker_property(TARGET linker PROPERTY memusage "${LINKERFLAGPREFIX},--print-memory-usage")

# -no-pie is not supported until binutils 2.37.
# If -no-pie is passed to old binutils <= 2.36, it is parsed
# as separate arguments -n and -o, which results in output file
# called "-pie".
if("${GNULD_VERSION_STRING}" VERSION_GREATER_EQUAL 2.37)
  set_property(TARGET linker PROPERTY no_position_independent "${LINKERFLAGPREFIX},-no-pie")
else()
  set_property(TARGET linker PROPERTY no_position_independent)
endif()

set_property(TARGET linker PROPERTY partial_linking "-r")

set_property(TARGET linker PROPERTY lto_arguments -flto -fno-ipa-sra -ffunction-sections -fdata-sections)

# Some linker flags might not be purely ld specific, but a combination of
# linker and compiler, such as:
# --coverage for clang
# --gcov for gcc
# So load those flags now.
include(${ZEPHYR_BASE}/cmake/linker/${LINKER}/${COMPILER}/linker_flags.cmake OPTIONAL)
