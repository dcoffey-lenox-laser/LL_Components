#ifndef CMD_AXIS_H
#define CMD_AXIS_H

#include "linear_axis.h"
#include "esp_console.h"

typedef struct {
    struct axis_t *axis;
}cmd_axis_context_t;

/**
 * @brief registers the commands for interfacing with a linear axis. Context must be created in the main task.
 */
void register_cmd_axis(cmd_axis_context_t *axis_ctx);
#endif