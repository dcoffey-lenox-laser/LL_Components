idf_component_register(SRCS "main.c"
                    INCLUDE_DIRS "."
                    REQUIRES stepper_driver esp_timer esp_adc)