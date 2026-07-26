add_executable(chatter
    main.cpp
    ui/ASCIISplash.h ui/ASCIISplash.cpp
    ui/MainWindow.h ui/MainWindow.cpp
    ui/LoginDialog.h ui/LoginDialog.cpp
    ui/ChatWidget.h ui/ChatWidget.cpp
    ui/ContactList.h ui/ContactList.cpp
    ui/ServerBar.h ui/ServerBar.cpp
    ui/MemberList.h ui/MemberList.cpp
    network/ClientNetwork.h network/ClientNetwork.cpp
    network/P2PManager.h network/P2PManager.cpp
    crypto/ClientCrypto.h crypto/ClientCrypto.cpp
    store/MessageStore.h store/MessageStore.cpp
)

target_link_libraries(chatter
    PRIVATE
    common
    Qt6::Widgets
    Qt6::Network
    Qt6::Sql
    OpenSSL::Crypto
    Threads::Threads
)
