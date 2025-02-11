# The coverage linker flag is specific for clang.
if (CONFIG_COVERAGE_NATIVE_GCOV)
  set_property(TARGET linker PROPERTY coverage --coverage)
elseif(CONFIG_COVERAGE_NATIVE_SOURCE)
  set_property(TARGET linker PROPERTY coverage -fprofile-instr-generate -fcoverage-mapping)
endif()

# Extra warnings options for twister run
set_property(TARGET linker PROPERTY ld_extra_warning_options -Wl,--fatal-warnings)
