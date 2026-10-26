/*
 * Copyright (c) 2026 EPAM Systems
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Xenlib Operating System Abstraction Layer (OSAL) single and double linked
 * list runtime interface interface implementation
 *
 */
#include <xenlib_osal_platform.h>

void xenlib_osal_dlist_init(xenlib_osal_dlist_t *list)
{
	sys_dlist_init(list);
}

void xenlib_osal_dnode_init(xenlib_osal_dnode_t *node)
{
	sys_dnode_init(node);
}

void xenlib_osal_dlist_append(xenlib_osal_dlist_t *list,
					xenlib_osal_dnode_t *node)
{
	sys_dlist_append(list, node);
}

bool xenlib_osal_dlist_is_empty(const xenlib_osal_dlist_t *list)
{
	return sys_dlist_is_empty(list);
}

void xenlib_osal_dlist_remove(xenlib_osal_dnode_t *node)
{
	sys_dlist_remove(node);
}

void xenlib_osal_slist_init(xenlib_osal_slist_t *list)
{
	sys_slist_init(list);
}

void xenlib_osal_slist_append(xenlib_osal_slist_t *list,
					xenlib_osal_snode_t *node)
{
	sys_slist_append(list, node);
}


bool xenlib_osal_slist_is_empty(const xenlib_osal_slist_t *list)
{
	return sys_slist_is_empty(list);
}

void xenlib_osal_slist_remove(xenlib_osal_slist_t *list,
					xenlib_osal_snode_t *prev,
					xenlib_osal_snode_t *node)
{
	sys_slist_remove(list, prev, node);
}
