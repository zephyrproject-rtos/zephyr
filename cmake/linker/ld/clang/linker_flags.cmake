# The coverage linker flag is specific for clang.
if (NOT CONFIG_COVERAGE_GCOV)
  set_property(TARGET linker PROPERTY coverage --coverage)
endif()

# Extra warnings options for twister run
set_property(TARGET linker PROPERTY ld_extra_warning_options -Wl,--fatal-warnings)
