idf_component_register(SRCS 
                    "main.c"                    
                    "xbox_hid/xbox_handler.c"
                    "kinematic/kinematic.c"
                    "motor/motor_driver.c"
                    "esp_hid/esp_hid_gap.c"
                    INCLUDE_DIRS 
                    "."
                    "Include"
                    "kinematic"
                    "motor"
                    "xbox_hid"
                    "esp_hid"
                    REQUIRES 
                    nvs_flash bt esp_hid driver )