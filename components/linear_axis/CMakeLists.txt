idf_component_register(SRCS "linear_axis.c"
                    INCLUDE_DIRS "include"
                    REQUIRES    esp_driver_gpio
                                esp_timer
                                stepper_driver
                                quadrature_encoder)
