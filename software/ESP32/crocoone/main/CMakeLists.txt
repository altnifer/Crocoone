idf_component_register(
    SRCS 
    "main.c"
    "packet_sender.c"
    "wifi_control.c"
    "at_command.c"
    "frame_analyzer.c"
    "frame_analyzer_parser.c"
    "hccapx_serializer.c"
    "pcap_serializer.c"
    "sniffer.c"
    "scanner.c"
    "uart_control.c"
    "attack_handshake.c"
    "attack_deauth.c"
    "attack_beacon.c"
    "attacks.c"
    "packet_monitor.c"
    "hc22000_converter.c"
    "attack_pmkid.c"
   
    INCLUDE_DIRS
    "."
)
target_link_libraries(${COMPONENT_LIB} -Wl,-zmuldefs)