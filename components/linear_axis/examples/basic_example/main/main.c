#include <stdio.h>
#include "sdkconfig.h"
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include "esp_system.h"
#include "linear_axis.h"

static const char *TAG = "main";

stepper_driver_cfg_t stepper_config = {
    .en_pin = CONFIG_ENABLE_PIN,
    .step_pin = CONFIG_STEP_PIN,
    .dir_pin = CONFIG_DIRECTION_PIN,
    .en_value = CONFIG_ENABLED_LEVEL,
    .cw_value = CONFIG_CLOCKWISE_LEVEL,
    .steps_per_rev = CONFIG_STEPS_PER_REV,
    .microstep_count = CONFIG_MICROSTEPS_PER_STEP,
    .max_steps_per_sec = 1000000
};

encoder_cfg_t encoder_config = {
    .a_pin = CONFIG_A_PIN,
    .b_pin = CONFIG_B_PIN,
    .z_pin = CONFIG_Z_PIN,
};

axis_cfg_t axis_config = {
    .stepper_config = &stepper_config,
    .encoder_config = &encoder_config,
    .enabled_encoder = CONFIG_USE_ENCODER,
    .minimum_position = -1000000,
    .maximum_position = 100000000,
    .units_per_revolution = CONFIG_UNITS_PER_REV,
    .encoder_steps_per_unit = CONFIG_ENCODER_STEPS_PER_UNIT,
    .maximum_error = 0.002,
    .homeLimitPin = 8,
    .axis_units = Millimeters,
    .awayLimitPin = 46
};


stepper_motor_t stepper_motor = {
    .stepper_cfg = &stepper_config,
    .steps_per_second = 3200,
    .timer_resolution_hz = 100000,
    .position = 0
};

encoder_t encoder = {};

axis_t axis = {
    .axis_config = &axis_config,
    .encoder = &encoder,
    .stepper_motor = &stepper_motor
};

void app_main()
{
    int loop_count = 0;
    int position_out_count = 10;
    ESP_ERROR_CHECK(linear_axis_new_axis(&axis_config, &axis));
    ESP_LOGI(TAG, "Linear axis initialized");
    ESP_LOGI(TAG, "Axis energized");
    linear_axis_enable(&axis);
    linear_axis_set_position(&axis, 0.0);
    linear_axis_move_abs(&axis, 1.0);
    while(axis.stepper_motor->InMotion)
    {
        printf("Position: %f\n", linear_axis_get_relative_position(&axis));
        vTaskDelay(100/ portTICK_PERIOD_MS);
    }
    printf("Position: %f\n", linear_axis_get_relative_position(&axis));
    linear_axis_disable(&axis);
    while(1)
    {
        printf("Position: %f\n", linear_axis_get_relative_position(&axis));
        vTaskDelay(100/ portTICK_PERIOD_MS);
    }
}