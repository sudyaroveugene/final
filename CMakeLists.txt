cmake_minimum_required(VERSION 2.8)

project(final)

#if (NOT CMAKE_RUNTIME_OUTPUT_DIRECTORY)
#	set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/final)
#endif()

#find_library(PTHREAD_LIBRARY pthread)

add_executable(${PROJECT_NAME} main.cpp requestparser.cpp )
set(EXECUTABLE_OUTPUT_PATH "./final")

#target_link_libraries(${PTHREAD_LIBRARY})
