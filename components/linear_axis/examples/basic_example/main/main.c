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
    .units_per_revolution = 3200,
    .encoder_steps_per_unit = CONFIG_ENCODER_STEPS,
    .maximum_error = 5,
    .homeLimitPin = CONFIG_HOME_LIMIT_PIN,
    .axis_units = Millimeters,
    .awayLimitPin = CONFIG_AWAY_LIMIT_PIN,
};

stepper_motor_t stepper_motor = {
    .stepper_cfg = &stepper_config,
    .steps_per_second = 64,
    .timer_resolution_hz = 100000,
    .position = 0
};

void app_main()
{
    int loop_count = 0;
    int position_out_count = 10;
    ESP_ERROR_CHECK(StepperDriver_new_stepper_motor(&stepper_motor, &stepper_config));
    ESP_LOGI(TAG, "Stepper motor initialized");
    ESP_LOGI(TAG, "Motor energized");
    StepperDriver_set_speed(&stepper_motor, 320);
    StepperDriver_enable(&stepper_motor);
    vTaskDelay(1000/portTICK_PERIOD_MS);
    ESP_LOGI(TAG, "Moving Clockwise 3200 steps");
    StepperDriver_move_num_steps(&stepper_motor, 3200);
    vTaskDelay(10/portTICK_PERIOD_MS);
    while(stepper_motor.InMotion)
    {
        loop_count += 1;
        vTaskDelay(10 / portTICK_PERIOD_MS);
        if(loop_count >= position_out_count)
        {
            ESP_LOGI(TAG, "Position: %d", StepperDriver_get_position(&stepper_motor));
            loop_count = 0;
        }
    }
}