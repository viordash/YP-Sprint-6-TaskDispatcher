set(target dispatcher_test)

file(GLOB QUEUE_TEST_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/queue/*.cpp")

add_executable(${target}
    main.cpp
    task_dispatcher.cpp
    ${QUEUE_TEST_SOURCES}
)

target_link_libraries(${target}
    PRIVATE
        GTest::GTest
        GTest::Main
        task_dispatcher
)

add_test(NAME ${target} COMMAND ${target})
