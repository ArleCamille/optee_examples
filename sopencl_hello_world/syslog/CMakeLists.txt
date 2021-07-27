project (693a5d5a-0f1c-4ecf-a617-05804d5c9b0b.plugin C)

set (CFG_TEE_PLUGIN_LOAD_PATH "/usr/lib/tee-supplicant/plugins/")
set (CMAKE_SHARED_LIBRARY_PREFIX "")

add_library(${PROJECT_NAME} SHARED syslog_plugin.c)

install (TARGETS ${PROJECT_NAME} DESTINATION ${CFG_TEE_PLUGIN_LOAD_PATH})
