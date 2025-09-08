/*
 * Copyright (c) 2025 Silicon Laboratories Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Ensure that image properties section is preserved in the final binary.
 */

_image_properties_start = .;
KEEP(*(.image_properties));
_image_properties_end = .;
_image_properties_size = ABSOLUTE(_image_properties_end - _image_properties_start);
