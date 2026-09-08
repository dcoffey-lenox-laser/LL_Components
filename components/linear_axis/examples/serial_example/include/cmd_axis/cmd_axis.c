#include <stdio.h>
#include "cmd_axis.h"
#include "argtable3/argtable3.h"

static const char *TAG = "cmd_axis";

static struct {
    struct arg_int *value;
    struct arg_end *end;
} move_dir_args;

static struct {
    struct arg_dbl *value;
    struct arg_end *end;
} set_speed_args;

static struct {
    struct arg_dbl *value;
    struct arg_end *end;
} move_rel_args;

static struct {
    struct arg_dbl *value;
    struct arg_end *end;
} move_abs_args;

static int set_axis_speed(void *context, int argc, char **argv)
{
    int nerrors = arg_parse(argc, argv, (void **) &set_speed_args);
    if(nerrors != 0)
    {
        arg_print_errors(stderr, set_speed_args.end, argv[0]);
        return 1;
    }

    const double *dist = set_speed_args.value->dval;
    cmd_axis_context_t *axis_ctx = NULL;
    if(context != NULL)
    {
        axis_ctx = (cmd_axis_context_t *) context;
    }

    axis_t *axis = NULL;
    if(axis_ctx != NULL)
    {
        axis = axis_ctx->axis;
        if(axis != NULL)
        {
            linear_axis_set_speed(axis, dist[0]);
        }
        else {
            return 1;
        }
    }
    return 0;
}

static int axis_move_dir(void *context, int argc, char **argv)
{
    int nerrors = arg_parse(argc, argv, (void **) &move_dir_args);
    if(nerrors != 0)
    {
        arg_print_errors(stderr, move_dir_args.end, argv[0]);
        return 1;
    }

    const int *dir = move_dir_args.value->ival;
    cmd_axis_context_t *axis_ctx = NULL;
    if(context != NULL)
    {
        axis_ctx = (cmd_axis_context_t *) context;
    }

    axis_t *axis = NULL;
    if(axis_ctx != NULL)
    {
        axis = axis_ctx->axis;
        if(axis != NULL)
        {
            linear_axis_move_dir(axis, dir[0]);
        }
        else {
            return 1;
        }
    }
    return 0;
}

static int move_relative_axis(void *context, int argc, char **argv)
{
    int nerrors = arg_parse(argc, argv, (void **) &move_rel_args);
    if(nerrors != 0)
    {
        arg_print_errors(stderr, move_rel_args.end, argv[0]);
        return 1;
    }

    const double *dist = move_rel_args.value->dval;
    cmd_axis_context_t *axis_ctx = NULL;
    if(context != NULL)
    {
        axis_ctx = (cmd_axis_context_t *) context;
    }

    axis_t *axis = NULL;
    if(axis_ctx != NULL)
    {
        axis = axis_ctx->axis;
        if(axis != NULL)
        {
            linear_axis_move_rel(axis, dist[0]);
        }
        else {
            return 1;
        }
    }
    return 0;
}

static int move_absolute_axis(void *context, int argc, char **argv)
{
    int nerrors = arg_parse(argc, argv, (void **) &move_abs_args);
    if(nerrors != 0)
    {
        arg_print_errors(stderr, move_abs_args.end, argv[0]);
        return 1;
    }

    const double *dist = move_abs_args.value->dval;
    cmd_axis_context_t *axis_ctx = NULL;
    if(context != NULL)
    {
        axis_ctx = (cmd_axis_context_t *) context;
    }

    axis_t *axis = NULL;
    if(axis_ctx != NULL)
    {
        axis = axis_ctx->axis;
        if(axis != NULL)
        {
            linear_axis_move_abs(axis, dist[0]);
        }
        else {
            return 1;
        }
    }
    return 0;
}

static int print_axis_status(void *context, int argc, char **argv)
{
    cmd_axis_context_t *axis_ctx = NULL;
    if(context != NULL)
    {
        axis_ctx = (cmd_axis_context_t *) context;
    }

    axis_t *axis = NULL;
    if(axis_ctx != NULL)
    {
        axis = axis_ctx->axis;
        if(axis != NULL)
        {
            printf("\nAxis status:\n");
            printf("\tPosition: %f\n", linear_axis_get_global_position(axis));
            printf("\tZero Offset: %f\n", axis->posOffset);
            double speed = axis->stepper_motor->steps_per_second / axis->axis_config->stepper_config->steps_per_rev * axis->axis_config->units_per_revolution / axis->axis_config->stepper_config->microstep_count;
            printf("\tSpeed: %f\n", speed);

        }
        else {
            return 1;
        }
    }
    return 0;
}

static int set_relative_zero(void *context, int argc, char **argv)
{
    cmd_axis_context_t *axis_ctx = NULL;
    if(context != NULL)
    {
        axis_ctx = (cmd_axis_context_t *) context;
    }

    axis_t *axis = NULL;
    if(axis_ctx != NULL)
    {
        axis = axis_ctx->axis;
        if(axis != NULL)
        {
           linear_axis_set_relative_zero(axis);
        }
        else {
            return 1;
        }
    }
    return 0;
}

static int get_relative_position(void *context, int argc, char **argv)
{
    cmd_axis_context_t *axis_ctx = NULL;
    if(context != NULL)
    {
        axis_ctx = (cmd_axis_context_t *) context;
    }

    axis_t *axis = NULL;
    if(axis_ctx != NULL)
    {
        axis = axis_ctx->axis;
        if(axis != NULL)
        {
           printf("%f\n",linear_axis_get_relative_position(axis));
        }
        else {
            return 1;
        }
    }
    return 0;
}

static int enable_axis(void *context, int argc, char **argv){
    cmd_axis_context_t *axis_ctx = NULL;
    if(context != NULL)
    {
        axis_ctx = (cmd_axis_context_t *) context;
    }

    axis_t *axis = NULL;
    if(axis_ctx != NULL)
    {
        axis = axis_ctx->axis;
        if(axis != NULL)
        {
            linear_axis_enable(axis);
        }
        else {
            return 1;
        }
    }
    return 0;
}

static int disable_axis(void *context, int argc, char **argv){
    cmd_axis_context_t *axis_ctx = NULL;
    if(context != NULL)
    {
        axis_ctx = (cmd_axis_context_t *) context;
    }

    axis_t *axis = NULL;
    if(axis_ctx != NULL)
    {
        axis = axis_ctx->axis;
        if(axis != NULL)
        {
            linear_axis_disable(axis);
        }
        else {
            return 1;
        }
    }
    return 0;
}

static int stop_motion(void *context, int argc, char **argv)
{
    cmd_axis_context_t *axis_ctx = NULL;
    if(context != NULL)
    {
        axis_ctx = (cmd_axis_context_t *) context;
    }

    axis_t *axis = NULL;
    if(axis_ctx != NULL)
    {
        axis = axis_ctx->axis;
        if(axis != NULL)
        {
           linear_axis_stop(axis);
        }
        else {
            return 1;
        }
    }
    return 0;
}
void register_cmd_axis(cmd_axis_context_t* axis_ctx)
{
    if(axis_ctx == NULL)
    {
        ESP_LOGE(TAG, "No axis context provided to command registration function.");
    }   
    
    // Enable command
    move_dir_args.value = arg_int1(NULL, NULL, "<int>", "-1 for counter clockwise. 1 for countclockwise.");
    move_dir_args.end = arg_end(1);

    const esp_console_cmd_t move_dir_cmd = {
        .command = "move_dir",
        .help = "starts motion in the specified direction.",
        .hint = NULL,
        .func = NULL,
        .func_w_context = &axis_move_dir,
        .context = axis_ctx
    };
    ESP_ERROR_CHECK(esp_console_cmd_register(&move_dir_cmd));

    set_speed_args.value = arg_dbl1(NULL, NULL, "<double>", "speed to set axis to in current units per second");
    set_speed_args.end = arg_end(1);

    const esp_console_cmd_t set_speed_cmd = {
        .command = "speed",
        .help = "Set the speed of the axis in choosen units per second",
        .hint = NULL,
        .func = NULL,
        .func_w_context = &set_axis_speed,
        .context = axis_ctx,
        .argtable = &set_speed_args,
    };

    ESP_ERROR_CHECK(esp_console_cmd_register(&set_speed_cmd));
    
    // Enable command

    const esp_console_cmd_t enable_cmd = {
        .command = "enable",
        .help = "Energizes the axis.",
        .hint = NULL,
        .func = NULL,
        .func_w_context = &enable_axis,
        .context = axis_ctx
    };
    ESP_ERROR_CHECK(esp_console_cmd_register(&enable_cmd));

    const esp_console_cmd_t disable_cmd = {
        .command = "disable",
        .help = "Denergizes the axis.",
        .hint = NULL,
        .func = NULL,
        .func_w_context = &disable_axis,
        .context = axis_ctx
    };
    ESP_ERROR_CHECK(esp_console_cmd_register(&disable_cmd));    


    // move relative command
    move_rel_args.value = arg_dbl1("d", "dist", "<double>", "Distance to be traveld in current units");
    move_rel_args.end = arg_end(1);

    const esp_console_cmd_t move_rel_cmd = {
        .command = "move_rel",
        .help = "Initiate relative move",
        .hint = NULL, 
        .func = NULL,
        .func_w_context = &move_relative_axis,
        .context = axis_ctx,
        .argtable = &move_rel_args,
    };

    ESP_ERROR_CHECK(esp_console_cmd_register(&move_rel_cmd));



    // move absolute command
    move_abs_args.value = arg_dbl1("p", "pos", "<double>", "Distance to be traveld in current units");
    move_abs_args.end = arg_end(1);

    const esp_console_cmd_t move_abs_cmd = {
        .command = "move_abs",
        .help = "Initiate abolute move",
        .hint = NULL,
        .func = NULL,
        .func_w_context = &move_absolute_axis,
        .context = axis_ctx,
        .argtable = &move_abs_args,
    };

    ESP_ERROR_CHECK(esp_console_cmd_register(&move_abs_cmd));

    const esp_console_cmd_t status_cmd = {
        .command = "status",
        .help = "Get the current axis status",
        .hint = NULL,
        .func = NULL,
        .func_w_context = &print_axis_status,
        .context = axis_ctx,
    };
    ESP_ERROR_CHECK(esp_console_cmd_register(&status_cmd));

    const esp_console_cmd_t zero_cmd = {
        .command = "zero",
        .help = "Set the current axis position to zero.",
        .hint = NULL,
        .func = NULL,
        .func_w_context = &set_relative_zero,
        .context = axis_ctx,
    };
    ESP_ERROR_CHECK(esp_console_cmd_register(&zero_cmd));

    const esp_console_cmd_t get_pos_cmd = {
        .command = "pos",
        .help = "Get the current relative position of the axis.",
        .hint = NULL,
        .func = NULL,
        .func_w_context = &get_relative_position,
        .context = axis_ctx,
    };
    ESP_ERROR_CHECK(esp_console_cmd_register(&get_pos_cmd));

    const esp_console_cmd_t stop_motion_cmd = {
        .command = "stop",
        .help = "Stops current motion of the axis.",
        .hint = NULL,
        .func = NULL,
        .func_w_context = &stop_motion,
        .context = axis_ctx
    };
    ESP_ERROR_CHECK(esp_console_cmd_register(&stop_motion_cmd));
}