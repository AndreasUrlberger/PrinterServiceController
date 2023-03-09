set(CMAKE_TOOLCHAIN_FILE /home/andreas/Documents/PrinterInterface/PrinterServiceController/Toolchain-RaspberryPi.cmake)
set(CMAKE_C_STANDARD 20)
set(CMAKE_CXX_STANDARD 20)
set(TARGET PrinterServiceController)

cmake_minimum_required(VERSION 3.0.0)
project(PrinterServiceController VERSION 0.1.0)

include(CTest)
enable_testing()

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
)

link_directories(${CMAKE_SOURCE_DIR}/Libraries)

add_library(wiringPi SHARED IMPORTED)
add_library(ssd1306 SHARED IMPORTED)

set_target_properties(wiringPi PROPERTIES IMPORTED_LOCATION ${CMAKE_SOURCE_DIR}/Libraries/libwiringPi.so)
set_target_properties(ssd1306 PROPERTIES IMPORTED_LOCATION ${CMAKE_SOURCE_DIR}/Libraries/libssd1306_i2c.so)

target_link_libraries(${TARGET} wiringPi ssd1306 pthread)

add_definitions(-Wall)

set(CPACK_PROJECT_NAME ${PROJECT_NAME})
set(CPACK_PROJECT_VERSION ${PROJECT_VERSION})
include(CPack)
