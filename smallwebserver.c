// smallwebserver.c

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <stdint.h>
#include <stdbool.h>
#include <threads.h>
#include <ctype.h>

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#pragma comment(lib, "ws2_32.lib")
#else
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#endif

#define IP_ADDRESS "127.0.0.1"
#define PORT 3000
#define BUFFER_SIZE 1024

//
// Platform-specific cleanup
//
void platform_cleanup()
{
#ifdef _WIN32
    WSACleanup();
#endif
}

//
// Safe socket close function
//
void safe_close(int socket_fd)
{
#ifdef _WIN32
    closesocket(socket_fd);
#else
    close(socket_fd);
#endif
}

//
// URL decode function
//
void url_decode(char *src, char *dest)
{
    char *psrc = src;
    char *pdest = dest;
    while (*psrc)
    {
        if (*psrc == '%')
        {
            if (isxdigit(*(psrc + 1)) && isxdigit(*(psrc + 2)))
            {
                char hex[3] = {*(psrc + 1), *(psrc + 2), '\0'};
                *pdest++ = (char)strtol(hex, NULL, 16);
                psrc += 3;
            }
            else
            {
                *pdest++ = *psrc++;
            }
        }
        else if (*psrc == '+')
        {
            *pdest++ = ' ';
            psrc++;
        }
        else
        {
            *pdest++ = *psrc++;
        }
    }
    *pdest = '\0';
}

//
// Function to handle client connection
//
int handle_client(void *client_socket_ptr)
{
    int client_socket = *(int *)client_socket_ptr;
    free(client_socket_ptr);

    char buffer[BUFFER_SIZE] = {0};
#ifdef _WIN32
    int bytes_read = recv(client_socket, buffer, BUFFER_SIZE - 1, 0);
#else
    ssize_t bytes_read = recv(client_socket, buffer, BUFFER_SIZE - 1, 0);
#endif
    if (bytes_read < 0)
    {
        safe_close(client_socket);
        return thrd_error;
    }

    buffer[bytes_read] = '\0'; // Null-terminate the request

    // Parse the request to extract the filename
    char *filename_start = strstr(buffer, "filename=");
    if (filename_start)
    {
        filename_start += 9; // Move past "filename="
        char *filename_end = strchr(filename_start, ' ');
        if (filename_end)
        {
            *filename_end = '\0';
        }

        char decoded_filename[BUFFER_SIZE];
        url_decode(filename_start, decoded_filename);

        // Open the requested file
        FILE *file = fopen(decoded_filename, "r");
        if (file)
        {
            char file_content[BUFFER_SIZE] = {0};
            fread(file_content, 1, sizeof(file_content) - 1, file);
            fclose(file);

            char response[BUFFER_SIZE * 2];
            snprintf(response, sizeof(response),
                     "HTTP/1.1 200 OK\r\nContent-Length: %zu\r\nContent-Type: text/plain\r\n\r\n%s",
                     strlen(file_content), file_content);

            send(client_socket, response, strlen(response), 0);
        }
        else
        {
            char *not_found_response = "HTTP/1.1 404 Not Found\r\nContent-Length: 0\r\n\r\n";
            send(client_socket, not_found_response, strlen(not_found_response), 0);
        }
    }
    else
    {
        char *bad_request_response = "HTTP/1.1 400 Bad Request\r\nContent-Length: 0\r\n\r\n";
        send(client_socket, bad_request_response, strlen(bad_request_response), 0);
    }

    safe_close(client_socket);
    return thrd_success;
}

//
// Main Entry Point
//
int main()
{
#ifdef _WIN32
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
    {
        exit(EXIT_FAILURE);
    }
#endif

    struct sockaddr_in server_addr;
    int server_fd;

#ifdef _WIN32
    server_fd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (server_fd == INVALID_SOCKET)
    {
        platform_cleanup();
        exit(EXIT_FAILURE);
    }
#else
    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd < 0)
    {
        exit(EXIT_FAILURE);
    }
#endif

    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);
    server_addr.sin_addr.s_addr = inet_addr(IP_ADDRESS);

#ifdef _WIN32
    if (bind(server_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) == SOCKET_ERROR)
    {
        safe_close(server_fd);
        platform_cleanup();
        exit(EXIT_FAILURE);
    }
#else
    if (bind(server_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0)
    {
        safe_close(server_fd);
        exit(EXIT_FAILURE);
    }
#endif

#ifdef _WIN32
    if (listen(server_fd, SOMAXCONN) == SOCKET_ERROR)
    {
        safe_close(server_fd);
        platform_cleanup();
        exit(EXIT_FAILURE);
    }
#else
    if (listen(server_fd, 5) < 0)
    {
        safe_close(server_fd);
        exit(EXIT_FAILURE);
    }
#endif

    while (true)
    {
        struct sockaddr_in client_addr;
        socklen_t addr_len = sizeof(client_addr);

#ifdef _WIN32
        SOCKET new_socket = accept(server_fd, (struct sockaddr *)&client_addr, &addr_len);
        if (new_socket == INVALID_SOCKET)
        {
            continue;
        }
#else
        int new_socket = accept(server_fd, (struct sockaddr *)&client_addr, &addr_len);
        if (new_socket < 0)
        {
            continue;
        }
#endif

        int *client_socket_ptr = malloc(sizeof(int));
        if (client_socket_ptr == NULL)
        {
            safe_close(new_socket);
            continue;
        }
        *client_socket_ptr = new_socket;

        thrd_t client_thread;
        if (thrd_create(&client_thread, handle_client, client_socket_ptr) != thrd_success)
        {
            safe_close(new_socket);
            free(client_socket_ptr);
            continue;
        }
        thrd_detach(client_thread);
    }

    safe_close(server_fd);
    platform_cleanup();
    return 0;
}