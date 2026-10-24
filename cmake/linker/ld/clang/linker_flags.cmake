# The coverage linker flag is specific for clang.
if(CONFIG_COVERAGE_NATIVE_GCOV)
  set_property(TARGET linker PROPERTY coverage --coverage)
elseif(CONFIG_COVERAGE_NATIVE_SOURCE)
  set_property(TARGET linker PROPERTY coverage -fprofile-instr-generate -fcoverage-mapping)
endif()

# Position Independent Executable (PIE) and no dynamic linker
set_property(TARGET linker PROPERTY position_independent_elf "${LINKERFLAGPREFIX},-pie  ${LINKERFLAGPREFIX},--no-dynamic-linker")

# Extra warnings options for twister run
set_property(TARGET linker PROPERTY ld_extra_warning_options ${LINKERFLAGPREFIX},--fatal-warnings)

# GNU ld and LLVM lld complains when used with llvm/clang:
#   error: section: init_array is not contiguous with other relro sections
#
# So do not create RELRO program header.
set_property(TARGET linker APPEND PROPERTY cpp_base ${LINKERFLAGPREFIX},-z,norelro)
