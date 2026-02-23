# Copilot Instructions for AITMotorsAssesment File Transfer Utility

## Project Overview

This is a **TCP-based file transfer utility** in C that enables secure, large-file transfers (up to 16GB) between networked computers with SHA256 integrity verification.

### Architecture
- **Client** (`src/client.c`): Sends files with pre-calculated SHA256 hash
- **Server** (`src/server.c`): Receives files and verifies integrity during transfer
- **Common Functions** (`src/common.c`): Network I/O, hashing, and shared utilities
- **Protocol Header** (`src/common.h`): Binary protocol with magic number validation

## Critical Patterns & Conventions

### 1. Large File Handling (16GB Support)
- **Buffer size**: Fixed 1MB chunks (`BUFFER_SIZE`) for memory efficiency
- **Data types**: Use `uint64_t` for file sizes to prevent overflow
- **Transfer verification**: Hash calculated incrementally during send/receive, not after
- **Key point**: Files streamed in chunks to avoid loading entire file in memory

### 2. Protocol Structure
Every transfer follows this sequence:
1. **START command** (header only) - metadata: file size, chunk size
2. **Filename** - sent separately after START
3. **DATA chunks** - file content in BUFFER_SIZE pieces
4. **END command** - includes pre-calculated SHA256 hash for verification

```c
/* Protocol magic number: 0xDEADBEEF */
/* Commands: CMD_START (0), CMD_DATA (1), CMD_END (2) */
```

### 3. Integrity Checking
- SHA256 is calculated **before sending** (client) and **during reception** (server)
- Hash comparison happens after all data received to detect corruption
- Mismatch automatically deletes received file to prevent incomplete data lingering
- Implementation in `calculate_sha256()` and incremental updates in transfer functions

### 4. Network Patterns
- **Socket options**: `SO_REUSEADDR` prevents TIME_WAIT delays on server restart
- **Connection flow**: Client initiates → Server accepts → Header validation → Data transfer
- **Error handling**: Graceful socket closure and resource cleanup on all error paths
- **MSG_WAITALL flag**: Ensures all bytes received before proceeding (line 100 in server.c)

### 5. Error Recovery
- File deleted on hash mismatch (prevents incomplete uploads)
- Network errors during transfer clear resources and return error code
- Port configurable at runtime (default 9999)
- Magic number validation prevents protocol mismatches

## Build & Test Workflow

```bash
make              # Build client and server binaries
make test         # Generate 1GB test file
make clean        # Remove build artifacts
```

### Testing Large Transfers
```bash
# Terminal 1 (Server)
./bin/server ./test_receive 9999

# Terminal 2 (Client)
./bin/client localhost ./test_data/testfile_1gb.bin 9999
```

## Key Files & Their Responsibilities

| File | Purpose |
|------|---------|
| `src/common.h` | Protocol definitions, error codes, function declarations |
| `src/common.c` | SHA256 hashing, socket creation, host/port handling |
| `src/client.c` | File reading, chunk transmission, hash pre-calculation |
| `src/server.c` | Connection acceptance, incremental hash verification, file writing |
| `Makefile` | Build configuration with object file separation |

## Important Implementation Details

1. **Filename handling**: Only basename sent over network (strip directory path)
2. **Progress tracking**: Real-time percentage shown using printf escape sequences
3. **File validation**: Server verifies save directory exists before listening
4. **Maximum size enforcement**: Client checks files don't exceed 16GB limit
5. **Hash format**: Printed as hex string for visibility (see `print_hash()`)

## Common Extensions

When modifying this codebase, maintain:
- **Binary protocol versioning** through `header.version` field
- **Incremental hash calculation** using `SHA256_Update()` for memory efficiency
- **Progress feedback** for user-facing threads (both client/server show %)
- **Clean resource lifecycle**: Open → Use → Close pattern for all file/socket handles

## Dependency Notes

- **OpenSSL**: Required for SHA256 (`-lssl -lcrypto` in LDFLAGS)
- **POSIX sockets**: Uses standard Linux/Unix socket APIs
- **GCC required**: Build uses C99 standard with `-std=c99` flag
