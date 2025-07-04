cmake_minimum_required(VERSION 3.10)

project(CppRectangleRecognition)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Force Release mode for fastest execution
set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)

set(CMAKE_CXX_FLAGS_RELEASE "-O3 -DNDEBUG -march=native -mtune=native -flto -ffast-math -s")
set(CMAKE_CXX_FLAGS_DEBUG "-O0 -g -Wall -Wextra")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pipe")

# Set output directories for all binaries
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/Output)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG ${CMAKE_SOURCE_DIR}/Output)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELEASE ${CMAKE_SOURCE_DIR}/Output)

# Find OpenMP for parallelization
find_package(OpenMP)
if(OpenMP_CXX_FOUND)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${OpenMP_CXX_FLAGS}")
    message(STATUS "OpenMP found - enabling parallel processing")
else()
    message(WARNING "OpenMP not found - performance may be reduced")
endif()

include_directories(Include)

file(GLOB_RECURSE SOURCES "Source/*.cpp")
file(GLOB_RECURSE HEADERS "Include/*.h" "Include/*.hpp")

# Remove VisualTest.cpp from main executable sources
list(REMOVE_ITEM SOURCES "${CMAKE_SOURCE_DIR}/Source/VisualTest.cpp")

add_executable(${PROJECT_NAME} ${SOURCES} ${HEADERS})

# Link OpenMP if found
if(OpenMP_CXX_FOUND)
    target_link_libraries(${PROJECT_NAME} OpenMP::OpenMP_CXX)
endif()

# GTest configuration
if(EXISTS "${CMAKE_SOURCE_DIR}/Test")
    enable_testing()
    
    # Find GTest
    find_package(GTest QUIET)
    
    if(NOT GTest_FOUND)
        # Download and build GTest
        include(FetchContent)
        FetchContent_Declare(
            googletest
            URL https://github.com/google/googletest/archive/03597a01ee50f33f9142a332f1e5e4bd3d71e787.zip
        )
        # For Windows: Prevent overriding the parent project's compiler/linker settings
        set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
        FetchContent_MakeAvailable(googletest)
    endif()
    
    # Create test executable
    file(GLOB_RECURSE TEST_SOURCES "Test/*.cpp")
    # Exclude TestPerformance.cpp from the test suite as it has its own main
    list(REMOVE_ITEM TEST_SOURCES "${CMAKE_SOURCE_DIR}/Test/TestPerformance.cpp")
    if(TEST_SOURCES)
        # Remove main.cpp from library sources for tests
        set(LIB_SOURCES ${SOURCES})
        list(REMOVE_ITEM LIB_SOURCES "${CMAKE_SOURCE_DIR}/Source/Main.cpp")
        
        add_executable(tests ${TEST_SOURCES} ${LIB_SOURCES})
        
        if(GTest_FOUND)
            target_link_libraries(tests GTest::gtest GTest::gtest_main)
        else()
            target_link_libraries(tests gtest gtest_main)
        endif()
        
        # Link OpenMP to tests if found
        if(OpenMP_CXX_FOUND)
            target_link_libraries(tests OpenMP::OpenMP_CXX)
        endif()
        
        include(GoogleTest)
        gtest_discover_tests(tests)
    endif()
endif()

# Performance test executable
if(EXISTS "${CMAKE_SOURCE_DIR}/Test/TestPerformance.cpp")
    set(PERF_SOURCES ${SOURCES})
    list(REMOVE_ITEM PERF_SOURCES "${CMAKE_SOURCE_DIR}/Source/Main.cpp")
    
    add_executable(TestPerformance Test/TestPerformance.cpp ${PERF_SOURCES})
    
    # Link OpenMP if found
    if(OpenMP_CXX_FOUND)
        target_link_libraries(TestPerformance OpenMP::OpenMP_CXX)
    endif()
endif()

# Visual test executable
if(EXISTS "${CMAKE_SOURCE_DIR}/Source/VisualTest.cpp")
    # Get all sources except Main.cpp and VisualTest.cpp
    file(GLOB_RECURSE ALL_SOURCES "Source/*.cpp")
    list(REMOVE_ITEM ALL_SOURCES "${CMAKE_SOURCE_DIR}/Source/Main.cpp")
    list(REMOVE_ITEM ALL_SOURCES "${CMAKE_SOURCE_DIR}/Source/VisualTest.cpp")
    
    add_executable(VisualTest Source/VisualTest.cpp ${ALL_SOURCES})
    
    # Link OpenMP if found
    if(OpenMP_CXX_FOUND)
        target_link_libraries(VisualTest OpenMP::OpenMP_CXX)
    endif()
endif()