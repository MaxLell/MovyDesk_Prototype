#ifndef ORIENTATION_H
#define ORIENTATION_H

#include <stdbool.h>
#include <stdint.h>

#include "ADXL345.h"

typedef struct
{
    bool has_previous_sample;
    uint32_t previous_sample_time_ms;
    float previous_roll_deg;
    float previous_pitch_deg;
    float previous_tilt_deg;
} orientation_estimator_t;

typedef struct
{
    float roll_deg;
    float pitch_deg;
    float tilt_deg;

    float abs_roll_deg;
    float abs_pitch_deg;
    float abs_tilt_deg;

    float roll_rate_deg_s;
    float pitch_rate_deg_s;
    float tilt_rate_deg_s;

    float abs_omega_deg_s;
} orientation_result_t;

void orientation_estimator_init(orientation_estimator_t* inout_estimator);

bool orientation_estimator_update(orientation_estimator_t* inout_estimator, const adxl345_accel_g_t* in_accel,
                                 uint32_t in_sample_time_ms, orientation_result_t* out_result);

#endif // ORIENTATION_H
