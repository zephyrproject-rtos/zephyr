/*
 * Copyright (c) 2026 Sylvain Mosnier
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_MECH_MECH_H_
#define ZEPHYR_INCLUDE_MECH_MECH_H_

/**
 * @defgroup mech Mechanical Subsystem
 * @brief Mechanical subsystem
 * @ingroup os_services
 *
 * The Mechanical subsystem provides a build-time kinematic tree defined in Devicetree.
 * It exposes forward kinematics (frame transforms between any two links), inertial property
 * aggregation (composite mass, centre of mass, inertia tensor), and actuator-to-joint
 * transmission mappings.
 *
 * @{
 */

#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/sys/util.h>
#include <zephyr/math/quat.h>
#include <zephyr/math/vec3.h>
#include <sys/types.h>

#if defined(CONFIG_MECH_THREAD_SAFE)
#include <zephyr/kernel.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Rigid-body transform (rotation + translation).
 *
 * Maps a point from frame A to frame B:
 * @code
 *   p_B = r * p_A + t
 * @endcode
 *
 * @p t is expressed in frame B (the output frame).
 */
struct mech_transform {
	quat_t r; /**< Rotation (unit quaternion, Hamilton scalar-first). */
	vec3_t t; /**< Translation in mm, expressed in the target frame. */
};

/** @brief Identity transform: no rotation, no translation. */
#define MECH_TRANSFORM_IDENTITY \
	((struct mech_transform){.r = QUAT_IDENTITY, .t = VEC3_ZERO})

/**
 * @brief Apply a transform to a point (rotation then translation).
 *
 * @param[in] tf Transform to apply.
 * @param[in] p  Point in the source frame.
 *
 * @return Point expressed in the target frame.
 */
static inline vec3_t mech_transform_point(const struct mech_transform *tf, vec3_t p)
{
	return vec3_add(quat_rotate(tf->r, p), tf->t);
}

/**
 * @brief Apply only the rotational part of a transform.
 *
 * Use for direction vectors and normals, where translation is irrelevant.
 *
 * @param[in] tf Transform whose rotation to apply.
 * @param[in] v  Vector in the source frame.
 *
 * @return Vector rotated into the target frame.
 */
static inline vec3_t mech_transform_rotate_vec(const struct mech_transform *tf, vec3_t v)
{
	return quat_rotate(tf->r, v);
}

/**
 * @brief Compose two transforms: (A->B) then (B->C) yields (A->C).
 *
 * @param[in] a_to_b Transform from frame A to frame B.
 * @param[in] b_to_c Transform from frame B to frame C.
 *
 * @return Composed transform from frame A to frame C.
 */
static inline struct mech_transform mech_transform_compose(const struct mech_transform *a_to_b,
							   const struct mech_transform *b_to_c)
{
	return (struct mech_transform){
		.r = quat_normalize(quat_mul(b_to_c->r, a_to_b->r)),
		.t = vec3_add(quat_rotate(b_to_c->r, a_to_b->t), b_to_c->t),
	};
}

/**
 * @brief Invert a transform: if @p tf maps A->B, the result maps B->A.
 *
 * @param[in] tf Transform to invert.
 *
 * @return Inverted transform.
 */
static inline struct mech_transform mech_transform_invert(const struct mech_transform *tf)
{
	quat_t r_inv = quat_conj(tf->r);

	return (struct mech_transform){
		.r = r_inv,
		.t = quat_rotate(r_inv, vec3_neg(tf->t)),
	};
}

/** @brief Joint type. */
enum mech_joint_type {
	MECH_JOINT_FIXED = 0,      /**< No motion; child rigidly attached to parent. */
	MECH_JOINT_REVOLUTE = 1,   /**< Single-axis rotation with position limits. */
	MECH_JOINT_CONTINUOUS = 2, /**< Single-axis rotation without limits (e.g. wheel). */
	MECH_JOINT_PRISMATIC = 3,  /**< Single-axis translation with position limits. */
	MECH_JOINT_FLOATING = 4,   /**< 6-DOF unconstrained (e.g. free body). */
};

/**
 * @brief Current kinematic state of a joint.
 *
 * Units depend on joint type:
 *   - revolute / continuous: position in mrad, velocity in mrad/s
 *   - prismatic:             position in mm,   velocity in mm/s
 *   - fixed / floating:      unused
 */
struct mech_joint_state {
	int32_t position; /**< Joint position (mrad for revolute/continuous, mm for prismatic). */
	int32_t velocity; /**< Joint velocity (mrad/s or mm/s). */
};

#if defined(CONFIG_MECH_INERTIAL)
/**
 * @brief Inertial properties of a link or composite subtree.
 *
 * The inertia tensor is symmetric; only the upper triangle is stored:
 * @code
 *   | ixx ixy ixz |
 *   | ixy iyy iyz |
 *   | ixz iyz izz |
 * @endcode
 *
 * Units: mass in grams, CoM in millimeters, inertia tensor in g·mm².
 *
 * @kconfig_dep{CONFIG_MECH_INERTIAL}
 */
struct mech_inertia {
	float mass_g;  /**< Total mass in grams. */
	vec3_t com_mm; /**< Centre of mass in mm, in the reference frame. */
	float ixx;     /**< Inertia tensor upper triangle in g·mm²: Ixx. */
	float ixy;     /**< Ixy. */
	float ixz;     /**< Ixz. */
	float iyy;     /**< Iyy. */
	float iyz;     /**< Iyz. */
	float izz;     /**< Izz. */
};
#endif /* CONFIG_MECH_INERTIAL */

/* Forward declarations needed for cross-referencing config structs. */
struct mech_link;
struct mech_joint;

/**
 * @brief Compile-time configuration for a joint.
 */
struct mech_joint_config {
	const char *name;                    /**< Joint name (DT node full name). */
	vec3_t axis;                         /**< Unit axis of motion in the joint frame. */
	const struct mech_link *parent_link; /**< Parent link; NULL for root joints. */
	vec3_t origin_translation_mm;        /**< Origin translation in mm, in the child frame. */
	float origin_euler_rad[3];           /**< Origin rotation ZYX [roll, pitch, yaw] in rad. */
	enum mech_joint_type type;           /**< Kinematic constraint type. */
};

/**
 * @brief Runtime state for a joint.
 */
struct mech_joint_data {
	struct mech_transform origin;  /**< Fixed origin transform, computed at init. */
	struct mech_joint_state state; /**< Current kinematic state. */
#if defined(CONFIG_MECH_THREAD_SAFE)
	struct k_mutex lock; /**< Protects @p state for non-fixed joints. */
#endif
};

/**
 * @brief Compile-time configuration for a link.
 */
struct mech_link_config {
	const char *name;                      /**< Link name (DT node full name). */
	const struct mech_joint *parent_joint; /**< Parent joint; NULL for the root link. */
#if defined(CONFIG_MECH_INERTIAL)
	struct mech_inertia inertia; /**< Inertial properties from DT. */
#endif
};

/**
 * @brief Compile-time configuration for a mounted device.
 */
struct mech_device_config {
	const struct device *dev;            /**< Zephyr device handle. */
	const struct mech_link *parent_link; /**< Link this device is mounted on. */
	vec3_t translation_mm;               /**< Mount translation in mm, in the parent frame. */
	float euler_rad[3];                  /**< Mount rotation ZYX [roll, pitch, yaw] in rad. */
};

/**
 * @brief Runtime state for a mounted device.
 */
struct mech_device_data {
	struct mech_transform mount; /**< Device frame relative to parent link. */
};

/**
 * @brief A kinematic joint node.
 */
struct mech_joint {
	const struct mech_joint_config *config; /**< Compile-time configuration. */
	struct mech_joint_data *data;           /**< Mutable runtime state. */
};

/**
 * @brief A kinematic link node.
 */
struct mech_link {
	const struct mech_link_config *config; /**< Compile-time configuration. */
};

/**
 * @brief A device mounted on a kinematic link.
 */
struct mech_device {
	const struct mech_device_config *config; /**< Compile-time configuration. */
	struct mech_device_data *data;           /**< Mutable runtime state. */
};

/** @brief Forward-declare the mech_link variable for a DT node. */
#define MECH_LINK_DT_DECLARE(node_id) \
	extern struct mech_link UTIL_CAT(_mech_link_, DT_DEP_ORD(node_id))

/** @brief Pointer to a mech_link from a DT node (static-initializer safe). */
#define MECH_LINK_DT_PTR(node_id) (&UTIL_CAT(_mech_link_, DT_DEP_ORD(node_id)))

/** @brief Pointer to a mech_link from a DT nodelabel. */
#define MECH_LINK_GET(label) MECH_LINK_DT_PTR(DT_NODELABEL(label))

/** @brief Forward-declare the mech_joint variable for a DT node. */
#define MECH_JOINT_DT_DECLARE(node_id) \
	extern struct mech_joint UTIL_CAT(_mech_joint_, DT_DEP_ORD(node_id))

/** @brief Pointer to a mech_joint from a DT node (static-initializer safe). */
#define MECH_JOINT_DT_PTR(node_id) (&UTIL_CAT(_mech_joint_, DT_DEP_ORD(node_id)))

/** @brief Pointer to a mech_joint from a DT nodelabel. */
#define MECH_JOINT_GET(label) MECH_JOINT_DT_PTR(DT_NODELABEL(label))

/** @brief Forward-declare the mech_device variable for a DT node. */
#define MECH_DEVICE_DT_DECLARE(node_id) \
	extern struct mech_device UTIL_CAT(_mech_device_, DT_DEP_ORD(node_id))

/** @brief Pointer to a mech_device from a DT node (static-initializer safe). */
#define MECH_DEVICE_DT_PTR(node_id) (&UTIL_CAT(_mech_device_, DT_DEP_ORD(node_id)))

/** @brief Pointer to a mech_device from a DT nodelabel. */
#define MECH_DEVICE_GET(label) MECH_DEVICE_DT_PTR(DT_NODELABEL(label))

/**
 * @brief Callback type for mech_link_foreach().
 *
 * @param[in] link   Current link.
 * @param[in] depth  Depth from the subtree root (0 = root itself).
 * @param[in] user   Opaque user data passed to mech_link_foreach().
 *
 * @return 0 to continue traversal, non-zero to stop early.
 */
typedef int (*mech_link_visitor_t)(const struct mech_link *link, uint8_t depth, void *user);

/**
 * @brief Update the state of a non-fixed joint.
 *
 * @param[in] joint     Joint to update.
 * @param[in] position  New position (mrad for revolute/continuous, mm for prismatic).
 * @param[in] velocity  New velocity (mrad/s or mm/s).
 *
 * @retval 0       Success.
 * @retval -EPERM  Joint is fixed.
 */
int mech_joint_update(const struct mech_joint *joint, int32_t position, int32_t velocity);

/**
 * @brief Depth-first traversal of a link subtree.
 *
 * Stops early if @p visitor returns non-zero.
 *
 * @param[in] root     Root of the subtree to visit.
 * @param[in] visitor  Callback invoked for each link.
 * @param[in] user     Opaque pointer forwarded to @p visitor.
 *
 * @return 0 on full traversal, or the first non-zero return from @p visitor.
 */
int mech_link_foreach(const struct mech_link *root, mech_link_visitor_t visitor, void *user);

/**
 * @brief Compute the transform from link @p from to link @p to.
 *
 * @param[in]  from  Source link.
 * @param[in]  to    Target link.
 * @param[out] out   Result transform: p_to = out.r * p_from + out.t
 *
 * @retval 0           Success.
 * @retval -ENOENT     Links belong to disconnected trees.
 * @retval -EOVERFLOW  Chain depth exceeds CONFIG_MECH_MAX_DEPTH.
 */
int mech_link_transform_get(const struct mech_link *from, const struct mech_link *to,
			    struct mech_transform *out);

/**
 * @brief Compute the transform from a mounted device to a reference link.
 *
 * @param[in]  dev          Mounted device.
 * @param[in]  expressed_in Reference link defining the output frame.
 * @param[out] out          Result transform: p_ref = out.r * p_dev + out.t
 *
 * @retval 0           Success.
 * @retval -ENOENT     Device link and expressed_in belong to disconnected trees.
 * @retval -EOVERFLOW  Chain depth exceeds CONFIG_MECH_MAX_DEPTH.
 */
int mech_device_transform_get(const struct mech_device *dev, const struct mech_link *expressed_in,
			      struct mech_transform *out);

#if defined(CONFIG_MECH_INERTIAL)

/**
 * @brief Compute composite inertial properties of a link subtree.
 *
 * Results are expressed in the frame of @p expressed_in.
 *
 * @kconfig_dep{CONFIG_MECH_INERTIAL}
 *
 * @param[in]  root          Root link of the subtree.
 * @param[in]  expressed_in  Reference link defining the output frame.
 * @param[out] out           Composite inertial properties.
 *
 * @retval 0           Success.
 * @retval -ENOENT     Links belong to disconnected trees.
 * @retval -EOVERFLOW  Chain depth exceeds CONFIG_MECH_MAX_DEPTH.
 */
int mech_inertia_get(const struct mech_link *root, const struct mech_link *expressed_in,
		     struct mech_inertia *out);

/**
 * @brief Compute the composite centre of mass of a link subtree.
 *
 * @kconfig_dep{CONFIG_MECH_INERTIAL}
 *
 * @param[in]  root          Root link of the subtree.
 * @param[in]  expressed_in  Reference link defining the output frame.
 * @param[out] out           CoM position in mm, in the reference frame.
 *
 * @retval 0           Success.
 * @retval -ENOENT     Links belong to disconnected trees.
 * @retval -EOVERFLOW  Chain depth exceeds CONFIG_MECH_MAX_DEPTH.
 */
int mech_inertia_com_get(const struct mech_link *root, const struct mech_link *expressed_in,
			 vec3_t *out);

#endif /* CONFIG_MECH_INERTIAL */

#if defined(CONFIG_MECH_TRANSMISSION)

/**
 * @brief Transmission coupling a physical actuator to a kinematic joint.
 *
 * The gear ratio @p numerator / @p denominator scales actuator units to joint
 * units. A negative numerator inverts the direction.
 *
 * @kconfig_dep{CONFIG_MECH_TRANSMISSION}
 */
struct mech_transmission {
	const struct mech_joint *joint; /**< Target joint driven by this transmission. */
	const struct device *actuator;  /**< Physical actuator device; NULL if disabled. */
	int32_t numerator;              /**< Gear ratio numerator (negative = inverted). */
	int32_t denominator;            /**< Gear ratio denominator. */
};

/** @brief Forward-declare the mech_transmission variable for a DT node. */
#define MECH_TRANSMISSION_DT_DECLARE(node_id) \
	extern struct mech_transmission UTIL_CAT(_mech_tx_, DT_DEP_ORD(node_id))

/** @brief Pointer to a mech_transmission from a DT node (static-initializer safe). */
#define MECH_TRANSMISSION_DT_PTR(node_id) (&UTIL_CAT(_mech_tx_, DT_DEP_ORD(node_id)))

/** @brief Pointer to a mech_transmission from a DT nodelabel. */
#define MECH_TRANSMISSION_GET(label) MECH_TRANSMISSION_DT_PTR(DT_NODELABEL(label))

/**
 * @brief Update a joint state from a raw actuator measurement.
 *
 * @kconfig_dep{CONFIG_MECH_TRANSMISSION}
 *
 * @param[in] tx                Transmission handle.
 * @param[in] actuator_position Raw actuator position (driver-native units).
 * @param[in] actuator_velocity Raw actuator velocity (driver-native units).
 *
 * @retval 0  Success.
 */
int mech_transmission_update(const struct mech_transmission *tx, int32_t actuator_position,
			     int32_t actuator_velocity);

/**
 * @brief Convert a joint position setpoint to an actuator setpoint.
 *
 * @kconfig_dep{CONFIG_MECH_TRANSMISSION}
 *
 * @param[in]  tx                Transmission handle.
 * @param[in]  joint_position    Desired joint position (mrad or mm).
 * @param[out] actuator_position Resulting actuator setpoint (driver-native units).
 *
 * @retval 0  Success.
 */
int mech_transmission_joint_to_actuator(const struct mech_transmission *tx,
					int32_t joint_position, int32_t *actuator_position);

#endif /* CONFIG_MECH_TRANSMISSION */

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_MECH_MECH_H_ */
