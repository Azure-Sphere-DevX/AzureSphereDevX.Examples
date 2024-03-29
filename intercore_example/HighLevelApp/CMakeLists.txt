#  Copyright (c) Microsoft Corporation. All rights reserved.
#  Licensed under the MIT License.

cmake_minimum_required (VERSION 3.10)
project (AzureSphereAzureIoT C)

# Check if there is a global azsphere_board.cmake file

get_filename_component(PARENT_DIR ${PROJECT_SOURCE_DIR} DIRECTORY)

if (EXISTS "${PARENT_DIR}/azsphere_board.global.txt")
    include("${PARENT_DIR}/azsphere_board.global.txt")
else()
    include(azsphere_board.txt)
endif()

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

set(Source
    "main.c"
)
source_group("Source" FILES ${Source})

if(AVNET OR AVNET_REV_2)

    set(Oem

        #"../LearningPathLibrary/AVNET/board.c"
    )
    source_group("Oem" FILES ${Oem})    

    add_definitions( -DOEM_AVNET=TRUE )

endif(AVNET OR AVNET_REV_2)

if(SEEED_STUDIO_RDB OR SEEED_STUDIO_MINI)

    set(Oem
        #"../LearningPathLibrary/SEEED_STUDIO/board.c"
    )
    source_group("Oem" FILES ${Oem})

    add_definitions( -DOEM_SEEED_STUDIO=TRUE )

endif(SEEED_STUDIO_RDB OR SEEED_STUDIO_MINI)

set(ALL_FILES
    ${Source}
    ${Oem}
)

# Create executable
add_executable(${PROJECT_NAME} ${ALL_FILES})

target_compile_definitions(${PROJECT_NAME} PUBLIC AZURE_IOT_HUB_CONFIGURED)
target_link_libraries(${PROJECT_NAME} applibs pthread gcc_s c azure_sphere_devx )

target_include_directories(${PROJECT_NAME} PUBLIC AzureSphereDevX/include )

target_compile_options(${PROJECT_NAME} PRIVATE -Wno-unknown-pragmas)

if(AVNET)
    azsphere_target_hardware_definition(${PROJECT_NAME} TARGET_DIRECTORY "HardwareDefinitions/avnet_mt3620_sk" TARGET_DEFINITION "azure_sphere_learning_path.json")
endif(AVNET)

if(AVNET_REV_2)
    azsphere_target_hardware_definition(${PROJECT_NAME} TARGET_DIRECTORY "HardwareDefinitions/avnet_mt3620_sk_rev2" TARGET_DEFINITION "azure_sphere_learning_path.json")
endif(AVNET_REV_2)

if (SEEED_STUDIO_RDB)
    azsphere_target_hardware_definition(${PROJECT_NAME} TARGET_DIRECTORY "HardwareDefinitions/mt3620_rdb" TARGET_DEFINITION "azure_sphere_learning_path.json")
endif()

if (SEEED_STUDIO_MINI)
    azsphere_target_hardware_definition(${PROJECT_NAME} TARGET_DIRECTORY "HardwareDefinitions/seeed_mt3620_mdb" TARGET_DEFINITION "azure_sphere_learning_path.json")
endif(SEEED_STUDIO_MINI)

azsphere_target_add_image_package(${PROJECT_NAME})