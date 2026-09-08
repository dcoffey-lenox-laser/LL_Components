#include "../include/console_settings.h"

#define CONSOLE_MAX_CMDLINE_ARGS 8
#define CONSOLE_MAX_CMDLINE_LENGTH 256
#define CONSOLE_PROMPT_MAX_LEN 32

char prompt[CONSOLE_PROMPT_MAX_LEN];

static const char* TAG = "console";


esp_err_t initialize_console_peripheral(uart_config_t *uart_cfg)
{
    if(uart_cfg == NULL)
    {
        ESP_LOGE(TAG, "No uart config provided");
        return ESP_ERR_INVALID_ARG;
    }

    /*Drain stdout before reconfiguring.*/
    fflush(stdout);
    fsync(fileno(stdout));

    uart_vfs_dev_port_set_rx_line_endings(CONFIG_ESP_CONSOLE_UART_NUM, ESP_LINE_ENDINGS_CR);
    uart_vfs_dev_port_set_tx_line_endings(CONFIG_ESP_CONSOLE_UART_NUM, ESP_LINE_ENDINGS_CRLF);

    ESP_ERROR_CHECK(uart_driver_install(CONFIG_ESP_CONSOLE_UART_NUM, 256, 0, 0, NULL, 0));
    ESP_ERROR_CHECK(uart_param_config(CONFIG_ESP_CONSOLE_UART_NUM, uart_cfg));

    /* Tell VFS to use UART driver */
    uart_vfs_dev_use_driver(CONFIG_ESP_CONSOLE_UART_NUM);
    
    /* Disable buffering on stdin */
    setvbuf(stdin, NULL, _IONBF, 0);
    return ESP_OK;
}

esp_err_t initialize_console_library(void)
{
    /* Initialize the console */
    esp_console_config_t console_config = {
        .max_cmdline_args = CONSOLE_MAX_CMDLINE_ARGS,
        .max_cmdline_length = CONSOLE_MAX_CMDLINE_LENGTH
    };

    ESP_ERROR_CHECK(esp_console_init(&console_config));

    linenoiseSetMultiLine(1);
    /* Tell linenoise where to get command completions and hints */
    linenoiseSetCompletionCallback(&esp_console_get_completion);
    linenoiseSetHintsCallback((linenoiseHintsCallback*) &esp_console_get_hint);

    linenoiseSetMaxLineLen(console_config.max_cmdline_length);

    linenoiseAllowEmpty(false);

    linenoiseSetDumbMode(1);

    return ESP_OK;
}


char *setup_prompt(const char *prompt_str)
{
    /* Set command line prompt */
    const char *prompt_temp = "esp>";
    if(prompt_str)
    {
        prompt_temp = prompt_str;
    }
    snprintf(prompt, CONSOLE_PROMPT_MAX_LEN - 1, "%s ", prompt_temp);

    return prompt;
}