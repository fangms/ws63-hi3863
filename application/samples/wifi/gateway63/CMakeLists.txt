#===============================================================================
# @brief    cmake file
# Copyright (c) CompanyNameMagicTag 2023-2023. All rights reserved.
#===============================================================================
set(SOURCES_LIST
    ${CMAKE_CURRENT_SOURCE_DIR}/wifi/wifi_connect.c
    ${CMAKE_CURRENT_SOURCE_DIR}/gateway63.c
    # ${CMAKE_CURRENT_SOURCE_DIR}/tcp_http_dns.c
    # ${CMAKE_CURRENT_SOURCE_DIR}/ble_uart_server/ble_uart_server.c
    # ${CMAKE_CURRENT_SOURCE_DIR}/ble_uart_server/ble_uart_server_adv.c
    # ${CMAKE_CURRENT_SOURCE_DIR}/ble_uart.c
)

set(HEADER_LIST 
    ${CMAKE_CURRENT_SOURCE_DIR}/wifi
    ${CMAKE_CURRENT_SOURCE_DIR}/ble_uart_server
)

set(SOURCES "${SOURCES}" ${SOURCES_LIST} PARENT_SCOPE)
set(PUBLIC_HEADER "${PUBLIC_HEADER}" ${HEADER_LIST} PARENT_SCOPE)
