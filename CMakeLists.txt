cmake_minimum_required(VERSION 2.8)

project(final)

#if (NOT CMAKE_RUNTIME_OUTPUT_DIRECTORY)
#	set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/final)
#endif()

#find_library(PTHREAD_LIBRARY pthread)
set(MODULES main.cpp queryparser.cpp prepresponse.cpp)
set(HEADERS queryparser.h prepresponse.h query.h )

add_executable(${PROJECT_NAME} ${MODULES} ${HEADERS})
#add_executable(${PROJECT_NAME} main.cpp queryparser.cpp queryparser.h prepresponse.cpp prepresponse.h query.h )
#set(EXECUTABLE_OUTPUT_PATH "./final")

#target_link_libraries(${PTHREAD_LIBRARY})
