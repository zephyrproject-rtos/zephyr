# The coverage linker flag is specific for gcc.

# Using a config check is ok for now, but in future it would be desired if
# linker flags themselves are not depending on actual configurations.
# All flags should be described, and the caller should know the flag name to use.
if (NOT CONFIG_COVERAGE_GCOV)
  set_property(TARGET linker PROPERTY coverage -lgcov)
endif()

check_set_linker_property(TARGET linker APPEND PROPERTY gprof -pg)

# Extra warnings options for twister run
set_property(TARGET linker PROPERTY warnings_as_errors -Wl,--fatal-warnings)

set_linker_property(PROPERTY specs -specs=)
