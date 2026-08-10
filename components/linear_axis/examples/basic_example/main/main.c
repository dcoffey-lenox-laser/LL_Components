#include <stdio.h>
#include "sdkconfig.h"
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include "esp_system.h"
#include "linear_axis.h"

static const char *TAG = "main";

void app_main()
{
    ESP_LOGI(TAG, "***      Start Example       ***");
    ESP_LOGI(TAG, " Setting up linear axis");
    vTaskDelay(1000 / portTICK_PERIOD_MS);
    ESP_LOGI(TAG, "***      End Example     ***");
}