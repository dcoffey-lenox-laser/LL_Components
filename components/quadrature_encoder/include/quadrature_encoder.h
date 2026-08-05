#ifndef QUADRATURE_ENCODER_H
#define QUADRATURE_ENCODER_H

#include <stdio.h>
#include <esp_system.h>
#include <esp_err.h>
#include <esp_check.h>
#include <driver/gpio.h>
#include <driver/pulse_cnt.h>

typedef struct encoder_config_t
{
    gpio_num_t a_pin;
    gpio_num_t b_pin;
    gpio_num_t z_pin;
} encoder_cfg_t;


typedef struct encoder_t
{
    encoder_cfg_t *config;
    pcnt_unit_handle_t pcnt_unit;
    pcnt_channel_handle_t pcnt_a_channel;
    pcnt_channel_handle_t pcnt_b_channel;
    pcnt_channel_handle_t pcnt_z_channel;
    int32_t position;
} encoder_t;

esp_err_t encoder_new_encoder(encoder_t *encoder, encoder_cfg_t* config);

esp_err_t encoder_set_position(encoder_t *encoder, int position);

int encoder_get_position(encoder_t *encoder);

#endif