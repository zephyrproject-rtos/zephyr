/*
 * Copyright (c) 2024 Telink Semiconductor
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <mbedtls/build_info.h>
#include <mbedtls/error.h>
#include <mbedtls/ecp.h>

#include <zephyr/init.h>
#include <ipc/ipc_based_driver.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(w91_ecc, CONFIG_MBEDTLS_LOG_LEVEL);

/*********************************************************************
 * HW unit definition
 *********************************************************************/
#define TELINK_W91_ECC_MAX_BIT_LEN                521

/*********************************************************************
 * auxiliary HW unit definition (do not edit)
 *********************************************************************/
#define TELINK_W91_ECC_GET_WORD_LEN(bit_len)      (((bit_len)+31)/32)
#define TELINK_W91_ECC_GET_BYTE_LEN(bit_len)      (((bit_len)+7)/8)
#define TELINK_W91_ECC_MAX_WORD_LEN               \
	TELINK_W91_ECC_GET_WORD_LEN(TELINK_W91_ECC_MAX_BIT_LEN)

/*********************************************************************
 * IPC data Types
 *********************************************************************/

enum {
	IPC_DISPATCHER_CRYPTO_ECC_CHECK_PUBKEY = IPC_DISPATCHER_CRYPTO_ECC,
	IPC_DISPATCHER_CRYPTO_ECC_MUL,
	IPC_DISPATCHER_CRYPTO_ECC_MULADD
};

struct telink_w91_ecp_alt_check_pubkey_request {
	uint8_t curve;               /* according to mbedtls_ecp_group_id */
	uint32_t p_x[TELINK_W91_ECC_MAX_WORD_LEN];
	uint32_t p_y[TELINK_W91_ECC_MAX_WORD_LEN];
};

struct telink_w91_ecp_alt_mul_request {
	uint8_t curve;               /* according to mbedtls_ecp_group_id */
	uint32_t m[TELINK_W91_ECC_MAX_WORD_LEN];
	uint32_t p_x[TELINK_W91_ECC_MAX_WORD_LEN];
	uint32_t p_y[TELINK_W91_ECC_MAX_WORD_LEN];

};

struct telink_w91_ecp_alt_muladd_request {
	uint8_t curve;               /* according to mbedtls_ecp_group_id */
	uint32_t m[TELINK_W91_ECC_MAX_WORD_LEN];
	uint32_t p_x[TELINK_W91_ECC_MAX_WORD_LEN];
	uint32_t p_y[TELINK_W91_ECC_MAX_WORD_LEN];
	uint32_t n[TELINK_W91_ECC_MAX_WORD_LEN];
	uint32_t q_x[TELINK_W91_ECC_MAX_WORD_LEN];
	uint32_t q_y[TELINK_W91_ECC_MAX_WORD_LEN];
};

struct telink_w91_ecp_alt_math_response {
	int err;
	uint32_t r_x[TELINK_W91_ECC_MAX_WORD_LEN];
	uint32_t r_y[TELINK_W91_ECC_MAX_WORD_LEN];
};

/*********************************************************************
 * IPC data
 *********************************************************************/
static struct ipc_based_driver telink_w91_ecp_alt_ipc;

static int telink_w91_ecp_alt_init(void)
{
	ipc_based_driver_init(&telink_w91_ecp_alt_ipc);

	return 0;
}

/*********************************************************************
 * Public API
 *********************************************************************/

/* API check public key */

static size_t pack_telink_w91_ecp_alt_check_pubkey(uint8_t inst, void *unpack_data,
	uint8_t *pack_data)
{
	struct telink_w91_ecp_alt_check_pubkey_request *check_pubkey = unpack_data;
	size_t pack_data_len = sizeof(uint32_t) + sizeof(check_pubkey->curve) +
		sizeof(check_pubkey->p_x) + sizeof(check_pubkey->p_y);

	if (pack_data != NULL) {
		uint32_t id = IPC_DISPATCHER_MK_ID(IPC_DISPATCHER_CRYPTO_ECC_CHECK_PUBKEY, 0);

		IPC_DISPATCHER_PACK_FIELD(pack_data, id);
		IPC_DISPATCHER_PACK_FIELD(pack_data, check_pubkey->curve);
		IPC_DISPATCHER_PACK_ARRAY(pack_data, check_pubkey->p_x, sizeof(check_pubkey->p_x));
		IPC_DISPATCHER_PACK_ARRAY(pack_data, check_pubkey->p_y, sizeof(check_pubkey->p_y));
	}

	return pack_data_len;
}

IPC_DISPATCHER_UNPACK_FUNC_ONLY_WITH_ERROR_PARAM(telink_w91_ecp_alt_check_pubkey);

int telink_w91_ecp_alt_check_pubkey(const mbedtls_ecp_group *grp,
	const mbedtls_ecp_point *pt)
{
	int result = MBEDTLS_ERR_PLATFORM_HW_ACCEL_FAILED;

	if (grp->pbits <= TELINK_W91_ECC_MAX_BIT_LEN) {
		if (grp && pt) {
#ifdef MBEDTLS_ECP_SHORT_WEIERSTRASS_ENABLED
			if (mbedtls_ecp_get_type(grp) == MBEDTLS_ECP_TYPE_SHORT_WEIERSTRASS) {
				/* Use HW unit */
				struct telink_w91_ecp_alt_check_pubkey_request request = {
					.curve = grp->id
				};
				size_t byte_len = TELINK_W91_ECC_GET_BYTE_LEN(grp->pbits);

				(void) mbedtls_mpi_write_binary_le(&pt->MBEDTLS_PRIVATE(X),
					(unsigned char *)request.p_x, byte_len);
				(void) mbedtls_mpi_write_binary_le(&pt->MBEDTLS_PRIVATE(Y),
					(unsigned char *)request.p_y, byte_len);

				IPC_DISPATCHER_HOST_SEND_DATA(&telink_w91_ecp_alt_ipc, 0,
					telink_w91_ecp_alt_check_pubkey, &request, &result,
					CONFIG_TELINK_W91_IPC_DISPATCHER_TIMEOUT_MS);

				if (result && result != MBEDTLS_ERR_ECP_INVALID_KEY) {
					LOG_ERR("%s failed on curve %s %d", __func__,
						mbedtls_ecp_curve_info_from_grp_id(grp->id)->name,
						result);
				}
			}
#endif /* MBEDTLS_ECP_SHORT_WEIERSTRASS_ENABLED */
		} else {
			result = MBEDTLS_ERR_ECP_BAD_INPUT_DATA;
		}
	}

	return result;
}

/* API multiply */

static size_t pack_telink_w91_ecp_alt_mul_restartable(uint8_t inst, void *unpack_data,
	uint8_t *pack_data)
{
	struct telink_w91_ecp_alt_mul_request *mul = unpack_data;
	size_t pack_data_len = sizeof(uint32_t) + sizeof(mul->curve) +
		sizeof(mul->m) + sizeof(mul->p_x) + sizeof(mul->p_y);

	if (pack_data != NULL) {
		uint32_t id = IPC_DISPATCHER_MK_ID(IPC_DISPATCHER_CRYPTO_ECC_MUL, 0);

		IPC_DISPATCHER_PACK_FIELD(pack_data, id);
		IPC_DISPATCHER_PACK_FIELD(pack_data, mul->curve);
		IPC_DISPATCHER_PACK_ARRAY(pack_data, mul->m, sizeof(mul->m));
		IPC_DISPATCHER_PACK_ARRAY(pack_data, mul->p_x, sizeof(mul->p_x));
		IPC_DISPATCHER_PACK_ARRAY(pack_data, mul->p_y, sizeof(mul->p_y));
	}

	return pack_data_len;
}

static void unpack_telink_w91_ecp_alt_mul_restartable(void *unpack_data,
	const uint8_t *pack_data, size_t pack_data_len)
{
	struct telink_w91_ecp_alt_math_response *product = unpack_data;
	size_t expect_len = sizeof(uint32_t) +
		sizeof(product->err) + sizeof(product->r_x) + sizeof(product->r_y);

	if (expect_len != pack_data_len) {
		product->err = MBEDTLS_ERR_PLATFORM_HW_ACCEL_FAILED;
		return;
	}

	pack_data += sizeof(uint32_t);
	IPC_DISPATCHER_UNPACK_FIELD(pack_data, product->err);
	IPC_DISPATCHER_UNPACK_ARRAY(pack_data, product->r_x, sizeof(product->r_x));
	IPC_DISPATCHER_UNPACK_ARRAY(pack_data, product->r_y, sizeof(product->r_y));
}

int telink_w91_ecp_alt_mul_restartable(mbedtls_ecp_group *grp,
	mbedtls_ecp_point *R, const mbedtls_mpi *m, const mbedtls_ecp_point *P,
	int (*f_rng)(void *, unsigned char *, size_t),
	void *p_rng, mbedtls_ecp_restart_ctx *rs_ctx)
{
	ARG_UNUSED(f_rng);
	ARG_UNUSED(p_rng);
	ARG_UNUSED(rs_ctx);

	int result = MBEDTLS_ERR_PLATFORM_HW_ACCEL_FAILED;

	if (grp->pbits <= TELINK_W91_ECC_MAX_BIT_LEN) {
		if (grp && R && m && P) {
			if (0
#ifdef MBEDTLS_ECP_SHORT_WEIERSTRASS_ENABLED
				|| (mbedtls_ecp_get_type(grp) == MBEDTLS_ECP_TYPE_SHORT_WEIERSTRASS)
#endif /* MBEDTLS_ECP_SHORT_WEIERSTRASS_ENABLED */
#ifdef MBEDTLS_ECP_MONTGOMERY_ENABLED
				|| (grp->id == MBEDTLS_ECP_DP_CURVE25519)
#endif /* MBEDTLS_ECP_MONTGOMERY_ENABLED */
				){
				struct telink_w91_ecp_alt_mul_request request = {
					.curve = grp->id
				};
				size_t byte_len = TELINK_W91_ECC_GET_BYTE_LEN(grp->pbits);

				(void) mbedtls_mpi_write_binary_le(m,
					(unsigned char *)request.m, byte_len);
				(void) mbedtls_mpi_write_binary_le(&P->MBEDTLS_PRIVATE(X),
					(unsigned char *)request.p_x, byte_len);
				(void) mbedtls_mpi_write_binary_le(&P->MBEDTLS_PRIVATE(Y),
					(unsigned char *)request.p_y, byte_len);

				struct telink_w91_ecp_alt_math_response response = {
					.err = result
				};

				IPC_DISPATCHER_HOST_SEND_DATA(&telink_w91_ecp_alt_ipc, 0,
					telink_w91_ecp_alt_mul_restartable, &request, &response,
					CONFIG_TELINK_W91_IPC_DISPATCHER_TIMEOUT_MS);

				result = response.err;
				if (!result) {
					(void) mbedtls_mpi_read_binary_le(&R->MBEDTLS_PRIVATE(X),
						(const unsigned char *)response.r_x, byte_len);
					(void) mbedtls_mpi_read_binary_le(&R->MBEDTLS_PRIVATE(Y),
						(const unsigned char *)response.r_y, byte_len);
					(void) mbedtls_mpi_lset(&R->MBEDTLS_PRIVATE(Z), 1);
				} else {
					LOG_ERR("%s failed on curve %s %d", __func__,
						mbedtls_ecp_curve_info_from_grp_id(grp->id)->name,
						result);
				}
			}
		} else {
			result = MBEDTLS_ERR_ECP_BAD_INPUT_DATA;
		}

	}

	return result;
}

/* API multiply with summation */

static size_t pack_telink_w91_ecp_alt_muladd_restartable(uint8_t inst, void *unpack_data,
	uint8_t *pack_data)
{
	struct telink_w91_ecp_alt_muladd_request *muladd = unpack_data;
	size_t pack_data_len = sizeof(uint32_t) + sizeof(muladd->curve) +
		sizeof(muladd->m) + sizeof(muladd->p_x) + sizeof(muladd->p_y) +
		sizeof(muladd->n) + sizeof(muladd->q_x) + sizeof(muladd->q_y);

	if (pack_data != NULL) {
		uint32_t id = IPC_DISPATCHER_MK_ID(IPC_DISPATCHER_CRYPTO_ECC_MULADD, 0);

		IPC_DISPATCHER_PACK_FIELD(pack_data, id);
		IPC_DISPATCHER_PACK_FIELD(pack_data, muladd->curve);
		IPC_DISPATCHER_PACK_ARRAY(pack_data, muladd->m, sizeof(muladd->m));
		IPC_DISPATCHER_PACK_ARRAY(pack_data, muladd->p_x, sizeof(muladd->p_x));
		IPC_DISPATCHER_PACK_ARRAY(pack_data, muladd->p_y, sizeof(muladd->p_y));
		IPC_DISPATCHER_PACK_ARRAY(pack_data, muladd->n, sizeof(muladd->n));
		IPC_DISPATCHER_PACK_ARRAY(pack_data, muladd->q_x, sizeof(muladd->q_x));
		IPC_DISPATCHER_PACK_ARRAY(pack_data, muladd->q_y, sizeof(muladd->q_y));
	}

	return pack_data_len;
}

static void unpack_telink_w91_ecp_alt_muladd_restartable(void *unpack_data,
	const uint8_t *pack_data, size_t pack_data_len)
{
	struct telink_w91_ecp_alt_math_response *product = unpack_data;
	size_t expect_len = sizeof(uint32_t) +
		sizeof(product->err) + sizeof(product->r_x) + sizeof(product->r_y);

	if (expect_len != pack_data_len) {
		product->err = MBEDTLS_ERR_PLATFORM_HW_ACCEL_FAILED;
		return;
	}

	pack_data += sizeof(uint32_t);
	IPC_DISPATCHER_UNPACK_FIELD(pack_data, product->err);
	IPC_DISPATCHER_UNPACK_ARRAY(pack_data, product->r_x, sizeof(product->r_x));
	IPC_DISPATCHER_UNPACK_ARRAY(pack_data, product->r_y, sizeof(product->r_y));
}

int telink_w91_ecp_alt_muladd_restartable(mbedtls_ecp_group *grp,
	mbedtls_ecp_point *R,
	const mbedtls_mpi *m, const mbedtls_ecp_point *P,
	const mbedtls_mpi *n, const mbedtls_ecp_point *Q,
	mbedtls_ecp_restart_ctx *rs_ctx)
{
	ARG_UNUSED(rs_ctx);

	int result = MBEDTLS_ERR_PLATFORM_HW_ACCEL_FAILED;

	if (grp->pbits <= TELINK_W91_ECC_MAX_BIT_LEN) {
		if (grp && R && m && P && n && Q) {
#ifdef MBEDTLS_ECP_SHORT_WEIERSTRASS_ENABLED
			if (mbedtls_ecp_get_type(grp) == MBEDTLS_ECP_TYPE_SHORT_WEIERSTRASS) {
				struct telink_w91_ecp_alt_muladd_request request = {
					.curve = grp->id
				};
				size_t byte_len = TELINK_W91_ECC_GET_BYTE_LEN(grp->pbits);

				(void) mbedtls_mpi_write_binary_le(m,
					(unsigned char *)request.m, byte_len);
				(void) mbedtls_mpi_write_binary_le(&P->MBEDTLS_PRIVATE(X),
					(unsigned char *)request.p_x, byte_len);
				(void) mbedtls_mpi_write_binary_le(&P->MBEDTLS_PRIVATE(Y),
					(unsigned char *)request.p_y, byte_len);
				(void) mbedtls_mpi_write_binary_le(n,
					(unsigned char *)request.n, byte_len);
				(void) mbedtls_mpi_write_binary_le(&Q->MBEDTLS_PRIVATE(X),
					(unsigned char *)request.q_x, byte_len);
				(void) mbedtls_mpi_write_binary_le(&Q->MBEDTLS_PRIVATE(Y),
					(unsigned char *)request.q_y, byte_len);

				struct telink_w91_ecp_alt_math_response response = {
					.err = result
				};

				IPC_DISPATCHER_HOST_SEND_DATA(&telink_w91_ecp_alt_ipc, 0,
					telink_w91_ecp_alt_muladd_restartable, &request, &response,
					CONFIG_TELINK_W91_IPC_DISPATCHER_TIMEOUT_MS);

				result = response.err;
				if (!result) {
					(void) mbedtls_mpi_read_binary_le(&R->MBEDTLS_PRIVATE(X),
						(const unsigned char *)response.r_x, byte_len);
					(void) mbedtls_mpi_read_binary_le(&R->MBEDTLS_PRIVATE(Y),
						(const unsigned char *)response.r_y, byte_len);
					(void) mbedtls_mpi_lset(&R->MBEDTLS_PRIVATE(Z), 1);
				} else {
					LOG_ERR("%s failed on curve %s %d", __func__,
						mbedtls_ecp_curve_info_from_grp_id(grp->id)->name,
						result);
				}
			}
#endif /* MBEDTLS_ECP_SHORT_WEIERSTRASS_ENABLED */
		} else {
			result = MBEDTLS_ERR_ECP_BAD_INPUT_DATA;
		}
	}

	return result;
}

SYS_INIT(telink_w91_ecp_alt_init, POST_KERNEL, CONFIG_TELINK_W91_IPC_DRIVERS_INIT_PRIORITY);
