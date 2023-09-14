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

    // Vars declaration
    char* upip = "";
    int upport = 0;
    char* lip = "";
    int lport = 0;
    int isDaemon = 0;
    int showAll = 0;

    // Parse arguments
    for (int i=0;i<argc;i++) {
        if (strcmp(argv[i], "-upip") == 0 && strcmp(argv[i+1], "") != 0) {
            upip = argv[i+1];
        }
        if (strcmp(argv[i], "-upport") == 0 && strcmp(argv[i+1], "") != 0) {
            upport = strtol(argv[i+1], NULL, 10);
        }
        if (strcmp(argv[i], "-lip") == 0 && strcmp(argv[i+1], "") != 0) {
            lip = argv[i+1];
        }
        if (strcmp(argv[i], "-lport") == 0 && strcmp(argv[i+1], "") != 0) {
            lport = strtol(argv[i+1], NULL, 10);
        }
        if (strcmp(argv[i], "-d") == 0) {
            isDaemon = 1;
        }
        
        if (strcmp(argv[i], "-v") == 0) {
            showAll = 1;
        }
    }

    // Check if required arguments are present
    if (
        upip == "" ||
        upport == 0 ||
        lip == "" ||
        lport == 0
    ) {
        fprintf(stderr, "[USAGE] udpproxy.exe -upip <UPSTREAMIP> -upport <UPSTREAMPORT> -lip <LISTENINGIP> -lport <LISTENINGPORT> [-d]\n");
        return 1;
    }

    // Check if daemon mode
    if (isDaemon == 1) {
        ShowWindow(GetConsoleWindow(), SW_HIDE);
    }

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
    if (connect(upsock, (SOCKADDR*)&upaddr, upaddrlen) == SOCKET_ERROR) {
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
    if (bind(lsock, (SOCKADDR*)&laddr, laddrlen) == SOCKET_ERROR) {
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

        printf("-----lbuf----start!\n");
        // Clear of previous data
        // memset(lbuf, '\0', 4096) 的作用是将 lbuf 数组中的前 4096 个字节（即数组的大小）都设置为 null 字符 ('\0')，实现了清除之前数据的目的。
        memset(lbuf, '\0', 4096);

        if (recvfrom(lsock, lbuf, 4096, 0, (SOCKADDR*)&laddr, &laddrlen) == SOCKET_ERROR) {
            fprintf(stderr, "[WARN] Receive from listener failed: %d\n", WSAGetLastError());
        }

        // Send to upstream
        if (sendto(upsock, lbuf, 4096, 0, (SOCKADDR*)&upaddr, upaddrlen) == SOCKET_ERROR) {
            fprintf(stderr, "[WARN] Send to upstream failed: %d\n", WSAGetLastError());
        }

        if (showAll) {
            printf("-----------lbuf content--------------------: %s\n", lbuf);
        }

        
        printf("-----upbuf----start!\n");
        // Clear of previous data
        memset(upbuf, '\0', 4096);

        // Receive from upstream
        if (recvfrom(upsock, upbuf, 4096, 0, (SOCKADDR*)&upaddr, &upaddrlen) == SOCKET_ERROR) {
            fprintf(stderr, "[WARN] Receive from upstream failed: %d\n", WSAGetLastError());
        }

        // Send to listener
        if (sendto(lsock, upbuf, 4096, 0, (SOCKADDR*)&laddr, laddrlen) == SOCKET_ERROR) {
            fprintf(stderr, "[WARN] Send to listener failed: %d\n", WSAGetLastError());
        }

    }

    // Close all sockets
    closesocket(upsock);
    closesocket(lsock);

    return 0;
}
