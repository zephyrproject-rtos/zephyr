# SPDX-License-Identifier: Apache-2.0

.domain_configs :
{
  _domain_configs_start = .;
  KEEP(*(.domain_configs))
  _domain_configs_end = .;
} > RAM
