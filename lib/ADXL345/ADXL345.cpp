#include "ADXL345.h"

#include <Arduino.h>
#include <Wire.h>

// Register map
static constexpr uint8_t ADXL345_REG_DEVID = 0x00;
static constexpr uint8_t ADXL345_REG_BW_RATE = 0x2C;
static constexpr uint8_t ADXL345_REG_POWER_CTL = 0x2D;
static constexpr uint8_t ADXL345_REG_DATA_FORMAT = 0x31;
static constexpr uint8_t ADXL345_REG_DATAX0 = 0x32;

static constexpr uint8_t ADXL345_DEVICE_ID = 0xE5;

static constexpr uint8_t ADXL345_POWER_CTL_MEASURE_BIT = 0x08;
static constexpr uint8_t ADXL345_DATA_FORMAT_FULL_RES_BIT = 0x08;

static constexpr float ADXL345_SCALE_FACTOR_G_PER_LSB = 0.0039f;

static bool g_is_initialized = false;
static uint8_t g_i2c_address = ADXL345_DEFAULT_I2C_ADDRESS;

static bool prv_write_register(uint8_t reg, uint8_t value);
static bool prv_read_register(uint8_t reg, uint8_t* out_value);
static bool prv_read_registers(uint8_t start_reg, uint8_t* out_buffer, size_t size);

bool adxl345_init(uint8_t i2c_address)
{
    g_i2c_address = i2c_address;

    Wire.begin();

    uint8_t device_id = 0;
    if (!prv_read_register(ADXL345_REG_DEVID, &device_id))
    {
        g_is_initialized = false;
        return false;
    }

    if (device_id != ADXL345_DEVICE_ID)
    {
        g_is_initialized = false;
        return false;
    }

    if (!adxl345_set_data_rate(ADXL345_RATE_100_HZ))
    {
        g_is_initialized = false;
        return false;
    }

    if (!adxl345_set_range(ADXL345_RANGE_2G))
    {
        g_is_initialized = false;
        return false;
    }

    if (!prv_write_register(ADXL345_REG_POWER_CTL, ADXL345_POWER_CTL_MEASURE_BIT))
    {
        g_is_initialized = false;
        return false;
    }

    g_is_initialized = true;
    return true;
}

bool adxl345_is_connected(void)
{
    uint8_t device_id = 0;
    if (!prv_read_register(ADXL345_REG_DEVID, &device_id))
    {
        return false;
    }

    return (device_id == ADXL345_DEVICE_ID);
}

bool adxl345_set_range(adxl345_range_e range)
{
    uint8_t data_format = ADXL345_DATA_FORMAT_FULL_RES_BIT | (uint8_t)range;
    return prv_write_register(ADXL345_REG_DATA_FORMAT, data_format);
}

bool adxl345_set_data_rate(adxl345_data_rate_e rate)
{
    return prv_write_register(ADXL345_REG_BW_RATE, (uint8_t)rate);
}

bool adxl345_read_raw(adxl345_raw_data_t* out_raw_data)
{
    if (NULL == out_raw_data)
    {
        return false;
    }

    uint8_t raw_bytes[6] = {0};
    if (!prv_read_registers(ADXL345_REG_DATAX0, raw_bytes, sizeof(raw_bytes)))
    {
        return false;
    }

    out_raw_data->x = (int16_t)((raw_bytes[1] << 8) | raw_bytes[0]);
    out_raw_data->y = (int16_t)((raw_bytes[3] << 8) | raw_bytes[2]);
    out_raw_data->z = (int16_t)((raw_bytes[5] << 8) | raw_bytes[4]);

    return true;
}

bool adxl345_read_accel_g(adxl345_accel_g_t* out_accel)
{
    if (NULL == out_accel)
    {
        return false;
    }

    adxl345_raw_data_t raw_data = {0};
    if (!adxl345_read_raw(&raw_data))
    {
        return false;
    }

    const float scale = adxl345_get_scale_factor_g_per_lsb();
    out_accel->x_g = ((float)raw_data.x) * scale;
    out_accel->y_g = ((float)raw_data.y) * scale;
    out_accel->z_g = ((float)raw_data.z) * scale;

    return true;
}

float adxl345_get_scale_factor_g_per_lsb(void)
{
    return ADXL345_SCALE_FACTOR_G_PER_LSB;
}

static bool prv_write_register(uint8_t reg, uint8_t value)
{
    Wire.beginTransmission(g_i2c_address);
    Wire.write(reg);
    Wire.write(value);
    return (Wire.endTransmission() == 0);
}

static bool prv_read_register(uint8_t reg, uint8_t* out_value)
{
    if (NULL == out_value)
    {
        return false;
    }

    Wire.beginTransmission(g_i2c_address);
    Wire.write(reg);
    if (Wire.endTransmission(false) != 0)
    {
        return false;
    }

    if (Wire.requestFrom((int)g_i2c_address, 1) != 1)
    {
        return false;
    }

    *out_value = Wire.read();
    return true;
}

static bool prv_read_registers(uint8_t start_reg, uint8_t* out_buffer, size_t size)
{
    if ((NULL == out_buffer) || (size == 0))
    {
        return false;
    }

    Wire.beginTransmission(g_i2c_address);
    Wire.write(start_reg);
    if (Wire.endTransmission(false) != 0)
    {
        return false;
    }

    if (Wire.requestFrom((int)g_i2c_address, (int)size) != (int)size)
    {
        return false;
    }

    for (size_t i = 0; i < size; i++)
    {
        out_buffer[i] = Wire.read();
    }

    return true;
}
