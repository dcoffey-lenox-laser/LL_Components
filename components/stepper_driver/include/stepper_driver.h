#ifndef STEPPER_DRIVER_H
#define STEPPER_DRIVER_H

#include <stdio.h>
#include <esp_system.h>
#include <esp_err.h>
#include <esp_check.h>
#include <driver/gpio.h>
#include <driver/pulse_cnt.h>
#include <driver/mcpwm_types.h>
#include <driver/mcpwm_prelude.h>


typedef struct stepper_driver_config_t 
{
    gpio_num_t step_pin;
    gpio_num_t en_pin;
    gpio_num_t dir_pin;
    int en_value;
    int cw_value;
    int steps_per_rev;
    int microstep_count;
    int max_steps_per_sec;              // This value is in microsteps (steps_per_rev * microstep_count)
    int min_steps_per_sec;              // This value is in microsteps (steps_per_rev * microstep_count)
} stepper_driver_cfg_t;

typedef struct stepper_motor_t
{
    bool InMotion;
    stepper_driver_cfg_t *stepper_cfg;
    mcpwm_timer_handle_t timer;
    mcpwm_oper_handle_t oper;
    mcpwm_cmpr_handle_t comparator;
    mcpwm_gen_handle_t generator;
    mcpwm_gen_timer_event_action_t event_action;
    pcnt_channel_handle_t pcnt_channel;
    pcnt_unit_handle_t pcnt_unit;
    int timer_resolution_hz;
    double steps_per_second;
    int travel_steps;
    int overflow_multiplier;
} stepper_motor_t;

esp_err_t StepperDriver_new_stepper_motor(stepper_motor_t *stepper_handle, stepper_driver_cfg_t *stepper_driver_cfg);

esp_err_t StepperDriver_move_num_steps(stepper_motor_t *stepper_handle, int num_steps);

esp_err_t StepperDriver_start_motion(stepper_motor_t *stepper_handle, int direction);

esp_err_t StepperDriver_stop_motion(stepper_motor_t *stepper_handle);

esp_err_t StepperDriver_enable(stepper_motor_t *stepper_handle);

esp_err_t StepperDriver_disable(stepper_motor_t *stepper_handle);

esp_err_t StepperDriver_set_speed(stepper_motor_t *stepper_handle, double steps_per_sec);

#endif