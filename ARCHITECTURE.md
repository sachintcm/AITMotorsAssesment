# Architecture Document: TCP File Transfer Utility

## Overview

This document describes the architecture of the TCP-based file transfer utility implemented in C, consisting of `client.c` and `server.c`. The system enables secure, large-file transfers (up to 16GB) between networked computers with SHA256 integrity verification.

## System Architecture

### High-Level Components

1. **Client (`client.c`)**: Initiates file transfers by connecting to the server, sending file metadata, and streaming file data in chunks.
2. **Server (`server.c`)**: Listens for incoming connections, receives file data, and verifies integrity upon completion.
3. **Common Library (`common.c`, `common.h`)**: Shared utilities for networking, hashing, and protocol handling.

### Protocol Overview

The system uses a custom binary protocol with three commands:
- **START**: Initiates transfer with file metadata
- **DATA**: Streams file content in 1MB chunks
- **END**: Completes transfer with integrity hash

## Client Architecture (`client.c`)

### Main Components

#### 1. File Validation and Preparation
```c
int send_file(const char *hostname, int port, const char *filename)
```
- Validates file existence and type
- Checks file size (max 16GB limit)
- Calculates SHA256 hash before transfer
- Establishes TCP connection to server

#### 2. Connection Establishment
- Uses `client_connect()` from common.c
- Resolves hostname to IP address
- Creates TCP socket and connects

#### 3. Transfer Protocol Sequence

**Phase 1: START Command**
- Sends `FileTransferHeader` with:
  - Magic number (0xDEADBEEF) for protocol validation
  - Protocol version (1)
  - Command type (CMD_START)
  - File size and chunk size
- Sends filename (basename only)

**Phase 2: Data Transfer**
- Opens file in binary read mode
- Streams data in 1MB chunks using `BUFFER_SIZE`
- Displays real-time progress indicator
- Handles partial sends and network errors

**Phase 3: END Command**
- Sends final header with CMD_END
- Includes pre-calculated SHA256 hash
- Closes connection

### Key Design Decisions

- **Pre-calculation of Hash**: SHA256 is computed before sending to avoid storing entire file in memory
- **Chunked Transfer**: 1MB chunks prevent memory exhaustion for large files
- **Progress Feedback**: Real-time percentage display for user experience
- **Error Recovery**: Immediate cleanup on network failures

## Server Architecture (`server.c`)

### Main Components

#### 1. Connection Listening
```c
int receive_file(int port, const char *save_path)
```
- Creates listening socket using `server_listen()`
- Accepts single client connection
- Validates save directory existence

#### 2. Protocol Validation
- Receives START header and validates:
  - Magic number (0xDEADBEEF)
  - Command type (CMD_START)
  - Protocol version compatibility

#### 3. File Reception and Hashing

**Phase 1: Metadata Reception**
- Receives filename
- Creates output file in save directory
- Initializes incremental SHA256 context

**Phase 2: Data Reception**
- Receives data in chunks matching client's BUFFER_SIZE
- Writes to file immediately
- Updates SHA256 context incrementally
- Displays progress indicator

**Phase 3: Integrity Verification**
- Receives END command with client's hash
- Finalizes server-calculated hash
- Compares hashes for integrity verification
- Deletes file on hash mismatch

### Key Design Decisions

- **Incremental Hashing**: SHA256 calculated during reception to handle large files without full memory load
- **Atomic Operations**: File only kept if integrity check passes
- **Single Connection**: Server handles one transfer at a time
- **Strict Validation**: Protocol magic number prevents misinterpretation

## Protocol Details

### FileTransferHeader Structure
```c
typedef struct {
    uint32_t magic;      // 0xDEADBEEF
    uint8_t version;     // Protocol version
    uint8_t command;     // CMD_START, CMD_DATA, CMD_END
    uint16_t reserved;   // Future use
    uint64_t file_size;  // Total bytes
    uint32_t chunk_size; // Buffer size (1MB)
    uint8_t hash[32];    // SHA256 (only in END)
} FileTransferHeader;
```

### Transfer Sequence

1. **Client → Server**: START header + filename
2. **Client → Server**: File data in chunks
3. **Client → Server**: END header with hash
4. **Server**: Hash verification and response

## Data Flow

### Client Data Flow
```
File Selection → Hash Calculation → Connection → START → Filename → Data Chunks → END → Disconnect
```

### Server Data Flow
```
Listen → Accept → Receive START → Receive Filename → Create File → Receive Data + Hash Update → Receive END → Hash Compare → Save/Delete → Disconnect
```

## Error Handling

### Error Codes
- `ERR_OK` (0): Success
- `ERR_INVALID_MAGIC` (1): Protocol mismatch
- `ERR_FILE_NOT_FOUND` (2): File access issues
- `ERR_HASH_MISMATCH` (3): Integrity failure
- `ERR_NETWORK` (4): Connection problems
- `ERR_MEMORY` (5): Allocation failures

### Recovery Mechanisms
- **Network Errors**: Immediate connection closure and cleanup
- **Hash Mismatch**: Automatic file deletion to prevent corrupted data
- **File Errors**: Validation before transfer initiation
- **Protocol Errors**: Strict header validation with magic number checks

## Performance Considerations

### Memory Efficiency
- 1MB buffer size prevents excessive memory usage
- Incremental hashing avoids loading entire file
- No file buffering in memory during transfer

### Network Optimization
- TCP streaming for reliable delivery
- SO_REUSEADDR prevents TIME_WAIT delays
- MSG_WAITALL ensures complete header reception

### Scalability Limits
- Maximum file size: 16GB (uint64_t limit)
- Single-threaded design (one transfer at a time)
- Fixed buffer size for predictable memory usage

## Security Features

### Integrity Verification
- SHA256 hashing ensures data integrity
- Hash calculated and verified across network
- Automatic deletion of corrupted files

### Protocol Validation
- Magic number prevents protocol confusion
- Version checking for compatibility
- Reserved fields for future extensions

## Build and Deployment

### Dependencies
- OpenSSL (libssl, libcrypto) for SHA256
- POSIX sockets (Linux/Unix networking)
- GCC with C99 standard support

### Compilation
```bash
make        # Build client and server binaries
make test   # Generate test files
make clean  # Remove build artifacts
```

### Usage Examples
```bash
# Server
./bin/server ./downloads 9999

# Client
./bin/client localhost ./largefile.iso 9999
```

## Future Enhancements

### Potential Extensions
- Multi-threaded transfers for parallel chunks
- Compression support
- Authentication/encryption
- Resume capability for interrupted transfers
- Multiple simultaneous connections
- Configuration file support

This architecture provides a robust, efficient foundation for secure file transfers with strong integrity guarantees.