#include "common.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <libgen.h>
#include <errno.h>

/* Send file to server with integrity checking */
int send_file(const char *hostname, int port, const char *filename) {
    /* Verify file exists and get size */
    struct stat st;
    if (stat(filename, &st) == -1) {
        perror("File not found");
        return ERR_FILE_NOT_FOUND;
    }

    if (!S_ISREG(st.st_mode)) {
        fprintf(stderr, "Error: Not a regular file\n");
        return ERR_FILE_NOT_FOUND;
    }

    uint64_t file_size = st.st_size;
    printf("File size: %llu bytes (%.2f GB)\n", 
           (unsigned long long)file_size,
           (double)file_size / (1024 * 1024 * 1024));

    /* Validate file size doesn't exceed 16GB */
    uint64_t max_size = 16ULL * 1024 * 1024 * 1024;
    if (file_size > max_size) {
        fprintf(stderr, "Error: File exceeds maximum size of 16GB\n");
        return ERR_FILE_NOT_FOUND;
    }

    /* Connect to server */
    int sock = client_connect(hostname, port);
    if (sock < 0) {
        return ERR_NETWORK;
    }

    printf("Connected to %s:%d\n", hostname, port);

    /* Calculate file hash for integrity verification */
    printf("Calculating SHA256 hash...\n");
    uint8_t file_hash[HASH_SIZE];
    if (calculate_sha256(filename, file_hash) != ERR_OK) {
        close(sock);
        return ERR_FILE_NOT_FOUND;
    }
    printf("File hash: ");
    print_hash(file_hash);

    /* Send START command with metadata */
    FileTransferHeader header;
    memset(&header, 0, sizeof(header));
    header.magic = 0xDEADBEEF;
    header.version = 1;
    header.command = CMD_START;
    header.file_size = file_size;
    header.chunk_size = BUFFER_SIZE;

    if (send(sock, &header, sizeof(header), 0) < 0) {
        perror("Failed to send header");
        close(sock);
        return ERR_NETWORK;
    }

    /* Send filename */
    const char *basename_ptr = strrchr(filename, '/');
    const char *send_filename = (basename_ptr) ? basename_ptr + 1 : filename;

    if (send(sock, send_filename, strlen(send_filename), 0) < 0) {
        perror("Failed to send filename");
        close(sock);
        return ERR_NETWORK;
    }

    printf("Sending file: %s\n", send_filename);

    /* Open file for reading */
    FILE *file = fopen(filename, "rb");
    if (!file) {
        perror("Cannot open file");
        close(sock);
        return ERR_FILE_NOT_FOUND;
    }

    /* Send file data in chunks */
    unsigned char buffer[BUFFER_SIZE];
    uint64_t total_sent = 0;
    size_t bytes_read;

    while ((bytes_read = fread(buffer, 1, BUFFER_SIZE, file)) > 0) {
        if (send(sock, buffer, bytes_read, 0) < 0) {
            perror("Failed to send data");
            fclose(file);
            close(sock);
            return ERR_NETWORK;
        }

        total_sent += bytes_read;

        /* Progress indicator */
        printf("\r[%.2f%%] Sent %llu / %llu bytes",
               (double)total_sent / file_size * 100,
               (unsigned long long)total_sent,
               (unsigned long long)file_size);
        fflush(stdout);
    }

    printf("\n");
    fclose(file);

    /* Send END command with hash */
    memset(&header, 0, sizeof(header));
    header.magic = 0xDEADBEEF;
    header.version = 1;
    header.command = CMD_END;
    header.file_size = total_sent;
    memcpy(header.hash, file_hash, HASH_SIZE);

    if (send(sock, &header, sizeof(header), 0) < 0) {
        perror("Failed to send END command");
        close(sock);
        return ERR_NETWORK;
    }

    printf("✓ File transfer complete\n");

    close(sock);

    return ERR_OK;
}

/* Main client entry point */
int main(int argc, char *argv[]) {
    if (argc < 4) {
        fprintf(stderr, "Usage: %s <hostname> <filepath> <port>\n", argv[0]);
        fprintf(stderr, "Example: %s 192.168.1.100 largefile.iso 9999\n", argv[0]);
        return 1;
    }

    const char *hostname = argv[1];
    const char *filepath = argv[2];
    int port = atoi(argv[3]);

    if (port <= 0 || port > 65535) {
        fprintf(stderr, "Error: Invalid port number (1-65535)\n");
        return 1;
    }

    int result = send_file(hostname, port, filepath);
    return (result == ERR_OK) ? 0 : 1;
}
