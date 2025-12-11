/*
 * Copyright (c) 2022 GARDENA GmbH
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "lwm2m_engine.h"
#include "lwm2m_util.h"

#include <zephyr/kernel.h>
#include <zephyr/ztest.h>

#define TEST_VERBOSE 0

#if TEST_VERBOSE
#define TEST_VERBOSE_PRINT(fmt, ...) TC_PRINT(fmt, ##__VA_ARGS__)
#else
#define TEST_VERBOSE_PRINT(fmt, ...)
#endif

static bool lwm2m_path_object_equal_upto(struct lwm2m_obj_path *path,
					 struct lwm2m_obj_path *compare_path, uint8_t level)
{

	if (level >= LWM2M_PATH_LEVEL_OBJECT && path->obj_id != compare_path->obj_id) {
		return false;
	}

	if (level >= LWM2M_PATH_LEVEL_OBJECT_INST &&
		path->obj_inst_id != compare_path->obj_inst_id) {
		return false;
	}

	if (level >= LWM2M_PATH_LEVEL_RESOURCE && path->res_id != compare_path->res_id) {
		return false;
	}

	if (level >= LWM2M_PATH_LEVEL_RESOURCE_INST &&
		path->res_inst_id != compare_path->res_inst_id) {
		return false;
	}

	return true;
}

static void assert_path_list_order(sys_slist_t *lwm2m_path_list)
{

	struct lwm2m_obj_path_list *prev = NULL;
	struct lwm2m_obj_path_list *entry, *tmp;

	uint16_t obj_id_max = 0;
	uint16_t obj_inst_id_max = 0;
	uint16_t res_id_max = 0;
	uint16_t res_inst_id_max = 0;

	if (sys_slist_is_empty(lwm2m_path_list)) {
		return;
	}

	SYS_SLIST_FOR_EACH_CONTAINER_SAFE(lwm2m_path_list, entry, tmp, node) {
		if (prev) {
			if (entry->path.level > prev->path.level) {

				bool is_after = false;

				if (prev->path.level >= LWM2M_PATH_LEVEL_OBJECT) {
					is_after = entry->path.obj_id >= prev->path.obj_id;
				}
				if (is_after && prev->path.level >= LWM2M_PATH_LEVEL_OBJECT_INST &&
					entry->path.obj_id == prev->path.obj_id) {
					is_after =
						entry->path.obj_inst_id >= prev->path.obj_inst_id;
				}

				if (is_after && prev->path.level >= LWM2M_PATH_LEVEL_RESOURCE &&
					entry->path.obj_inst_id == prev->path.obj_inst_id) {
					is_after = entry->path.res_id >= prev->path.res_id;
				}

				if (is_after &&
					prev->path.level >= LWM2M_PATH_LEVEL_RESOURCE_INST &&
					entry->path.res_id == prev->path.res_id) {
					is_after =
						entry->path.res_inst_id >= prev->path.res_inst_id;
				}

				zassert_true(is_after, "Next element %p must be before previous %p",
						 entry, prev);
			} else if (entry->path.level == prev->path.level) {

				if (entry->path.level >= LWM2M_PATH_LEVEL_OBJECT) {
					zassert_true(entry->path.obj_id >= obj_id_max,
							 "Next element has object %d which is smaller "
							 "than previous max object %d",
							 entry->path.obj_id, obj_id_max);
				}

				if (entry->path.level >= LWM2M_PATH_LEVEL_OBJECT_INST) {
					if (!lwm2m_path_object_equal_upto(
							&entry->path, &prev->path,
							LWM2M_PATH_LEVEL_OBJECT)) {
						/* reset max id when path changed */
						obj_inst_id_max = 0;
					}

					zassert_true(entry->path.obj_inst_id >= obj_inst_id_max,
							 "Next element has object instance %d which is "
							 "smaller "
							 "than previous max object instance %d",
							 entry->path.obj_inst_id, obj_inst_id_max);
				}

				if (entry->path.level >= LWM2M_PATH_LEVEL_RESOURCE) {
					if (!lwm2m_path_object_equal_upto(
							&entry->path, &prev->path,
							LWM2M_PATH_LEVEL_OBJECT_INST)) {
						/* reset max id when path changed */
						res_id_max = 0;
					}

					zassert_true(
						entry->path.res_id >= res_id_max,
						"Next element has resource %d which is smaller "
						"than previous max resource %d",
						entry->path.res_id, res_id_max);
				}

				if (entry->path.level >= LWM2M_PATH_LEVEL_RESOURCE_INST) {
					if (!lwm2m_path_object_equal_upto(
							&entry->path, &prev->path,
							LWM2M_PATH_LEVEL_RESOURCE)) {
						/* reset max id when path changed */
						res_inst_id_max = 0;
					}
					zassert_true(entry->path.res_inst_id >= res_inst_id_max,
							 "Next element has resource instance %d which "
							 "is smaller "
							 "than previous max resource instance %d",
							 entry->path.res_inst_id, res_inst_id_max);
				}

			} else { /* entry->path.level < prev->path.level */
				if (entry->path.level >= LWM2M_PATH_LEVEL_OBJECT) {
					zassert_true(entry->path.obj_id >= obj_id_max,
							 "Next element has object %d which is smaller "
							 "than previous max object %d",
							 entry->path.obj_id, obj_id_max);
				}

				if (entry->path.level >= LWM2M_PATH_LEVEL_OBJECT_INST) {
					zassert_true(entry->path.obj_inst_id >= obj_inst_id_max,
							 "Next element has object instance %d which is "
							 "smaller "
							 "than previous max object instance %d",
							 entry->path.obj_inst_id, obj_inst_id_max);
				}

				if (entry->path.level >= LWM2M_PATH_LEVEL_RESOURCE) {
					zassert_true(
						entry->path.res_id >= res_id_max,
						"Next element has resource %d which is smaller "
						"than previous max resource %d",
						entry->path.res_id, res_id_max);
				}

				if (entry->path.level >= LWM2M_PATH_LEVEL_RESOURCE_INST) {
					zassert_true(entry->path.res_inst_id >= res_inst_id_max,
							 "Next element has resource instance %d which "
							 "is bigger "
							 "than previous max resource instance %d",
							 entry->path.res_inst_id, res_inst_id_max);
				}

				zassert_true(!lwm2m_path_object_equal_upto(
						&entry->path, &prev->path, entry->path.level),
						 "Next element equals previous up to level %d "
						 "and thus must be before previous",
						 entry->path.level);
			}
		}

		if (entry->path.level >= LWM2M_PATH_LEVEL_OBJECT) {
			obj_id_max = entry->path.obj_id;
		} else {
			obj_id_max = 0;
		}

		if (entry->path.level >= LWM2M_PATH_LEVEL_OBJECT_INST &&
			(prev == NULL || entry->path.obj_id == prev->path.obj_id)) {
			obj_inst_id_max = entry->path.obj_inst_id;
		} else {
			obj_inst_id_max = 0;
		}

		if (entry->path.level >= LWM2M_PATH_LEVEL_RESOURCE &&
			(prev == NULL || entry->path.obj_id == prev->path.obj_id) &&
			(prev == NULL || entry->path.obj_inst_id == prev->path.obj_inst_id)) {
			res_id_max = entry->path.res_id;
		} else {
			res_id_max = 0;
		}

		if (entry->path.level >= LWM2M_PATH_LEVEL_RESOURCE_INST &&
			(prev == NULL || entry->path.obj_id == prev->path.obj_id) &&
			(prev == NULL || entry->path.obj_inst_id == prev->path.obj_inst_id) &&
			(prev == NULL || entry->path.res_id == prev->path.res_id)) {
			res_inst_id_max = entry->path.res_inst_id;
		} else {
			res_inst_id_max = 0;
		}

		prev = entry;
	}
}

static void run_insertion_test(char const *insert_path_str[], int insertions_count,
				   char const *expected_path_str[])
{
	/* GIVEN: different paths */
	struct lwm2m_obj_path_list lwm2m_path_list_buf[insertions_count];
	sys_slist_t lwm2m_path_list;
	sys_slist_t lwm2m_path_free_list;
	int ret;

	lwm2m_engine_path_list_init(&lwm2m_path_list, &lwm2m_path_free_list, lwm2m_path_list_buf,
					insertions_count);

	/* WHEN: inserting each path */
	struct lwm2m_obj_path insert_path;

	for (int i = 0; i < insertions_count; ++i) {
		ret = lwm2m_string_to_path(insert_path_str[i], &insert_path, '/');
		zassert_true(ret >= 0, "Conversion to path #%d failed", i);

		ret = lwm2m_engine_add_path_to_list(&lwm2m_path_list, &lwm2m_path_free_list,
							&insert_path);

		zassert_true(ret >= 0, "Insertion #%d failed", i);
		/* THEN: path order is maintained */
		assert_path_list_order(&lwm2m_path_list);
	}

	/* AND: final list matches expectation */
	struct lwm2m_obj_path_list *entry, *tmp;
	int path_num = 0;

	SYS_SLIST_FOR_EACH_CONTAINER_SAFE(&lwm2m_path_list, entry, tmp, node) {
		struct lwm2m_obj_path expected_path;

		lwm2m_string_to_path(expected_path_str[path_num++], &expected_path, '/');
		zassert_mem_equal(&entry->path, &expected_path, sizeof(struct lwm2m_obj_path),
				  "Path #%d did not match expectation", path_num);
	}
}

ZTEST(lwm2m_observation, test_add_path_to_list)
{
	/* clang-format off */
	char const *insert_path_str[] = {
		LWM2M_PATH(2),
		LWM2M_PATH(1),
		LWM2M_PATH(1, 2),
		LWM2M_PATH(1, 1),
		LWM2M_PATH(1, 1, 10),
		LWM2M_PATH(1, 1, 10, 10),
		LWM2M_PATH(1, 1, 10, 9),
		LWM2M_PATH(1, 2, 10, 11),

		LWM2M_PATH(100),

		LWM2M_PATH(41),
		LWM2M_PATH(43, 3),
		LWM2M_PATH(45, 2, 2),
		LWM2M_PATH(47, 1, 1, 1),

		LWM2M_PATH(57, 1, 1, 1),
		LWM2M_PATH(55, 2, 2),
		LWM2M_PATH(53, 3),
		LWM2M_PATH(51),
	};

	char const *expected_path_str[] = {
		LWM2M_PATH(1),
		LWM2M_PATH(1, 1),
		LWM2M_PATH(1, 1, 10),
		LWM2M_PATH(1, 1, 10, 9),
		LWM2M_PATH(1, 1, 10, 10),
		LWM2M_PATH(1, 2),
		LWM2M_PATH(1, 2, 10, 11),
		LWM2M_PATH(2),
		LWM2M_PATH(41),
		LWM2M_PATH(43, 3),
		LWM2M_PATH(45, 2, 2),
		LWM2M_PATH(47, 1, 1, 1),
		LWM2M_PATH(51),
		LWM2M_PATH(53, 3),
		LWM2M_PATH(55, 2, 2),
		LWM2M_PATH(57, 1, 1, 1),
		LWM2M_PATH(100),
	};
	/* clang-format on */

	run_insertion_test(insert_path_str, ARRAY_SIZE(insert_path_str), expected_path_str);
}

ZTEST(lwm2m_observation, test_add_path_to_list_inverse_non_overlapping)
{
	/* clang-format off */
	char const *insert_path_str[] = {
		LWM2M_PATH(41),
		LWM2M_PATH(43, 3),
		LWM2M_PATH(45, 2, 2),
		LWM2M_PATH(47, 1, 1, 1),
	};

	char const *expected_path_str[] = {
		LWM2M_PATH(41),
		LWM2M_PATH(43, 3),
		LWM2M_PATH(45, 2, 2),
		LWM2M_PATH(47, 1, 1, 1),
	};
	/* clang-format on */

	run_insertion_test(insert_path_str, ARRAY_SIZE(insert_path_str), expected_path_str);
}

ZTEST(lwm2m_observation, test_add_path_to_list_inverse_non_overlapping_2)
{
	/* clang-format off */
	char const *insert_path_str[] = {
		LWM2M_PATH(57, 1, 1, 1),
		LWM2M_PATH(55, 2, 2),
		LWM2M_PATH(53, 3),
		LWM2M_PATH(51),
	};

	char const *expected_path_str[] = {
		LWM2M_PATH(51),
		LWM2M_PATH(53, 3),
		LWM2M_PATH(55, 2, 2),
		LWM2M_PATH(57, 1, 1, 1),
	};
	/* clang-format on */

	run_insertion_test(insert_path_str, ARRAY_SIZE(insert_path_str), expected_path_str);
}

ZTEST(lwm2m_observation, test_add_path_to_list_object_before_resource_inst)
{
	/* clang-format off */
	char const *insert_path_str[] = {
		LWM2M_PATH(1, 1, 1, 1),
		LWM2M_PATH(1),
	};

	char const *expected_path_str[6] = {
		LWM2M_PATH(1),
		LWM2M_PATH(1, 1, 1, 1),
	};
	/* clang-format on */

	run_insertion_test(insert_path_str, ARRAY_SIZE(insert_path_str), expected_path_str);
}

ZTEST(lwm2m_observation, test_add_path_to_list_object_inst_before_resource_inst)
{
	/* clang-format off */
	char const *insert_path_str[] = {
		LWM2M_PATH(1, 1, 1, 1),
		LWM2M_PATH(1, 1),
	};

	char const *expected_path_str[6] = {
		LWM2M_PATH(1, 1),
		LWM2M_PATH(1, 1, 1, 1),
	};
	/* clang-format on */

	run_insertion_test(insert_path_str, ARRAY_SIZE(insert_path_str), expected_path_str);
}

ZTEST(lwm2m_observation, test_add_path_to_list_resource_before_resource_inst)
{
	/* clang-format off */
	char const *insert_path_str[] = {
		LWM2M_PATH(1, 1, 1, 1),
		LWM2M_PATH(1, 1, 1),
	};

	char const *expected_path_str[] = {
		LWM2M_PATH(1, 1, 1),
		LWM2M_PATH(1, 1, 1, 1),
	};
	/* clang-format on */

	run_insertion_test(insert_path_str, ARRAY_SIZE(insert_path_str), expected_path_str);
}

ZTEST(lwm2m_observation, test_add_path_to_list_resource_order)
{
	/* clang-format off */
	char const *insert_path_str[] = {
		LWM2M_PATH(32765, 1, 6, 0),
		LWM2M_PATH(32765, 1, 6, 1),
		LWM2M_PATH(32765, 1, 6),
		LWM2M_PATH(32765, 1, 5),
		LWM2M_PATH(32765, 1, 5, 2),
		LWM2M_PATH(32765, 1, 5, 1),
	};

	char const *expected_path_str[] = {
		LWM2M_PATH(32765, 1, 5),
		LWM2M_PATH(32765, 1, 5, 1),
		LWM2M_PATH(32765, 1, 5, 2),
		LWM2M_PATH(32765, 1, 6),
		LWM2M_PATH(32765, 1, 6, 0),
		LWM2M_PATH(32765, 1, 6, 1),
	};
	/* clang-format on */

	run_insertion_test(insert_path_str, ARRAY_SIZE(insert_path_str), expected_path_str);
}

ZTEST(lwm2m_observation, test_add_path_to_list_resource_before_instance)
{
	/* clang-format off */
	char const *insert_path_str[] = {
		LWM2M_PATH(32765, 1, 6, 0),
		LWM2M_PATH(32765, 1, 6, 1),
		LWM2M_PATH(32765, 1, 6),
	};

	char const *expected_path_str[6] = {
		LWM2M_PATH(32765, 1, 6),
		LWM2M_PATH(32765, 1, 6, 0),
		LWM2M_PATH(32765, 1, 6, 1),
	};
	/* clang-format on */

	run_insertion_test(insert_path_str, ARRAY_SIZE(insert_path_str), expected_path_str);
}

ZTEST(lwm2m_observation, test_add_path_to_list_resource_inverse)
{
	/* clang-format off */
	char const *insert_path_str[] = {
		LWM2M_PATH(1, 1, 1, 1),
		LWM2M_PATH(1, 1, 1),
		LWM2M_PATH(1, 1),
		LWM2M_PATH(1),
	};

	char const *expected_path_str[] = {
		LWM2M_PATH(1),
		LWM2M_PATH(1, 1),
		LWM2M_PATH(1, 1, 1),
		LWM2M_PATH(1, 1, 1, 1),
	};
	/* clang-format on */

	run_insertion_test(insert_path_str, ARRAY_SIZE(insert_path_str), expected_path_str);
}

ZTEST(lwm2m_observation, test_add_path_to_list_obj_after_resource)
{
	/* clang-format off */
	char const *insert_path_str[] = {
		LWM2M_PATH(1),
		LWM2M_PATH(1, 1),
		LWM2M_PATH(1, 1, 1),
		LWM2M_PATH(1, 2),
	};

	char const *expected_path_str[] = {
		LWM2M_PATH(1),
		LWM2M_PATH(1, 1),
		LWM2M_PATH(1, 1, 1),
		LWM2M_PATH(1, 2),
	};
	/* clang-format on */

	run_insertion_test(insert_path_str, ARRAY_SIZE(insert_path_str), expected_path_str);
}

ZTEST(lwm2m_observation, test_add_path_to_list_duplicate)
{
	/* clang-format off */
	char const *insert_path_str[] = {
		LWM2M_PATH(1),
		LWM2M_PATH(1, 1),
		LWM2M_PATH(1),
	};

	char const *expected_path_str[] = {
		LWM2M_PATH(1),
		LWM2M_PATH(1, 1),
	};
	/* clang-format on */

	run_insertion_test(insert_path_str, ARRAY_SIZE(insert_path_str), expected_path_str);
}

ZTEST_SUITE(lwm2m_observation, NULL, NULL, NULL, NULL, NULL);
