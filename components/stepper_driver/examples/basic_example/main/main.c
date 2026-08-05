#include <stdio.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include "sdkconfig.h"
#include "esp_system.h"
#include "stepper_driver.h"

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

stepper_motor_t stepper_motor = {
    .stepper_cfg = &stepper_config,
    .steps_per_second = 25600,
    .timer_resolution_hz = 10000000,
};


void app_main()
{
    ESP_LOGI(TAG, "*** Start Example ***");

    ESP_ERROR_CHECK(StepperDriver_new_stepper_motor(&stepper_motor, &stepper_config));
    ESP_LOGI(TAG, "Stepper motor initialized");
    ESP_LOGI(TAG, "Motor energized");
    StepperDriver_enable(&stepper_motor);
    vTaskDelay(1000/portTICK_PERIOD_MS);
    ESP_LOGI(TAG, "Moving Clockwise");
    StepperDriver_move_num_steps(&stepper_motor, 3200);
    vTaskDelay(10/portTICK_PERIOD_MS);
    while(stepper_motor.InMotion)
    {
        vTaskDelay(10 / portTICK_PERIOD_MS);
    }
    ESP_LOGI(TAG, "Moving Counter-Clockwise");
    StepperDriver_move_num_steps(&stepper_motor, -3200);
    while(stepper_motor.InMotion)
    {
        vTaskDelay(10 / portTICK_PERIOD_MS);
    }
    StepperDriver_disable(&stepper_motor);
    ESP_LOGI(TAG, "Motor denergized");

    ESP_LOGI(TAG, "*** End Example ***");
}