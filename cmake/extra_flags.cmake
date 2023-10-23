# SPDX-License-Identifier: Apache-2.0

foreach(extra_flags EXTRA_CPPFLAGS EXTRA_LDFLAGS EXTRA_CFLAGS EXTRA_CXXFLAGS EXTRA_AFLAGS)
  # Note: zephyr_get MERGE should not be used until issue #43959 is resolved.
  zephyr_get(${extra_flags})
  list(LENGTH ${extra_flags} flags_length)
  if(flags_length LESS_EQUAL 1)
    # A length of zero means no argument.
    # A length of one means a single argument or a space separated list was provided.
    # In both cases, it is safe to do a separate_arguments on the argument.
    separate_arguments(${extra_flags}_AS_LIST UNIX_COMMAND ${${extra_flags}})
  else()
    # Already a proper list, no conversion needed.
    set(${extra_flags}_AS_LIST "${${extra_flags}}")
  endif()
endforeach()

if(EXTRA_CPPFLAGS)
  zephyr_compile_options(${EXTRA_CPPFLAGS_AS_LIST})
endif()
if(EXTRA_LDFLAGS)
  zephyr_link_libraries(${EXTRA_LDFLAGS_AS_LIST})
endif()
if(EXTRA_CFLAGS)
  foreach(F ${EXTRA_CFLAGS_AS_LIST})
    zephyr_compile_options($<$<COMPILE_LANGUAGE:C>:${F}>)
  endforeach()
endif()
if(EXTRA_CXXFLAGS)
  foreach(F ${EXTRA_CXXFLAGS_AS_LIST})
    zephyr_compile_options($<$<COMPILE_LANGUAGE:CXX>:${F}>)
  endforeach()
endif()
if(EXTRA_AFLAGS)
  foreach(F ${EXTRA_AFLAGS_AS_LIST})
    zephyr_compile_options($<$<COMPILE_LANGUAGE:ASM>:${F}>)
  endforeach()
endif()
