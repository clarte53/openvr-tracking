cmake_minimum_required(VERSION 3.2)

project(OpenVR-Tracking VERSION 0.1.0 LANGUAGES CXX)

include_directories(
	${CMAKE_CURRENT_SOURCE_DIR}/src/
	${CMAKE_CURRENT_SOURCE_DIR}/openvr/headers/
	${CMAKE_CURRENT_SOURCE_DIR}/sockpp/include/
)

link_directories(
	${CMAKE_CURRENT_SOURCE_DIR}/openvr/lib/win64/
	${CMAKE_CURRENT_SOURCE_DIR}/sockpp/lib/
)

add_executable(openvr-tracking
	src/main.cpp
)

target_link_libraries(openvr-tracking
	openvr_api
	sockpp-static
	ws2_32
)

add_custom_command(TARGET openvr-tracking POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
    "${CMAKE_CURRENT_SOURCE_DIR}/openvr/bin/win64/openvr_api.dll"
    $<TARGET_FILE_DIR:openvr-tracking>
)
