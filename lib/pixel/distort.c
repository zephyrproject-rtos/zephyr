/*
 * Copyright (c) 2025 tinyVision.ai Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <string.h>
#include <stdint.h>
#include <stdarg.h>

#include <zephyr/sys/__assert.h>
#include <zephyr/pixel/distort.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(pixel_distort, CONFIG_PIXEL_LOG_LEVEL);

static inline uint8_t *pixel_buf_addr(struct pixel_buf *buf, int16_t row, int16_t col)
{
	row = CLAMP(row, 0, buf->height - 1);
	col = CLAMP(col, 0, buf->width - 1);
	return &buf->data[buf->width * row + col];
}

static inline struct pixel_pt pixel_grid_point(const struct pixel_grid *grid,
					       uint16_t grid_row, uint16_t grid_col)
{
	return grid->points[grid_row * grid->width + grid_col];
}

/*
 * To speed-up the processing, instead of computing every value, an input grid acts
 * as a lookup table. To reduce the lookup table size, the grid only contains some
 * values, and the intermediate values needs to be interpolated. The first step is
 * therefore to select a tile of that grid with 4 coordinates.
 */

__weak void pixel_distort_segment(struct pixel_buf *sbuf, struct pixel_buf *dbuf,
				  struct pixel_pt srow[2])
{
	/*
	 * For the segment of the destination image, import each pixel from the correct coordinate
	 * from the source segment.
	 *
	 * Source: distorted segment       Destination: corrected segment
	 *
	 *             s1
	 *            +
	 *           +           get source
	 *          +            coordinates   d0 + + + + d1
	 *         +                 <=
	 *       s0
	 */
	for (uint32_t a = 0, z = dbuf->width; a < dbuf->width; a++, z--) {
		struct pixel_pt pt = {
			.col = (srow[0].col * z + srow[1].col * a) / dbuf->width,
			.row = (srow[0].row * z + srow[1].row * a) / dbuf->width,
		};

		dbuf->data[a] = *pixel_buf_addr(sbuf, pt.row, pt.col);
	}
}

__weak void pixel_distort_tile(struct pixel_buf *sbuf, struct pixel_buf *dbuf,
			       struct pixel_pt tile[4], int16_t dcol, int16_t dwidth)
{
	for (int16_t a = 0, z = dbuf->height; a < dbuf->height; a++, z--) {
		/*
		 * Work on a selected segment of the image, horizontal on the destination image:
		 * this is a horizontal line of the destination and a diagonal line on the source.
		 *
		 * Source: distorted tile          Destination: corrected tile
		 *
		 *           src1
		 *             +  srow1
		 *            / \                        dst0     dst1
		 *           /  /\  src3                   +-------+  -
		 *          +  /  +       get source       |       |  a
		 *     src0  \/  /        coordinates      |-------|  =
		 *            \ /             <=           |       |  z
		 *     srow0   +                           +-------+  -
		 *          src2                         dst2     dst3
		 *
		 * Get the source segment coordinates matching this row.
		 */
		struct pixel_pt srow[] = {
			{.row = (tile[0].row * z + tile[2].row * a) / dbuf->height,
			 .col = (tile[0].col * z + tile[2].col * a) / dbuf->height},
			{.row = (tile[1].row * z + tile[3].row * a) / dbuf->height,
			 .col = (tile[1].col * z + tile[3].col * a) / dbuf->height},
		};
		struct pixel_buf dnew = {
			.data = pixel_buf_addr(dbuf, a, dcol),
			.width = dwidth,
			.height = 1,
		};

		/* Then, copy the source segment to the destination */
		pixel_distort_segment(sbuf, &dnew, srow);
	}
}

static void pixel_distort_raw8frame_to_raw8lines(struct pixel_buf *sbuf, struct pixel_buf *dbuf,
						 const struct pixel_grid *grid, int16_t grid_row)
{
	struct pixel_pt pt0 = pixel_grid_point(grid, grid_row + 0, 0);
	struct pixel_pt pt2 = pixel_grid_point(grid, grid_row + 1, 0);
	int16_t dcol0 = 0;

	/*
	 * Loop horizontally over every pair of value in the grid.
	 *
	 * - -+----+----0====1----+- - < grid_row+0
	 *    |    |    #    #    |
	 * - -+----+----2====3----+- - < grid_row+1
	 *
	 *              ^    ^
	 *      grid_col+0  grid_col+1
	 *
	 * Then, the offset within this tile will be used for interpolation between
	 * the 4 coordinates in the corners (00, 01, 10 11) of this tile.
	 */
	for (int i0 = 0, i1 = 1; i1 < grid->width; i0++, i1++) {
		/* Get the tile coordinates */
		struct pixel_pt tile[4] = {
			pt0, pixel_grid_point(grid, grid_row + 0, i1),
			pt2, pixel_grid_point(grid, grid_row + 1, i1),
		};
		/* Get the destination row coordinates */
		int16_t dcol1 = dbuf->width * i1 / (grid->width - 1);

		pixel_distort_tile(sbuf, dbuf, tile, dcol0, dcol1 - dcol0);

		memcpy(&pt0, &tile[1], sizeof(pt0));
		memcpy(&pt2, &tile[3], sizeof(pt2));

		dcol0 = dcol1;
	}
}

void pixel_distort_raw8frame_to_raw8frame(struct pixel_buf *sbuf, struct pixel_buf *dbuf,
					  const struct pixel_grid *grid)
{
	int16_t next_row;
	int16_t prev_row = 0;

	/*
	 * Loop vertically over every row of tiles of the grid
	 *
	 *       .  . . .  .        .    .    .    .    .
	 *    ___.-+--+--+-.___     +----+----+----+----+
	 *   /    /   |   \    \    |    |    |    |    |
	 *  #===================#   #===================# < grid_row+0
	 *  #    |    |    |    #   #    |    |    |    #
	 *  #===================#   #===================# < grid_row+1
	 *   \    \   |   /    /    |    |    |    |    |
	 *    """"-+--+--+-""""     +----+----+----+----+
	 *       '  ' ' '  '        '    '    '    '    '
	 *
	 * Then, the offset within this tile will be used for interpolation between
	 * the 4 coordinates in the corners (00, 01, 10 11) of this tile.
	 *
	 */
	for (int i = 0; i + 1 < grid->height; i++) {
		next_row = dbuf->height * (i + 1) / (grid->height - 1);

		struct pixel_buf drow = {
			.data = pixel_buf_addr(dbuf, prev_row, 0),
			.width = dbuf->width,
			.height = next_row - prev_row,
		};

		prev_row = next_row;
		pixel_distort_raw8frame_to_raw8lines(sbuf, &drow, grid, i);
	}
}
