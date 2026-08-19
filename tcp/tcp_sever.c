#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>

int main(void)
{
    int server_fd;
    int client_fd;

    struct sockaddr_in addr;

    char buffer[128];
    /*1. socket */
    /*AF_INET -> IPv4*/
    /*SOCK_STREM -> TCP*/
    /*SOCK_DGRAM -> UDP*/
    server_fd = socket(AF_INET, SOCK_STREAM, 0);

    if (server_fd < 0) {
        perror("socket");
        return 1;
    }

    memset(&addr, 0, sizeof(addr));

    addr.sin_family = AF_INET;
    addr.sin_port = htons(5000);
    addr.sin_addr.s_addr = INADDR_ANY;
    /*2. bind */
    if (bind(server_fd,
             (struct sockaddr *)&addr,
             sizeof(addr)) < 0) {
        perror("bind");
        return 1;
    }
    /*3. listen */
    if (listen(server_fd, 5) < 0) {
        perror("listen");
        return 1;
    }

    printf("Waiting for connection...\n");
    /*4. accept */
    client_fd = accept(server_fd, NULL, NULL);

    if (client_fd < 0) {
        perror("accept");
        return 1;
    }

    while (1) {
        /*5. receive */
        int n = recv(client_fd, buffer, sizeof(buffer) - 1, 0);

        if (n <= 0)
            break;

        buffer[n] = '\0';

        printf("Received: %s\n", buffer);
        /*6. send */
        send(client_fd, buffer, n, 0);
    }

    /*7. close */
    close(client_fd);
    close(server_fd);

    return 0;
}