#ifndef CONSOLE_SETTINGS_H
#define CONSOLE_SETTINGS_H

#include <stdio.h>
#include <esp_system.h>
#include <esp_err.h>
#include <esp_check.h>
#include <driver/gpio.h>
#include "linenoise/linenoise.h"
#include "argtable3/argtable3.h"
#include "driver/uart.h"
#include "driver/uart_vfs.h"
#include "sdkconfig.h"
#include "esp_console.h"

/**
 * @brief Initialize console peripheral type
 * 
 */

 esp_err_t initialize_console_peripheral(uart_config_t *uart_cfg);

/**
 * @brief Initialize console library 
 */
esp_err_t initialize_console_library(void);

 /**
 * @brief Initialize console prompt
 *
 * This function adds color code to the prompt (if the console supports escape sequences)
 *
 * @param prompt_str Prompt in form of string eg esp32>
 *
 * @return
 *     - pointer to initialized prompt
 */
char *setup_prompt(const char *prompt_str);

#endif