#ifndef ADXL345_H
#define ADXL345_H

#include <stdbool.h>
#include <stdint.h>

typedef struct
{
    int16_t x;
    int16_t y;
    int16_t z;
} adxl345_raw_data_t;

typedef struct
{
    float x_g;
    float y_g;
    float z_g;
} adxl345_accel_g_t;

typedef enum
{
    ADXL345_RANGE_2G = 0,
    ADXL345_RANGE_4G = 1,
    ADXL345_RANGE_8G = 2,
    ADXL345_RANGE_16G = 3,
} adxl345_range_e;

typedef enum
{
    ADXL345_RATE_12_5_HZ = 0x07,
    ADXL345_RATE_25_HZ = 0x08,
    ADXL345_RATE_50_HZ = 0x09,
    ADXL345_RATE_100_HZ = 0x0A,
    ADXL345_RATE_200_HZ = 0x0B,
    ADXL345_RATE_400_HZ = 0x0C,
} adxl345_data_rate_e;

bool adxl345_init(uint8_t i2c_address);

bool adxl345_is_connected(void);

bool adxl345_set_range(adxl345_range_e range);

bool adxl345_set_data_rate(adxl345_data_rate_e rate);

bool adxl345_read_raw(adxl345_raw_data_t* out_raw_data);

bool adxl345_read_accel_g(adxl345_accel_g_t* out_accel);

float adxl345_get_scale_factor_g_per_lsb(void);

#define ADXL345_DEFAULT_I2C_ADDRESS (0x53)

#endif // ADXL345_H
