# LURP
**Lightweight UDP Reverse Proxy** written in C

*Windows only because of WinSock2 dependency*

## Compilation
Compiled with gcc from MSYS
```
gcc -o udpproxy udpproxy.c -lws2_32
```

## Usage
```
udpproxy.exe -upip <UPSTREAMIP> -upport <UPSTREAMPORT> -lip <LISTENINGIP> -lport <LISTENINGPORT> [-d]
```