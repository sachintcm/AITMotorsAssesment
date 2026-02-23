# AITMotorsAssesment
A cross-platform file transfer utility

## Overview

This is a TCP-based file transfer utility in C that enables secure, large-file transfers (up to 16GB) between networked computers with SHA256 integrity verification. It consists of a client and server component that communicate over TCP using a custom binary protocol.

## Features

- **Large File Support**: Handles files up to 16GB in size
- **Integrity Verification**: SHA256 hashing ensures data integrity
- **Progress Tracking**: Real-time transfer progress display
- **Error Recovery**: Automatic cleanup on transfer failures
- **Cross-Platform**: Works on Linux, macOS, and other POSIX systems

## Dependencies

- GCC compiler (C99 standard)
- OpenSSL development libraries
- POSIX sockets (standard on Linux/macOS)

## Compilation

### Prerequisites

Ensure you have the required dependencies installed:

**Ubuntu/Debian:**
```bash
sudo apt-get update
sudo apt-get install build-essential libssl-dev
```

**macOS (with Homebrew):**
```bash
brew install openssl
export LDFLAGS="-L/usr/local/opt/openssl/lib"
export CPPFLAGS="-I/usr/local/opt/openssl/include"
```

**Fedora/CentOS:**
```bash
sudo dnf install gcc openssl-devel
```

### Build Instructions

1. Clone or download the repository
2. Navigate to the project directory
3. Run make to build the binaries:

```bash
make
```

This will create the following files:
- `bin/client` - The file transfer client
- `bin/server` - The file transfer server

### Clean Build

To remove all build artifacts:

```bash
make clean
```

## Usage

### Command Line Syntax

#### Server
```bash
./bin/server <save_directory> [port]
```

- `save_directory`: Directory where received files will be saved
- `port`: Optional port number (default: 9999)

#### Client
```bash
./bin/client <server_hostname> <file_path> [port]
```

- `server_hostname`: IP address or hostname of the server
- `port`: Optional port number (default: 9999)

### Testing on the Same System

The Makefile includes a test target that creates a 1GB test file for testing:

1. Build and create test files:
```bash
make test
```

This creates:
- `test_data/testfile_1gb.bin` - A 1GB test file
- `test_receive/` - Directory for received files

2. Start the server in one terminal:
```bash
./bin/server ./test_receive 9999
```

3. In another terminal, start the client:
```bash
./bin/client localhost ./test_data/testfile_1gb.bin 9999
```

4. Monitor the progress in both terminals. The transfer should complete successfully with hash verification.

### Running Client and Server on Different Systems

#### Setup

1. **On the receiving system (Server):**
   - Ensure the save directory exists and is writable
   - Start the server:
   ```bash
   ./bin/server /path/to/save/directory 9999
   ```
   - Note the server's IP address (use `ip addr show` or `ifconfig`)

2. **On the sending system (Client):**
   - Ensure the file to transfer exists and is readable
   - Start the client, replacing `server_ip` with the actual server IP:
   ```bash
   ./bin/client server_ip /path/to/file/to/send 9999
   ```

#### Example Scenario

**Server Machine (192.168.1.100):**
```bash
mkdir ~/received_files
./bin/server ~/received_files 9999
```

**Client Machine:**
```bash
./bin/client 192.168.1.100 ./my_large_file.iso 9999
```

#### Network Considerations

- Ensure both systems are on the same network
- The server port (9999) must be open in the server's firewall
- For large files, ensure sufficient disk space on both systems
- Network speed will affect transfer time

### Troubleshooting

#### Common Issues

1. **Connection refused:**
   - Check if server is running
   - Verify IP address and port
   - Check firewall settings

2. **Hash mismatch:**
   - Indicates data corruption during transfer
   - Check network stability
   - Retry the transfer

3. **Permission denied:**
   - Ensure write permissions in save directory
   - Check file permissions on source file

#### Debug Information

Both client and server display progress information including:
- Transfer percentage
- Bytes transferred
- Estimated time remaining
- Final hash verification status

## Architecture

- **Client**: Calculates SHA256 hash, sends file in 1MB chunks
- **Server**: Receives chunks, verifies hash on completion
- **Protocol**: Custom binary protocol with magic number validation
- **Security**: SHA256 integrity checking prevents silent corruption

## Limitations

- Maximum file size: 16GB (uint64_t limit)
- Single client per server instance
- No authentication or encryption beyond integrity checking
- Requires POSIX-compliant system


