#include "stepper_driver.h"

static const char* TAG = "Stepper Driver";
static const int pcnt_high_limit = 32767;
static const int pcnt_low_limit = -32768;

int defualt_timer_resolution_hz = 100000;
bool check_speed_valid(stepper_driver_cfg_t *config, int steps_per_sec);
uint32_t get_speed_timer_period_ticks(stepper_motor_t *stepper_handle, double steps_per_sec);

static bool StepperDriver_stop_motion_callback(pcnt_unit_handle_t units, const pcnt_watch_event_data_t *edata, void *user_ctx)
{
    stepper_motor_t *stepper = (stepper_motor_t *)user_ctx;
    int count = 0;
    esp_err_t ret = pcnt_unit_get_count(stepper->pcnt_unit, &count);
    count = count + stepper->overflow_multiplier * pcnt_high_limit;

    if(count >= stepper->travel_steps || ret != ESP_OK)
    {
        stepper->overflow_multiplier = 0;
        if(stepper->Clockwise)
        {
            stepper->position += count;
        }
        else 
        {
            stepper->position -= count;
        }
        StepperDriver_stop_motion(stepper);
    }
    else {
        stepper->overflow_multiplier += 1;
    }
    return ESP_OK;
}

static const pcnt_event_callbacks_t pcnt_callback = {
    .on_reach = StepperDriver_stop_motion_callback
};

esp_err_t StepperDriver_new_stepper_motor(stepper_motor_t *stepper_handle, stepper_driver_cfg_t *stepper_driver_cfg)
{
    if(!stepper_handle || !stepper_driver_cfg)
    {
        return ESP_ERR_INVALID_ARG;   
    }
    else {
        stepper_handle->stepper_cfg = stepper_driver_cfg;
    }

    gpio_set_direction(stepper_driver_cfg->en_pin, GPIO_MODE_OUTPUT);
    gpio_set_direction(stepper_driver_cfg->dir_pin, GPIO_MODE_OUTPUT);
    gpio_set_direction(stepper_driver_cfg->step_pin, GPIO_MODE_OUTPUT);

    mcpwm_timer_config_t timer_cfg = {
        .group_id = 0,
        .intr_priority = 0,
        .clk_src = MCPWM_TIMER_CLK_SRC_DEFAULT,
        .resolution_hz = stepper_handle->timer_resolution_hz,
        .period_ticks = 2 * stepper_handle->timer_resolution_hz / stepper_handle->steps_per_second,
        .count_mode = MCPWM_TIMER_COUNT_MODE_UP,
    };

    ESP_ERROR_CHECK(mcpwm_new_timer(&timer_cfg, &stepper_handle->timer));

    mcpwm_operator_config_t operator_cfg = {
        .group_id = 0,
    };
    ESP_ERROR_CHECK(mcpwm_new_operator(&operator_cfg, &stepper_handle->oper));
    ESP_ERROR_CHECK(mcpwm_operator_connect_timer(stepper_handle->oper, stepper_handle->timer));

    mcpwm_generator_config_t gen_config = {
        .gen_gpio_num = stepper_handle->stepper_cfg->step_pin,
    };
    ESP_ERROR_CHECK(mcpwm_new_generator(stepper_handle->oper, &gen_config, &stepper_handle->generator));

    mcpwm_comparator_config_t comp_cfg = {
        .flags.update_cmp_on_tez = true,
    };
    ESP_ERROR_CHECK(mcpwm_new_comparator(stepper_handle->oper, &comp_cfg, &stepper_handle->comparator));
    ESP_ERROR_CHECK(mcpwm_generator_set_action_on_timer_event(stepper_handle->generator, MCPWM_GEN_TIMER_EVENT_ACTION(MCPWM_TIMER_DIRECTION_UP, MCPWM_TIMER_EVENT_FULL, MCPWM_GEN_ACTION_TOGGLE)));
    
    ESP_ERROR_CHECK(mcpwm_timer_enable(stepper_handle->timer));

    // setup pulse counting
    pcnt_unit_config_t unit_config = {
        .high_limit = pcnt_high_limit,
        .low_limit = pcnt_low_limit,
        .flags.accum_count = 0, 
    };
    ESP_ERROR_CHECK(pcnt_new_unit(&unit_config, &stepper_handle->pcnt_unit));

    pcnt_glitch_filter_config_t glitch_cfg = {
        .max_glitch_ns = 1000,
    };
    ESP_ERROR_CHECK(pcnt_unit_set_glitch_filter(stepper_handle->pcnt_unit, &glitch_cfg));

    pcnt_chan_config_t chan_cfg = {
        .edge_gpio_num = stepper_driver_cfg->step_pin,
        .level_gpio_num = -1,
    };
    ESP_ERROR_CHECK(pcnt_new_channel(stepper_handle->pcnt_unit, &chan_cfg, &stepper_handle->pcnt_channel));
    ESP_ERROR_CHECK(pcnt_channel_set_edge_action(stepper_handle->pcnt_channel, PCNT_CHANNEL_EDGE_ACTION_INCREASE, PCNT_CHANNEL_EDGE_ACTION_HOLD));
    ESP_ERROR_CHECK(pcnt_unit_register_event_callbacks(stepper_handle->pcnt_unit, &pcnt_callback, stepper_handle));
    ESP_ERROR_CHECK(pcnt_unit_enable(stepper_handle->pcnt_unit));

    return ESP_OK;
}

esp_err_t StepperDriver_move_num_steps(stepper_motor_t *stepper_handle, int num_steps)
{
    if(!stepper_handle)
    {
        return ESP_ERR_INVALID_ARG;
    }

    if(stepper_handle->InMotion)
    {
        ESP_LOGI(TAG, "Stepper already in motion.");
    }


    stepper_handle->travel_steps = abs(num_steps);
    int watchpoint_value = stepper_handle->travel_steps % pcnt_high_limit;
    esp_err_t ret = pcnt_unit_add_watch_point(stepper_handle->pcnt_unit, watchpoint_value);
    ESP_ERROR_CHECK(pcnt_unit_clear_count(stepper_handle->pcnt_unit));
    if(ret == ESP_OK)
    {
        StepperDriver_start_motion(stepper_handle, num_steps);
    }
    else 
    {
        ESP_LOGE(TAG, "Failed to add watch point with %s", esp_err_to_name(ret));
    }

    return ESP_OK;
}

esp_err_t StepperDriver_start_motion(stepper_motor_t *stepper_handle, int direction)
{
    if(!stepper_handle)
    {
        return ESP_ERR_INVALID_ARG;
    }

    int dir_val = stepper_handle->stepper_cfg->cw_value;
    if(direction <= 0)
    {
        gpio_set_level(stepper_handle->stepper_cfg->dir_pin, dir_val);
        stepper_handle->Clockwise = false;
    }
    else
    {
        gpio_set_level(stepper_handle->stepper_cfg->dir_pin, !dir_val);
        stepper_handle->Clockwise = true;
    }
    ESP_ERROR_CHECK(pcnt_unit_start(stepper_handle->pcnt_unit));
    mcpwm_timer_start_stop(stepper_handle->timer, MCPWM_TIMER_START_NO_STOP);
        
    stepper_handle->InMotion = true;
    return ESP_OK;
}

esp_err_t StepperDriver_stop_motion(stepper_motor_t *stepper_handle)
{
    if(!stepper_handle)
    {
        return ESP_ERR_INVALID_ARG;
    }
    else {
        mcpwm_timer_start_stop(stepper_handle->timer, MCPWM_TIMER_STOP_FULL);
        esp_err_t ret = pcnt_unit_stop(stepper_handle->pcnt_unit);
       
        if(ret == ESP_OK)
        {
            int val = pcnt_unit_get_count(stepper_handle->pcnt_unit, &val);
            if(stepper_handle->Clockwise){
                stepper_handle->position += val;
            }
            else {
                stepper_handle->position -= val;
            }
        }
        if(ret == ESP_OK)
        {
            ret = pcnt_unit_clear_count(stepper_handle->pcnt_unit);
        }

        if(ret == ESP_OK)
        {
            ret = pcnt_unit_remove_watch_point(stepper_handle->pcnt_unit, stepper_handle->travel_steps % pcnt_high_limit);
        }
    }
    stepper_handle->InMotion = false;
    return ESP_OK;
}

esp_err_t StepperDriver_enable(stepper_motor_t *stepper_handle)
{
    if(!stepper_handle)
    {
        return ESP_ERR_INVALID_ARG;
    }
    gpio_set_level(stepper_handle->stepper_cfg->en_pin, stepper_handle->stepper_cfg->en_value);
    return ESP_OK;
}

esp_err_t StepperDriver_disable(stepper_motor_t *stepper_handle)
{
    if(!stepper_handle)
    {
        return ESP_ERR_INVALID_ARG;
    }
    int enable_value = stepper_handle->stepper_cfg->en_value;
    if(enable_value == 0)
    {
        enable_value = 1;
    }
    else 
    {
        enable_value = 0;
    }
    gpio_set_level(stepper_handle->stepper_cfg->en_pin, enable_value);
    return ESP_OK;
}

esp_err_t StepperDriver_set_speed(stepper_motor_t *stepper_handle, double steps_per_sec)
{
    if(stepper_handle == NULL)
    {
        ESP_LOGE(TAG, "No stepper motor provided");
        return ESP_ERR_INVALID_ARG;
    }

    esp_err_t ret = ESP_OK;

    bool speed_valid = check_speed_valid(stepper_handle->stepper_cfg, steps_per_sec);
    if(speed_valid)
    {
        int period_ticks = get_speed_timer_period_ticks(stepper_handle, steps_per_sec);
        ret = mcpwm_timer_set_period(stepper_handle->timer, period_ticks);
    }
    return ret;
}

esp_err_t StepperDriver_set_position(stepper_motor_t *stepper_handle, int position)
{
    if(!stepper_handle)
    {
        return ESP_ERR_INVALID_ARG;
    }
    stepper_handle->position = position;
    return ESP_OK;
}

int StepperDriver_get_position(stepper_motor_t *stepper_handle)
{
    if(!stepper_handle)
    {
        return ESP_ERR_INVALID_ARG;
    }
    int pos = stepper_handle->position;
    if(stepper_handle->InMotion)
    {
        int val;
        pcnt_unit_get_count(stepper_handle->pcnt_unit, &val);
        if(stepper_handle->Clockwise){
            pos += val;
        }
        else {
            pos -= val;
        }
    }
    return pos;
}

bool check_speed_valid(stepper_driver_cfg_t *config, int steps_per_sec)
{
    bool ret;
    assert(config);
    if(steps_per_sec < 0)
    {
        ret = false;
    }
    else if(steps_per_sec < config->min_steps_per_sec || steps_per_sec > config->max_steps_per_sec)
    {
        ret = false;
    }
    else 
    {
        ret = true;
    }
    return ret;
}

uint32_t get_speed_timer_period_ticks(stepper_motor_t *stepper_handle, double steps_per_sec)
{
    uint32_t ret;
    assert(stepper_handle);
    ret = ((double)stepper_handle->timer_resolution_hz / steps_per_sec / 2.0);
    return ret;
}