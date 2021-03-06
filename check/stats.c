#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/un.h>
#include "syscall.h"

#define ERROR        -1
#define RED          "\x1B[31m"
#define GRN          "\x1B[32m"
#define RESET        "\x1B[0m"

void help(void)
{
    int i = 0;
    const char *message[] = {
        "Usage:",
        "       stats /run/haproxy/admin.sock \"show stat\"",
        "or",
        "       stats 192.168.1.100 9999 \"show stat\"",
        NULL
    };

    while (message[i] != NULL)
    {
        printf(GRN "%s\n" RESET, message[i]);
        i++;
    }
}

int main(int argc, char **argv)
{
    char *socketPath = NULL;
    char *cmd = NULL;
    char *host = NULL;
    int port;
    int sock;
    struct sockaddr_un addrUn;
    struct sockaddr_in addrIn;
    int ret;
    char buffer[BUFSIZ];
    unsigned long ip = 0;
    int domain; /* 1 - Unix domain socket (AF_UNIX), 2 - Internet IP protocol (AF_INET) */

    if (argc < 3 || argc > 4)
    {
        help();
        return EXIT_FAILURE;
    }

    /* check first argument */
    if (inet_pton(AF_INET, argv[1], &ip) == 0)
    {
        /*  Unix domain socket  */
        socketPath = argv[1];
        cmd = argv[2];
        domain = AF_UNIX;
    }
    else
    {
        /* Internet IP protocol */
        host = argv[1];
        port = atoi(argv[2]);
        cmd = argv[3];
        domain = AF_INET;
    }
    
    sock = socket(domain, SOCK_STREAM, 0);
    if(sock < 0)
    {
        printf(RED "%s\n", "Cannot create socket" RESET);
        exit(EXIT_FAILURE);
    }

    if (domain == AF_UNIX) /* Unix domain socket */
    {
        memset(&addrUn, 0, sizeof(struct sockaddr_un));
        addrUn.sun_family = AF_UNIX;
        strncpy(addrUn.sun_path, socketPath, sizeof(addrUn.sun_path) - 1);
    }
    else if(domain == AF_INET) /* Internet IP protocol */
    {
        addrIn.sin_family = AF_INET;
        addrIn.sin_port = htons(port);
        inet_pton(AF_INET, host, &addrIn.sin_addr);
    }

    if (domain == AF_UNIX)
        ret = connect(sock, (struct sockaddr *) &addrUn, sizeof(addrUn));
    else if (domain == AF_INET)
        ret = connect(sock, (struct sockaddr *) &addrIn, sizeof(addrIn));
    
    if (ret == ERROR)
    {
        printf(RED "%s\n", "The server is down" RESET);
        exit(EXIT_FAILURE);
    }

    strcat(cmd, "\n");
    ret = write(sock, cmd, strlen(cmd));
    if (ret == ERROR)
    {
        printf(RED "%s\n", "Cannot write to socket" RESET);
        exit(EXIT_FAILURE);
    }

    ret = read(sock, buffer, BUFSIZ);
    if (ret == ERROR)
    {
        printf(RED "%s\n", "Cannot read from socket" RESET);
        exit(EXIT_FAILURE);
    }
    printf("%s", buffer);

    close(sock);
    return EXIT_SUCCESS;
}