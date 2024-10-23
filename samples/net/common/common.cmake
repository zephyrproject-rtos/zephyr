# SPDX-License-Identifier: Apache-2.0

# Common routines used in net samples

target_include_directories(app PRIVATE ${ZEPHYR_BASE}/samples/net/common/)
target_sources(app PRIVATE $ENV{ZEPHYR_BASE}/samples/net/common/net_sample_common.c)
