# SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
# SPDX-License-Identifier: Apache-2.0

# Add the swapped app to the build
ExternalZephyrProject_Add(
  APPLICATION swapped_app
  SOURCE_DIR ${APP_DIR}/swapped_app
)

add_dependencies(${DEFAULT_IMAGE} swapped_app)
sysbuild_add_dependencies(CONFIGURE ${DEFAULT_IMAGE} swapped_app)
