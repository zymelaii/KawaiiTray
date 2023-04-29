aux_source_directory(. SOURCE_FILES)

add_executable(${CMAKE_PROJECT_NAME} ${SOURCE_FILES})

if(CMAKE_BUILD_TYPE STREQUAL Release)
    set_target_properties(
        ${CMAKE_PROJECT_NAME}
        PROPERTIES WIN32_EXECUTABLE ON
    )
endif()

include(GNUInstallDirs)

install(
    TARGETS ${CMAKE_PROJECT_NAME}
    RUNTIME DESTINATION ${CMAKE_INSTALL_PREFIX}
)

install(
    DIRECTORY ${CMAKE_SOURCE_DIR}/assets
    DESTINATION ${CMAKE_INSTALL_PREFIX}
)
