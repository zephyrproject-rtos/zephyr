/*
 * Copyright (c) 2026 Sylvain Mosnier
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <math.h>
#include <zephyr/devicetree.h>
#include <zephyr/init.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/mech/mech.h>

LOG_MODULE_REGISTER(mech, CONFIG_MECH_LOG_LEVEL);

/* DT property helpers. */
#define MECH_HELPER_DT_VEC3(node_id, prop)                                                         \
	VEC3((float)DT_PROP_BY_IDX(node_id, prop, 0), (float)DT_PROP_BY_IDX(node_id, prop, 1),     \
	     (float)DT_PROP_BY_IDX(node_id, prop, 2))

#define MECH_HELPER_DT_EULER_RAD(node_id, prop)                                                    \
	{(float)DT_PROP_BY_IDX(node_id, prop, 0) * 1e-6f,                                          \
	 (float)DT_PROP_BY_IDX(node_id, prop, 1) * 1e-6f,                                          \
	 (float)DT_PROP_BY_IDX(node_id, prop, 2) * 1e-6f}

/* Symbol name helpers. */
#define MECH_HELPER_LINK_SYM(node_id)   UTIL_CAT(_mech_link_, DT_DEP_ORD(node_id))
#define MECH_HELPER_JOINT_SYM(node_id)  UTIL_CAT(_mech_joint_, DT_DEP_ORD(node_id))
#define MECH_HELPER_DEVICE_SYM(node_id) UTIL_CAT(_mech_device_, DT_DEP_ORD(node_id))

/* mech_inertia initializer helper */
#define MECH_HELPER_DT_INERTIA(node_id)                                                            \
	{.mass_g = (float)DT_PROP_OR(node_id, mass_g, 0),                                          \
	 .com_mm = COND_CODE_1(DT_NODE_HAS_PROP(node_id, center_of_mass_mm), \
		(MECH_HELPER_DT_VEC3(node_id, center_of_mass_mm)), (VEC3_ZERO)),                    \
		  COND_CODE_1(DT_NODE_HAS_PROP(node_id, inertia_tensor_gmm2), \
		(.ixx = (float)DT_PROP_BY_IDX(node_id, inertia_tensor_gmm2, 0), \
		 .ixy = (float)DT_PROP_BY_IDX(node_id, inertia_tensor_gmm2, 1), \
		 .ixz = (float)DT_PROP_BY_IDX(node_id, inertia_tensor_gmm2, 2), \
		 .iyy = (float)DT_PROP_BY_IDX(node_id, inertia_tensor_gmm2, 3), \
		 .iyz = (float)DT_PROP_BY_IDX(node_id, inertia_tensor_gmm2, 4), \
		 .izz = (float)DT_PROP_BY_IDX(node_id, inertia_tensor_gmm2, 5),), \
		())}

/* Parent pointer helpers. */
#define MECH_HELPER_PARENT_LINK(node_id)                                                           \
	COND_CODE_1(DT_NODE_HAS_PROP(node_id, parent_link), \
		(&MECH_HELPER_LINK_SYM(DT_PHANDLE(node_id, parent_link))), (NULL))

#define MECH_HELPER_PARENT_JOINT(node_id)                                                          \
	COND_CODE_1(DT_NODE_HAS_PROP(node_id, parent_joint), \
		(&MECH_HELPER_JOINT_SYM(DT_PHANDLE(node_id, parent_joint))), (NULL))

/* Forward declaration helper for mechanical link. */
#define MECH_LINK_EXTERN(node_id) extern struct mech_link MECH_HELPER_LINK_SYM(node_id);

/* Definition helper for mechanical joint. */
#define MECH_JOINT_DT_DEFINE(node_id)                                                              \
	static struct mech_joint_data UTIL_CAT(_mech_joint_data_, DT_DEP_ORD(node_id)) = {0};      \
	static const struct mech_joint_config UTIL_CAT(_mech_joint_cfg_, DT_DEP_ORD(node_id)) = {  \
		.name = DT_NODE_FULL_NAME(node_id),                                                \
		.type = DT_ENUM_IDX(node_id, joint_type),                                          \
		.axis = MECH_HELPER_DT_VEC3(node_id, axis),                                        \
		.parent_link = MECH_HELPER_PARENT_LINK(node_id),                                   \
		.origin_translation_mm = MECH_HELPER_DT_VEC3(node_id, origin_translation_mm),      \
		.origin_euler_rad =                                                                \
			MECH_HELPER_DT_EULER_RAD(node_id, origin_rotation_euler_zyx_urad),         \
	};                                                                                         \
	struct mech_joint MECH_HELPER_JOINT_SYM(node_id) = {                                       \
		.config = &UTIL_CAT(_mech_joint_cfg_, DT_DEP_ORD(node_id)),                        \
		.data = &UTIL_CAT(_mech_joint_data_, DT_DEP_ORD(node_id)),                         \
	};

/* Definition helper for mechanical link. */
#define MECH_LINK_DT_DEFINE(node_id)                                                               \
	static const struct mech_link_config UTIL_CAT(_mech_link_cfg_, DT_DEP_ORD(node_id)) = {    \
		.name = DT_NODE_FULL_NAME(node_id),                                                \
		.parent_joint = MECH_HELPER_PARENT_JOINT(node_id),                                 \
		IF_ENABLED(CONFIG_MECH_INERTIAL, (                               \
		.inertia = MECH_HELPER_DT_INERTIA(node_id),)) };    \
	struct mech_link MECH_HELPER_LINK_SYM(node_id) = {                                         \
		.config = &UTIL_CAT(_mech_link_cfg_, DT_DEP_ORD(node_id)),                         \
	};
/* Definition helper for devices on mechanical links. */
#define MECH_DEVICE_DT_DEFINE(node_id)                                                             \
	static struct mech_device_data UTIL_CAT(_mech_device_data_, DT_DEP_ORD(node_id));          \
	static const struct mech_device_config UTIL_CAT(_mech_device_cfg_,                         \
							DT_DEP_ORD(node_id)) = {                   \
		.dev = COND_CODE_1(                                      \
			DT_NODE_HAS_STATUS(DT_PHANDLE(node_id, device), okay),   \
			(DEVICE_DT_GET(DT_PHANDLE(node_id, device))), (NULL)),                       \
			 .parent_link = &MECH_HELPER_LINK_SYM(DT_PARENT(node_id)),                 \
			 .translation_mm = MECH_HELPER_DT_VEC3(node_id, translation_mm),           \
			 .euler_rad = MECH_HELPER_DT_EULER_RAD(node_id, rotation_euler_zyx_urad),  \
	};                                                                                         \
	struct mech_device MECH_HELPER_DEVICE_SYM(node_id) = {                                     \
		.config = &UTIL_CAT(_mech_device_cfg_, DT_DEP_ORD(node_id)),                       \
		.data = &UTIL_CAT(_mech_device_data_, DT_DEP_ORD(node_id)),                        \
	};

#define MECH_LINK_DEVICES_DEFINE(node_id)                                                          \
	DT_FOREACH_CHILD_STATUS_OKAY(node_id, MECH_DEVICE_DT_DEFINE)

/* Forward-declare links before expanding joints (joint config refs links). */
DT_FOREACH_STATUS_OKAY(zephyr_mech_link, MECH_LINK_EXTERN)
DT_FOREACH_STATUS_OKAY(zephyr_mech_joint, MECH_JOINT_DT_DEFINE)
DT_FOREACH_STATUS_OKAY(zephyr_mech_link, MECH_LINK_DT_DEFINE)
DT_FOREACH_STATUS_OKAY(zephyr_mech_link, MECH_LINK_DEVICES_DEFINE)

#define MECH_LINK_PTR(node_id) &MECH_HELPER_LINK_SYM(node_id),

static const struct mech_link *const _mech_links[] = {
	DT_FOREACH_STATUS_OKAY(zephyr_mech_link, MECH_LINK_PTR)};

#define MECH_LINK_COUNT ARRAY_SIZE(_mech_links)

static int32_t joint_position_get(const struct mech_joint *joint)
{
#if defined(CONFIG_MECH_THREAD_SAFE)
	k_mutex_lock(&joint->data->lock, K_FOREVER);
#endif
	int32_t pos = joint->data->state.position;
#if defined(CONFIG_MECH_THREAD_SAFE)
	k_mutex_unlock(&joint->data->lock);
#endif
	return pos;
}

/* Get updated joint transform.*/
static struct mech_transform joint_transform(const struct mech_joint *joint)
{
	if (joint->config->type == MECH_JOINT_FIXED) {
		return joint->data->origin;
	}

	struct mech_transform tf = joint->data->origin;
	int32_t pos = joint_position_get(joint);

	switch (joint->config->type) {
	case MECH_JOINT_REVOLUTE:
	case MECH_JOINT_CONTINUOUS: {
		/* Angular position converted back to radians */
		quat_t q = quat_from_axis_angle(joint->config->axis, (float)pos * 0.001f);
		tf.r = quat_mul(q, joint->data->origin.r);
		break;
	}
	case MECH_JOINT_PRISMATIC: {
		/* Linear position kept as is, as mm is translation unit*/
		vec3_t d = vec3_scale(joint->config->axis, (float)pos);
		tf.t = vec3_add(joint->data->origin.t, quat_rotate(joint->data->origin.r, d));
		break;
	}
	default:
		break;
	}

	return tf;
}

/* Walk from @p link to the root, filling path[] and ancestors[]. */
static inline int lca_build_path(const struct mech_link *link, const struct mech_joint **path,
				 const struct mech_link **ancestors, int *depth)
{
	ancestors[0] = link;
	for (const struct mech_link *cur = link; cur->config->parent_joint != NULL;) {
		if (*depth >= CONFIG_MECH_MAX_DEPTH) {
			return -EOVERFLOW;
		}
		const struct mech_joint *pj = cur->config->parent_joint;

		path[(*depth)] = pj;
		cur = pj->config->parent_link;
		ancestors[++(*depth)] = cur;
	}
	return 0;
}

int mech_link_transform_get(const struct mech_link *from, const struct mech_link *to,
			    struct mech_transform *out)
{
	const struct mech_joint *path_from[CONFIG_MECH_MAX_DEPTH];
	const struct mech_joint *path_to[CONFIG_MECH_MAX_DEPTH];
	const struct mech_link *anc_from[CONFIG_MECH_MAX_DEPTH + 1];
	const struct mech_link *anc_to[CONFIG_MECH_MAX_DEPTH + 1];
	int depth_from = 0;
	int depth_to = 0;
	int ret;

	ret = lca_build_path(from, path_from, anc_from, &depth_from);
	if (ret != 0) {
		return ret;
	}
	ret = lca_build_path(to, path_to, anc_to, &depth_to);
	if (ret != 0) {
		return ret;
	}

	int lca_from = -1;
	int lca_to = -1;

	for (int i = 0; i <= depth_from; i++) {
		for (int j = 0; j <= depth_to; j++) {
			if (anc_from[i] == anc_to[j]) {
				lca_from = i;
				lca_to = j;
				goto found_lca;
			}
		}
	}
	return -ENOENT;

found_lca:
	struct mech_transform tf = MECH_TRANSFORM_IDENTITY;

	/* from -> LCA: invert joint transforms going up. */
	for (int i = 0; i < lca_from; i++) {
		struct mech_transform jt = joint_transform(path_from[i]);
		struct mech_transform inv = mech_transform_invert(&jt);

		tf = mech_transform_compose(&tf, &inv);
	}

	/* LCA -> to: apply joint transforms going down. */
	for (int i = lca_to - 1; i >= 0; i--) {
		struct mech_transform jt = joint_transform(path_to[i]);

		tf = mech_transform_compose(&tf, &jt);
	}

	*out = tf;
	return 0;
}

int mech_device_transform_get(const struct mech_device *dev, const struct mech_link *expressed_in,
			      struct mech_transform *out)
{
	struct mech_transform link_tf;
	int ret = mech_link_transform_get(dev->config->parent_link, expressed_in, &link_tf);

	if (ret != 0) {
		return ret;
	}

	*out = mech_transform_compose(&dev->data->mount, &link_tf);
	return 0;
}

int mech_joint_update(const struct mech_joint *joint, int32_t position, int32_t velocity)
{
	if (joint->config->type == MECH_JOINT_FIXED) {
		return -EPERM;
	}

#if defined(CONFIG_MECH_THREAD_SAFE)
	k_mutex_lock(&joint->data->lock, K_FOREVER);
#endif
	joint->data->state.position = position;
	joint->data->state.velocity = velocity;
#if defined(CONFIG_MECH_THREAD_SAFE)
	k_mutex_unlock(&joint->data->lock);
#endif

	return 0;
}

int mech_link_foreach(const struct mech_link *root, mech_link_visitor_t visitor, void *user)
{
	if (MECH_LINK_COUNT == 0) {
		return 0;
	}

	struct {
		const struct mech_link *link;
		uint8_t depth;
	} stack[MECH_LINK_COUNT];

	int top = 0;

	stack[top].link = root;
	stack[top].depth = 0;
	top++;

	while (top > 0) {
		top--;
		const struct mech_link *cur = stack[top].link;
		uint8_t depth = stack[top].depth;

		int ret = visitor(cur, depth, user);

		if (ret != 0) {
			return ret;
		}

		for (size_t i = 0; i < MECH_LINK_COUNT; i++) {
			const struct mech_link *l = _mech_links[i];

			if (l->config->parent_joint == NULL ||
			    l->config->parent_joint->config->parent_link != cur) {
				continue;
			}
			stack[top].link = l;
			stack[top].depth = depth + 1;
			top++;
		}
	}

	return 0;
}

#define MECH_JOINT_INIT(node_id)                                                                                        \
	{                                                                                                               \
		struct mech_joint *j = &MECH_HELPER_JOINT_SYM(node_id);                                                 \
		const struct mech_joint_config *cfg = j->config;                                                        \
		j->data->origin.r =                                                                                     \
			quat_from_euler_zyx(cfg->origin_euler_rad[0], cfg->origin_euler_rad[1],                         \
					    cfg->origin_euler_rad[2]);                                                  \
		j->data->origin.t = cfg->origin_translation_mm;                                                         \
		IF_ENABLED(CONFIG_MECH_THREAD_SAFE, (                            \
		if (cfg->type != MECH_JOINT_FIXED) { k_mutex_init(&j->data->lock);                            \
		})) \
	}

#define MECH_DEVICE_INIT(node_id)                                                                  \
	{                                                                                          \
		struct mech_device *d = &MECH_HELPER_DEVICE_SYM(node_id);                          \
		const struct mech_device_config *cfg = d->config;                                  \
		d->data->mount.r = quat_from_euler_zyx(cfg->euler_rad[0], cfg->euler_rad[1],       \
						       cfg->euler_rad[2]);                         \
		d->data->mount.t = cfg->translation_mm;                                            \
	}

#define MECH_LINK_DEVICES_INIT(node_id) DT_FOREACH_CHILD_STATUS_OKAY(node_id, MECH_DEVICE_INIT)

static int mech_init(void)
{
	DT_FOREACH_STATUS_OKAY(zephyr_mech_joint, MECH_JOINT_INIT)
	DT_FOREACH_STATUS_OKAY(zephyr_mech_link, MECH_LINK_DEVICES_INIT)

	LOG_DBG("%zu links, %zu joints", (size_t)MECH_LINK_COUNT,
		(size_t)DT_NUM_INST_STATUS_OKAY(zephyr_mech_joint));

	return 0;
}

SYS_INIT(mech_init, POST_KERNEL, CONFIG_MECH_INIT_PRIORITY);
