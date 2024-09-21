idf_component_register(SRC_DIRS "src"
                        INCLUDE_DIRS "include"
                        REQUIRES device_macro
                        PRIV_REQUIRES driver device_memory wifi_service device_common periodic_task sound_generator clock_module esp_timer
                        REQUIRES adc_reader
                    ) 