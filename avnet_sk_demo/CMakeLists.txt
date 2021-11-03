#  Copyright (c) Microsoft Corporation. All rights reserved.
#  Licensed under the MIT License.

cmake_minimum_required (VERSION 3.10)

project (avnet_sk_demo C)

# Check if there is a global azsphere_board.cmake file
get_filename_component(PARENT_DIR ${PROJECT_SOURCE_DIR} DIRECTORY)


include(azsphere_board_avnet.txt)


if (EXISTS "${PARENT_DIR}/tools/cmake/azsphere_config.cmake")
    include(../tools/cmake/azsphere_config.cmake)
    auto_generate_azsphere_config()
else()

    # For information on setting tools revision and target api set see 
    # https://docs.microsoft.com/en-us/azure-sphere/app-development/using-cmake-functions

    azsphere_configure_tools(TOOLS_REVISION "21.07")
    azsphere_configure_api(TARGET_API_SET "10")

endif()

add_subdirectory("AzureSphereDevX" out)

set_source_files_properties(lsm6dso_reg.c PROPERTIES COMPILE_FLAGS -Wno-conversion)
set_source_files_properties(lps22hh_reg.c PROPERTIES COMPILE_FLAGS -Wno-conversion)
set_source_files_properties(i2c.c PROPERTIES COMPILE_FLAGS -Wno-conversion)
set_source_files_properties(oled.c PROPERTIES COMPILE_FLAGS -Wno-conversion)
set_source_files_properties(sd1306.c PROPERTIES COMPILE_FLAGS -Wno-conversion)

# Create executable
add_executable (${PROJECT_NAME} main.c
                                lps22hh_reg.c 
                                lsm6dso_reg.c 
                                i2c.c 
                                oled.c
                                sd1306.c)

target_link_libraries (${PROJECT_NAME} applibs pthread gcc_s c azure_sphere_devx)
target_include_directories(${PROJECT_NAME} PUBLIC ../../../include )


set(BOARD_COUNTER 0)

if(AVNET)
    MATH(EXPR BOARD_COUNTER "${BOARD_COUNTER}+1")
    add_definitions( -DOEM_AVNET=TRUE )
    azsphere_target_hardware_definition(${PROJECT_NAME} TARGET_DIRECTORY "HardwareDefinitions/avnet_mt3620_sk" TARGET_DEFINITION "sample_appliance.json")
    message(STATUS "Azure Sphere board selected: AVNET REV 1")
endif(AVNET)

if(AVNET_REV_2)
    MATH(EXPR BOARD_COUNTER "${BOARD_COUNTER}+1")
    add_definitions( -DOEM_AVNET=TRUE )
    azsphere_target_hardware_definition(${PROJECT_NAME} TARGET_DIRECTORY "HardwareDefinitions/avnet_mt3620_sk_rev2" TARGET_DEFINITION "sample_appliance.json")
    message(STATUS "Azure Sphere board selected: AVNET REV 2")
endif(AVNET_REV_2)

if(BOARD_COUNTER EQUAL 0)
    message(FATAL_ERROR "No Azure Sphere boards selected. Ensure one board set")
endif()

if(BOARD_COUNTER GREATER 1)
    message(FATAL_ERROR "Multiple (${BOARD_COUNTER}) Azure Sphere boards selected. Ensure only one board set")
endif()

azsphere_target_add_image_package(${PROJECT_NAME})