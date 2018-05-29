/*
 * Copyright (c) 2012-2014 Wind River Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr.h>
#include <misc/printk.h>
#include <tfm_sst_veneers.h>
#include <tfm_ss_core_test_veneers.h>
#include <tfm_ss_core_test_2_veneers.h>

#define CORE_TEST_ID_NS_THREAD          1001
#define CORE_TEST_ID_NS_SVC             1002
#define CORE_TEST_ID_CHECK_INIT         1003
#define CORE_TEST_ID_RECURSION          1004
#define CORE_TEST_ID_MEMORY_PERMISSIONS 1005
#define CORE_TEST_ID_MPU_ACCESS         1006
#define CORE_TEST_ID_BUFFER_CHECK       1007
#define CORE_TEST_ID_SS_TO_SS           1008
#define CORE_TEST_ID_SHARE_REDIRECTION  1009
#define CORE_TEST_ID_SS_TO_SS_BUFFER    1010
#define CORE_TEST_ID_TWO_SFN_ONE_SVC    1011
#define CORE_TEST_ID_PERIPHERAL_ACCESS  1012
#define CORE_TEST_ID_BLOCK              2001
#define CORE_TEST_RETURN_ERROR(x) return (((__LINE__) << 16) | x)
#define CORE_TEST_ERROR_GET_EXTRA(x) (x >> 16)
#define CORE_TEST_ERROR_GET_CODE(x) (x & 0xFFFF)

enum core_test_errno_t {
    CORE_TEST_ERRNO_SUCCESS = 0,
    CORE_TEST_ERRNO_SERVICE_NOT_INITED = TFM_SERVICE_SPECIFIC_ERROR_MIN,
    CORE_TEST_ERRNO_UNEXPECTED_CORE_BEHAVIOUR,
    CORE_TEST_ERRNO_SERVICE_RECURSION_NOT_REJECTED,
    CORE_TEST_ERRNO_INVALID_BUFFER,
    CORE_TEST_ERRNO_SLAVE_SERVICE_CALL_FAILURE,
    CORE_TEST_ERRNO_SLAVE_SERVICE_BUFFER_FAILURE,
    CORE_TEST_ERRNO_FIRST_CALL_FAILED,
    CORE_TEST_ERRNO_SECOND_CALL_FAILED,
    CORE_TEST_ERRNO_PERIPHERAL_ACCESS_FAILED,
    CORE_TEST_ERRNO_TEST_FAULT,
    CORE_TEST_ERRNO_INVALID_TEST_ID,
    /* Following entry is only to ensure the error code of int size */
    CORE_TEST_ERRNO_FORCE_INT_SIZE = INT_MAX
};

static char *error_to_string(const char *desc, int32_t err)
{
    static char info[80];

    sprintf(info, "%s. Error code: %d, extra data: %d",
        desc,
        CORE_TEST_ERROR_GET_CODE(err),
        CORE_TEST_ERROR_GET_EXTRA(err));
    return info;
}


#define TOSTRING(x) #x
#define CORE_TEST_DESCRIPTION(number, fn, description) \
    {fn, "TFM_CORE_TEST_"TOSTRING(number),\
     description, {0} }

enum test_status_t {
    TEST_PASSED = 0,  /*!< Test has passed */
    TEST_FAILED = 1,  /*!< Test has failed */
};

struct test_result_t {
    enum test_status_t val;  /*!< Test result \ref test_status_t */
    const char *info_msg;    /*!< Information message to show in case of
                              *   failure
                              */
    const char *filename;    /*!< Filename where the failure has occured */
    uint32_t    line;        /*!< Line where the failure has occured */
};

static void tfm_core_test_ns_thread(struct test_result_t *ret);
static void tfm_core_test_ns_svc(struct test_result_t *ret);
static void tfm_core_test_check_init(struct test_result_t *ret);
static void tfm_core_test_recursion(struct test_result_t *ret);
static void tfm_core_test_permissions(struct test_result_t *ret);
static void tfm_core_test_mpu_access(struct test_result_t *ret);
static void tfm_core_test_buffer_check(struct test_result_t *ret);
static void tfm_core_test_ss_to_ss(struct test_result_t *ret);
static void tfm_core_test_share_change(struct test_result_t *ret);
static void tfm_core_test_ss_to_ss_buffer(struct test_result_t *ret);
static void tfm_core_test_two_sfn_one_svc(struct test_result_t *ret);
static void tfm_core_test_peripheral_access(struct test_result_t *ret);

typedef void TEST_FUN(struct test_result_t *ret);

struct test_t {
	TEST_FUN * const test;
	const char *name;
	const char *desc;
	struct test_result_t ret;
};

enum tfm_svc_num {
    SVC_INVALID = 0,

/* SVC API for SST */
    SVC_TFM_SST_GET_HANDLE = 4,
    SVC_TFM_SST_CREATE,
    SVC_TFM_SST_GET_ATTRIBUTES,
    SVC_TFM_SST_READ,
    SVC_TFM_SST_WRITE,
    SVC_TFM_SST_DELETE,

    SVC_TFM_CORE_TEST,
    SVC_TFM_CORE_TEST_MULTIPLE_CALLS,
};

#define SVC(code) __asm__ volatile ("svc %0" : : "I" (code))

__attribute__ ((naked)) int32_t tfm_core_test_svc(void *fn_ptr, int32_t args[])
{
    SVC(SVC_TFM_CORE_TEST);
    __asm__ volatile ("BX LR");
}

__attribute__ ((naked)) int32_t tfm_core_test_multiple_calls_svc(void *fn_ptr,
                                                                 int32_t args[])
{
    SVC(SVC_TFM_CORE_TEST_MULTIPLE_CALLS);
    __asm__ volatile ("BX LR");
}

int32_t args[4] = {0};

void tfm_core_test_ns_thread(struct test_result_t *ret)
{
   int32_t err = tfm_core_test_sfn(CORE_TEST_ID_NS_THREAD, 0, 0, 0);

    printk("%s - test started\n", __func__);
    if (err == TFM_SUCCESS) {
        printk("%s - TFM Core allowed service to run.", __func__);
        return;
    }

    printk("%s - test passed\n", __func__);
    ret->val = TEST_PASSED;
}

void tfm_core_test_ns_svc(struct test_result_t *ret)
{
    args[0] = CORE_TEST_ID_NS_SVC;
    int32_t err = tfm_core_test_svc(tfm_core_test_sfn, args);

    printk("%s - test started\n", __func__);
    if (err != TFM_SUCCESS) {
        printk("%s - Service failed to run.", __func__);
        return;
    }

    printk("%s - test passed\n", __func__);
    ret->val = TEST_PASSED;
}

void tfm_core_test_check_init(struct test_result_t *ret)
{
    int32_t err;

    printk("%s - test started\n", __func__);
    err = tfm_core_test_svc(tfm_core_test_sfn_init_success, args);

    if ((err != TFM_SUCCESS) && (err < TFM_SERVICE_SPECIFIC_ERROR_MIN)) {
        printk("%s - TFM Core returned error.", __func__);
        return;
    }

    if (err != CORE_TEST_ERRNO_SUCCESS) {
        printk("%s- Failed to initialise test service.", __func__);
        return;
    }

    printk("%s - test passed\n", __func__);
    ret->val = TEST_PASSED;
}

static void tfm_core_test_recursion(struct test_result_t *ret)
{
    int32_t err;

    printk("%s - test started\n", __func__);
    /* Initialize depth argument to 0 */
    args[0] = 0;
    err = tfm_core_test_svc(tfm_core_test_sfn_direct_recursion, args);

    if (err != TFM_SUCCESS && err < 256) {
        printk("%s - TFM Core returned error.", __func__);
        return;
    }

    switch (err) {
    case CORE_TEST_ERRNO_SUCCESS:
        ret->val = TEST_PASSED;
	printk("%s - test passed\n", __func__);
        return;
    case CORE_TEST_ERRNO_SERVICE_RECURSION_NOT_REJECTED:
        printk("%s - TF-M Core failed to reject recursive service call.", __func__);
        return;
    default:
        printk("%s - Unexpected return value received.", __func__);
        return;
    }
}

static void tfm_core_test_mpu_access(struct test_result_t *ret)
{
    int32_t err;

    printk("%s - test started\n", __func__);
    int32_t args[] = {
        CORE_TEST_ID_MPU_ACCESS,
        (int32_t)args,
        /* Make sure the pointer in code segment is word-aligned */
        (int32_t)tfm_core_test_mpu_access & (~(0x3)),
        (int32_t)args};

    err = tfm_core_test_svc(tfm_core_test_sfn, args);

    if (err != TFM_SUCCESS && err < TFM_SERVICE_SPECIFIC_ERROR_MIN) {
        printk("%s - TFM Core returned error.", __func__);
        return;
    }

    if (err != CORE_TEST_ERRNO_SUCCESS) {
        char *info = error_to_string(
            "Service memory accesses configured incorrectly.", err);
        printk("%s - %s\n", __func__, info);
        return;
    }

    printk("%s - test passed\n", __func__);
    ret->val = TEST_PASSED;
}

static void tfm_core_test_permissions(struct test_result_t *ret)
{
    int32_t err;

    printk("%s - test started\n", __func__);
    int32_t args[] = {
        CORE_TEST_ID_MEMORY_PERMISSIONS,
        (int32_t)args,
        /* Make sure the pointer in code segment is word-aligned */
        (int32_t)tfm_core_test_mpu_access & (~(0x3)),
        (int32_t)args};

    err = tfm_core_test_svc(tfm_core_test_sfn, args);

    if (err != TFM_SUCCESS && err < TFM_SERVICE_SPECIFIC_ERROR_MIN) {
        printk("%s - TFM Core returned error.", __func__);
        return;
    }

    if (err != CORE_TEST_ERRNO_SUCCESS) {
        char *info = error_to_string(
            "Service memory accesses configured incorrectly.", err);
        printk("%s - %s\n", __func__, info);
        return;
    }

    printk("%s - test passed\n", __func__);
    ret->val = TEST_PASSED;
}

static void tfm_core_test_buffer_check(struct test_result_t *ret)
{
    int32_t res, i;

    uint32_t inbuf[] = {1, 2, 3, 4, 0xAAAFFF, 0xFFFFFFFF};
    uint32_t outbuf[16] = {0};
    int32_t result;
    int32_t args[] = {(int32_t)&result, (int32_t)inbuf,
                  (int32_t)outbuf, (int32_t)sizeof(inbuf) >> 2};

    printk("%s - test started\n", __func__);
    res = tfm_core_test_svc(tfm_core_test_2_sfn_invert, args);
    if ((res != TFM_SUCCESS) && (res < TFM_SERVICE_SPECIFIC_ERROR_MIN)) {
        printk("%s - Call to secure service should be successful.", __func__);
        return;
    }
    if ((result == 0) && (res == TFM_SUCCESS)) {
        for (i = 0; i < sizeof(inbuf) >> 2; i++) {
            if (outbuf[i] != ~inbuf[i]) {
                printk("%s - Secure function failed to modify buffer.", __func__);
                return;
            }
        }
        for (; i < sizeof(outbuf) >> 2; i++) {
            if (outbuf[i] != 0) {
                printk("%s - Secure function buffer access overflow.", __func__);
                return;
            }
        }
    } else {
        printk("%s - Secure service returned error.", __func__);
        return;
    }

    printk("%s - test passed\n", __func__);
    ret->val = TEST_PASSED;
}

static void tfm_core_test_ss_to_ss(struct test_result_t *ret)
{
    int32_t err;

    printk("%s - test started\n", __func__);
    args[0] = CORE_TEST_ID_SS_TO_SS;
    err = tfm_core_test_svc(tfm_core_test_sfn, args);

    if (err != TFM_SUCCESS && err < TFM_SERVICE_SPECIFIC_ERROR_MIN) {
        printk("%s - Call to secure service should be successful.", __func__);
        return;
    }

    if (err != CORE_TEST_ERRNO_SUCCESS) {
        printk("%s - The internal service call failed.", __func__);
        return;
    }

    printk("%s - test passed\n", __func__);
    ret->val = TEST_PASSED;
}

static void tfm_core_test_share_change(struct test_result_t *ret)
{
    int32_t err;

    printk("%s - test started\n", __func__);
    args[0] = CORE_TEST_ID_SHARE_REDIRECTION;
    err = tfm_core_test_svc(tfm_core_test_sfn, args);

    if ((err != TFM_SUCCESS) && (err < TFM_SERVICE_SPECIFIC_ERROR_MIN)) {
        printk("%s - TFM Core returned error.", __func__);
        return;
    }

    if (err != CORE_TEST_ERRNO_SUCCESS) {
        printk("%s - Failed to redirect share region in service.", __func__);
        return;
    }

    printk("%s - test passed\n", __func__);
    ret->val = TEST_PASSED;
}

static void tfm_core_test_ss_to_ss_buffer(struct test_result_t *ret)
{
    int32_t res, i;

    uint32_t inbuf[] = {1, 2, 3, 4, 0xAAAFFF, 0xFFFFFFFF};
    uint32_t outbuf[16] = {0};
    int32_t args[] = {CORE_TEST_ID_SS_TO_SS_BUFFER, (int32_t)inbuf,
                  (int32_t)outbuf, (int32_t)sizeof(inbuf) >> 2};

    printk("%s - test started\n", __func__);
    res = tfm_core_test_svc(tfm_core_test_sfn, args);
    if ((res != TFM_SUCCESS) && (res < TFM_SERVICE_SPECIFIC_ERROR_MIN)) {
        printk("%s - Call to secure service should be successful.", __func__);
        return;
    }
    switch (res) {
    case TFM_SUCCESS:
        for (i = 0; i < sizeof(inbuf) >> 2; i++) {
            if (outbuf[i] != ~inbuf[i]) {
                printk("%s - Secure function failed to modify buffer.", __func__);
                return;
            }
        }
        for (; i < sizeof(outbuf) >> 2; i++) {
            if (outbuf[i] != 0) {
                printk("%s - Secure function buffer access overflow.", __func__);
                return;
            }
        }
	printk("%s - test passed\n", __func__);
        ret->val = TEST_PASSED;
        return;
    case CORE_TEST_ERRNO_INVALID_BUFFER:
        printk("%s - NS buffer rejected by TF-M core.", __func__);
        return;
    case CORE_TEST_ERRNO_SLAVE_SERVICE_CALL_FAILURE:
        printk("%s - Slave service call failed.", __func__);
        return;
    case CORE_TEST_ERRNO_SLAVE_SERVICE_BUFFER_FAILURE:
        printk("%s - Slave secure function failed to modify buffer.", __func__);
        return;
    default:
        printk("%s - Secure service returned error.", __func__);
        return;
    }
}

static void tfm_core_test_two_sfn_one_svc(struct test_result_t *ret)
{
    int32_t err;

    printk("%s - test started\n", __func__);
    args[0] = CORE_TEST_ID_TWO_SFN_ONE_SVC;
    err = tfm_core_test_multiple_calls_svc(tfm_core_test_sfn, args);

    if ((err != TFM_SUCCESS) && (err < TFM_SERVICE_SPECIFIC_ERROR_MIN)) {
        printk("%s - TFM Core returned error.", __func__);
        return;
    }

    switch (err) {
    case TFM_SUCCESS:
        ret->val = TEST_PASSED;
	printk("%s - test passed\n", __func__);
        return;
    case CORE_TEST_ERRNO_FIRST_CALL_FAILED:
        printk("%s - First call failed.", __func__);
        return;
    case CORE_TEST_ERRNO_SECOND_CALL_FAILED:
        printk("%s - Second call failed.", __func__);
        return;
    default:
        printk("%s - Secure service returned unknown error.", __func__);
        return;
    }
}

static void tfm_core_test_peripheral_access(struct test_result_t *ret)
{
    args[0] = CORE_TEST_ID_PERIPHERAL_ACCESS;

    int32_t err = tfm_core_test_svc(tfm_core_test_sfn, args);

    printk("%s - test started\n", __func__);
    if ((err != TFM_SUCCESS) && (err < TFM_SERVICE_SPECIFIC_ERROR_MIN)) {
        printk("%s - TFM Core returned error.", __func__);
        return;
    }

    switch (err) {
    case CORE_TEST_ERRNO_SUCCESS:
        ret->val = TEST_PASSED;
	printk("%s - test passed\n", __func__);
        return;
    case CORE_TEST_ERRNO_PERIPHERAL_ACCESS_FAILED:
        printk("%s - Service peripheral access failed.", __func__);
        return;
    default:
        printk("%s - Unexpected return value received.", __func__);
        return;
    }
}

#define SST_ASSET_ID_X509_CERT_LARGE 10
#define INVALID_ASSET_ID           0xFFFF
static struct test_result_t transient_result;

static void tfm_sst_test_1001(void)
{
    const uint16_t asset_uuid = SST_ASSET_ID_X509_CERT_LARGE;
    enum tfm_sst_err_t err;
    uint32_t hdl;
    struct test_result_t *ret = &transient_result;
    uint32_t args[4] = {0};

    printk("%s - test started\n", __func__);
    /* Checks write permissions in create function */
    args[0] = 11;
    args[1] = asset_uuid;
    err = tfm_core_test_svc(tfm_sst_veneer_create, args);
    if (err != TFM_SST_ERR_SUCCESS) {
        printk("%s - %d: Create should not fail for Thread_C", __func__, err);
        return;
    }

    args[0] = 11;
    args[1] = asset_uuid;
    args[2] = (uint32_t)&hdl;
    err = tfm_core_test_svc(tfm_sst_veneer_get_handle, args);
    if (err != TFM_SST_ERR_SUCCESS) {
        printk("%s - Get handle should return a valid asset handle\n", __func__);
        return;
    }

    /* Calls create with invalid asset ID */
    args[0] = 11;
    args[1] = INVALID_ASSET_ID;
    err = tfm_core_test_svc(tfm_sst_veneer_create, args);
    if (err != TFM_SST_ERR_ASSET_NOT_FOUND) {
        printk("%s - Create should fail for invalid ASSET ID\n", __func__);
        return;
    }

    /* Calls delete asset to clean up SST area for next test */
    args[0] = 11;
    args[1] = hdl;
    err = tfm_core_test_svc(tfm_sst_veneer_delete, args);
    if (err != TFM_SST_ERR_SUCCESS) {
        printk("%s - The delete action should work correctly\n", __func__);
        return;
    }

    printk("%s - test passed\n", __func__);
    ret->val = TEST_PASSED;
}

static void tfm_sst_test_1002(void)
{
    struct test_result_t *ret = &transient_result;
    enum tfm_sst_err_t err;
    const uint16_t asset_uuid = SST_ASSET_ID_X509_CERT_LARGE;
    /* Calls create with invalid application ID */
    printk("%s - test started\n", __func__);
    args[0] = 10;
    args[1] = asset_uuid;
    err = tfm_core_test_svc(tfm_sst_veneer_create, args);
    if (err != TFM_SST_ERR_ASSET_NOT_FOUND) {
        printk("%s - Create should fail for thread without write permissions", __func__);
	return;
    }

    ret->val = TEST_PASSED;
    printk("%s - test passed\n", __func__);
}


void main(void)
{
	struct test_result_t foo;

	while (1) {

		printk("=============================================================\n");
		/* core tests */
		tfm_core_test_ns_thread(&foo);
		k_sleep(K_MSEC(1000));
		tfm_core_test_ns_svc(&foo);
		k_sleep(K_MSEC(1000));
		tfm_core_test_check_init(&foo);
		k_sleep(K_MSEC(1000));
		tfm_core_test_recursion(&foo);
		k_sleep(K_MSEC(1000));
		tfm_core_test_permissions(&foo);
		k_sleep(K_MSEC(1000));
		tfm_core_test_mpu_access(&foo);
		k_sleep(K_MSEC(1000));
		tfm_core_test_buffer_check(&foo);
		k_sleep(K_MSEC(1000));
		tfm_core_test_ss_to_ss(&foo);
		k_sleep(K_MSEC(1000));
		tfm_core_test_share_change(&foo);
		k_sleep(K_MSEC(1000));
		tfm_core_test_ss_to_ss_buffer(&foo);
		k_sleep(K_MSEC(1000));
		tfm_core_test_two_sfn_one_svc(&foo);
		k_sleep(K_MSEC(1000));
		tfm_core_test_peripheral_access(&foo);
		k_sleep(K_MSEC(1000));

		/* sst tests */
		tfm_sst_test_1001();
		k_sleep(K_MSEC(1000));
		tfm_sst_test_1002();
		k_sleep(K_MSEC(1000));
	}
}
