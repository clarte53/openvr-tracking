cmake_minimum_required(VERSION 3.2)

project(OpenVR-Tracking VERSION 0.1.0 LANGUAGES CXX)

include_directories(
	${CMAKE_CURRENT_SOURCE_DIR}/src/
	${CMAKE_CURRENT_SOURCE_DIR}/openvr/headers/
)

link_directories(
	${CMAKE_CURRENT_SOURCE_DIR}/openvr/lib/win64/
)

add_executable(openvr-tracking
	src/main.cpp
)

target_link_libraries(openvr-tracking
	openvr_api
)
