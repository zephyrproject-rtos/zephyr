/*
 * Copyright (c) 2016 Linaro Ltd.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <touchscreen.h>

/*
 * For three touchscreen coordinates:
 *
 *   (T[0].x, T[0].y), (T[1].x, T[1].y) and (T[2].x, T[2].y)
 *
 * the corresponding display coordinates:
 *
 *   (D[0].x, D[0].y), (D[1].x, D[1].y) and (D[2].x, D[2].y)
 *
 * can be calculated by
 *
 * ( D[0].x )     ( T[0].x T[0].y 1 )     ( A )
 * ( D[1].x )  =  ( T[1].x T[1].y 1 )  X  ( B )
 * ( D[2].x )     ( T[2].x T[2].y 1 )     ( C )
 *
 * ( D[0].y )     ( T[0].x T[0].y 1 )     ( D )
 * ( D[1].y )  =  ( T[1].x T[1].y 1 )  X  ( E )
 * ( D[2].y )     ( T[2].x T[2].y 1 )     ( F )
 *
 * The unknowns A though F can be calculated by
 *
 * ( A )      ( D[0].x )
 * ( B ) = M' ( D[1].x )
 * ( C )      ( D[2].x )
 *
 * ( D )      ( D[0].y )
 * ( E ) = M' ( D[1].y )
 * ( F )      ( D[2].y )
 *
 * where M is the matrix
 *
 * ( T[0].x T[0].y 1 )
 * ( T[1].x T[1].y 1 )
 * ( T[2].x T[2].y 1 )
 *
 * and M' is the inverse of M, given by
 *
 * M' = 1/det(M) Adj(M)
 *
 */

/*
 * Shift value to scale calculations by to get greater precision but without
 * risking overflow.
 */
static const int xlat_scale = 12;

/*
 * Initialise translation constants using 3 touchscreen coordinates which
 * correspond to known display coordinates. These constants will be used by
 * subsequent calls to touchscreen_translate to translate arbitrary points.
 */
void touchscreen_set_calibration(struct touchscreen_xlat *xlat,
				 const struct touchscreen_point display[3],
				 const struct touchscreen_point touchscreen[3])
{
	/* Use shorter aliases for coords */
	const struct touchscreen_point *T = touchscreen;
	const struct touchscreen_point *D = display;

	/* Determinant of M, value limit is just under +/- 2^32 */
	s64_t det = (s64_t)T[0].x * T[1].y * 1
		  + (s64_t)T[0].y * 1      * T[2].x
		  + (s64_t)1      * T[1].x * T[2].y
		  - (s64_t)T[0].y * T[1].x * 1
		  - (s64_t)T[0].x * 1      * T[2].y
		  - (s64_t)1      * T[1].y * T[2].x;

	if (!det) {
		/*
		 * If det is zero, then seriously unsuitable calibration points
		 * have been chosen but lets avoid division by zero exception
		 * anyway, even though results will be garbage touchscreen
		 * translations...
		 */
		det = 1;
	}

	/*
	 * Calculate the adjugate of M, that is the transpose of the cofactor
	 * matrix. Values here are laid out in the order of cofactor matrix
	 * rows, and the array indices for 'adj' swapped to give the transpose.
	 * Limits for each value is just under +/- 2^32
	 */
	s64_t adj[3][3]; /* [row][column] */

	adj[0][0] = +((s64_t)T[1].y * 1      - (s64_t)1      * T[2].y);
	adj[1][0] = -((s64_t)T[1].x * 1      - (s64_t)1      * T[2].x);
	adj[2][0] = +((s64_t)T[1].x * T[2].y - (s64_t)T[1].y * T[2].x);

	adj[0][1] = -((s64_t)T[0].y * 1      - (s64_t)1      * T[2].y);
	adj[1][1] = +((s64_t)T[0].x * 1      - (s64_t)1      * T[2].x);
	adj[2][1] = -((s64_t)T[0].x * T[2].y - (s64_t)T[0].y * T[2].x);

	adj[0][2] = +((s64_t)T[0].y * 1      - (s64_t)1      * T[1].y);
	adj[1][2] = -((s64_t)T[0].x * 1      - (s64_t)1      * T[1].x);
	adj[2][2] = +((s64_t)T[0].x * T[1].y - (s64_t)T[0].y * T[1].x);

	/*
	 * In the following calculations the bits required for values are:
	 *
	 *   D[n].x	= 16 bits
	 *   adj[i][j]	= 32 bits + sign
	 *
	 * so for (D[n].x * adj[i][j]) that's 48 bits + sign.
	 *
	 * As we're summing 3 of these products here (and similar again in
	 * touchscreen_translate) then we need headroom for values 6 times the
	 * magnitude, i.e. an extra 3 bits; giving a grand total of 52 bits
	 * (48 + sign + 3). This means we have 12 more bits free in an s64_t
	 * so that's what we use for xlat_scale.
	 */
	xlat->A = ((D[0].x * adj[0][0]
		 + D[1].x * adj[0][1]
		 + D[2].x * adj[0][2]) << xlat_scale) / det;

	xlat->B = ((D[0].x * adj[1][0]
		 + D[1].x * adj[1][1]
		 + D[2].x * adj[1][2]) << xlat_scale) / det;

	xlat->C = ((D[0].x * adj[2][0]
		 + D[1].x * adj[2][1]
		 + D[2].x * adj[2][2]) << xlat_scale) / det;

	xlat->D = ((D[0].y * adj[0][0]
		 + D[1].y * adj[0][1]
		 + D[2].y * adj[0][2]) << xlat_scale) / det;

	xlat->E = ((D[0].y * adj[1][0]
		 + D[1].y * adj[1][1]
		 + D[2].y * adj[1][2]) << xlat_scale) / det;

	xlat->F = ((D[0].y * adj[2][0]
		 + D[1].y * adj[2][1]
		 + D[2].y * adj[2][2]) << xlat_scale) / det;
}


void touchscreen_translate(struct touchscreen_xlat *xlat,
			   int *display_x, int *display_y,
			   int touch_x, int touch_y)
{
	*display_x = (xlat->A * touch_x + xlat->B * touch_y + xlat->C)
			>> xlat_scale;
	*display_y = (xlat->D * touch_x + xlat->E * touch_y + xlat->F)
			>> xlat_scale;
}
