#ifndef COMMON_H
#define COMMON_H

#include <stdint.h>
#include <time.h>

#define BUFFER_SIZE (1024 * 1024)  /* 1MB chunks for efficient transfer */
#define MAX_FILENAME 256
#define HASH_SIZE 32  /* SHA256 produces 32 bytes */
#define PORT 9999

/* Protocol Header Structure */
typedef struct {
    uint32_t magic;             /* Magic number for protocol validation (0xDEADBEEF) */
    uint8_t version;            /* Protocol version */
    uint8_t command;            /* Command type (0=START, 1=DATA, 2=END) */
    uint16_t reserved;          /* Reserved for future use */
    uint64_t file_size;         /* Total file size in bytes */
    uint32_t chunk_size;        /* Size of current chunk */
    uint8_t hash[HASH_SIZE];    /* Final file hash (only in END command) */
} FileTransferHeader;

/* Command Types */
#define CMD_START 0
#define CMD_DATA 1
#define CMD_END 2

/* Error codes */
#define ERR_OK 0
#define ERR_INVALID_MAGIC 1
#define ERR_FILE_NOT_FOUND 2
#define ERR_HASH_MISMATCH 3
#define ERR_NETWORK 4
#define ERR_MEMORY 5

/* Function declarations */
int calculate_sha256(const char *filename, uint8_t *hash);
int send_file(const char *hostname, int port, const char *filename);
int receive_file(int port, const char *save_path);
int server_listen(int port);
int client_connect(const char *hostname, int port);
void print_hash(const uint8_t *hash);

#endif /* COMMON_H */
