# See the build system documentation in IDF programming guide
# for more information about component CMakeLists.txt files.

idf_component_register(
    SRCS                # list the source files of this component
    "main.c"
    "attack_handshake.c"
    "attack_deauth.c"
    "attack_beacon.c"
    "attack_pmkid.c"
    "tasks_mannager.c"
    "packet_monitor.c"
    "data_converter.c"
    INCLUDE_DIRS        # optional, add here public include directories
    PRIV_INCLUDE_DIRS   # optional, add here private include directories
    REQUIRES            # optional, list the public requirements (component names)
    PRIV_REQUIRES       # optional, list the private requirements
)
