//
// Created by andre on 5/31/25.
//

#include "stepper_ramp.h"

extern const struct stepper_ramp_api stepper_ramp_constant_api;

extern const struct stepper_ramp_api stepper_ramp_trapezoidal_api;

const struct stepper_ramp_api *stepper_ramp_get_api(const struct stepper_ramp *ramp)
{
	switch (ramp->profile.type) {
	case STEPPER_RAMP_TYPE_SQUARE:
		return &stepper_ramp_constant_api;
	case STEPPER_RAMP_TYPE_TRAPEZOIDAL:
		return &stepper_ramp_trapezoidal_api;
	default:
		return NULL;
	}
}

uint64_t stepper_ramp_get_next_interval(struct stepper_ramp *ramp)
{
	const struct stepper_ramp_api *api = stepper_ramp_get_api(ramp);

	return api->get_next_interval(ramp);
}

uint64_t stepper_ramp_prepare_move(struct stepper_ramp *ramp, const uint32_t step_count)
{
	const struct stepper_ramp_api *api = stepper_ramp_get_api(ramp);

	return api->prepare_move(ramp, step_count);
}

uint64_t stepper_ramp_prepare_stop(struct stepper_ramp *ramp)
{
	const struct stepper_ramp_api *api = stepper_ramp_get_api(ramp);

	return api->prepare_stop(ramp);
}
