aux_source_directory(. SOURCE_FILES)

add_executable(${CMAKE_PROJECT_NAME} ${SOURCE_FILES})

if(CMAKE_BUILD_TYPE STREQUAL Release)
    set_target_properties(
        ${CMAKE_PROJECT_NAME}
        PROPERTIES WIN32_EXECUTABLE ON
    )
endif()
