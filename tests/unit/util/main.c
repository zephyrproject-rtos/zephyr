/*
 * Copyright (c) 2019 Oticon A/S
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/ztest.h>
#include <zephyr/sys/util.h>
#include <string.h>

#include "test.inc"

#if __cplusplus
extern "C" {
#endif

ZTEST(util_cxx, test_u8_to_dec) {
	run_u8_to_dec();
}

ZTEST(util_cxx, test_COND_CODE_1) {
	run_COND_CODE_1();
}

ZTEST(util_cxx, test_COND_CODE_0) {
	run_COND_CODE_0();
}

ZTEST(util_cxx, test_UTIL_OR) {
	run_UTIL_OR();
}

ZTEST(util_cxx, test_UTIL_AND) {
	run_UTIL_AND();
}

ZTEST(util_cxx, test_IF_ENABLED) {
	run_IF_ENABLED();
}

ZTEST(util_cxx, test_LISTIFY) {
	run_LISTIFY();
}

ZTEST(util_cxx, test_MACRO_MAP_CAT) {
	run_MACRO_MAP_CAT();
}

ZTEST(util_cxx, test_z_max_z_min_z_clamp) {
	run_z_max_z_min_z_clamp();
}

ZTEST(util_cxx, test_CLAMP) {
	run_CLAMP();
}

ZTEST(util_cxx, test_IN_RANGE) {
	run_IN_RANGE();
}

ZTEST(util_cxx, test_FOR_EACH) {
	run_FOR_EACH();
}

ZTEST(util_cxx, test_FOR_EACH_NONEMPTY_TERM) {
	run_FOR_EACH_NONEMPTY_TERM();
}

ZTEST(util_cxx, test_FOR_EACH_FIXED_ARG) {
	run_FOR_EACH_FIXED_ARG();
}

ZTEST(util_cxx, test_FOR_EACH_IDX) {
	run_FOR_EACH_IDX();
}

ZTEST(util_cxx, test_FOR_EACH_IDX_FIXED_ARG) {
	run_FOR_EACH_IDX_FIXED_ARG();
}

ZTEST(util_cxx, test_IS_EMPTY) {
	run_IS_EMPTY();
}

ZTEST(util_cxx, test_IS_EQ) {
	run_IS_EQ();
}

ZTEST(util_cxx, test_LIST_DROP_EMPTY) {
	run_LIST_DROP_EMPTY();
}

ZTEST(util_cxx, test_nested_FOR_EACH) {
	run_nested_FOR_EACH();
}

ZTEST(util_cxx, test_GET_ARG_N) {
	run_GET_ARG_N();
}

ZTEST(util_cxx, test_GET_ARGS_LESS_N) {
	run_GET_ARGS_LESS_N();
}

ZTEST(util_cxx, test_mixing_GET_ARG_and_FOR_EACH) {
	run_mixing_GET_ARG_and_FOR_EACH();
}

ZTEST(util_cxx, test_IS_ARRAY_ELEMENT)
{
	run_IS_ARRAY_ELEMENT();
}

ZTEST(util_cxx, test_ARRAY_INDEX)
{
	run_ARRAY_INDEX();
}

ZTEST(util_cxx, test_PART_OF_ARRAY)
{
	run_PART_OF_ARRAY();
}

ZTEST(util_cxx, test_ARRAY_INDEX_FLOOR)
{
	run_ARRAY_INDEX_FLOOR();
}

ZTEST(util_cxx, test_BIT_MASK)
{
	run_BIT_MASK();
}

ZTEST(util_cxx, test_BIT_MASK64)
{
	run_BIT_MASK64();
}

ZTEST(util_cxx, test_IS_BIT_MASK)
{
	run_IS_BIT_MASK();
}

ZTEST(util_cxx, test_IS_SHIFTED_BIT_MASK)
{
	run_IS_SHIFTED_BIT_MASK();
}

ZTEST(util_cxx, test_DIV_ROUND_UP)
{
	run_DIV_ROUND_UP();
}

ZTEST_SUITE(util_cxx, NULL, NULL, NULL, NULL, NULL);

#if __cplusplus
}
#endif

ZTEST(util_cc, test_u8_to_dec) {
	run_u8_to_dec();
}

ZTEST(util_cc, test_COND_CODE_1) {
	run_COND_CODE_1();
}

ZTEST(util_cc, test_COND_CODE_0) {
	run_COND_CODE_0();
}

ZTEST(util_cc, test_UTIL_OR) {
	run_UTIL_OR();
}

ZTEST(util_cc, test_UTIL_AND) {
	run_UTIL_AND();
}

ZTEST(util_cc, test_IF_ENABLED) {
	run_IF_ENABLED();
}

ZTEST(util_cc, test_LISTIFY) {
	run_LISTIFY();
}

ZTEST(util_cc, test_MACRO_MAP_CAT) {
	run_MACRO_MAP_CAT();
}

ZTEST(util_cc, test_z_max_z_min_z_clamp) {
	run_z_max_z_min_z_clamp();
}

ZTEST(util_cc, test_CLAMP) {
	run_CLAMP();
}

ZTEST(util_cc, test_IN_RANGE) {
	run_IN_RANGE();
}

ZTEST(util_cc, test_FOR_EACH) {
	run_FOR_EACH();
}

ZTEST(util_cc, test_FOR_EACH_NONEMPTY_TERM) {
	run_FOR_EACH_NONEMPTY_TERM();
}

ZTEST(util_cc, test_FOR_EACH_FIXED_ARG) {
	run_FOR_EACH_FIXED_ARG();
}

ZTEST(util_cc, test_FOR_EACH_IDX) {
	run_FOR_EACH_IDX();
}

ZTEST(util_cc, test_FOR_EACH_IDX_FIXED_ARG) {
	run_FOR_EACH_IDX_FIXED_ARG();
}

ZTEST(util_cc, test_IS_EMPTY) {
	run_IS_EMPTY();
}

ZTEST(util_cc, test_IS_EQ) {
	run_IS_EQ();
}

ZTEST(util_cc, test_LIST_DROP_EMPTY) {
	run_LIST_DROP_EMPTY();
}

ZTEST(util_cc, test_nested_FOR_EACH) {
	run_nested_FOR_EACH();
}

ZTEST(util_cc, test_GET_ARG_N) {
	run_GET_ARG_N();
}

ZTEST(util_cc, test_GET_ARGS_LESS_N) {
	run_GET_ARGS_LESS_N();
}

ZTEST(util_cc, test_mixing_GET_ARG_and_FOR_EACH) {
	run_mixing_GET_ARG_and_FOR_EACH();
}

ZTEST(util_cc, test_IS_ARRAY_ELEMENT)
{
	run_IS_ARRAY_ELEMENT();
}

ZTEST(util_cc, test_ARRAY_INDEX)
{
	run_ARRAY_INDEX();
}

ZTEST(util_cc, test_PART_OF_ARRAY)
{
	run_PART_OF_ARRAY();
}

ZTEST(util_cc, test_ARRAY_INDEX_FLOOR)
{
	run_ARRAY_INDEX_FLOOR();
}

ZTEST(util_cc, test_BIT_MASK)
{
	run_BIT_MASK();
}

ZTEST(util_cc, test_BIT_MASK64)
{
	run_BIT_MASK64();
}

ZTEST(util_cc, test_IS_BIT_MASK)
{
	run_IS_BIT_MASK();
}

ZTEST(util_cc, test_IS_SHIFTED_BIT_MASK)
{
	run_IS_SHIFTED_BIT_MASK();
}

ZTEST(util_cc, test_DIV_ROUND_UP)
{
	run_DIV_ROUND_UP();
}

ZTEST_SUITE(util_cc, NULL, NULL, NULL, NULL, NULL);
