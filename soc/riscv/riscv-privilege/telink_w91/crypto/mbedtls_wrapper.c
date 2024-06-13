/*
 * Copyright (c) 2024 Telink Semiconductor
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <mbedtls/build_info.h>
#include <mbedtls/error.h>
#include <mbedtls/ecp.h>

/*********************************************************************
 * HW accelerated functions
 *********************************************************************/

extern int telink_w91_ecp_alt_check_pubkey(const mbedtls_ecp_group *grp,
	const mbedtls_ecp_point *pt);
extern int telink_w91_ecp_alt_mul_restartable(mbedtls_ecp_group *grp,
	mbedtls_ecp_point *R, const mbedtls_mpi *m, const mbedtls_ecp_point *P,
	int (*f_rng)(void *, unsigned char *, size_t),
	void *p_rng, mbedtls_ecp_restart_ctx *rs_ctx);
extern int telink_w91_ecp_alt_muladd_restartable(mbedtls_ecp_group *grp,
	mbedtls_ecp_point *R,
	const mbedtls_mpi *m, const mbedtls_ecp_point *P,
	const mbedtls_mpi *n, const mbedtls_ecp_point *Q,
	mbedtls_ecp_restart_ctx *rs_ctx);
#ifdef MBEDTLS_SELF_TEST
extern int telink_w91_ecp_alt_self_test(int verbose);
#endif /* MBEDTLS_SELF_TEST */

/*********************************************************************
 * LD transformed software functions
 *********************************************************************/

extern int __real_mbedtls_ecp_check_pubkey(const mbedtls_ecp_group *grp,
	const mbedtls_ecp_point *pt);
extern int __real_mbedtls_ecp_mul_restartable(mbedtls_ecp_group *grp,
	mbedtls_ecp_point *R, const mbedtls_mpi *m, const mbedtls_ecp_point *P,
	int (*f_rng)(void *, unsigned char *, size_t),
	void *p_rng, mbedtls_ecp_restart_ctx *rs_ctx);
extern int __real_mbedtls_ecp_muladd_restartable(mbedtls_ecp_group *grp,
	mbedtls_ecp_point *R,
	const mbedtls_mpi *m, const mbedtls_ecp_point *P,
	const mbedtls_mpi *n, const mbedtls_ecp_point *Q,
	mbedtls_ecp_restart_ctx *rs_ctx);
#ifdef MBEDTLS_SELF_TEST
extern int __real_mbedtls_ecp_self_test(int verbose);
#endif /* MBEDTLS_SELF_TEST */

/*********************************************************************
 * Call HW accelerated functionality if fails use software
 *********************************************************************/

int __wrap_mbedtls_ecp_check_pubkey(const mbedtls_ecp_group *grp,
	const mbedtls_ecp_point *pt)
{
	int result = telink_w91_ecp_alt_check_pubkey(grp, pt);

	if (result == MBEDTLS_ERR_PLATFORM_HW_ACCEL_FAILED) {
		result = __real_mbedtls_ecp_check_pubkey(grp, pt);
	}
	return result;
}

int __wrap_mbedtls_ecp_mul_restartable(mbedtls_ecp_group *grp,
	mbedtls_ecp_point *R, const mbedtls_mpi *m, const mbedtls_ecp_point *P,
	int (*f_rng)(void *, unsigned char *, size_t),
	void *p_rng, mbedtls_ecp_restart_ctx *rs_ctx)
{
	int result = telink_w91_ecp_alt_mul_restartable(grp, R, m, P, f_rng, p_rng, rs_ctx);

	if (result == MBEDTLS_ERR_PLATFORM_HW_ACCEL_FAILED) {
		result = __real_mbedtls_ecp_mul_restartable(grp, R, m, P, f_rng, p_rng, rs_ctx);
	}
	return result;
}

int __wrap_mbedtls_ecp_mul(mbedtls_ecp_group *grp,
	mbedtls_ecp_point *R, const mbedtls_mpi *m, const mbedtls_ecp_point *P,
	int (*f_rng)(void *, unsigned char *, size_t), void *p_rng)
{
	return __wrap_mbedtls_ecp_mul_restartable(grp, R, m, P, f_rng, p_rng, NULL);
}

int __wrap_mbedtls_ecp_muladd_restartable(mbedtls_ecp_group *grp,
	mbedtls_ecp_point *R,
	const mbedtls_mpi *m, const mbedtls_ecp_point *P,
	const mbedtls_mpi *n, const mbedtls_ecp_point *Q,
	mbedtls_ecp_restart_ctx *rs_ctx)
{
	int result = telink_w91_ecp_alt_muladd_restartable(grp, R, m, P, n, Q, rs_ctx);

	if (result == MBEDTLS_ERR_PLATFORM_HW_ACCEL_FAILED) {
		result = __real_mbedtls_ecp_muladd_restartable(grp, R, m, P, n, Q, rs_ctx);
	}
	return result;
}

int __wrap_mbedtls_ecp_muladd(mbedtls_ecp_group *grp,
	mbedtls_ecp_point *R,
	const mbedtls_mpi *m, const mbedtls_ecp_point *P,
	const mbedtls_mpi *n, const mbedtls_ecp_point *Q)
{
	return __wrap_mbedtls_ecp_muladd_restartable(grp, R, m, P, n, Q, NULL);
}

#ifdef MBEDTLS_SELF_TEST
int __wrap_mbedtls_ecp_self_test(int verbose)
{
	int result = telink_w91_ecp_alt_self_test(verbose);

	if (result == MBEDTLS_ERR_PLATFORM_HW_ACCEL_FAILED) {
		result = __real_mbedtls_ecp_self_test(verbose);
	}
	return result;
}
#endif /* MBEDTLS_SELF_TEST */
