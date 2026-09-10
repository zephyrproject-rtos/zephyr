/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stddef.h>
#include <string.h>

#include <zephyr/kernel.h>

#include <zephyr/logging/log.h>

#include "babblekit/testcase.h"

LOG_MODULE_REGISTER(bt_settings_hook, LOG_LEVEL_DBG);

struct settings_list_node {
	sys_snode_t node;
	char *key;
};

static K_MUTEX_DEFINE(settings_list_mutex);
static atomic_t settings_record_enabled = ATOMIC_INIT(0);

static sys_slist_t settings_list = SYS_SLIST_STATIC_INIT(settings_list);

static void add_key(const char *key)
{
	struct settings_list_node *new_node;
	size_t key_size = strlen(key);
	bool key_already_in = false;

	struct settings_list_node *loop_node;

	if (settings_record_enabled == 1) {
		k_mutex_lock(&settings_list_mutex, K_FOREVER);

		SYS_SLIST_FOR_EACH_CONTAINER(&settings_list, loop_node, node) {
			if (strcmp(loop_node->key, key) == 0) {
				key_already_in = true;
			}
		}

		if (!key_already_in) {
			new_node = k_malloc(sizeof(struct settings_list_node));
			TEST_ASSERT(new_node != NULL, "Failed to malloc new_node");

			new_node->key = k_malloc(sizeof(char) * key_size);
			TEST_ASSERT(new_node->key != NULL, "Failed to malloc new_node->key");

			memcpy(new_node->key, key, sizeof(char) * key_size);

			sys_slist_append(&settings_list, &new_node->node);
		}

		k_mutex_unlock(&settings_list_mutex);
	}
}

static void remove_key(const char *key)
{
	struct settings_list_node *loop_node, *prev_loop_node;

	prev_loop_node = NULL;

	if (settings_record_enabled == 1) {
		k_mutex_lock(&settings_list_mutex, K_FOREVER);

		SYS_SLIST_FOR_EACH_CONTAINER(&settings_list, loop_node, node) {
			if (strcmp(loop_node->key, key) == 0) {
				break;
			}

			prev_loop_node = loop_node;
		}

		if (loop_node != NULL) {
			sys_slist_remove(&settings_list, &prev_loop_node->node, &loop_node->node);

			k_free(loop_node->key);
			k_free(loop_node);
		}

		k_mutex_unlock(&settings_list_mutex);
	}
}

void start_settings_record(void)
{
	atomic_set(&settings_record_enabled, 1);
}

void stop_settings_record(void)
{
	atomic_set(&settings_record_enabled, 0);
}

void settings_list_cleanup(void)
{
	struct settings_list_node *loop_node, *next_loop_node, *prev_loop_node;

	prev_loop_node =  NULL;

	k_mutex_lock(&settings_list_mutex, K_FOREVER);

	SYS_SLIST_FOR_EACH_CONTAINER_SAFE(&settings_list, loop_node, next_loop_node, node) {
		sys_slist_remove(&settings_list, &prev_loop_node->node, &loop_node->node);

		k_free(loop_node->key);
		k_free(loop_node);
	}

	k_mutex_unlock(&settings_list_mutex);
}

int get_settings_list_size(void)
{
	struct settings_list_node *loop_node;
	int number_of_settings = 0;

	SYS_SLIST_FOR_EACH_CONTAINER(&settings_list, loop_node, node) {
		LOG_ERR("Setting registered: %s", loop_node->key);
		number_of_settings += 1;
	}

	return number_of_settings;
}

void bt_testing_settings_store_hook(const char *key, const void *value, size_t val_len)
{
	LOG_DBG("Store: %s", key);
	LOG_HEXDUMP_DBG(value, val_len, "Data:");

	add_key(key);
}

void bt_testing_settings_delete_hook(const char *key)
{
	LOG_DBG("Delete: %s", key);

	remove_key(key);
}
