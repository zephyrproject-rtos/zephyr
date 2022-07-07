/*
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */


#include <ztest.h>

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
void test_byteorder_memcpy_swap(void)
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
void test_byteorder_mem_swap(void)
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
void test_sys_get_be64(void)
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
void test_sys_put_be64(void)
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
 * @brief Test sys_get_be48() functionality
 *
 * @details Test if sys_get_be48() correctly handles endianness.
 *
 * @see sys_get_be48()
 */
void test_sys_get_be48(void)
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
void test_sys_put_be48(void)
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
void test_sys_get_be32(void)
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
void test_sys_put_be32(void)
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
void test_sys_get_be24(void)
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
void test_sys_put_be24(void)
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
void test_sys_get_be16(void)
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
void test_sys_put_be16(void)
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
void test_sys_get_le16(void)
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
void test_sys_put_le16(void)
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
void test_sys_get_le24(void)
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
void test_sys_put_le24(void)
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
void test_sys_get_le32(void)
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
void test_sys_put_le32(void)
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
 * @brief Test sys_get_le48() functionality
 *
 * @details Test if sys_get_le48() correctly handles endianness.
 *
 * @see sys_get_le48()
 */
void test_sys_get_le48(void)
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
void test_sys_put_le48(void)
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
void test_sys_get_le64(void)
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
void test_sys_put_le64(void)
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
 * @}
 */
