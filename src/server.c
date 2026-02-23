#include "common.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <openssl/evp.h>
#include <errno.h>

/* Receive file from client with integrity checking */
int receive_file(int port, const char *save_path) {
    int server_sock = server_listen(port);
    if (server_sock < 0) {
        return ERR_NETWORK;
    }

    struct sockaddr_in client_addr;
    socklen_t client_addr_len = sizeof(client_addr);

    printf("Waiting for connection...\n");
    int client_sock = accept(server_sock, (struct sockaddr *)&client_addr, &client_addr_len);
    if (client_sock < 0) {
        perror("Accept failed");
        close(server_sock);
        return ERR_NETWORK;
    }

    printf("Connection established from %s:%d\n", inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));

    /* Receive START command with file metadata */
    FileTransferHeader header;
    ssize_t received = recv(client_sock, &header, sizeof(header), MSG_WAITALL);
    if (received != sizeof(header)) {
        perror("Failed to receive header");
        close(client_sock);
        close(server_sock);
        return ERR_NETWORK;
    }

    /* Validate magic number */
    if (header.magic != 0xDEADBEEF) {
        fprintf(stderr, "Invalid protocol magic number\n");
        close(client_sock);
        close(server_sock);
        return ERR_INVALID_MAGIC;
    }

    if (header.command != CMD_START) {
        fprintf(stderr, "Expected START command\n");
        close(client_sock);
        close(server_sock);
        return ERR_NETWORK;
    }

    printf("Receiving file: %llu bytes\n", (unsigned long long)header.file_size);

    /* Receive filename */
    char filename[MAX_FILENAME];
    received = recv(client_sock, filename, sizeof(filename)-1, 0);
    if (received <= 0) {
        perror("Failed to receive filename");
        close(client_sock);
        close(server_sock);
        return ERR_NETWORK;
    }
    filename[received] = '\0';

    /* Construct full path */
    char full_path[512];
    snprintf(full_path, sizeof(full_path), "%s/%s", save_path, filename);

    FILE *file = fopen(full_path, "wb");
    if (!file) {
        perror("Cannot create file");
        close(client_sock);
        close(server_sock);
        return ERR_FILE_NOT_FOUND;
    }

    /* Calculate hash while receiving data */
    EVP_MD_CTX *ctx = EVP_MD_CTX_new();
    if (!ctx) {
        perror("EVP_MD_CTX_new failed");
        close(client_sock);
        close(server_sock);
        return ERR_MEMORY;
    }

    if (EVP_DigestInit_ex(ctx, EVP_sha256(), NULL) != 1) {
        perror("EVP_DigestInit_ex failed");
        EVP_MD_CTX_free(ctx);
        close(client_sock);
        close(server_sock);
        return ERR_MEMORY;
    }

    uint64_t total_received = 0;
    unsigned char buffer[BUFFER_SIZE];

    while (total_received < header.file_size) {
        uint32_t to_receive = (header.file_size - total_received > BUFFER_SIZE) 
                              ? BUFFER_SIZE 
                              : (header.file_size - total_received);

        received = recv(client_sock, buffer, to_receive, MSG_WAITALL);
        if (received <= 0) {
            perror("Connection lost during transfer");
            fclose(file);
            unlink(full_path);
            close(client_sock);
            close(server_sock);
            return ERR_NETWORK;
        }

        fwrite(buffer, 1, received, file);
        if (EVP_DigestUpdate(ctx, buffer, received) != 1) {
            perror("EVP_DigestUpdate failed");
            fclose(file);
            unlink(full_path);
            EVP_MD_CTX_free(ctx);
            close(client_sock);
            close(server_sock);
            return ERR_MEMORY;
        }
        total_received += received;

        /* Progress indicator */
        printf("\r[%.2f%%] Received %llu / %llu bytes", 
               (double)total_received / header.file_size * 100,
               (unsigned long long)total_received,
               (unsigned long long)header.file_size);
        fflush(stdout);
    }

    printf("\n");
    fclose(file);

    /* Receive END command with hash verification */
    received = recv(client_sock, &header, sizeof(header), MSG_WAITALL);
    if (received != sizeof(header)) {
        perror("Failed to receive END command");
        unlink(full_path);
        close(client_sock);
        close(server_sock);
        return ERR_NETWORK;
    }

    if (header.command != CMD_END) {
        fprintf(stderr, "Expected END command\n");
        unlink(full_path);
        close(client_sock);
        close(server_sock);
        return ERR_NETWORK;
    }

    /* Finalize hash calculation */
    uint8_t calculated_hash[HASH_SIZE];
    if (EVP_DigestFinal_ex(ctx, calculated_hash, NULL) != 1) {
        perror("EVP_DigestFinal_ex failed");
        unlink(full_path);
        EVP_MD_CTX_free(ctx);
        close(client_sock);
        close(server_sock);
        return ERR_MEMORY;
    }

    EVP_MD_CTX_free(ctx);

    /* Compare hashes */
    if (memcmp(calculated_hash, header.hash, HASH_SIZE) != 0) {
        fprintf(stderr, "Hash mismatch! File integrity check failed.\n");
        fprintf(stderr, "Expected: ");
        print_hash(header.hash);
        fprintf(stderr, "Calculated: ");
        print_hash(calculated_hash);
        unlink(full_path);
        close(client_sock);
        close(server_sock);
        return ERR_HASH_MISMATCH;
    }

    printf("✓ File integrity verified\n");
    printf("✓ File saved to: %s\n", full_path);

    close(client_sock);
    close(server_sock);

    return ERR_OK;
}

/* Main server entry point */
int main(int argc, char *argv[]) {
    if (argc < 2) {
        fprintf(stderr, "Usage: %s <save_directory> [port]\n", argv[0]);
        fprintf(stderr, "Example: %s ./downloads 9999\n", argv[0]);
        return 1;
    }

    const char *save_path = argv[1];
    int port = (argc > 2) ? atoi(argv[2]) : PORT;

    /* Verify save directory exists */
    struct stat st;
    if (stat(save_path, &st) == -1 || !S_ISDIR(st.st_mode)) {
        fprintf(stderr, "Error: Save directory '%s' does not exist\n", save_path);
        return 1;
    }

    int result = receive_file(port, save_path);
    return (result == ERR_OK) ? 0 : 1;
}
