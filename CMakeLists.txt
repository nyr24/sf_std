cmake_minimum_required(VERSION 3.25)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(SRC_DIR src)
set(BUILD_DIR build)
set(INCLUDE_DIR include)

file(GLOB_RECURSE SRCS CONFIGURE_DEPENDS ${SRC_DIR}/*.cpp)
file(GLOB_RECURSE HEADERS CONFIGURE_DEPENDS ${INCLUDE_DIR}/*.hpp)

if (DEFINED SF_TESTS)
  project(sf_std_test VERSION 1.0.0 LANGUAGES CXX)
  add_executable(${PROJECT_NAME} ${SRCS} ${HEADERS})
  target_compile_definitions(${PROJECT_NAME} PUBLIC SF_TESTS)
else()
  project(sf_std VERSION 1.0.0 LANGUAGES CXX)
  add_library(${PROJECT_NAME} SHARED ${SRCS} ${HEADERS})
endif()

target_compile_options(
  ${PROJECT_NAME}
  PUBLIC
  $<$<CONFIG:Debug>:
    -g
    -O0
    -DSF_DEBUG=1
  >
  $<$<CONFIG:Release>:
    -O3
    -DSF_RELEASE=1
  >
)

if (DEFINED SF_BUILD_WAYLAND)
  target_include_directories(${PROJECT_NAME} PRIVATE ${SRC_DIR} ${INCLUDE_DIR})
  target_compile_definitions(${PROJECT_NAME} PUBLIC SF_PLATFORM_WAYLAND)
elseif (DEFINED SF_BUILD_X11)
  target_include_directories(${PROJECT_NAME} PRIVATE ${SRC_DIR} ${INCLUDE_DIR})
  target_compile_definitions(${PROJECT_NAME} PUBLIC SF_PLATFORM_X11)
else ()
  target_include_directories(${PROJECT_NAME} PRIVATE ${SRC_DIR} ${INCLUDE_DIR})
  target_compile_definitions(${PROJECT_NAME} PUBLIC SF_PLATFORM_WINDOWS)
endif()

