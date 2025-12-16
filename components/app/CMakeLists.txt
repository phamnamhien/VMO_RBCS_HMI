idf_component_register(
                    SRCS            "app_params.c"
                                    "app_states.c"
                                    "app_ui_helpers.c"
                    INCLUDE_DIRS    "include"
                    REQUIRES        driver 
                                    nvs_flash
                                    esp_timer
                                    esp_ticks
                                    hsm 

                                    modbus_master_manager

                                    esp_lcd_touch_gt911
                                    esp_lcd_touch
                                    lvgl
                                    ui   
                                    ui_support                            
)