# Nullchat

Encrypted terminal-style chat application with a Discord-like UI.

- **Client**: C++20 with Qt6 (Widgets, Network, Sql)
- **Server**: C++20 with Qt6, or Node.js
- **E2EE**: ECDH P-256 + AES-256-GCM for DM encryption
- **Auth**: Login/register with invite password
- **Look**: Terminal aesthetic — green-on-black, JetBrains Mono, 4-panel layout

## Building

```bash
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j$(nproc)
```

### Dependencies

- Qt6 (Widgets, Network, Sql)
- OpenSSL 3.x
- CMake 3.22+

## Running

```bash
./build/src/client/chatter    # Client
./build/src/server/chatter-server  # C++ server
node src/server/node/server.js     # Node.js server
```

## Packaging

```bash
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j$(nproc)
cpack --config build/CPackConfig.cmake
```

On Linux this produces a `.deb`; on Windows (with NSIS) an `.exe` installer.
