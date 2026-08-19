#ifndef LINEAR_AXIS_H
#define LINEAR_AXIS_H

#include <stdio.h>
#include <esp_err.h>
#include <esp_check.h>
#include <driver/gpio.h>
#include <esp_system.h>

#include "stepper_driver.h"
#include "quadrature_encoder.h"

/**
 * @brief Units for the axis
 */
typedef enum
{
    Native,
    Millimeters,
    Inches,
}axis_units_t;

typedef struct axis_config_t
{
    stepper_driver_cfg_t *stepper_config;
    encoder_cfg_t *encoder_config;
    bool enabled_encoder;
    axis_units_t axis_units;
    int minimum_position;           // in native steps
    int maximum_position;           // in native steps
    double units_per_revolution;
    double encoder_steps_per_unit;
    double maximum_steps_per_sec;
    double minimum_steps_per_sec;
    double maximum_error;
    gpio_num_t homeLimitPin;
    gpio_num_t awayLimitPin;

} axis_cfg_t;


typedef struct axis_t
{
    axis_cfg_t * axis_config;
    stepper_motor_t* stepper_motor;
    encoder_t * encoder;
    int32_t position;
    bool MotorEnabled;
    double posOffset;
} axis_t;

esp_err_t linear_axis_new_axis(axis_cfg_t* axis_config, axis_t* axis_handle);
esp_err_t linear_axis_move_rel(axis_t* axis_handle, double distance);
esp_err_t linear_axis_move_abs(axis_t* axis_handle, double position);
esp_err_t linear_axis_enable(axis_t* axis_handle);
esp_err_t linear_axis_disable(axis_t* axis_handle);
esp_err_t linear_axis_set_speed(axis_t* axis_handle, double units_per_sec);
esp_err_t linear_axis_set_position(axis_t* axis_handle, double position);
esp_err_t linear_axis_set_relative_zero(axis_t* axis_handle);
esp_err_t linear_axis_configure(axis_cfg_t* axis_config, axis_t* axis_handle);

bool linear_axis_get_motion_status(axis_t* axis_handle);
double linear_axis_get_global_position(axis_t* axis_handle);
double linear_axis_get_relative_position(axis_t* axis_handle);


#endif