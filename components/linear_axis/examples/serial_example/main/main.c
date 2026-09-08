#include <stdio.h>
#include "sdkconfig.h"
#include "console_settings.h"
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include "esp_system.h"
#include "esp_log.h"
#include "esp_system_console.h"
#include "cmd_axis.h"
#include "linear_axis.h"

static const char *TAG = "main";

#define PROMPT_STR CONFIG_IDF_TARGET

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
    .units_per_revolution = CONFIG_UNITS_PER_REV,
    .encoder_steps_per_unit = CONFIG_ENCODER_STEPS_PER_UNIT,
    .maximum_error = 0.002,
    .homeLimitPin = 8,
    .axis_units = Millimeters,
    .awayLimitPin = 46
};


stepper_motor_t stepper_motor = {
    .stepper_cfg = &stepper_config,
    .steps_per_second = 3200,
    .timer_resolution_hz = 100000,
    .position = 0
};

encoder_t encoder = {};

axis_t axis = {
    .axis_config = &axis_config,
    .encoder = &encoder,
    .stepper_motor = &stepper_motor
};

uart_config_t uart_cfg = {
    .baud_rate = CONFIG_ESP_CONSOLE_UART_BAUDRATE,
    .data_bits = UART_DATA_8_BITS,
    .parity = UART_PARITY_DISABLE,
    .stop_bits = UART_STOP_BITS_1,
#if SOC_UART_SUPPORT_REF_TICK
            .source_clk = UART_SCLK_REF_TICK,
#elif SOC_UART_SUPPORT_XTAL_CLK
            .source_clk = UART_SCLK_XTAL,
#endif
};

void app_main()
{
    /* register commands */
    initialize_console_peripheral(&uart_cfg);
    initialize_console_library();

    esp_console_register_help_command(); 
    

    ESP_ERROR_CHECK(linear_axis_new_axis(&axis_config, &axis));
    ESP_LOGI(TAG, "Linear axis initialized");

    
    
    cmd_axis_context_t axis_ctx = {
        .axis = &axis
    };
    register_cmd_axis(&axis_ctx);

    /* Setup control console */
    const char* prompt = setup_prompt(PROMPT_STR ">");

    while(1)
    {
        char *line = linenoise(prompt);
        if(line == NULL)
        {
            continue;
        }

        linenoiseSetDumbMode(1);

        int ret;
        esp_err_t err = esp_console_run(line, &ret);
        if(err == ESP_ERR_NOT_FOUND)
        {
            printf("Unrecognized command\n");
        }
        else if(err == ESP_ERR_INVALID_ARG)
        {
            // command was empty
        }
        else if(err == ESP_OK && ret != ESP_OK)
        {
            printf("Command returned non-zero error code: 0x%x (%s)\n", ret, esp_err_to_name(ret));
        }
        else if(err != ESP_OK)
        {
            printf("Internal error: %s\n", esp_err_to_name(err));
        }
        linenoiseFree(line);
        vTaskDelay(10/ portTICK_PERIOD_MS);
        // ESP_LOGI(TAG, "Encoder position: %d", encoder_get_position(&encoder));
    }

    ESP_LOGE(TAG, "Error or end-of-input, terminating console");
    esp_console_deinit();
}