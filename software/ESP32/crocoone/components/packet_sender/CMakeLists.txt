idf_component_register(SRCS "packet_sender.c"
                    INCLUDE_DIRS "include"
                    REQUIRES esp_wifi)

target_link_libraries(${COMPONENT_LIB} -Wl,-zmuldefs)