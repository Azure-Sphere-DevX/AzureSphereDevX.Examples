#  Copyright (c) Microsoft Corporation. All rights reserved.
#  Licensed under the MIT License.

###################################################################################################################

# Select your developer board by removing the # tag from the beginning of the line

# The default board selected is the AVNET Azure Sphere Starter Kit Revision 1.
# If you are NOT using the AVNET Revision 1 board be sure to comment out the AVNET board
# If you select the incorrect board, the RelayClick functionality and app Led will not work correctly

set(AVNET TRUE "AVNET Azure Sphere Starter Kit Revision 1 ")    
# set(AVNET_REV_2 TRUE "AVNET Azure Sphere Starter Kit Revision 2 ")             

###################################################################################################################

cmake_minimum_required (VERSION 3.10)

project (azure_end_to_end C)

azsphere_configure_tools(TOOLS_REVISION "21.07")
azsphere_configure_api(TARGET_API_SET "10")

add_subdirectory("../../../" out)

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


if(AVNET)
    add_definitions( -DOEM_AVNET=TRUE )
    azsphere_target_hardware_definition(${PROJECT_NAME} TARGET_DIRECTORY "../../HardwareDefinitions/avnet_mt3620_sk" TARGET_DEFINITION "sample_appliance.json")
endif(AVNET)

if(AVNET_REV_2)
    add_definitions( -DOEM_AVNET=TRUE )
    azsphere_target_hardware_definition(${PROJECT_NAME} TARGET_DIRECTORY "../../HardwareDefinitions/avnet_mt3620_sk_rev2" TARGET_DEFINITION "sample_appliance.json")
endif(AVNET_REV_2)

azsphere_target_add_image_package(${PROJECT_NAME})