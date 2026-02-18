#include "Orientation.h"

#include <math.h>
#include <stddef.h>

void orientation_estimator_init(orientation_estimator_t* inout_estimator)
{
    if (NULL == inout_estimator)
    {
        return;
    }

    inout_estimator->has_previous_sample = false;
    inout_estimator->previous_sample_time_ms = 0;
    inout_estimator->previous_roll_deg = 0.0f;
    inout_estimator->previous_pitch_deg = 0.0f;
    inout_estimator->previous_tilt_deg = 0.0f;
}

bool orientation_estimator_update(orientation_estimator_t* inout_estimator, const adxl345_accel_g_t* in_accel,
                                 uint32_t in_sample_time_ms, orientation_result_t* out_result)
{
    if ((NULL == inout_estimator) || (NULL == in_accel) || (NULL == out_result))
    {
        return false;
    }

    const float roll_deg = atan2f(in_accel->y_g, in_accel->z_g) * (180.0f / PI);
    const float pitch_denominator = sqrtf((in_accel->y_g * in_accel->y_g) + (in_accel->z_g * in_accel->z_g));
    const float pitch_deg = atan2f(-in_accel->x_g, pitch_denominator) * (180.0f / PI);
    const float xy_magnitude = sqrtf((in_accel->x_g * in_accel->x_g) + (in_accel->y_g * in_accel->y_g));
    const float tilt_deg = atan2f(xy_magnitude, in_accel->z_g) * (180.0f / PI);

    out_result->roll_deg = roll_deg;
    out_result->pitch_deg = pitch_deg;
    out_result->tilt_deg = tilt_deg;

    out_result->abs_roll_deg = fabsf(roll_deg);
    out_result->abs_pitch_deg = fabsf(pitch_deg);
    out_result->abs_tilt_deg = fabsf(tilt_deg);

    out_result->roll_rate_deg_s = 0.0f;
    out_result->pitch_rate_deg_s = 0.0f;
    out_result->tilt_rate_deg_s = 0.0f;

    if (inout_estimator->has_previous_sample)
    {
        const uint32_t dt_ms = in_sample_time_ms - inout_estimator->previous_sample_time_ms;
        const float dt_s = ((float)dt_ms) / 1000.0f;

        if (dt_s > 0.0f)
        {
            out_result->roll_rate_deg_s = (roll_deg - inout_estimator->previous_roll_deg) / dt_s;
            out_result->pitch_rate_deg_s = (pitch_deg - inout_estimator->previous_pitch_deg) / dt_s;
            out_result->tilt_rate_deg_s = (tilt_deg - inout_estimator->previous_tilt_deg) / dt_s;
        }
    }

    out_result->abs_omega_deg_s =
        sqrtf((out_result->roll_rate_deg_s * out_result->roll_rate_deg_s)
              + (out_result->pitch_rate_deg_s * out_result->pitch_rate_deg_s)
              + (out_result->tilt_rate_deg_s * out_result->tilt_rate_deg_s));

    inout_estimator->has_previous_sample = true;
    inout_estimator->previous_sample_time_ms = in_sample_time_ms;
    inout_estimator->previous_roll_deg = roll_deg;
    inout_estimator->previous_pitch_deg = pitch_deg;
    inout_estimator->previous_tilt_deg = tilt_deg;

    return true;
}
