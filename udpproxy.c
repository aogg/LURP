#include <stdio.h>
#include <signal.h>

#include <winsock2.h>
#include <windows.h>

SOCKET upsock;
SOCKET lsock;

void terminate(int signum) {
    printf("[INFO] Terminate detect. Closing sockets\n");
    closesocket(upsock);
    closesocket(lsock);
    exit(signum);
}

int main(int argc, char* argv[]) {
    if (argc < 5) {
        fprintf(stderr, "[ERROR] Arguments insufficient\n");
        printf("[INFO] Usage: udpproxy.exe \"UpstreamIP\" UpstreamPORT \"ListenerIP\" ListenerPORT");
        return 1;
    }

    char* upip = argv[1];
    int upport = strtol(argv[2], NULL, 10);
    char* lip = argv[3];
    int lport = strtol(argv[4], NULL, 10);

    // Close sockets if terminated
    signal(SIGINT, terminate);

    // Initialize WinSock
    WSADATA wsad;
    if (WSAStartup(MAKEWORD(2,1), &wsad) != 0) {
        fprintf(stderr, "[ERROR] WinSock failed to initialize: %d\n", WSAGetLastError());
        return 1;
    }

    // Make UDP socket to connect to server
    upsock = socket(AF_INET, SOCK_DGRAM, 0);
    if (upsock == SOCKET_ERROR) {
        fprintf(stderr, "[ERROR] Failed to create upstream socket: %d\n", WSAGetLastError());
        return 1;
    }

    // Make upstream SOCKADDR
    SOCKADDR_IN upaddr;
    upaddr.sin_family = AF_INET;
    upaddr.sin_addr.S_un.S_addr = inet_addr(upip);
    upaddr.sin_port = htons(upport);
    int upaddrlen = sizeof(upaddr);

    // Try to connect to upstream
    printf("[INFO] Connecting to upstream server at [%s:%d]\n", upip, upport);
    if (connect(upsock, (SOCKADDR*)&upaddr, upaddrlen) != 0) {
        fprintf(stderr, "[ERROR] Can't connect to upstream server: %d\n", WSAGetLastError());
        closesocket(upsock);
        return 1;
    }
    printf("[INFO] Successfully connected to upstream server\n");

    // Setup listener
    lsock = socket(AF_INET, SOCK_DGRAM, 0);
    if (lsock == SOCKET_ERROR) {
        fprintf(stderr, "[ERROR] Failed to create listener socket: %d\n", WSAGetLastError());
        closesocket(upsock);
        return 1;
    }

    // Make listener SOCKADDR
    SOCKADDR_IN laddr;
    laddr.sin_family = AF_INET;
    laddr.sin_addr.S_un.S_addr = inet_addr(lip);
    laddr.sin_port = htons(lport);
    int laddrlen = sizeof(laddr);

    // Try to bind
    if (bind(lsock, (SOCKADDR*)&laddr, laddrlen) != 0) {
        fprintf(stderr, "[ERROR] Failed to bind: %d\n", WSAGetLastError());
        closesocket(upsock);
        closesocket(lsock);
        return 1;
    }
    printf("[INFO] Listening at [%s:%d]\n", lip, lport);

    // Listen buffer
    char lbuf[4096];
    char upbuf[4096];

    // Listen for connection
    while(1) {

        // Clear of previous data
        memset(lbuf, '\0', 4096);

        if (recvfrom(lsock, lbuf, 4096, 0, (SOCKADDR*)&laddr, &laddrlen) == SOCKET_ERROR) {
            fprintf(stderr, "[ERROR] Receive failed: %d\n", WSAGetLastError());
            closesocket(upsock);
            closesocket(lsock);
            return 1;
        } else {
            // Send to upstream
            if (sendto(upsock, lbuf, 4096, 0, (SOCKADDR*)&upaddr, upaddrlen) < 0) {
                fprintf(stderr, "[ERROR] Send to upstream failed: %d\n", WSAGetLastError());
                closesocket(upsock);
                closesocket(lsock);
                return 1;
            }
            while(1) {
                // Clear of previous data
                memset(upbuf, '\0', 4096);

                // Receive from upstream
                if (recvfrom(upsock, upbuf, 4096, 0, (SOCKADDR*)&upaddr, &upaddrlen) == SOCKET_ERROR) {
                    fprintf(stderr, "[ERROR] Receive from upstream failed: %d\n", WSAGetLastError());
                    closesocket(upsock);
                    closesocket(lsock);
                    return 1;
                } else {
                    if (sendto(lsock, upbuf, 4096, 0, (SOCKADDR*)&laddr, laddrlen) < 0) {
                        fprintf(stderr, "[ERROR] Send to listener failed: %d\n", WSAGetLastError());
                        closesocket(upsock);
                        closesocket(lsock);
                        return 1;
                    }
                    break;
                }
            }
        }

    }

    // Close all sockets
    closesocket(upsock);
    closesocket(lsock);

    return 0;
}