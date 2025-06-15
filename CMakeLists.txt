cmake_minimum_required(VERSION 3.10)

project(CppRectangleRecognition)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find OpenMP for parallelization
find_package(OpenMP)
if(OpenMP_CXX_FOUND)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${OpenMP_CXX_FLAGS}")
endif()

include_directories(Include)

file(GLOB_RECURSE SOURCES "Source/*.cpp")
file(GLOB_RECURSE HEADERS "Include/*.h" "Include/*.hpp")

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