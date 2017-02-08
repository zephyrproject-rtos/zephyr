find_path(Regex_INCLUDE_DIR NAMES regex.h DOC "libregex include directory")
mark_as_advanced(Regex_INCLUDE_DIR)

find_library(Regex_LIBRARY "regex" DOC "libregex libraries")
mark_as_advanced(Regex_LIBRARY)

find_package_handle_standard_args(Regex
  FOUND_VAR Regex_FOUND
  REQUIRED_VARS Regex_INCLUDE_DIR
  FAIL_MESSAGE "Failed to find libregex"
)

if(Regex_FOUND)
  set(Regex_INCLUDE_DIRS ${Regex_INCLUDE_DIRS})
  if(Regex_LIBRARY)
    set(Regex_LIBRARIES ${Regex_LIBRARY})
  else()
    unset(Regex_LIBRARIES)
  endif()
endif()
