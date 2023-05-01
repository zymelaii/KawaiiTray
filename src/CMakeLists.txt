aux_source_directory(. SOURCE_FILES)
set(RC_FILES ${CMAKE_SOURCE_DIR}/assets/app.rc)

add_executable(
    ${CMAKE_PROJECT_NAME}
    ${SOURCE_FILES}
    ${RC_FILES}
)

target_link_libraries(
    ${CMAKE_PROJECT_NAME}
    PRIVATE yaml-cpp::yaml-cpp
)

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
    DIRECTORY ${CMAKE_SOURCE_DIR}/assets/app/assets
    DESTINATION ${CMAKE_INSTALL_PREFIX}
)
