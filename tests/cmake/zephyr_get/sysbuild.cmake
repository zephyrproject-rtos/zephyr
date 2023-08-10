# SPDX-License-Identifier: Apache-2.0

# Add a few copies of the same image, so that we can configure
# multiple instances of the same test suite with minor tweaks,
# including different arguments given to them via testcase.yaml.
foreach(suffix "2nd" "3rd")
  set(image ${DEFAULT_IMAGE}_${suffix})
  if(NOT TARGET ${image})
    ExternalZephyrProject_Add(
      APPLICATION ${image}
      SOURCE_DIR ${APP_DIR}
    )
    list(APPEND IMAGES ${image})
  endif()
endforeach()

function(test_report)
  set(failures "")
  foreach(image ${IMAGES})
    sysbuild_get(ASSERT_FAIL_COUNT IMAGE ${image} CACHE)
    if(ASSERT_FAIL_COUNT GREATER 0)
      set(failures "${failures}\n  - ${image}: ${ASSERT_FAIL_COUNT} assertion(s) failed")
    endif()
    unset(ASSERT_FAIL_COUNT)
  endforeach()

  if(failures)
    message(FATAL_ERROR "Test failures per sysbuild image: ${failures}")
  endif()
endfunction()

if(NOT DEFERRED_TEST_REPORT)
  # Defer until all test suite copies have been executed.
  cmake_language(DEFER ID_VAR DEFERRED_TEST_REPORT CALL test_report)
endif()
