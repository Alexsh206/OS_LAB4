#include <iostream>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#include <fcntl.h>
#include <cstring>
#include <errno.h>

#define SOCKET_PATH "/tmp/unix_socket"

void setNonBlocking(int fd) {
    int flags = fcntl(fd, F_GETFL, 0);
    if (flags == -1) {
        perror("fcntl F_GETFL");
        exit(EXIT_FAILURE);
    }
    if (fcntl(fd, F_SETFL, flags | O_NONBLOCK) == -1) {
        perror("fcntl F_SETFL");
        exit(EXIT_FAILURE);
    }
}

int main() {
    int server_fd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (server_fd == -1) {
        perror("socket");
        return EXIT_FAILURE;
    }

    sockaddr_un server_addr{};
    server_addr.sun_family = AF_UNIX;
    strncpy(server_addr.sun_path, SOCKET_PATH, sizeof(server_addr.sun_path) - 1);

    unlink(SOCKET_PATH); 

    if (bind(server_fd, (struct sockaddr*)&server_addr, sizeof(server_addr)) == -1) {
        perror("bind");
        return EXIT_FAILURE;
    }

    if (listen(server_fd, 5) == -1) {
        perror("listen");
        return EXIT_FAILURE;
    }

    setNonBlocking(server_fd);

    std::cout << "Server is listening on " << SOCKET_PATH << std::endl;

    while (true) {
        sockaddr_un client_addr{};
        socklen_t client_len = sizeof(client_addr);

        int client_fd = accept(server_fd, (struct sockaddr*)&client_addr, &client_len);
        if (client_fd == -1) {
            if (errno != EAGAIN && errno != EWOULDBLOCK) {
                perror("accept");
            }
        }
        else {
            std::cout << "Client connected" << std::endl;
            setNonBlocking(client_fd);

            char buffer[256];
            while (true) {
                ssize_t bytes_read = read(client_fd, buffer, sizeof(buffer) - 1);
                if (bytes_read > 0) {
                    buffer[bytes_read] = '\0';
                    std::cout << "Received: " << buffer << std::endl;
                    write(client_fd, "Acknowledged\n", 13);
                }
                else if (bytes_read == 0) {
                    std::cout << "Client disconnected" << std::endl;
                    close(client_fd);
                    break;
                }
                else if (errno != EAGAIN && errno != EWOULDBLOCK) {
                    perror("read");
                    close(client_fd);
                    break;
                }
            }
        }
    }

    close(server_fd);
    unlink(SOCKET_PATH);
    return 0;
}
