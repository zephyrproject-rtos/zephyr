/*
 * @file
 * @brief Notifier chain related functions
 */

/*
 * Copyright (c) 2018, Unisoc Communications Inc
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "notifier.h"

static
struct wifimgr_notifier *wifimgr_search_notifier(struct wifimgr_notifier_chain
						 *chain,
						 wifi_notifier_fn_t
						 notifier_call)
{
	struct wifimgr_notifier *notifier;

	/* Loop through list to find the corresponding event */
	wifimgr_list_for_each_entry(notifier, &chain->list,
				    struct wifimgr_notifier, node) {
		if (notifier->notifier_call == notifier_call)
			return notifier;
	}

	return NULL;
}

int wifimgr_register_notifier(struct wifimgr_notifier_chain *chain,
			      wifi_notifier_fn_t notifier_call)
{
	struct wifimgr_notifier *notifier;

	if (!chain || !notifier_call)
		return -EINVAL;

	sem_wait(&chain->exclsem);

	/* Check whether the notifier already exist */
	notifier = wifimgr_search_notifier(chain, notifier_call);
	if (notifier) {
		sem_post(&chain->exclsem);
		return -EEXIST;
	}

	/* Allocate a notifier struct */
	notifier = malloc(sizeof(struct wifimgr_notifier));
	if (!notifier) {
		sem_post(&chain->exclsem);
		return -ENOMEM;
	}

	notifier->notifier_call = notifier_call;

	/* Link the notifier into the disconnection chain */
	wifimgr_list_append(&chain->list, &notifier->node);
	sem_post(&chain->exclsem);

	return 0;
}

int wifimgr_unregister_notifier(struct wifimgr_notifier_chain *chain,
				wifi_notifier_fn_t notifier_call)
{
	struct wifimgr_notifier *notifier;

	if (!chain || !notifier_call)
		return -EINVAL;

	/* Get exclusive access to the struct */
	sem_wait(&chain->exclsem);

	notifier = wifimgr_search_notifier(chain, notifier_call);
	if (!notifier) {
		sem_post(&chain->exclsem);
		return -ENOENT;
	}

	wifimgr_list_remove(&chain->list, &notifier->node);
	sem_post(&chain->exclsem);
	free(notifier);

	return 0;
}
