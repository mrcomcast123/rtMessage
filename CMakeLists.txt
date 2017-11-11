cmake_minimum_required(VERSION 2.8)
project(rtmessage)
include(ExternalProject)

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_C_FLAGS_DEBUG "-Werror -Wall -Wextra -g -O0 -fno-inline")
set(CMAKE_C_FLAGS_RELEASE "-Werror -Wall -Wextra")
set(CMAKE_CXX_FLAGS_DEBUG "-Wall -Wextra -g -O0 -fno-inline")
set(CMAKE_CXX_FLAGS_RELEASE "-Wall -Wextra") # -Werror")

option(BUILD_FOR_DESKTOP "BUILD_FOR_DESKTOP" OFF)
option(BUILD_RTMESSAGE_LIB "BUILD_RTMESSAGE_LIB" ON)
option(ENABLE_RTMESSAGE_PROFILE "ENABLE_RTMESSAGE_PROFILE" OFF)
option(BUILD_RTMESSAGE_SAMPLE_APP "BUILD_RTMESSAGE_SAMPLE_APP" OFF)
option(BUILD_RTMESSAGE_ROUTED "BUILD_RTMESSAGE_ROUTED" ON)
option(BUILD_DATAPROVIDER_LIB "BUILD_DATAPROVIDER_LIB" OFF)
option(BUILD_DMCLI "BUILD_DMCLI" ON)

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
  include_directories(${CMAKE_CURRENT_SOURCE_DIR} ${INCLUDE_DIR} ${INCLUDE_DIR}/cjson)
  link_directories(${CMAKE_CURRENT_SOURCE_DIR} ${LIBRARY_DIR})
endif (BUILD_FOR_DESKTOP)

add_definitions(-DRT_PLATFORM_LINUX)

set(RTMESSAGE_LINK_DIRECTORIES ${CMAKE_CURRENT_SOURCE_DIR} ${LIBRARY_DIR})

if (ENABLE_RTMESSAGE_PROFILE)
  message("Enabling rtMessage profile")
  set(CMAKE_CFLAGS "${CMAKE_C_FLAGS} -pg")
endif (ENABLE_RTMESSAGE_PROFILE)

if (BUILD_RTMESSAGE_LIB)
    message("Building rtMessage lib")
    add_library(
      rtMessage
      SHARED
      rtConnection.c
      rtLog.c
      rtError.c
      rtMessageHeader.c
      rtEncoder.c
      rtMessage.c
      rtSocket.c
      rtVector.c)
    add_dependencies(rtMessage cJSON)
    target_link_libraries(rtMessage ${LIBRARY_LINKER_OPTIONS} -pthread -lcjson)
endif (BUILD_RTMESSAGE_LIB)

if (BUILD_DATAPROVIDER_LIB)
  add_library(
    dataProvider
    SHARED
    dataProvider/dmProviderDatabase.cpp
    dataProvider/dmProviderHost.cpp
    dataProvider/dmPropertyInfo.cpp
    dataProvider/dmProviderInfo.cpp
    dataProvider/dmQueryResult.cpp
    dataProvider/dmValue.cpp)
  add_dependencies(
    dataProvider
    rtMessage)
  target_link_libraries(
    dataProvider
    rtMessage
    cjson)
endif (BUILD_DATAPROVIDER_LIB)

if (BUILD_RTMESSAGE_ROUTED)
    message ("Building rtrouted")
    set(CMAKE_CFLAGS " ${CMAKE_C_FLAGS}")
    add_executable(rtrouted rtrouted.c)
    if (BUILD_FOR_DESKTOP)
      add_dependencies(rtrouted cJSON)
    endif(BUILD_FOR_DESKTOP)
    target_link_libraries(rtrouted ${LIBRARY_LINKER_OPTIONS} rtMessage)
    target_compile_definitions(rtrouted PRIVATE RT_PLATFORM_LINUX)
endif (BUILD_RTMESSAGE_ROUTED)

if (BUILD_DMCLI)
  add_executable(dmcli dataProvider/dmcli.cpp)
  if (BUILD_FOR_DESKTOP)
    add_dependencies(dmcli cJSON rtMessage dataProvider)
  endif (BUILD_FOR_DESKTOP)
  target_link_libraries(dmcli rtMessage dataProvider cjson)
  add_definitions("-DDEFAULT_DATAMODELDIR=\"${CMAKE_CURRENT_SOURCE_DIR}/dataProvider/data\"")
endif (BUILD_DMCLI)

if (BUILD_RTMESSAGE_SAMPLE_APP)
    # sample_send
    add_executable(sample_send sample_send.c)
    if (BUILD_FOR_DESKTOP)
      add_dependencies(sample_send cJSON)
    endif (BUILD_FOR_DESKTOP)
    add_dependencies(sample_send rtMessage)
    target_link_libraries(sample_send ${LIBRARY_LINKER_OPTIONS} rtMessage)
    target_compile_definitions(sample_send PRIVATE RT_PLATFORM_LINUX)

    # sample_recv
    add_executable(sample_recv sample_recv.c)
    if (BUILD_FOR_DESKTOP)
      add_dependencies(sample_recv cJSON)
    endif (BUILD_FOR_DESKTOP)
    add_dependencies(sample_send rtMessage)
    target_link_libraries(sample_recv ${LIBRARY_LINKER_OPTIONS} rtMessage)
    target_compile_definitions(sample_recv PRIVATE RT_PLATFORM_LINUX)

    # sample_req
    add_executable(sample_req sample_req.c)
    if (BUILD_FOR_DESKTOP)
      add_dependencies(sample_req cJSON)
    endif (BUILD_FOR_DESKTOP)
    add_dependencies(sample_send rtMessage)
    target_link_libraries(sample_req ${LIBRARY_LINKER_OPTIONS} rtMessage)
    target_compile_definitions(sample_req PRIVATE RT_PLATFORM_LINUX)

    # sample_res
    add_executable(sample_res sample_res.c)
    if (BUILD_FOR_DESKTOP)
      add_dependencies(sample_res cJSON)
    endif (BUILD_FOR_DESKTOP)
    add_dependencies(sample_send rtMessage)
    target_link_libraries(sample_res ${LIBRARY_LINKER_OPTIONS} rtMessage)
    target_compile_definitions(sample_res PRIVATE RT_PLATFORM_LINUX)
endif (BUILD_RTMESSAGE_SAMPLE_APP)

install (TARGETS LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR})
install (TARGETS ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR})
