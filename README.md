
# Simple HTTP Client

This project is a simple HTTP client implemented in C. It fetches a web page, saves it locally, and serves it from the local filesystem if it already exists. Additionally, it supports opening the fetched file in a web browser if the `-s` flag is provided.

## Features

- Fetches a web page from a given URL.
- Saves the fetched web page locally.
- Serves the web page from the local filesystem if it already exists.
- Opens the fetched web page in a web browser using the `-s` flag.
- Extracts and prints the HTTP status code and response headers.

## Prerequisites

- A C compiler (e.g., `gcc`).
- Internet connection for fetching web pages.
- Firefox browser (for the `-s` flag functionality).

## Usage

### Compilation

Compile the source code using a C compiler:

```bash
gcc -o http_client http_client.c
```

### Running the Program

Run the compiled executable with the required URL argument:

```bash
./http_client <URL> [-s]
```

- `<URL>`: The URL of the web page to fetch.
- `-s` (optional): Open the fetched web page in Firefox.

### Example

Fetch and save a web page:

```bash
./http_client http://www.example.com/
```

Fetch and save a web page, then open it in Firefox:

```bash
./http_client http://www.example.com/ -s
```

## Code Overview

The main functionalities of the program include:

1. **Error Handling**:
   - `error(const char *msg)`: Prints an error message and exits the program.

2. **File Operations**:
   - `getFileSize(FILE *file)`: Returns the size of a given file.
   - `printFileContent(FILE *file)`: Prints the content of a given file.

3. **Directory Creation**:
   - `createDirectories(char *url, char *path2)`: Creates necessary directories based on the URL.

4. **HTTP Status Code Extraction**:
   - `extractStatusCode(const char *response)`: Extracts and returns the HTTP status code from the server's response.

5. **Main Function**:
   - Validates command-line arguments.
   - Parses the URL to extract the host, path, and port.
   - Checks if the file exists locally and serves it if found.
   - Constructs and sends an HTTP GET request to the server.
   - Receives and processes the HTTP response.
   - Saves the response body to a local file.
   - Opens the file in Firefox if the `-s` flag is provided.




