/*
 * Copyright (c) 2026 Sylvain Mosnier
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <zephyr/mech/mech.h>

/* -------------------------------------------------------------------------
 * Internal float accumulator (avoids repeated int<->float casts mid-loop)
 * ---------------------------------------------------------------------- */

struct inertia_acc_f {
	float  mass_g;
	vec3_t com_mm;
	float  ixx, ixy, ixz, iyy, iyz, izz; /* g*mm2 */
};

/* -------------------------------------------------------------------------
 * mech_link_foreach visitor
 * ---------------------------------------------------------------------- */

struct inertia_acc {
	const struct mech_link  *expressed_in;
	struct inertia_acc_f     f;
	int                      err;
};

static int inertia_visitor(const struct mech_link *link,
			   uint8_t depth,
			   void *user)
{
	ARG_UNUSED(depth);

	struct inertia_acc *acc = user;
	const struct mech_inertia *idt = &link->config->inertia;

	if (idt->mass_g == 0.0f) {
		return 0;
	}

	/* Transform from this link's frame to the reference frame. */
	struct mech_transform tf;
	int ret = mech_link_transform_get(link, acc->expressed_in, &tf);

	if (ret != 0) {
		acc->err = ret;
		return ret;
	}

	/* CoM of this link in the reference frame. */
	vec3_t com_ref = mech_transform_point(&tf, idt->com_mm);

	/* Update composite mass and CoM (mass-weighted average). */
	float new_mass = acc->f.mass_g + idt->mass_g;

	if (new_mass > 0.0f) {
		float w_old = acc->f.mass_g / new_mass;
		float w_new = idt->mass_g   / new_mass;

		acc->f.com_mm = vec3_add(vec3_scale(acc->f.com_mm, w_old),
					 vec3_scale(com_ref,       w_new));
	}
	acc->f.mass_g = new_mass;

	/*
	 * Rotate the link's inertia tensor into the reference frame:
	 *   I_ref = R * I_link * R^T
	 *
	 * Tensor stored as upper triangle in mg*mm2; apply column-by-column.
	 */
	float ixx = idt->ixx;
	float ixy = idt->ixy;
	float ixz = idt->ixz;
	float iyy = idt->iyy;
	float iyz = idt->iyz;
	float izz = idt->izz;

	vec3_t rc0 = quat_rotate(tf.r, VEC3(ixx, ixy, ixz));
	vec3_t rc1 = quat_rotate(tf.r, VEC3(ixy, iyy, iyz));
	vec3_t rc2 = quat_rotate(tf.r, VEC3(ixz, iyz, izz));

	float r_ixx = rc0.x;
	float r_ixy = rc1.x;
	float r_ixz = rc2.x;
	float r_iyy = rc1.y;
	float r_iyz = rc2.y;
	float r_izz = rc2.z;

	/*
	 * Parallel axis theorem about the reference origin:
	 *   I += I_rotated + m * (||d||^2 * E - d * d^T)
	 *
	 * where d = com_ref (link CoM in reference frame).
	 */
	float m  = idt->mass_g;
	float dx = com_ref.x, dy = com_ref.y, dz = com_ref.z;
	float d2 = dx*dx + dy*dy + dz*dz;

	acc->f.ixx += r_ixx + m * (d2 - dx*dx);
	acc->f.ixy += r_ixy + m * (   - dx*dy);
	acc->f.ixz += r_ixz + m * (   - dx*dz);
	acc->f.iyy += r_iyy + m * (d2 - dy*dy);
	acc->f.iyz += r_iyz + m * (   - dy*dz);
	acc->f.izz += r_izz + m * (d2 - dz*dz);

	return 0;
}

/* -------------------------------------------------------------------------
 * Public API
 * ---------------------------------------------------------------------- */

int mech_inertia_get(const struct mech_link *root,
		     const struct mech_link *expressed_in,
		     struct mech_inertia *out)
{
	struct inertia_acc acc = {
		.expressed_in = expressed_in,
	};

	int ret = mech_link_foreach(root, inertia_visitor, &acc);

	if (ret != 0) {
		return acc.err != 0 ? acc.err : ret;
	}

	out->mass_g = acc.f.mass_g;
	out->com_mm = acc.f.com_mm;
	out->ixx    = acc.f.ixx;
	out->ixy    = acc.f.ixy;
	out->ixz    = acc.f.ixz;
	out->iyy    = acc.f.iyy;
	out->iyz    = acc.f.iyz;
	out->izz    = acc.f.izz;

	return 0;
}

int mech_inertia_com_get(const struct mech_link *root,
			 const struct mech_link *expressed_in,
			 vec3_t *out)
{
	struct mech_inertia inertia;
	int ret = mech_inertia_get(root, expressed_in, &inertia);

	if (ret != 0) {
		return ret;
	}

	*out = inertia.com_mm;
	return 0;
}
