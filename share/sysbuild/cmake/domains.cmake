# Copyright (c) 2021 Nordic Semiconductor
#
# SPDX-License-Identifier: Apache-2.0

set(domains_yaml "default: ${DEFAULT_IMAGE}")
set(domains_yaml "${domains_yaml}\nbuild_dir: ${CMAKE_BINARY_DIR}")
set(domains_yaml "${domains_yaml}\ndomains:")
foreach(image ${IMAGES})
  set(domains_yaml "${domains_yaml}\n  - name: ${image}")
  set(domains_yaml "${domains_yaml}\n    build_dir: $<TARGET_PROPERTY:${image},_EP_BINARY_DIR>")
endforeach()
file(GENERATE OUTPUT ${CMAKE_BINARY_DIR}/domains.yaml CONTENT "${domains_yaml}")
