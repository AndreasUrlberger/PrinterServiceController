set(CMAKE_TOOLCHAIN_FILE /home/andreas/Documents/PrinterInterface/PrinterServiceController/Toolchain-RaspberryPi.cmake)
cmake_minimum_required(VERSION 3.0.0)

set(CMAKE_C_STANDARD 20)
set(CMAKE_CXX_STANDARD 20)
set(TARGET PrinterServiceController)

project(PrinterServiceController VERSION 0.1.0)

include(CTest)
enable_testing()


# Find Protobuf.
find_package(Protobuf REQUIRED)

# Find all proto files and configure directories.
file(GLOB PROTO_FILES CONFIGURE_DEPENDS "proto/*.proto")
FILE(TO_NATIVE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/proto PROTOMODEL_PATH)
FILE(TO_NATIVE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/src/proto PROTOBINDING_PATH)

# Compile the proto files.
FOREACH(proto ${PROTO_FILES})
    EXECUTE_PROCESS(COMMAND ${PROTOBUF_PROTOC_EXECUTABLE} --proto_path=${PROTOMODEL_PATH} --cpp_out=${PROTOBINDING_PATH} ${proto}
    RESULT_VARIABLE rv)
    # Optional, but that can show the user if something have gone wrong with the proto generation.
    IF(${rv})
        MESSAGE("Generation of data model returned ${rv} for proto ${proto}")
    ENDIF()
ENDFOREACH(proto)

# Move generated proto headers in header directory.
file(GLOB GENERATED_HEADERS CONFIGURE_DEPENDS "${PROTOBINDING_PATH}/*.h")
EXECUTE_PROCESS(COMMAND mv ${GENERATED_HEADERS} ${CMAKE_CURRENT_SOURCE_DIR}/include/proto/)
# Find all generated proto headers and sources to add them to the target.
file(GLOB PROTO_HEADERS CONFIGURE_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/include/proto/)
file(GLOB PROTO_SOURCES CONFIGURE_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/src/proto/)


include_directories(LibIncludes)
include_directories(include)

add_subdirectory(src)


add_executable(${TARGET})
target_sources(${TARGET} PRIVATE 
    main.cpp

    ./src/ButtonController.cpp
    ./src/Buzzer.cpp
    ./src/DisplayController.cpp
    ./src/FanController.cpp
    ./src/Logger.cpp
    ./src/PowerButtonController.cpp
    ./src/PrintConfigs.cpp
    ./src/PrinterServer.cpp
    ./src/ServiceController.cpp
    ./src/Utils.cpp
    ./src/ProtoMessageHandler.cpp

    ./include/ButtonController.h
    ./include/Buzzer.h
    ./include/DisplayController.h
    ./include/FanController.h
    ./include/Logger.h
    ./include/PowerButtonController.h
    ./include/PrintConfigs.h
    ./include/PrinterServer.h
    ./include/ServiceController.h
    ./include/Utils.h
    ./include/ProtoMessageHandler.h

    # Include generated proto files
    ${PROTO_HEADERS}
    ${PROTO_SOURCES}
)




# Link against the Protobuf library
target_link_libraries(${TARGET} ${PROTOBUF_LIBRARIES})



link_directories(${CMAKE_SOURCE_DIR}/Libraries)

add_library(wiringPi SHARED IMPORTED)
add_library(ssd1306 SHARED IMPORTED)

set_target_properties(wiringPi PROPERTIES IMPORTED_LOCATION ${CMAKE_SOURCE_DIR}/Libraries/libwiringPi.so)
set_target_properties(ssd1306 PROPERTIES IMPORTED_LOCATION ${CMAKE_SOURCE_DIR}/Libraries/libssd1306_i2c.so)

target_link_libraries(${TARGET} wiringPi ssd1306 pthread)

add_definitions(-Wall -g)

set(CPACK_PROJECT_NAME ${PROJECT_NAME})
set(CPACK_PROJECT_VERSION ${PROJECT_VERSION})
include(CPack)
