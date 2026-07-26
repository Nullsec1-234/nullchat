add_library(common
    Constants.h
    Protocol.h Protocol.cpp
    Message.h Message.cpp
    Crypto.h Crypto.cpp
)

target_include_directories(common PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(common PUBLIC Qt6::Core OpenSSL::Crypto OpenSSL::SSL Threads::Threads)
target_compile_features(common PUBLIC cxx_std_20)
