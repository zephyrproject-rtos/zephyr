/*
 * Copyright (c) 2020 Intel Corporation
 * SPDX-License-Identifier: Apache-2.0
 *
 * Licensed under the Apache License, Version 2.0 (the "License"); you may
 * not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
 * WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#ifndef ZEPHYR_INCLUDE_SYS_GRAPH_H_
#define ZEPHYR_INCLUDE_SYS_GRAPH_H_

#include <zephyr/types.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Topological sorting for Directed Acyclic Graph (DAG)
 *
 * This routine sorts DAG with topological methods.
 * A topological sort is a graph traversal in which each point is
 * only visited after all of its dependencies have been visited
 *
 * @param point_num     number of points to be sorted
 * @param points_data   data of each point
 * @param edge_num      number of edges for DAG (connection between two points is an edge)
 * @param edges_vertex  [2] [0]:vertex u, [1]:vertex v (vertex u comes before vertex v)
 * @param order         sequence for points with topological ordering
 *
 * @retval 0            Topological sorting success.
 * @retval -ENOMEM      if memory couldn't be allocated
 * @retval -EINVAL      not Directed Acyclic Graph (DAG)
 */
int topological_sort(int point_num, uint16_t *points_data, int edge_num,
		     uint16_t (*edges_vertex)[2], uint16_t *order);

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_SYS_GRAPH_H_ */
