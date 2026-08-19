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
    .steps_per_second = 64,
    .timer_resolution_hz = 100000,
    .position = 0
};


void app_main()
{
    ESP_LOGI(TAG, "*** Start Example ***");
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
    ESP_LOGI(TAG, "Position: %d reached", StepperDriver_get_position(&stepper_motor));
    ESP_LOGI(TAG, "Zeroing stepper at current position.");
    StepperDriver_set_position(&stepper_motor, 0);
    vTaskDelay(100 / portTICK_PERIOD_MS);
    ESP_LOGI(TAG, "Moving Counter-Clockwise -3200 steps");
    StepperDriver_move_num_steps(&stepper_motor, -3200);
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
    ESP_LOGI(TAG, "Position: %d reached", StepperDriver_get_position(&stepper_motor));

    ESP_LOGI(TAG, "Setting speed to 3200 steps per second");
    StepperDriver_set_speed(&stepper_motor, 3200);
    vTaskDelay(1000/portTICK_PERIOD_MS);
    ESP_LOGI(TAG, "Moving Clockwise 6400 steps");
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
    ESP_LOGI(TAG, "Position: %d reached", StepperDriver_get_position(&stepper_motor));
    ESP_LOGI(TAG, "Zeroing stepper at current position.");
    StepperDriver_set_position(&stepper_motor, 0);
    vTaskDelay(100 / portTICK_PERIOD_MS);
    ESP_LOGI(TAG, "Moving Counter-Clockwise -6400 steps");
    StepperDriver_move_num_steps(&stepper_motor, -6400);
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
    ESP_LOGI(TAG, "Position: %d reached", StepperDriver_get_position(&stepper_motor));

    ESP_LOGI(TAG, "Moving clockwise for 2 seconds");
    StepperDriver_start_motion(&stepper_motor, 1);
    vTaskDelay(3000 / portTICK_PERIOD_MS);
    ESP_LOGI(TAG, "Position: %d reached", StepperDriver_get_position(&stepper_motor));
    StepperDriver_stop_motion(&stepper_motor);
    StepperDriver_disable(&stepper_motor);
    ESP_LOGI(TAG, "Motor denergized");

    ESP_LOGI(TAG, "*** End Example ***");
}