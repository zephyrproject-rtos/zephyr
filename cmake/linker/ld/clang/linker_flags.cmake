# The coverage linker flag is specific for clang.
if (CONFIG_COVERAGE_NATIVE_GCOV)
  set_property(TARGET linker PROPERTY coverage --coverage)
elseif(CONFIG_COVERAGE_NATIVE_SOURCE)
  set_property(TARGET linker PROPERTY coverage -fprofile-instr-generate -fcoverage-mapping)
endif()

# Extra warnings options for twister run
set_property(TARGET linker PROPERTY ld_extra_warning_options ${LINKERFLAGPREFIX},--fatal-warnings)

# GNU ld and LLVM lld complains when used with llvm/clang:
#   error: section: init_array is not contiguous with other relro sections
#
# So do not create RELRO program header.
set_property(TARGET linker APPEND PROPERTY cpp_base ${LINKERFLAGPREFIX},-z,norelro)
