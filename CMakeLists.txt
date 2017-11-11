cmake_minimum_required(VERSION 2.8)
project(rtmessage)
include(ExternalProject)

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})

option(BUILD_FOR_DESKTOP "BUILD_FOR_DESKTOP" OFF)
option(BUILD_RTMESSAGE_LIB "BUILD_RTMESSAGE_LIB" ON)
option(ENABLE_RTMESSAGE_DEBUG "ENABLE_RTMESSAGE_DEBUG" OFF)
option(ENABLE_RTMESSAGE_PROFILE "ENABLE_RTMESSAGE_PROFILE" OFF)
option(BUILD_RTMESSAGE_SAMPLE_APP "BUILD_RTMESSAGE_SAMPLE_APP" OFF)
option(BUILD_RTMESSAGE_ROUTED "BUILD_RTMESSAGE_ROUTED" ON)

set(CMAKE_C_FLAGS "")

ExternalProject_Add(
  cJSON
  GIT_REPOSITORY https://github.com/DaveGamble/cJSON.git
  GIT_TAG v1.6.0
  INSTALL_DIR ${CMAKE_CURRENT_BINARY_DIR}
  INSTALL_COMMAND ""
  CONFIGURE_COMMAND ""
  BUILD_IN_SOURCE 1
  PREFIX deps
  BUILD_COMMAND make)

if (BUILD_FOR_DESKTOP)
    include_directories(
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_CURRENT_BINARY_DIR}/deps/src/cJSON)

    link_directories(
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_CURRENT_BINARY_DIR}/deps/src/cJSON
        ${CMAKE_BINARY_DIR}
        ${LIBRARY_DIR})
else()
     set(INCLUDE_DIR ${CMAKE_INSTALL_PREFIX}/include)
     set(LIBRARY_DIR ${CMAKE_INSTALL_PREFIX}/lib)
     include_directories(${CMAKE_CURRENT_SOURCE_DIR}
		    ${INCLUDE_DIR}
                    ${INCLUDE_DIR}/cjson
                   )
     link_directories(${CMAKE_CURRENT_SOURCE_DIR}
	         ${LIBRARY_DIR}
                )
endif (BUILD_FOR_DESKTOP)

set(RTMESSAGE_SOURCE_FILES rtConnection.c rtLog.c rtError.c rtMessageHeader.c 
			   rtEncoder.c rtMessage.c rtSocket.c rtVector.c)

set(RTMESSAGE_APP_FILES sample_send.c sample_recv.c)

add_definitions(-DRT_PLATFORM_LINUX)

set(LIBRARY_LINKER_OPTIONS -Werror -Wall -Wextra -I. -fPIC)
set(RTMESSAGE_LINK_DIRECTORIES ${CMAKE_CURRENT_SOURCE_DIR} ${LIBRARY_DIR})

if (ENABLE_RTMESSAGE_DEBUG)
    message("Enabling rtMessage debug")
    add_definitions(-DRT_RPC_DEBUG -DRT_DEBUG)
    set(CMAKE_CFLAGS "${CMAKE_C_FLAGS} -g -O0 -fno-inline")
else()
    set(CMAKE_CFLAGS "${CMAKE_C_FLAGS} -O2")

endif (ENABLE_RTMESSAGE_DEBUG)

if (ENABLE_RTMESSAGE_PROFILE)
    message("Enabling rtMessage profile")
    set(CMAKE_CFLAGS "${CMAKE_C_FLAGS} -pg")
endif (ENABLE_RTMESSAGE_PROFILE)

if (BUILD_RTMESSAGE_LIB)
    message("Building rtMessage lib")
    add_library(rtMessaging_lib SHARED ${RTMESSAGE_SOURCE_FILES})
    set_target_properties(rtMessaging_lib PROPERTIES OUTPUT_NAME "rtMessaging")
    target_link_libraries(rtMessaging_lib ${LIBRARY_LINKER_OPTIONS} -pthread -lcjson)
endif (BUILD_RTMESSAGE_LIB)

if (BUILD_RTMESSAGE_ROUTED)
    message ("Building rtrouted")
    set(CMAKE_CFLAGS " ${CMAKE_C_FLAGS}")
    add_executable(rtrouted_app rtrouted.c)
    if (BUILD_FOR_DESKTOP)
      add_dependencies(rtrouted_app cJSON)
    endif(foo)
    set_target_properties(rtrouted_app PROPERTIES OUTPUT_NAME "rtrouted")
    target_link_libraries(rtrouted_app ${LIBRARY_LINKER_OPTIONS} rtMessaging_lib)
    target_compile_definitions(rtrouted_app PRIVATE RT_PLATFORM_LINUX)
endif (BUILD_RTMESSAGE_ROUTED)

if (BUILD_RTMESSAGE_SAMPLE_APP)
    message ("Building rtMessage sample app with send and receive")
    set(CMAKE_CFLAGS " ${CMAKE_C_FLAGS}")
    link_directories(${RTMESSAGE_LINK_DIRECTORIES})
    #send
    add_executable(rtmessage_sample_app_send sample_send.c)
    if (BUILD_FOR_DESKTOP)
      add_dependencies(rtmessage_sample_app_send cJSON)
    endif (BUILD_FOR_DESKTOP)
    set_target_properties(rtmessage_sample_app_send PROPERTIES OUTPUT_NAME "sample_send")
    target_link_libraries(rtmessage_sample_app_send ${LIBRARY_LINKER_OPTIONS} rtMessaging_lib)
    target_compile_definitions(rtmessage_sample_app_send PRIVATE RT_PLATFORM_LINUX)
    #recv
    add_executable(rtmessage_sample_app_recv sample_recv.c)
    if (BUILD_FOR_DESKTOP)
      add_dependencies(rtmessage_sample_app_recv cJSON)
    endif (BUILD_FOR_DESKTOP)
    set_target_properties(rtmessage_sample_app_recv PROPERTIES OUTPUT_NAME "sample_recv")
    target_link_libraries(rtmessage_sample_app_recv ${LIBRARY_LINKER_OPTIONS} rtMessaging_lib)
    target_compile_definitions(rtmessage_sample_app_recv PRIVATE RT_PLATFORM_LINUX)
    #request
    add_executable(rtmessage_sample_app_req sample_req.c)
    if (BUILD_FOR_DESKTOP)
      add_dependencies(rtmessage_sample_app_req cJSON)
    endif (BUILD_FOR_DESKTOP)
    set_target_properties(rtmessage_sample_app_req PROPERTIES OUTPUT_NAME "sample_req")
    target_link_libraries(rtmessage_sample_app_req ${LIBRARY_LINKER_OPTIONS} rtMessaging_lib)
    target_compile_definitions(rtmessage_sample_app_req PRIVATE RT_PLATFORM_LINUX)
    #response
    add_executable(rtmessage_sample_app_res sample_res.c)
    if (BUILD_FOR_DESKTOP)
      add_dependencies(rtmessage_sample_app_res cJSON)
    endif (BUILD_FOR_DESKTOP)
    set_target_properties(rtmessage_sample_app_res PROPERTIES OUTPUT_NAME "sample_res")
    target_link_libraries(rtmessage_sample_app_res ${LIBRARY_LINKER_OPTIONS} rtMessaging_lib)
    target_compile_definitions(rtmessage_sample_app_res PRIVATE RT_PLATFORM_LINUX)
endif (BUILD_RTMESSAGE_SAMPLE_APP)

install (TARGETS LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR})
install (TARGETS ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR})
