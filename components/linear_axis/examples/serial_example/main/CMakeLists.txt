idf_component_register(SRCS "main.c" "console_settings.c"
                    INCLUDE_DIRS "." "../include"
                    REQUIRES linear_axis
                        esp_driver_uart
                        console
                        cmd_axis)