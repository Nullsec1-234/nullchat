add_executable(chatter-tests
    test_protocol.cpp
    test_crypto.cpp
)

target_link_libraries(chatter-tests PRIVATE common GTest::gtest_main)

add_test(NAME chatter-tests COMMAND chatter-tests)
