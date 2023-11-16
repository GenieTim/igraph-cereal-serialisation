cmake_minimum_required(VERSION 3.15.0)
project(igraph_cereal VERSION "0.1.0")

include(${CMAKE_CURRENT_SOURCE_DIR}/vendor/CMakeLists.txt)

# specify the C++ standard
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED True)
set(CMAKE_BUILD_TYPE Debug)

# set(CMAKE_FIND_DEBUG_MODE ON)

if(CMAKE_COMPILER_IS_GNUCXX)
set(CMAKE_CXX_FLAGS "-Wall -Wextra")
set(CMAKE_CXX_FLAGS_DEBUG "-g")
# memory leak analysis:
set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fno-omit-frame-pointer -fsanitize=address -fsanitize=leak")
set (CMAKE_LINKER_FLAGS "${CMAKE_LINKER_FLAGS} -fno-omit-frame-pointer -fsanitize=address -fsanitize=leak")
set(CMAKE_CXX_FLAGS_RELEASE "-O3 -Ofast")
endif()

include_directories(${igraph_INCLUDE_DIRS})
include_directories(${cereal_INCLUDE_DIRS})

add_executable(main ${CMAKE_CURRENT_SOURCE_DIR}/main.cpp)

add_dependencies(main igraph)
add_dependencies(main cerealLib)

target_link_libraries(main 
	PRIVATE 
	${igraph_LIBRARIES}
	-fsanitize=address
)
