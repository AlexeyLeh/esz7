cmake_minimum_required(VERSION 3.25)

set(CMAKE_SYSTEM_NAME Generic)
set(CMAKE_SYSTEM_PROCESSOR arm)

project(BBI2-ESZ7)
set(EXE esz7)

set(CMAKE_C_COMPILER arm-none-linux-gnueabihf-gcc.exe)

set(CMAKE_C_STANDARD 11)
#set(CMAKE_C_FLAGS "-Wall -Wextra -std=gnu11")
add_compile_options(-Wall -Wextra -std=gnu11)
set(CMAKE_EXE_LINKER_FLAGS ${CMAKE_EXE_LINKER_FLAGS} "-static") #  -pthread")

message("CMAKE_HOST_SYSTEM:      ${CMAKE_HOST_SYSTEM}")
message("CMAKE_SYSTEM_NAME:      ${CMAKE_SYSTEM_NAME}")
message("CMAKE_SYSTEM:           ${CMAKE_SYSTEM}")
message("CMAKE_SYSTEM_PROCESSOR: ${CMAKE_SYSTEM_PROCESSOR}")
message("CMAKE_C_COMPILER:       ${CMAKE_C_COMPILER}")
message("CMAKE_SOURCE_DIR:       ${CMAKE_SOURCE_DIR}")

#set(CMAKE_BUILD_TYPE debug)
#set(CMAKE_BUILD_TYPE release)

#if (${CMAKE_BUILD_TYPE} MATCHES "debug")
#    add_compile_options(-O0)
#    add_compile_options(-g)
#    message("DEBUG")
#else ()
#    #add_compile_options(-O3)
#    message("RELEASE")
#endif ()

#set(CMAKE_VERBOSE_MAKEFILE "true")

include_directories(
        src
        src/ads
        src/aps_wits
        src/atlas
        src/cfg
        src/dtr
        src/core
        src/esz7_hw
        src/esz7_pkt
        src/misc
        src/siu
        src/socket
        src/thr
        src/wits
)

set(SOURCES
        src/main.c
        src/main.h
        src/thr_callbacks.c
        src/thr_callbacks.h
        src/ver.c
        src/ver.h

        src/ads/ads.c
        src/ads/ads.h
        src/ads/ads_cmd.c
        src/ads/ads_cmd_socket.c
        src/ads/ads_data.c
        src/ads/ads_data_socket.c
        src/ads/ads_dump.c
        src/ads/ads_packet.c
        src/ads/ads_t.h

        src/atlas/atlas.c
        src/atlas/atlas.h
        src/atlas/atlas_packet.c
        src/atlas/atlas_packet.h
        src/atlas/atlas_packet_t.h
        src/atlas/atlas_socket.c
        src/atlas/atlas_socket.h

        src/cfg/cfg.c
        src/cfg/cfg.h
        src/cfg/cfg_t.h

        src/dtr/dtr.c
        src/dtr/dtr.h
        src/dtr/dtr_cmd.c
        src/dtr/dtr_cmd.c
        src/dtr/dtr_data.c
        src/dtr/dtr_cmd_socket.c
        src/dtr/dtr_data_socket.c
        src/dtr/dtr_packet.c
        src/dtr/dtr_socket.h

        src/esz7_hw/adc_t.h
        src/esz7_hw/calculateCRC.c
        src/esz7_hw/calculateCRC.h
        src/esz7_hw/adc.c
        src/esz7_hw/adc.h
        src/esz7_hw/esz7_dev.c
        src/esz7_hw/esz7_dev.h
        src/esz7_hw/enc.c
        src/esz7_hw/enc.h
        src/esz7_hw/enc_t.h
        src/esz7_hw/memmap.c
        src/esz7_hw/esz7_regs.h
        src/esz7_hw/sram.c
        src/esz7_hw/sram.h
        src/esz7_hw/sram_t.h

        src/esz7_pkt/cmd_pkt.c
        src/esz7_pkt/cmd_pkt.h
        src/esz7_pkt/cmd_pkt_t.h

        src/misc/dbg.c
        src/misc/dbg.h
        src/misc/hwclock.c
        src/misc/hwclock.h
        src/misc/sig.c
        src/misc/sig.h
        src/misc/utils.c
        src/misc/utils.h
        src/misc/wsa.c
        src/misc/wsa.h

        src/core/core.c
        src/core/core.h
        src/core/epl.c
        src/core/epl.h
        src/core/cmd.c

        src/siu/mpt_t.h
        src/siu/siu_cfg.c
        src/siu/siu.c
        src/siu/siu.h
        src/siu/siu_cmd.c
        src/siu/siu_cmd.c
        src/siu/siu_cmd_response.c
        src/siu/siu_data.c
        src/siu/siu_cmd_socket.c
        src/siu/siu_data_socket.c
        src/siu/siu_packet.c
        src/siu/siu_socket.h
        src/siu/siu_vars_t.h

        src/socket/socket.c
        src/socket/socket.h
        src/socket/socket_t.h

        src/thr/thr.c
        src/thr/thr.h
        src/thr/thr_t.h

        src/aps_wits/aps_wits.c
        src/aps_wits/aps_wits.h
        src/aps_wits/aps_wits_packet.c
        src/aps_wits/aps_wits_packet.h
        src/aps_wits/aps_wits_socket.c
        src/aps_wits/aps_wits_socket.h
        src/aps_wits/aps_wits_t.h
        src/core/core_test.c
        src/misc/dump_siu_text_packet.c
        src/misc/dump_siu_text_packet.h
        src/misc/log.h
        src/misc/log.c
        src/wits/wits.c
        src/wits/wits.h
        src/wits/wits_t.h
        src/wits/wits_thread.c
        src/wits/wits_socket.c
        src/wits/wits_packet.c
)

add_executable(${EXE} ${SOURCES})

if (WIN32)
    message(STATUS "add win lib")
    target_link_libraries(${EXE} wsock32 ws2_32)
endif ()

if (${CMAKE_SYSTEM_PROCESSOR} MATCHES arm)
    message(STATUS "add math lib")
    target_link_libraries(${EXE} m)
endif ()

function (make_dist TARGET)
    SET(VERFILE "${CMAKE_CURRENT_SOURCE_DIR}/src/ver.h")
    if(EXISTS "${VERFILE}")
        file(READ "${VERFILE}" FILE_CONTENT)

        # 2. Извлекаем номер версии (строка #define VER_NUM "3.2.1")
        if(FILE_CONTENT MATCHES [[#define[ \t]+VER_NUM[ \t]+"([^"]+)"]])
            set(VERSION_NUM "${CMAKE_MATCH_1}")
        endif()

        # 3. Извлекаем дату версии (строка #define VER_DATE "Jul 30 2026")
        if(FILE_CONTENT MATCHES [[#define[ \t]+VER_DATE[ \t]+"([^"]+)"]])
            set(VERSION_DATE "${CMAKE_MATCH_1}")
        endif()
    else()
        message(WARNING "Файл ver.h не найден! Используются версии по умолчанию.")
        set(VERSION_NUM "0.0.0")
    endif()
    message(STATUS "Версия: ${VERSION_NUM} от ${VERSION_DATE}")
    string(TOLOWER "${CMAKE_SYSTEM_PROCESSOR}" ARCH_NAME)

    set(DIST_NAME "${TARGET}-${VERSION_NUM}-bin-${ARCH_NAME}")
    set(DIST_DIR "${CMAKE_SOURCE_DIR}/dist-${CMAKE_BUILD_TYPE}")
    message(STATUS "Папка установки: ${DIST_DIR}")
    set(CMAKE_INSTALL_PREFIX "${DIST_DIR}/tmp" CACHE PATH "Install path" FORCE)
    message("CMAKE_INSTALL_PREFIX:   ${CMAKE_INSTALL_PREFIX}")
    install(TARGETS ${TARGET} RUNTIME DESTINATION ".")

    install(CODE "
    string(TIMESTAMP CURRENT_DT \"%Y%m%d-%H%M\")
    set(TARGET_NAME \"${DIST_NAME}-\${CURRENT_DT}\")
    set(TARGET_DIR \"${DIST_DIR}/\${TARGET_NAME}\")
    set(ZIP_NAME \"${DIST_DIR}/\${TARGET_NAME}.zip\")
    file(COPY \"\${CMAKE_INSTALL_PREFIX}/${TARGET}\" DESTINATION \"\${TARGET_DIR}\")
    message(STATUS \"Created: \${TARGET_DIR}\")
    execute_process(
        COMMAND \${CMAKE_COMMAND} -E tar cf \"\${ZIP_NAME}\" --format=zip \"\${TARGET_NAME}\"
        WORKING_DIRECTORY \"${DIST_DIR}\"
        RESULT_VARIABLE ARCHIVE_RESULT
    )

    if(NOT ARCHIVE_RESULT EQUAL 0)
        message(FATAL_ERROR \"Failed to create ZIP package \${ZIP_NAME}\")
    else()
        message(STATUS \"Created: \${ZIP_NAME} ...\")
    endif()

    file(REMOVE_RECURSE \"\${CMAKE_INSTALL_PREFIX}\")
")

endfunction()

make_dist(${EXE})
