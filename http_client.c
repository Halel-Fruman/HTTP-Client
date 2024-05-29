#include <arpa/inet.h>
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#define MAX_REQUEST_SIZE 1324
#define MAX_RESPONSE_SIZE 4096

/**
 * Prints an error message and exits the program.
 * @param msg The error message to print.
 */
void error(const char *msg) {
    perror(msg);
    exit(1);
}

/**
 * Gets the size of a given file.
 * @param file The file whose size is to be determined.
 * @return The size of the file in bytes.
 */
int getFileSize(FILE *file) {
    int size;

    // Save the current position
    long currentPosition = ftell(file);

    // Move to the end of the file
    fseek(file, 0, SEEK_END);

    // Get the file size
    size = (int) ftell(file);

    // Restore the position
    fseek(file, currentPosition, SEEK_SET);

    return size;
}

/**
 * Prints the content of a file to the standard output.
 * @param file The file whose content is to be printed.
 */
void printFileContent(FILE *file) {
    // Buffer to store each line read from the file
    char buffer[1024];

    // Read each line from the file and print it
    while (fgets(buffer, sizeof(buffer), file) != NULL) {
        printf("%s", buffer);
    }
}

/**
 * Creates the necessary directories based on the given URL.
 * @param url The URL from which directories are to be created.
 * @param path2 The path part of the URL.
 */
void createDirectories(char *url, char *path2) {
    // Find the last occurrence of '/'
    char *last_slash = strrchr(url, '/');
    if (last_slash == NULL) {
        // Handle the case where '/' is not found in the URL
        return;
    }

    // Copy the path after the last '/'
    strcpy(path2, last_slash);

    // Truncate the URL at the last '/'
    *last_slash = '\0';

    // Use the modified URL to create directories
    char path[1024];
    snprintf(path, sizeof(path), "mkdir -p %s", url);
    system(path);
}

/**
 * Extracts the HTTP status code from the server's response.
 * @param response The server's response.
 * @return The HTTP status code.
 */
int extractStatusCode(const char *response) {
    int status_code = -1;

    // Find the start of the status code
    const char *status_start = strstr(response, "HTTP/1.");
    if (status_start != NULL) {
        // Move past the HTTP version
        status_start += strlen("HTTP/1.") + 2;

        // Extract the status code
        while (*status_start == ' ') {
            status_start++; // Skip spaces
        }

        if (*status_start >= '0' && *status_start <= '9') {
            // Found a digit, parse the status code
            status_code = 0;
            while (*status_start >= '0' && *status_start <= '9') {
                status_code = status_code * 10 + (*status_start - '0');
                status_start++;
            }
        }
    }

    return status_code;
}

int main(int argc, char *argv[]) {
    // Validate the command-line arguments
    if (argc < 2 || (argc == 3 && strcmp(argv[2], "-s") != 0) || argc > 3) {
        fprintf(stderr, "Usage: %s <URL> [-s]\n", argv[0]);
        exit(1);
    }

    // Extract URL and browser flag from arguments
    char *url = argv[1];
    int use_browser = 0;
    if (argc == 3 && strcmp(argv[2], "-s") == 0) {
        use_browser = 1;
    }

    char host[256];
    char path[1024] = "/";
    int port = 80;

    // Parse URL to extract host, path, and port
    if (sscanf(url, "http://%[^:/]:%d/%s", host, &port, path) != 3 &&
        sscanf(url, "http://%[^:/]/%s", host, path) != 2) {
        sscanf(url, "http://%[^:/]", host);
    }

    // Prepare the directory path for storing the file locally
    char dir[1024];
    snprintf(dir, sizeof(dir), "%s/%s", host, path);
    if (path[strlen(path) - 1] == '/') {
        strcat(dir, "index.html");
    }

    // Check if the file already exists in the local filesystem
    if (access(dir, F_OK) != -1) {
        // Open and read the local file
        FILE *file = fopen(dir, "r");
        if (file == NULL) {
            error("Error opening file");
        }
        int n = getFileSize(file);
        char locRes[256];
        snprintf(locRes, 256, "HTTP/1.0 200 OK\r\nContent-Length: %d\r\n\r\n", n);
        printf("File is given from local filesystem\n");
        printf("%s", locRes);
        printFileContent(file);
        int size = n + (int)strlen(locRes);
        printf("\n Total response bytes: %d\n", size);
        fclose(file);
    } else {
        // Create the HTTP GET request
        char request[MAX_REQUEST_SIZE];
        snprintf(request, MAX_REQUEST_SIZE, "GET /%s HTTP/1.0\r\nHost: %s\r\n\r\n", path, host);
        printf("HTTP request =\n%s\nLEN = %d\n", request, (int)strlen(request));

        // Create a socket and connect to the server
        int sockfd = socket(AF_INET, SOCK_STREAM, 0);
        if (sockfd < 0) {
            error("Error opening socket");
        }

        struct hostent *server = gethostbyname(host);
        if (server == NULL) {
            herror("gethostbyname");
            exit(1);
        }

        struct sockaddr_in server_addr;
        memset(&server_addr, 0, sizeof(server_addr));
        server_addr.sin_family = AF_INET;
        memcpy(&server_addr.sin_addr.s_addr, server->h_addr, server->h_length);
        server_addr.sin_port = htons(port);

        if (connect(sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
            error("Error connecting to server");
        }

        // Send the HTTP request
        int request_len = (int)strlen(request);
        if (send(sockfd, request, request_len, 0) < 0) {
            error("Error sending request to server");
        }

        // Prepare to save the file locally
        createDirectories(dir, path);
        strcat(dir, path);

        FILE *file = fopen(dir, "wb");
        if (file == NULL) {
            error("Error opening file");
        }

        // Receive the HTTP response
        unsigned char response[MAX_RESPONSE_SIZE] = "\0";
        memset(response, 0, MAX_RESPONSE_SIZE);
        int bytes_received;
        int total_total_received = 0;
        int headers_finished = 0;
        int header_finish_time = 0;
        int statusCode = -1;
        char *loc;

        // Process the response from the server
        while ((bytes_received = (int)recv(sockfd, response, MAX_RESPONSE_SIZE, 0)) > 0) {
            // Find the end of headers marker "\r\n\r\n"
            for (int i = 0; i < bytes_received - 3; ++i) {
                if (response[i] == '\r' && response[i + 1] == '\n' && response[i + 2] == '\r' && response[i + 3] == '\n') {
                    headers_finished = 1;
                    loc = (char *)&response[0];
                    statusCode = extractStatusCode(loc);

                    if (statusCode != 200) {
                        remove(dir);
                    }
                    fwrite(response + i + 4, 1, bytes_received - i - 4, file); // Write body to file

                    break;
                }
            }

            // Write remaining data (body) to file after headers are finished
            if (headers_finished && statusCode == 200) {
                header_finish_time++;
                if (header_finish_time > 1)
                    fwrite(response, 1, bytes_received, file); // Write remaining data (body) to file
            }
            total_total_received += bytes_received;
            // Display response
            response[bytes_received] = '\0';
            printf("%s", response);
            memset(response, 0, MAX_RESPONSE_SIZE);
        }

        if (bytes_received < 0) {
            error("Error receiving response from server");
        }

        fclose(file);

        // Display the total number of bytes received
        printf("Total response bytes: %d\n", total_total_received);
        close(sockfd);
    }

    // If -s flag is specified, open the file in a web browser
    if (use_browser) {
        char cmd[1024];
        snprintf(cmd, sizeof(cmd), "firefox %s", dir);
        system(cmd);
    }

    return 0;
}
