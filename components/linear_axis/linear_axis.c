#include <stdio.h>
#include "linear_axis.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "sdkconfig.h"
#include "esp_timer.h"
#include <math.h>


static const char* TAG = "Linear Axis";

static QueueHandle_t gpio_evt_queue = NULL;
static volatile uint64_t last_isr_time = 0;
#define DEBOUNCE_DELAY_US 200000ULL

static void IRAM_ATTR gpio_isr_handler(void *arg)
{
    uint64_t now = esp_timer_get_time();
    if(now - last_isr_time > DEBOUNCE_DELAY_US)
    {
        last_isr_time = now;
        uint32_t gpio_num = (uint32_t) arg;
        xQueueSendFromISR(gpio_evt_queue, &gpio_num, NULL);
    }
}

static void gpio_task(void *arg)
{
    assert(arg);
    axis_t* axis = (axis_t *) arg;
    uint32_t io_num;
    for(;;)
    {
        if(xQueueReceive(gpio_evt_queue, &io_num, portMAX_DELAY))
        {
            if(io_num == axis->axis_config->awayLimitPin)
            {
                printf("\nAway Limit hit");
                if(axis->stepper_motor->InMotion)
                {
                    StepperDriver_stop_motion(axis->stepper_motor);
                }
            }
            else if(io_num == axis->axis_config->homeLimitPin)
            {
                printf("\nHome limit pin level: %d", gpio_get_level(axis->axis_config->homeLimitPin));
                if(axis->stepper_motor->InMotion)
                {
                    StepperDriver_stop_motion(axis->stepper_motor);
                }
            }
        }
        fflush(stdout);
    }
}


int get_motor_native_steps(axis_t* axis_handle, double value)
{
    assert(axis_handle);
    double steps_per_rev = axis_handle->axis_config->stepper_config->microstep_count * axis_handle->axis_config->stepper_config->steps_per_rev;
    double steps_per_unit = steps_per_rev / axis_handle->axis_config->units_per_revolution;
    int native_steps = steps_per_unit * value;
    return native_steps;
}

int get_encoder_native_steps(axis_t* axis_handle, double value)
{
    assert(axis_handle);
    double steps_per_unit = axis_handle->axis_config->encoder_steps_per_unit;
    int steps = steps_per_unit * value;
    return steps;
}

double get_encoder_unit_value(axis_t* axis_handle, int val)
{
    assert(axis_handle);
    double steps_per_unit = axis_handle->axis_config->encoder_steps_per_unit;
    double value = val / steps_per_unit;
    return value;
}

typedef struct {
    axis_t *axis;
    double target;
} axis_feedback_ctx_t;

static axis_feedback_ctx_t axis_ctx = {0};
TaskHandle_t feedback_task = NULL;

void position_feedback_task(void *pvParameter)
{
    assert(pvParameter);
    axis_feedback_ctx_t *axis_ctx = (axis_feedback_ctx_t *) pvParameter;
    axis_t *axis = axis_ctx->axis;
    bool target_reached = false; 
    // Give time for motion to start
    if(!axis->stepper_motor->InMotion){
        vTaskDelay(5 / portTICK_PERIOD_MS);
    }
    while(!target_reached)
    {
        if(!axis->stepper_motor->InMotion)
        {
            double error_val = axis_ctx->target - linear_axis_get_relative_position(axis);
            if(fabs(error_val) >= axis->axis_config->maximum_error)
            {
                int error_steps = get_motor_native_steps(axis, error_val);
                StepperDriver_move_num_steps(axis->stepper_motor, error_steps);
            }
            else {
                target_reached = true;
            }
        }
        vTaskDelay(10 / portTICK_PERIOD_MS);
    }

    feedback_task = NULL;
    vTaskDelete(NULL);
} 

esp_err_t linear_axis_new_axis(axis_cfg_t* axis_config, axis_t* axis_handle)
{
    assert(axis_config);
    assert(axis_handle);

    uint64_t io_pin_mask = ((1ULL<<(axis_config->awayLimitPin) | 1ULL<<(axis_config->homeLimitPin)));
    
    gpio_config_t io_conf = {};
    io_conf.intr_type = GPIO_INTR_NEGEDGE;
    io_conf.pin_bit_mask = io_pin_mask;
    io_conf.mode = GPIO_MODE_INPUT;
    io_conf.pull_up_en = GPIO_PULLUP_ENABLE;
    gpio_config(&io_conf);

    gpio_evt_queue = xQueueCreate(10, sizeof(uint32_t));
    xTaskCreate(gpio_task, "gpio_task", 2048, axis_handle, 10, NULL);

    if(axis_config->awayLimitPin != -1 || axis_config->homeLimitPin != -1)
    {
        gpio_install_isr_service(0);
    }
    // hook isr handler for specific gpio pin
    if(axis_config->awayLimitPin != -1)
    {
        gpio_isr_handler_add(axis_config->awayLimitPin, gpio_isr_handler, (void *)axis_config->awayLimitPin);
    }

    if(axis_config->homeLimitPin != -1)
    {
        gpio_isr_handler_add(axis_config->homeLimitPin, gpio_isr_handler, (void *)axis_config->homeLimitPin);
    }
    ESP_ERROR_CHECK(StepperDriver_new_stepper_motor(axis_handle->stepper_motor, axis_config->stepper_config));
    
    if(axis_handle->axis_config->enabled_encoder){
        ESP_ERROR_CHECK(encoder_new_encoder(axis_handle->encoder, axis_config->encoder_config));
    }   
    linear_axis_set_position(axis_handle, 0);
    return ESP_OK;

}

esp_err_t linear_axis_move_rel(axis_t* axis_handle, double distance)
{
    if(axis_handle == NULL)
    {
        return ESP_ERR_INVALID_ARG;
    }
    double target = distance + linear_axis_get_relative_position(axis_handle);
    int steps = get_motor_native_steps(axis_handle, distance);
    StepperDriver_move_num_steps(axis_handle->stepper_motor, steps);

    if(axis_handle->axis_config->enabled_encoder)
    {   
        if(feedback_task != NULL)
        {
            vTaskDelete(feedback_task);
            feedback_task = NULL;
        }
        axis_ctx.axis =axis_handle;
        axis_ctx.target = target;
        xTaskCreate(&position_feedback_task, "position", 4096, &axis_ctx, 5, &feedback_task);
    }
    return ESP_OK;
}

esp_err_t linear_axis_move_abs(axis_t* axis_handle, double position)
{
    double pos = linear_axis_get_relative_position(axis_handle);
    double dist = position - pos;
    int steps = get_motor_native_steps(axis_handle, dist);
    StepperDriver_move_num_steps(axis_handle->stepper_motor, steps);
    if(axis_handle->axis_config->enabled_encoder)
    {   
        if(feedback_task != NULL)
        {
            vTaskDelete(feedback_task);
            feedback_task = NULL;
        }
        axis_ctx.axis = axis_handle;
        axis_ctx.target = position;
        xTaskCreate(&position_feedback_task, "position", 4096, &axis_ctx, 5, &feedback_task);
    }
    return ESP_OK;
}

esp_err_t linear_axis_enable(axis_t* axis_handle)
{
    if(axis_handle == NULL)
    {
        ESP_LOGE(TAG, "No axis_handle provided");
        return ESP_ERR_INVALID_ARG;
    }
    else if(axis_handle->stepper_motor == NULL)
    {
        ESP_LOGE(TAG, "No stepper motor provided");
        return ESP_ERR_INVALID_ARG;
    }
    
    StepperDriver_enable(axis_handle->stepper_motor);
    axis_handle->MotorEnabled = true;
    return ESP_OK;
}

esp_err_t linear_axis_disable(axis_t* axis_handle)
{
    if(axis_handle == NULL)
    {
        ESP_LOGE(TAG, "No axis_handle provided");
        return ESP_ERR_INVALID_ARG;
    }
    else if(axis_handle->stepper_motor == NULL)
    {
        ESP_LOGE(TAG, "No stepper motor provided");
        return ESP_ERR_INVALID_ARG;
    }
    
    StepperDriver_disable(axis_handle->stepper_motor);
    axis_handle->MotorEnabled = true;
    return ESP_OK;
}

//TODO: implement this method
esp_err_t linear_axis_set_speed(axis_t* axis_handle, double units_per_sec)
{
    return ESP_OK;
}

esp_err_t linear_axis_set_position(axis_t* axis_handle, double position)
{
    if(axis_handle == NULL)
    {
        ESP_LOGE(TAG, "No axis_handle provided");
        return ESP_ERR_INVALID_ARG;
    }
    
    if(axis_handle->encoder == NULL)
    {
        ESP_LOGE(TAG, "No encoder provided");
        return ESP_ERR_INVALID_ARG;
    }
    int pos;
    if(axis_handle->axis_config->enabled_encoder)
    {
        pos = get_encoder_native_steps(axis_handle, position);
        encoder_set_position(axis_handle->encoder, pos);
    }
    else 
    {
        pos = get_motor_native_steps(axis_handle, position);
    }
    axis_handle->position = pos;
    
    return ESP_OK;
}

esp_err_t linear_axis_set_relative_zero(axis_t* axis_handle)
{
    assert(axis_handle);
    double steps_per_unit = axis_handle->axis_config->encoder_steps_per_unit;
    double pos = encoder_get_position(axis_handle->encoder) / steps_per_unit;
    axis_handle->posOffset = pos;
    axis_handle->position = 0;
    return ESP_OK;
}

esp_err_t linear_axis_stop_motion(axis_t* axis_handle)
{
    assert(axis_handle);
    if(feedback_task != NULL)
    {
        vTaskDelete(feedback_task);
    }
    if(axis_handle->stepper_motor->InMotion)
    {
        StepperDriver_stop_motion(axis_handle->stepper_motor);
    }
    return ESP_OK;
}

//TODO implment this function
esp_err_t linear_axis_configure(axis_cfg_t* axis_config, axis_t* axis_handle)
{
    return ESP_OK;
}

bool linear_axis_get_motion_status(axis_t* axis_handle)
{
    assert(axis_handle);
    return axis_handle->stepper_motor->InMotion;
}

double linear_axis_get_global_position(axis_t* axis_handle)
{
    if(axis_handle == NULL)
    {
        ESP_LOGE(TAG, "No axis_handle provided");
        return ESP_ERR_INVALID_ARG;
    }
    if(axis_handle->encoder == NULL)
    {
        ESP_LOGE(TAG, "No encoder provided");
        return ESP_ERR_INVALID_ARG;
    }

    double steps_per_unit = axis_handle->axis_config->encoder_steps_per_unit;
    double pos = encoder_get_position(axis_handle->encoder) / steps_per_unit;
    return pos;
}

double linear_axis_get_relative_position(axis_t* axis_handle)
{
    if(axis_handle == NULL)
    {
        ESP_LOGE(TAG, "No axis_handle provided");
        return ESP_ERR_INVALID_ARG;
    }
    if(axis_handle->encoder == NULL)
    {
        ESP_LOGE(TAG, "No encoder provided");
        return ESP_ERR_INVALID_ARG;
    }

    double steps_per_unit = axis_handle->axis_config->encoder_steps_per_unit;
    double pos = encoder_get_position(axis_handle->encoder) / steps_per_unit;
    pos = pos - axis_handle->posOffset;
    return pos;
}
