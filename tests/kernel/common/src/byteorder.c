/*
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */


#include <zephyr/ztest.h>

#include <zephyr/sys/byteorder.h>

/**
 * @addtogroup kernel_common_tests
 * @{
 */
/**
 * @brief Test swapping for memory contents
 *
 * @details Verify the functionality provided by
 * sys_memcpy_swap()
 *
 * @see sys_memcpy_swap()
 */
ZTEST(byteorder, test_byteorder_memcpy_swap)
{
	uint8_t buf_orig[8] = { 0x00, 0x01, 0x02, 0x03,
			     0x04, 0x05, 0x06, 0x07 };
	uint8_t buf_chk[8] = { 0x07, 0x06, 0x05, 0x04,
			    0x03, 0x02, 0x01, 0x00 };
	uint8_t buf_dst[8] = { 0 };

	sys_memcpy_swap(buf_dst, buf_orig, 8);
	zassert_true((memcmp(buf_dst, buf_chk, 8) == 0),
		     "Swap memcpy failed");

	sys_memcpy_swap(buf_dst, buf_chk, 8);
	zassert_true((memcmp(buf_dst, buf_orig, 8) == 0),
		     "Swap memcpy failed");
}

/**
 * @brief Test sys_mem_swap() functionality
 *
 * @details Test if sys_mem_swap() reverses the contents
 *
 * @see sys_mem_swap()
 */
ZTEST(byteorder, test_byteorder_mem_swap)
{
	uint8_t buf_orig_1[8] = { 0x00, 0x01, 0x02, 0x03,
			       0x04, 0x05, 0x06, 0x07 };
	uint8_t buf_orig_2[11] = { 0x00, 0x01, 0x02, 0x03,
				0x04, 0x05, 0x06, 0x07,
				0x08, 0x09, 0xa0 };
	uint8_t buf_chk_1[8] = { 0x07, 0x06, 0x05, 0x04,
			      0x03, 0x02, 0x01, 0x00 };
	uint8_t buf_chk_2[11] = { 0xa0, 0x09, 0x08, 0x07,
			       0x06, 0x05, 0x04, 0x03,
			       0x02, 0x01, 0x00 };

	sys_mem_swap(buf_orig_1, 8);
	zassert_true((memcmp(buf_orig_1, buf_chk_1, 8) == 0),
		     "Swapping buffer failed");

	sys_mem_swap(buf_orig_2, 11);
	zassert_true((memcmp(buf_orig_2, buf_chk_2, 11) == 0),
		     "Swapping buffer failed");
}

/**
 * @brief Test sys_get_be64() functionality
 *
 * @details Test if sys_get_be64() correctly handles endianness.
 *
 * @see sys_get_be64()
 */
ZTEST(byteorder, test_sys_get_be64)
{
	uint64_t val = 0xf0e1d2c3b4a59687, tmp;
	uint8_t buf[] = {
		0xf0, 0xe1, 0xd2, 0xc3, 0xb4, 0xa5, 0x96, 0x87
	};

	tmp = sys_get_be64(buf);

	zassert_equal(tmp, val, "sys_get_be64() failed");
}

/**
 * @brief Test sys_put_be64() functionality
 *
 * @details Test if sys_put_be64() correctly handles endianness.
 *
 * @see sys_put_be64()
 */
ZTEST(byteorder, test_sys_put_be64)
{
	uint64_t val = 0xf0e1d2c3b4a59687;
	uint8_t buf[] = {
		0xf0, 0xe1, 0xd2, 0xc3, 0xb4, 0xa5, 0x96, 0x87
	};
	uint8_t tmp[sizeof(uint64_t)];

	sys_put_be64(val, tmp);

	zassert_mem_equal(tmp, buf, sizeof(uint64_t), "sys_put_be64() failed");
}

/**
 * @brief Test sys_get_be40() functionality
 *
 * @details Test if sys_get_be40() correctly handles endianness.
 *
 * @see sys_get_be40()
 */
ZTEST(byteorder, test_sys_get_be40)
{
	uint64_t val = 0xf0e1d2c3b4, tmp;
	uint8_t buf[] = {0xf0, 0xe1, 0xd2, 0xc3, 0xb4};

	tmp = sys_get_be40(buf);

	zassert_equal(tmp, val, "sys_get_be64() failed");
}

/**
 * @brief Test sys_put_be40() functionality
 *
 * @details Test if sys_put_be40() correctly handles endianness.
 *
 * @see sys_put_be40()
 */
ZTEST(byteorder, test_sys_put_be40)
{
	uint64_t val = 0xf0e1d2c3b4;
	uint8_t buf[] = {0xf0, 0xe1, 0xd2, 0xc3, 0xb4};
	uint8_t tmp[sizeof(buf)];

	sys_put_be40(val, tmp);
	zassert_mem_equal(tmp, buf, sizeof(buf), "sys_put_be40() failed");
}

/**
 * @brief Test sys_get_be48() functionality
 *
 * @details Test if sys_get_be48() correctly handles endianness.
 *
 * @see sys_get_be48()
 */
ZTEST(byteorder, test_sys_get_be48)
{
	uint64_t val = 0xf0e1d2c3b4a5, tmp;
	uint8_t buf[] = {
		0xf0, 0xe1, 0xd2, 0xc3, 0xb4, 0xa5
	};

	tmp = sys_get_be48(buf);

	zassert_equal(tmp, val, "sys_get_be64() failed");
}

/**
 * @brief Test sys_put_be48() functionality
 *
 * @details Test if sys_put_be48() correctly handles endianness.
 *
 * @see sys_put_be48()
 */
ZTEST(byteorder, test_sys_put_be48)
{
	uint64_t val = 0xf0e1d2c3b4a5;
	uint8_t buf[] = {
		0xf0, 0xe1, 0xd2, 0xc3, 0xb4, 0xa5
	};
	uint8_t tmp[sizeof(buf)];

	sys_put_be48(val, tmp);
	zassert_mem_equal(tmp, buf, sizeof(buf), "sys_put_be48() failed");
}

/**
 * @brief Test sys_get_be32() functionality
 *
 * @details Test if sys_get_be32() correctly handles endianness.
 *
 * @see sys_get_be32()
 */
ZTEST(byteorder, test_sys_get_be32)
{
	uint32_t val = 0xf0e1d2c3, tmp;
	uint8_t buf[] = {
		0xf0, 0xe1, 0xd2, 0xc3
	};

	tmp = sys_get_be32(buf);

	zassert_equal(tmp, val, "sys_get_be32() failed");
}

/**
 * @brief Test sys_put_be32() functionality
 *
 * @details Test if sys_put_be32() correctly handles endianness.
 *
 * @see sys_put_be32()
 */
ZTEST(byteorder, test_sys_put_be32)
{
	uint64_t val = 0xf0e1d2c3;
	uint8_t buf[] = {
		0xf0, 0xe1, 0xd2, 0xc3
	};
	uint8_t tmp[sizeof(uint32_t)];

	sys_put_be32(val, tmp);

	zassert_mem_equal(tmp, buf, sizeof(uint32_t), "sys_put_be32() failed");
}

/**
 * @brief Test sys_get_be24() functionality
 *
 * @details Test if sys_get_be24() correctly handles endianness.
 *
 * @see sys_get_be24()
 */
ZTEST(byteorder, test_sys_get_be24)
{
	uint32_t val = 0xf0e1d2, tmp;
	uint8_t buf[] = {
		0xf0, 0xe1, 0xd2
	};

	tmp = sys_get_be24(buf);

	zassert_equal(tmp, val, "sys_get_be24() failed");
}

/**
 * @brief Test sys_put_be24() functionality
 *
 * @details Test if sys_put_be24() correctly handles endianness.
 *
 * @see sys_put_be24()
 */
ZTEST(byteorder, test_sys_put_be24)
{
	uint64_t val = 0xf0e1d2;
	uint8_t buf[] = {
		0xf0, 0xe1, 0xd2
	};
	uint8_t tmp[sizeof(buf)];

	sys_put_be24(val, tmp);

	zassert_mem_equal(tmp, buf, sizeof(buf), "sys_put_be24() failed");
}

/**
 * @brief Test sys_get_be16() functionality
 *
 * @details Test if sys_get_be16() correctly handles endianness.
 *
 * @see sys_get_be16()
 */
ZTEST(byteorder, test_sys_get_be16)
{
	uint32_t val = 0xf0e1, tmp;
	uint8_t buf[] = {
		0xf0, 0xe1
	};

	tmp = sys_get_be16(buf);

	zassert_equal(tmp, val, "sys_get_be16() failed");
}

/**
 * @brief Test sys_put_be16() functionality
 *
 * @details Test if sys_put_be16() correctly handles endianness.
 *
 * @see sys_put_be16()
 */
ZTEST(byteorder, test_sys_put_be16)
{
	uint64_t val = 0xf0e1;
	uint8_t buf[] = {
		0xf0, 0xe1
	};
	uint8_t tmp[sizeof(uint16_t)];

	sys_put_be16(val, tmp);

	zassert_mem_equal(tmp, buf, sizeof(uint16_t), "sys_put_be16() failed");
}

/**
 * @brief Test sys_get_le16() functionality
 *
 * @details Test if sys_get_le16() correctly handles endianness.
 *
 * @see sys_get_le16()
 */
ZTEST(byteorder, test_sys_get_le16)
{
	uint32_t val = 0xf0e1, tmp;
	uint8_t buf[] = {
		0xe1, 0xf0
	};

	tmp = sys_get_le16(buf);

	zassert_equal(tmp, val, "sys_get_le16() failed");
}

/**
 * @brief Test sys_put_le16() functionality
 *
 * @details Test if sys_put_le16() correctly handles endianness.
 *
 * @see sys_put_le16()
 */
ZTEST(byteorder, test_sys_put_le16)
{
	uint64_t val = 0xf0e1;
	uint8_t buf[] = {
		0xe1, 0xf0
	};
	uint8_t tmp[sizeof(uint16_t)];

	sys_put_le16(val, tmp);

	zassert_mem_equal(tmp, buf, sizeof(uint16_t), "sys_put_le16() failed");
}

/**
 * @brief Test sys_get_le24() functionality
 *
 * @details Test if sys_get_le24() correctly handles endianness.
 *
 * @see sys_get_le24()
 */
ZTEST(byteorder, test_sys_get_le24)
{
	uint32_t val = 0xf0e1d2, tmp;
	uint8_t buf[] = {
		0xd2, 0xe1, 0xf0
	};

	tmp = sys_get_le24(buf);

	zassert_equal(tmp, val, "sys_get_le24() failed");
}

/**
 * @brief Test sys_put_le24() functionality
 *
 * @details Test if sys_put_le24() correctly handles endianness.
 *
 * @see sys_put_le24()
 */
ZTEST(byteorder, test_sys_put_le24)
{
	uint64_t val = 0xf0e1d2;
	uint8_t buf[] = {
		0xd2, 0xe1, 0xf0
	};
	uint8_t tmp[sizeof(uint32_t)];

	sys_put_le24(val, tmp);

	zassert_mem_equal(tmp, buf, sizeof(buf), "sys_put_le24() failed");
}

/**
 * @brief Test sys_get_le32() functionality
 *
 * @details Test if sys_get_le32() correctly handles endianness.
 *
 * @see sys_get_le32()
 */
ZTEST(byteorder, test_sys_get_le32)
{
	uint32_t val = 0xf0e1d2c3, tmp;
	uint8_t buf[] = {
		0xc3, 0xd2, 0xe1, 0xf0
	};

	tmp = sys_get_le32(buf);

	zassert_equal(tmp, val, "sys_get_le32() failed");
}

/**
 * @brief Test sys_put_le32() functionality
 *
 * @details Test if sys_put_le32() correctly handles endianness.
 *
 * @see sys_put_le32()
 */
ZTEST(byteorder, test_sys_put_le32)
{
	uint64_t val = 0xf0e1d2c3;
	uint8_t buf[] = {
		0xc3, 0xd2, 0xe1, 0xf0
	};
	uint8_t tmp[sizeof(uint32_t)];

	sys_put_le32(val, tmp);

	zassert_mem_equal(tmp, buf, sizeof(uint32_t), "sys_put_le32() failed");
}

/**
 * @brief Test sys_get_le40() functionality
 *
 * @details Test if sys_get_le40() correctly handles endianness.
 *
 * @see sys_get_le40()
 */
ZTEST(byteorder, test_sys_get_le40)
{
	uint64_t val = 0xf0e1d2c3b4, tmp;
	uint8_t buf[] = {0xb4, 0xc3, 0xd2, 0xe1, 0xf0};

	tmp = sys_get_le40(buf);

	zassert_equal(tmp, val, "sys_get_le40() failed");
}

/**
 * @brief Test sys_put_le40() functionality
 *
 * @details Test if sys_put_le40() correctly handles endianness.
 *
 * @see sys_put_le40()
 */
ZTEST(byteorder, test_sys_put_le40)
{
	uint64_t val = 0xf0e1d2c3b4;
	uint8_t buf[] = {0xb4, 0xc3, 0xd2, 0xe1, 0xf0};
	uint8_t tmp[sizeof(uint64_t)];

	sys_put_le40(val, tmp);

	zassert_mem_equal(tmp, buf, sizeof(buf), "sys_put_le40() failed");
}

/**
 * @brief Test sys_get_le48() functionality
 *
 * @details Test if sys_get_le48() correctly handles endianness.
 *
 * @see sys_get_le48()
 */
ZTEST(byteorder, test_sys_get_le48)
{
	uint64_t val = 0xf0e1d2c3b4a5, tmp;
	uint8_t buf[] = {
		0xa5, 0xb4, 0xc3, 0xd2, 0xe1, 0xf0
	};

	tmp = sys_get_le48(buf);

	zassert_equal(tmp, val, "sys_get_le48() failed");
}

/**
 * @brief Test sys_put_le48() functionality
 *
 * @details Test if sys_put_le48() correctly handles endianness.
 *
 * @see sys_put_le48()
 */
ZTEST(byteorder, test_sys_put_le48)
{
	uint64_t val = 0xf0e1d2c3b4a5;
	uint8_t buf[] = {
		0xa5, 0xb4, 0xc3, 0xd2, 0xe1, 0xf0
	};
	uint8_t tmp[sizeof(uint64_t)];

	sys_put_le48(val, tmp);

	zassert_mem_equal(tmp, buf, sizeof(buf), "sys_put_le48() failed");
}

/**
 * @brief Test sys_get_le64() functionality
 *
 * @details Test if sys_get_le64() correctly handles endianness.
 *
 * @see sys_get_le64()
 */
ZTEST(byteorder, test_sys_get_le64)
{
	uint64_t val = 0xf0e1d2c3b4a59687, tmp;
	uint8_t buf[] = {
		0x87, 0x96, 0xa5, 0xb4, 0xc3, 0xd2, 0xe1, 0xf0
	};

	tmp = sys_get_le64(buf);

	zassert_equal(tmp, val, "sys_get_le64() failed");
}

/**
 * @brief Test sys_put_le64() functionality
 *
 * @details Test if sys_put_le64() correctly handles endianness.
 *
 * @see sys_put_le64()
 */
ZTEST(byteorder, test_sys_put_le64)
{
	uint64_t val = 0xf0e1d2c3b4a59687;
	uint8_t buf[] = {
		0x87, 0x96, 0xa5, 0xb4, 0xc3, 0xd2, 0xe1, 0xf0
	};
	uint8_t tmp[sizeof(uint64_t)];

	sys_put_le64(val, tmp);

	zassert_mem_equal(tmp, buf, sizeof(uint64_t), "sys_put_le64() failed");
}

/**
 * @brief Test sys_uint16_to_array() functionality
 *
 * @details Test if sys_uint16_to_array() correctly handles endianness.
 *
 * @see sys_uint16_to_array()
 */
ZTEST(byteorder, test_sys_uint16_to_array)
{
	#define VAL 0xf0e1
	uint8_t tmp[sizeof(uint16_t)] = sys_uint16_to_array(VAL);
	uint8_t buf[] = {
		COND_CODE_1(CONFIG_LITTLE_ENDIAN,
		(0xe1, 0xf0),
		(0xf0, 0xe1))
	};

	zassert_mem_equal(tmp, buf, sizeof(uint16_t), "sys_uint16_to_array() failed");
	#undef VAL
}

/**
 * @brief Test sys_uint32_to_array() functionality
 *
 * @details Test if sys_uint32_to_array() correctly handles endianness.
 *
 * @see sys_uint32_to_array()
 */
ZTEST(byteorder, test_sys_uint32_to_array)
{
	#define VAL 0xf0e1d2c3
	uint8_t tmp[sizeof(uint32_t)] = sys_uint32_to_array(VAL);
	uint8_t buf[] = {
		COND_CODE_1(CONFIG_LITTLE_ENDIAN,
		(0xc3, 0xd2, 0xe1, 0xf0),
		(0xf0, 0xe1, 0xd2, 0xc3))
	};

	zassert_mem_equal(tmp, buf, sizeof(uint32_t), "sys_uint32_to_array() failed");
	#undef VAL
}

/**
 * @brief Test sys_uint64_to_array() functionality
 *
 * @details Test if sys_uint64_to_array() correctly handles endianness.
 *
 * @see sys_uint64_to_array()
 */
ZTEST(byteorder, test_sys_uint64_to_array)
{
	#define VAL 0xf0e1d2c3b4a59687
	uint8_t tmp[sizeof(uint64_t)] = sys_uint64_to_array(VAL);
	uint8_t buf[] = {
		COND_CODE_1(CONFIG_LITTLE_ENDIAN,
		(0x87, 0x96, 0xa5, 0xb4, 0xc3, 0xd2, 0xe1, 0xf0),
		(0xf0, 0xe1, 0xd2, 0xc3, 0xb4, 0xa5, 0x96, 0x87))
	};

	zassert_mem_equal(tmp, buf, sizeof(uint64_t), "sys_uint64_to_array() failed");
	#undef VAL
}

ZTEST(byteorder, test_sys_le_to_cpu)
{
	uint8_t val[9] = { 0x87, 0x95, 0xa5, 0xb4, 0xc3, 0xd2, 0xe1, 0xf0, 0xab };
	uint8_t exp[9] = {
		COND_CODE_1(CONFIG_LITTLE_ENDIAN,
		(0x87, 0x95, 0xa5, 0xb4, 0xc3, 0xd2, 0xe1, 0xf0, 0xab),
		(0xab, 0xf0, 0xe1, 0xd2, 0xc3, 0xb4, 0xa5, 0x95, 0x87))
	};

	sys_le_to_cpu(val, sizeof(val));

	zassert_mem_equal(val, exp, sizeof(exp), "sys_le_to_cpu() failed");
}

ZTEST(byteorder, test_sys_cpu_to_le)
{
	uint8_t val[9] = { 0x87, 0x96, 0xa5, 0xb4, 0xc3, 0xd2, 0xe1, 0xf0, 0xab };
	uint8_t exp[9] = {
		COND_CODE_1(CONFIG_LITTLE_ENDIAN,
		(0x87, 0x96, 0xa5, 0xb4, 0xc3, 0xd2, 0xe1, 0xf0, 0xab),
		(0xab, 0xf0, 0xe1, 0xd2, 0xc3, 0xb4, 0xa5, 0x96, 0x87))
	};

	sys_cpu_to_le(val, sizeof(val));

	zassert_mem_equal(val, exp, sizeof(exp), "sys_cpu_to_le() failed");
}

ZTEST(byteorder, test_sys_be_to_cpu)
{
	uint8_t val[9] = { 0x87, 0x97, 0xa5, 0xb4, 0xc3, 0xd2, 0xe1, 0xf0, 0xab };
	uint8_t exp[9] = {
		COND_CODE_1(CONFIG_LITTLE_ENDIAN,
		(0xab, 0xf0, 0xe1, 0xd2, 0xc3, 0xb4, 0xa5, 0x97, 0x87),
		(0x87, 0x97, 0xa5, 0xb4, 0xc3, 0xd2, 0xe1, 0xf0, 0xab))
	};

	sys_be_to_cpu(val, sizeof(val));

	zassert_mem_equal(val, exp, sizeof(exp), "sys_be_to_cpu() failed");
}

ZTEST(byteorder, test_sys_cpu_to_be)
{
	uint8_t val[9] = { 0x87, 0x98, 0xa5, 0xb4, 0xc3, 0xd2, 0xe1, 0xf0, 0xab };
	uint8_t exp[9] = {
		COND_CODE_1(CONFIG_LITTLE_ENDIAN,
		(0xab, 0xf0, 0xe1, 0xd2, 0xc3, 0xb4, 0xa5, 0x98, 0x87),
		(0x87, 0x98, 0xa5, 0xb4, 0xc3, 0xd2, 0xe1, 0xf0, 0xab))
	};

	sys_cpu_to_be(val, sizeof(val));

	zassert_mem_equal(val, exp, sizeof(exp), "sys_cpu_to_be() failed");
}

ZTEST(byteorder, test_sys_put_le)
{
	uint8_t host[9] = { 0x87, 0x12, 0xa5, 0xb4, 0xc3, 0xd2, 0xe1, 0xf0, 0xba };
	uint8_t prot[9] = { 0 };
	uint8_t exp[9] = {
		COND_CODE_1(CONFIG_LITTLE_ENDIAN,
		(0x87, 0x12, 0xa5, 0xb4, 0xc3, 0xd2, 0xe1, 0xf0, 0xba),
		(0xba, 0xf0, 0xe1, 0xd2, 0xc3, 0xb4, 0xa5, 0x12, 0x87))
	};

	sys_put_le(prot, host, sizeof(prot));

	zassert_mem_equal(prot, exp, sizeof(exp), "sys_put_le() failed");
}

ZTEST(byteorder, test_sys_put_be)
{
	uint8_t host[9] = { 0x87, 0x13, 0xa5, 0xb4, 0xc3, 0xd2, 0xe1, 0xf0, 0xba };
	uint8_t prot[9] = { 0 };
	uint8_t exp[9] = {
		COND_CODE_1(CONFIG_LITTLE_ENDIAN,
		(0xba, 0xf0, 0xe1, 0xd2, 0xc3, 0xb4, 0xa5, 0x13, 0x87),
		(0x87, 0x13, 0xa5, 0xb4, 0xc3, 0xd2, 0xe1, 0xf0, 0xba))
	};

	sys_put_be(prot, host, sizeof(prot));

	zassert_mem_equal(prot, exp, sizeof(exp), "sys_put_be() failed");
}

ZTEST(byteorder, test_sys_get_le)
{
	uint8_t prot[9] = { 0x87, 0x14, 0xa5, 0xb4, 0xc3, 0xd2, 0xe1, 0xf0, 0xba };
	uint8_t host[9] = { 0 };
	uint8_t exp[9] = {
		COND_CODE_1(CONFIG_LITTLE_ENDIAN,
		(0x87, 0x14, 0xa5, 0xb4, 0xc3, 0xd2, 0xe1, 0xf0, 0xba),
		(0xba, 0xf0, 0xe1, 0xd2, 0xc3, 0xb4, 0xa5, 0x14, 0x87))
	};

	sys_get_le(host, prot, sizeof(host));

	zassert_mem_equal(host, exp, sizeof(exp), "sys_get_le() failed");
}

ZTEST(byteorder, test_sys_get_be)
{
	uint8_t prot[9] = { 0x87, 0x15, 0xa5, 0xb4, 0xc3, 0xd2, 0xe1, 0xf0, 0xba };
	uint8_t host[9] = { 0 };
	uint8_t exp[9] = {
		COND_CODE_1(CONFIG_LITTLE_ENDIAN,
		(0xba, 0xf0, 0xe1, 0xd2, 0xc3, 0xb4, 0xa5, 0x15, 0x87),
		(0x87, 0x15, 0xa5, 0xb4, 0xc3, 0xd2, 0xe1, 0xf0, 0xba))
	};

	sys_get_be(host, prot, sizeof(host));

	zassert_mem_equal(host, exp, sizeof(exp), "sys_get_be() failed");
}

extern void *common_setup(void);
ZTEST_SUITE(byteorder, NULL, common_setup, NULL, NULL, NULL);

/**
 * @}
 */
