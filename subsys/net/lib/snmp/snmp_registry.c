/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdbool.h>
#include <string.h>

#include <zephyr/net/snmp.h>
#include <zephyr/sys/util.h>

#include "snmp_internal.h"

int snmp_oid_cmp(const uint32_t *a, size_t a_len, const uint32_t *b, size_t b_len)
{
	size_t n = MIN(a_len, b_len);

	for (size_t i = 0U; i < n; i++) {
		if (a[i] != b[i]) {
			return (a[i] < b[i]) ? -1 : 1;
		}
	}

	if (a_len == b_len) {
		return 0;
	}

	return (a_len < b_len) ? -1 : 1;
}

bool snmp_oid_has_prefix(const uint32_t *oid, size_t oid_len, const uint32_t *prefix,
			 size_t prefix_len)
{
	if (oid_len < prefix_len) {
		return false;
	}

	for (size_t i = 0U; i < prefix_len; i++) {
		if (oid[i] != prefix[i]) {
			return false;
		}
	}

	return true;
}

enum snmp_lookup_result snmp_registry_lookup(const uint32_t *name, size_t name_len,
					     const struct snmp_object **obj, void **row)
{
	bool object_exists = false;

	*obj = NULL;

	STRUCT_SECTION_FOREACH(snmp_object, candidate) {
		const uint32_t *suffix;
		size_t suffix_len;

		if (!snmp_oid_has_prefix(name, name_len, candidate->oid, candidate->oid_len)) {
			continue;
		}

		object_exists = true;
		*obj = candidate;
		suffix = &name[candidate->oid_len];
		suffix_len = name_len - candidate->oid_len;

		if (candidate->table == NULL) {
			if (suffix_len == 1U && suffix[0] == 0U) {
				*obj = candidate;
				*row = NULL;
				return SNMP_LOOKUP_FOUND;
			}

			continue;
		}

		if (suffix_len == 0U) {
			continue;
		}

		*row = candidate->table->row_lookup(candidate->table, suffix, suffix_len);
		if (*row != NULL) {
			*obj = candidate;
			return SNMP_LOOKUP_FOUND;
		}
	}

	return object_exists ? SNMP_LOOKUP_NO_SUCH_INSTANCE : SNMP_LOOKUP_NO_SUCH_OBJECT;
}

/* State of the search for the smallest row index greater than a suffix. */
struct next_row_ctx {
	const struct snmp_table *tbl;
	const uint32_t *after;
	size_t after_len;
	size_t max_arcs;
	uint32_t best[SNMP_OID_MAX_LEN];
	size_t best_len;
	void *best_row;
	bool found;
};

static void next_row_cb(void *row, void *user_data)
{
	struct next_row_ctx *ctx = user_data;
	uint32_t idx[SNMP_OID_MAX_LEN];
	int len;

	len = ctx->tbl->row_index(ctx->tbl, row, idx, ctx->max_arcs);
	if (len <= 0) {
		return;
	}

	if (snmp_oid_cmp(idx, (size_t)len, ctx->after, ctx->after_len) <= 0) {
		return;
	}

	if (ctx->found && snmp_oid_cmp(idx, (size_t)len, ctx->best, ctx->best_len) >= 0) {
		return;
	}

	memcpy(ctx->best, idx, (size_t)len * sizeof(uint32_t));
	ctx->best_len = (size_t)len;
	ctx->best_row = row;
	ctx->found = true;
}

/*
 * Smallest instance of obj that is lexicographically greater than target.
 * Returns true and fills inst, *inst_len and *row, or false when obj has no
 * such instance.
 */
static bool object_next_instance(const struct snmp_object *obj, const uint32_t *target,
				 size_t target_len, uint32_t *inst, size_t *inst_len, void **row)
{
	bool under = snmp_oid_has_prefix(target, target_len, obj->oid, obj->oid_len);
	struct next_row_ctx ctx;

	/* A target past the object, and not inside it, has nothing to offer. */
	if (!under && snmp_oid_cmp(target, target_len, obj->oid, obj->oid_len) > 0) {
		return false;
	}

	if (obj->oid_len >= SNMP_OID_MAX_LEN) {
		return false;
	}

	memcpy(inst, obj->oid, obj->oid_len * sizeof(uint32_t));

	if (obj->table == NULL) {
		/* The only instance is .0, which is greater than the bare OID. */
		if (under && target_len > obj->oid_len) {
			return false;
		}

		inst[obj->oid_len] = 0U;
		*inst_len = obj->oid_len + 1U;
		*row = NULL;

		return true;
	}

	ctx.tbl = obj->table;
	ctx.after = under ? &target[obj->oid_len] : NULL;
	ctx.after_len = under ? target_len - obj->oid_len : 0U;
	ctx.max_arcs = SNMP_OID_MAX_LEN - obj->oid_len;
	ctx.found = false;

	obj->table->foreach_row(obj->table, next_row_cb, &ctx);
	if (!ctx.found) {
		return false;
	}

	memcpy(&inst[obj->oid_len], ctx.best, ctx.best_len * sizeof(uint32_t));
	*inst_len = obj->oid_len + ctx.best_len;
	*row = ctx.best_row;

	return true;
}

const struct snmp_object *snmp_registry_next(const uint32_t *target, size_t target_len,
					     uint32_t *inst, size_t *inst_len, void **row)
{
	const struct snmp_object *best = NULL;
	uint32_t cand[SNMP_OID_MAX_LEN];
	size_t cand_len;
	void *cand_row;

	STRUCT_SECTION_FOREACH(snmp_object, obj) {
		if (!object_next_instance(obj, target, target_len, cand, &cand_len, &cand_row)) {
			continue;
		}

		if (best != NULL && snmp_oid_cmp(cand, cand_len, inst, *inst_len) >= 0) {
			continue;
		}

		best = obj;
		memcpy(inst, cand, cand_len * sizeof(uint32_t));
		*inst_len = cand_len;
		*row = cand_row;
	}

	return best;
}
