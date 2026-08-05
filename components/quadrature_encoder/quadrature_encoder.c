#include <stdio.h>
#include "quadrature_encoder.h"

static const char* TAG = "Encoder";
static const int pcnt_high_limit = 32767;
static const int pcnt_low_limit = -32767;

static bool encoder_pcnt_reach_callback(pcnt_unit_handle_t unit, const pcnt_watch_event_data_t * edata, void *user_ctx)
{
    encoder_t *encoder = (encoder_t *)user_ctx;
    int count = 0;
    pcnt_unit_get_count(encoder->pcnt_unit, &count);
    encoder->position += count;
    // pcnt_unit_clear_count(encoder->pcnt_unit);
    return ESP_OK;
}

static const pcnt_event_callbacks_t pcnt_callback = {
    .on_reach = encoder_pcnt_reach_callback
};

esp_err_t encoder_new_encoder(encoder_t* encoder, encoder_cfg_t *config)
{
    gpio_set_direction(config->a_pin, GPIO_MODE_INPUT);
    gpio_set_direction(config->b_pin, GPIO_MODE_INPUT);
    gpio_set_direction(config->z_pin, GPIO_MODE_INPUT);

    pcnt_unit_config_t unit_config = {
        .high_limit = pcnt_high_limit,
        .low_limit = pcnt_low_limit,
        .flags.accum_count = 0,
    };
    ESP_ERROR_CHECK(pcnt_new_unit(&unit_config, &encoder->pcnt_unit));

    pcnt_glitch_filter_config_t glitch_cfg = {
        .max_glitch_ns = 1000,
    };
    ESP_ERROR_CHECK(pcnt_unit_set_glitch_filter(encoder->pcnt_unit, &glitch_cfg));

    pcnt_chan_config_t pcnt_a_cfg = {
        .edge_gpio_num = config->a_pin,
        .level_gpio_num = config->b_pin
    };
    ESP_ERROR_CHECK(pcnt_new_channel(encoder->pcnt_unit, &pcnt_a_cfg, &encoder->pcnt_a_channel));

    pcnt_chan_config_t pcnt_b_cfg = {
        .edge_gpio_num = config->b_pin,
        .level_gpio_num = config->a_pin,
    };
    ESP_ERROR_CHECK(pcnt_new_channel(encoder->pcnt_unit, &pcnt_b_cfg, &encoder->pcnt_b_channel));

    ESP_ERROR_CHECK(pcnt_channel_set_edge_action(encoder->pcnt_a_channel, PCNT_CHANNEL_EDGE_ACTION_DECREASE, PCNT_CHANNEL_EDGE_ACTION_INCREASE));
    ESP_ERROR_CHECK(pcnt_channel_set_level_action(encoder->pcnt_a_channel, PCNT_CHANNEL_LEVEL_ACTION_KEEP, PCNT_CHANNEL_LEVEL_ACTION_INVERSE));
    ESP_ERROR_CHECK(pcnt_channel_set_edge_action(encoder->pcnt_b_channel, PCNT_CHANNEL_EDGE_ACTION_INCREASE, PCNT_CHANNEL_EDGE_ACTION_DECREASE));
    ESP_ERROR_CHECK(pcnt_channel_set_level_action(encoder->pcnt_b_channel, PCNT_CHANNEL_LEVEL_ACTION_KEEP, PCNT_CHANNEL_LEVEL_ACTION_INVERSE));
    ESP_ERROR_CHECK(pcnt_unit_add_watch_point(encoder->pcnt_unit, pcnt_high_limit - 1));
    ESP_ERROR_CHECK(pcnt_unit_add_watch_point(encoder->pcnt_unit, pcnt_low_limit + 1));
    ESP_ERROR_CHECK(pcnt_unit_register_event_callbacks(encoder->pcnt_unit, &pcnt_callback, encoder)); 
    ESP_ERROR_CHECK(pcnt_unit_enable(encoder->pcnt_unit));
    ESP_ERROR_CHECK(pcnt_unit_clear_count(encoder->pcnt_unit));
    ESP_ERROR_CHECK(pcnt_unit_start(encoder->pcnt_unit));

    return ESP_OK;   
}

esp_err_t encoder_set_position(encoder_t* encoder, int position)
{
    encoder->position = position;
    pcnt_unit_clear_count(encoder->pcnt_unit);
    return ESP_OK;
}

int encoder_get_position(encoder_t* encoder)
{
    int count = 0;
    esp_err_t ret = pcnt_unit_get_count(encoder->pcnt_unit, &count);
    if(ret != ESP_OK)
    {
        ESP_LOGE(TAG, "%s", esp_err_to_name(ret));
    }
    count = encoder->position + count;
    return count;
}