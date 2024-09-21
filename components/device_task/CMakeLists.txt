idf_component_register(SRC_DIRS "src"
                        INCLUDE_DIRS "include"
                        PRIV_REQUIRES  periodic_task clock_module wifi_service setting_server device_common telegram_client sound_generator adc_reader driver  esp_timer
                    ) 