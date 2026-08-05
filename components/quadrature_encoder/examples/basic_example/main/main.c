#include <stdio.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include "sdkconfig.h"
#include "esp_system.h"
#include "quadrature_encoder.h"

static const char *TAG = "main";

encoder_cfg_t encoder_config = {
    .a_pin = CONFIG_A_PIN,
    .b_pin = CONFIG_B_PIN,
    .z_pin = CONFIG_Z_PIN,
};

encoder_t encoder = {
    .config = &encoder_config,
    .position = 0,
};

void app_main()
{
    ESP_LOGI(TAG, "*** Start Example ***");
    ESP_ERROR_CHECK(encoder_new_encoder(&encoder, &encoder_config));
    ESP_LOGI(TAG, "Encoder started");

    while(true)
    {
        printf("Position: %d\n", encoder_get_position(&encoder));
        vTaskDelay(100/ portTICK_PERIOD_MS);
    }
}