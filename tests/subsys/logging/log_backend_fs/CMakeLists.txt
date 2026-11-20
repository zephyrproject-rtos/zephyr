# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.28.0)
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(log_backend_fs_test)

if(DEFINED LOG_BACKEND_FS_TEST_REMAINING_SPACE)
  set(app_sources "src/log_fs_startup_test.c")
  set(log_dir "/lfs1/startup")
  target_compile_definitions(app PRIVATE
    TEST_REMAINING_SPACE=${LOG_BACKEND_FS_TEST_REMAINING_SPACE}
  )
else()
  set(app_sources "src/log_fs_test.c")
  set(log_dir "/lfs1")
endif()
target_sources(app PRIVATE
  ${ZEPHYR_BASE}/subsys/logging/backends/log_backend_fs.c
  ${app_sources}
)

target_compile_definitions(app PRIVATE
  CONFIG_LOG_BACKEND_FS_OUTPUT_DEFAULT=0
  CONFIG_LOG_BACKEND_FS_FILE_PREFIX="log."
  CONFIG_LOG_BACKEND_FS_DIR="${log_dir}"
  CONFIG_LOG_BACKEND_FS_FILE_SIZE=128
  CONFIG_LOG_BACKEND_FS_FILES_LIMIT=4
  CONFIG_LOG_BACKEND_FS_OVERWRITE=1
  CONFIG_LOG_BACKEND_FS_APPEND_TO_NEWEST_FILE=1
)
