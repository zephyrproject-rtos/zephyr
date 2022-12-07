/*
 * Copyright (c) 2022 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/logging/log.h>
#include <zephyr/sys/__assert.h>
#include <stdlib.h>
#include <zephyr/sys/slist.h>
#include <zephyr/sys/graph.h>
#include <zephyr/kernel.h>
#include <zephyr/device.h>

LOG_MODULE_REGISTER(topo, LOG_LEVEL_INF);

K_HEAP_DEFINE(sort_buf_pool, CONFIG_MAX_POINT_NUM * 2 * sizeof(uint16_t) + 128);

struct topo_edge {
	uint16_t sink_pos;
	sys_snode_t node;
};

struct topo_point {
	uint16_t data;
	sys_slist_t edge_list;
};

struct topo_graph {
	uint16_t point_num;
	uint16_t edge_num;
	struct topo_point *points;
};

static inline int get_vertex_position(struct topo_graph *graph, uint16_t data)
{
	int i;

	__ASSERT(graph, "topological graph struct should not be NULL");

	for (i = 0; i < graph->point_num; i++)
		if (graph->points[i].data == data)
			return i;

	return -1;
}

static int sorting(struct topo_graph *graph, uint16_t *order)
{
	uint16_t i, j, index = 0;
	struct topo_edge *edge;
	void *buf = NULL;
	uint16_t *queue, *in_degree;
	uint16_t head = 0, tail = 0;
	int ret = 0;

	__ASSERT(graph->point_num < CONFIG_MAX_POINT_NUM,
		"topological point number:%d no more than max point number:%d",
		graph->point_num, CONFIG_MAX_POINT_NUM);
	__ASSERT(graph, "topological graph struct should not be NULL");
	__ASSERT(order, "order array should not be NULL");

	buf = k_heap_alloc(&sort_buf_pool, graph->point_num * sizeof(uint16_t) * 2, K_NO_WAIT);
	if (!buf) {
		LOG_ERR("failed to allocate memory for topological sort");
		return -ENOMEM;
	}
	memset(buf, 0x00, graph->point_num * sizeof(uint16_t) * 2);

	queue = (uint16_t *)buf;
	in_degree = (uint16_t *)((uint8_t *)buf + graph->point_num * sizeof(uint16_t));

	for (i = 0; i < graph->point_num; i++) {
		SYS_SLIST_FOR_EACH_CONTAINER(&graph->points[i].edge_list, edge, node) {
			in_degree[edge->sink_pos]++;
		}
	}
	/* put all 0 in_degree points to the queue */
	for (i = 0; i < graph->point_num; i++) {
		if (in_degree[i] == 0)
			queue[tail++] = i;
		LOG_DBG("%s, point:%d, tail:%d, in_degree:%d", __func__, i, tail, in_degree[i]);
	}

	while (head != tail) {
		j = queue[head++];
		order[index++] = graph->points[j].data;
		LOG_DBG("%s, head:%d, tail:%d, point:%d, data:%d",
			__func__, head, tail, j, graph->points[j].data);
		SYS_SLIST_FOR_EACH_CONTAINER(&graph->points[j].edge_list, edge, node) {
			in_degree[edge->sink_pos]--;
			if (in_degree[edge->sink_pos] == 0)
				queue[tail++] = edge->sink_pos;
			LOG_DBG("%s, sink_pos:%d, tail:%d, ", __func__, edge->sink_pos, tail);
		}
	}
	if (index != graph->point_num) {
		LOG_ERR("index:%d not equal to point_num:%d, Graph has a cycle\n",
								index, graph->point_num);
		ret = -EINVAL;
	}

	if (!ret) {
		for (i = 0; i < graph->point_num; i++) {
			LOG_DBG("%s, i:%d, order:%c", __func__, i, order[i]);
		}
	}

	k_heap_free(&sort_buf_pool, buf);

	return ret;
}

int topological_sort(int point_num, uint16_t *points_data,
		     int edge_num, uint16_t (*edges_vertex)[2], uint16_t *order)
{
	struct topo_graph *graph = NULL;
	struct topo_edge *edge = NULL, *tmp;
	uint16_t source, sink;
	int source_pos, sink_pos;
	int i;
	int ret = 0;

	__ASSERT(points_data, "topological graph points data should not be NULL");
	__ASSERT(order, "order array should not be NULL");

	graph = (struct topo_graph *)malloc(sizeof(*graph));
	if (!graph) {
		LOG_ERR("malloc memory for topologic sorting graph error");
		ret = -ENOMEM;
		goto err;
	}
	graph->points = (struct topo_point *)malloc(point_num * sizeof(struct topo_point));
	if (!graph->points) {
		LOG_ERR("malloc memory for topologic sorting points error");
		ret = -ENOMEM;
		goto err;
	}
	edge = (struct topo_edge *)malloc(edge_num * sizeof(*edge));
	if (!edge) {
		LOG_ERR("malloc memory for topologic sorting edge error");
		ret = -ENOMEM;
		goto err;
	}
	graph->point_num = point_num;
	graph->edge_num = edge_num;

	LOG_DBG("%s, point_num:%d, edge_num:%d", __func__, point_num, edge_num);
	/* init topological graph points */
	for (i = 0; i < graph->point_num; i++) {
		graph->points[i].data = points_data[i];
		sys_slist_init(&graph->points[i].edge_list);
		LOG_DBG("%s, index:%d, point:%c", __func__, i, graph->points[i].data);
	}

	/* init topological graph edge */
	for (i = 0; i < graph->edge_num; i++) {
		source = edges_vertex[i][0];
		sink = edges_vertex[i][1];
		source_pos = get_vertex_position(graph, source);
		__ASSERT(source_pos >= 0 && source_pos < point_num,
			"source position:%d should no less than 0, no more than point num:%d",
			source_pos, point_num);
		sink_pos = get_vertex_position(graph, sink);
		__ASSERT(sink_pos >= 0 && sink_pos < point_num,
			"sink position:%d should no less than 0, no more than point num:%d",
			sink_pos, point_num);
		LOG_DBG("%s, edge:%d, source:%c, sink:%c, pos:(%d, %d)",
					__func__, i, source, sink, source_pos, sink_pos);
		edge[i].sink_pos = sink_pos;
		sys_slist_append(&graph->points[source_pos].edge_list, &edge[i].node);
	}

	ret = sorting(graph, order);
	if (ret)
		goto err;

err:
	for (i = 0; i < graph->point_num; i++) {
		SYS_SLIST_FOR_EACH_CONTAINER_SAFE(&graph->points[i].edge_list, edge, tmp, node) {
			sys_slist_remove(&graph->points[i].edge_list, NULL, &edge->node);
		}
	}
	free(graph->points);
	free(graph);
	free(edge);

	return ret;
}
