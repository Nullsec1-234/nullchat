add_executable(chatter-server
    main.cpp
    Server.h Server.cpp
    Session.h Session.cpp
    Database.h Database.cpp
    GroupManager.h GroupManager.cpp
    P2PNegotiator.h P2PNegotiator.cpp
)

target_link_libraries(chatter-server
    PRIVATE
    common
    Qt6::Core
    Qt6::Network
    Qt6::Sql
    OpenSSL::SSL
    OpenSSL::Crypto
    Threads::Threads
)

target_compile_definitions(chatter-server PRIVATE SERVER_BUILD)
