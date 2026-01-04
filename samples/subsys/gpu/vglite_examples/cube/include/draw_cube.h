/*
 * Copyright 2020-2021, 2023-2025 NXP
 * All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
#ifndef _DRAW_CUBE_H_
#define _DRAW_CUBE_H_

#include <stdbool.h>
/*
 *****************************************************************************
 * API
 *****************************************************************************
 */

#if defined(__cplusplus)
extern "C" {
#endif

/*!
 * @brief Load Texture images.
 *
 */
bool load_texture_images(void);

/*!
 * @brief Draw cube.
 *
 */
void draw_cube(vg_lite_buffer_t *rt);

#if defined(__cplusplus)
}
#endif

/*!
 * @}
 */
#endif /* _DRAW_CUBE_H_ */

/*
 *****************************************************************************
 * EOF
 *****************************************************************************
 */
