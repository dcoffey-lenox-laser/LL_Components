idf_component_register(SRCS "main.c"
                    INCLUDE_DIRS "."
                    REQUIRES linear_axis
                            esp_timer)