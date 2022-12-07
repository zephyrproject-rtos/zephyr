/*
 * Copyright (c) 2022 Intel Corporation
 * SPDX-License-Identifier: Apache-2.0
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <zephyr/kernel.h>
#include <zephyr/ztest.h>
#include <zephyr/sys/graph.h>
#include <stdlib.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(test_topological, LOG_LEVEL_INF);

static inline int get_point_idx(uint16_t data, int point_num, uint16_t *order)
{
	int i;

	for (i = 0; i < point_num; i++) {
		if (order[i] == data)
			return i;
	}

	return -1;
}
/*test case main entry*/
ZTEST_SUITE(topological_tests, NULL, NULL, NULL, NULL, NULL);

/**
 * @brief Test the following 4 points/3 edges Directed Acyclic Graph(DAG)
 *         0         1
 *         ^       ^ ^
 *         |      /  |
 *         |    /    |
 *         |  /      |
 *         2         3
 * This test could be used in senss, for example:
 * point 0: move detection virtual sensor
 * point 1: hinge virutal sensor
 * point 2: lid acc physical sensor
 * point 3: base acc physical sensor
 * hinge sensor is dependent on both base acc and lid acc
 * move detection sensor based on lid acc
 */
ZTEST(topological_tests, test_4points_3edges_DAG)
{
	/* points: 0, 1, 2, 3 */
	int point_num = 4;
	/* edges: 1->2, 0->3, 1->3 */
	int edge_num = 3;
	uint16_t (*edges_vertex)[2];
	uint16_t points[4] = {'0', '1', '2', '3'};
	uint16_t order[3];
	int ret;

	edges_vertex = (uint16_t(*)[2])malloc(sizeof(uint16_t) * edge_num * 2);
	zassert_not_null(edges_vertex, "edges_vertex is null");
	edges_vertex[0][0] = '2';
	edges_vertex[0][1] = '0';

	edges_vertex[1][0] = '2';
	edges_vertex[1][1] = '1';

	edges_vertex[2][0] = '3';
	edges_vertex[2][1] = '1';

	ret = topological_sort(point_num, points, edge_num, edges_vertex, order);
	zassert_equal(ret, 0, "topologic_sort failed");

	zassert_true(get_point_idx('0', point_num, order) > get_point_idx('2', point_num, order),
		"vertex 0 comes after vertex 2\n");
	zassert_true(get_point_idx('1', point_num, order) > get_point_idx('2', point_num, order),
		"vertex 1 comes after vertex 2\n");
	zassert_true(get_point_idx('1', point_num, order) > get_point_idx('3', point_num, order),
		"vertex 1 comes after vertex 3\n");

	free(edges_vertex);
}

/**
 * @brief Test the following 7 points/7 edges Directed Acyclic Graph(DAG)
 *         B ------> A ------> G
 *         |                   ^
 *         V                   |
 *         D ------> F <------ C
 *         |
 *         V
 *         E
 */
ZTEST(topological_tests, test_7points_7edges_DAG)
{
	/* points: A, B, C, D, E, F, G */
	int point_num = 7;
	/* edges: B->A, A->G, C->G, C->F, D->F, B->D, D->E */
	int edge_num = 7;
	uint16_t (*edges_vertex)[2];
	uint16_t points[7] = {'A', 'B', 'C', 'D', 'E', 'F', 'G'};
	uint16_t order[7];
	int ret;

	edges_vertex = (uint16_t(*)[2])malloc(sizeof(uint16_t) * edge_num * 2);
	zassert_not_null(edges_vertex, "edges_vertex is null");
	edges_vertex[0][0] = 'B';
	edges_vertex[0][1] = 'A';

	edges_vertex[1][0] = 'A';
	edges_vertex[1][1] = 'G';

	edges_vertex[2][0] = 'C';
	edges_vertex[2][1] = 'G';

	edges_vertex[3][0] = 'C';
	edges_vertex[3][1] = 'F';

	edges_vertex[4][0] = 'D';
	edges_vertex[4][1] = 'F';

	edges_vertex[5][0] = 'B';
	edges_vertex[5][1] = 'D';

	edges_vertex[6][0] = 'D';
	edges_vertex[6][1] = 'E';

	ret = topological_sort(point_num, points, edge_num, edges_vertex, order);
	zassert_equal(ret, 0, "topologic_sort failed");

	zassert_true(get_point_idx('A', point_num, order) > get_point_idx('B', point_num, order),
		"vertex A comes after vertex B\n");
	zassert_true(get_point_idx('G', point_num, order) > get_point_idx('A', point_num, order),
		"vertex G comes after vertex A\n");
	zassert_true(get_point_idx('G', point_num, order) > get_point_idx('C', point_num, order),
		"vertex G comes after vertex C\n");
	zassert_true(get_point_idx('F', point_num, order) > get_point_idx('C', point_num, order),
		"vertex F comes after vertex C\n");
	zassert_true(get_point_idx('F', point_num, order) > get_point_idx('D', point_num, order),
		"vertex F comes after vertex D\n");
	zassert_true(get_point_idx('D', point_num, order) > get_point_idx('B', point_num, order),
		"vertex D comes after vertex B\n");
	zassert_true(get_point_idx('E', point_num, order) > get_point_idx('D', point_num, order),
		"vertex E comes after vertex D\n");

	free(edges_vertex);
}
