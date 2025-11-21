#include <iostream>
#include <chrono>
#include <cstring>
#include <iomanip>
#include <winsock2.h>
#include <ws2tcpip.h>
#pragma comment(lib, "ws2_32.lib")

 
std::string getCurrentDate() {
    time_t now = time(nullptr);
    tm localTime{};
    localtime_s(&localTime, &now);
    char buffer[20];
    strftime(buffer, sizeof(buffer), "%d-%m-%Y", &localTime);
    return std::string(buffer);
}

bool sendTestPacket(const char* ip, int size, double& rtt_ms) {

    // WINDOWS VERSION (ACTIVE CODE)

    // Initialize Winsock 
        WSADATA wsaData;
        if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) return false;

        SOCKET sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
        if (sock == INVALID_SOCKET) {
            WSACleanup();
            return false;
        }

        sockaddr_in addr{};
        addr.sin_family = AF_INET;
        addr.sin_port = htons(9000);
        inet_pton(AF_INET, ip, &addr.sin_addr);

        std::string packet(size, 'A');

        auto start = std::chrono::high_resolution_clock::now();
        int sent = sendto(sock, packet.data(), packet.size(), 0,
            (sockaddr*)&addr, sizeof(addr));
        if (sent == SOCKET_ERROR) {
            closesocket(sock);
            WSACleanup();
            return false;
        }

        int timeoutMs = 1000;
        setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO,
            (char*)&timeoutMs, sizeof(timeoutMs));

        char buffer[65536];
        int addrLen = sizeof(addr);
        int received = recvfrom(sock, buffer, sizeof(buffer), 0,
            (sockaddr*)&addr, &addrLen);

        auto end = std::chrono::high_resolution_clock::now();

        closesocket(sock);
        WSACleanup();

        if (received == SOCKET_ERROR) return false;

        rtt_ms = std::chrono::duration<double, std::milli>(end - start).count();
        return true;
}


int main() {

 
 

    // Target IP pf   echo server 
    const char* target_ip = "127.0.0.1"; 

    //Set the name of your Infrastracture 
    std::string interfaceName = "eth0";   
    int testSize = 32000;                

    int mtuValues[] = { 1500, 1400, 1300, 1200 };
    int count = sizeof(mtuValues) / sizeof(int);

    // Print table  
    std::cout << "| Test Date | MTU Tested | Interface | Test Size (bytes) | RTT (ms) | Throughput (KB/s) | Errors (#) |\n";
 

    for (int i = 0; i < count; i++) {
        double rtt;
        bool success = sendTestPacket(target_ip, mtuValues[i], rtt);

        // Calculate approximate throughput in KB/s
        double throughput = success ? (testSize / rtt / 1.024) : 0; // simple approximation
        int errors = success ? 0 : 1;

        std::cout << "| " << getCurrentDate()
            << " | " << mtuValues[i]
            << " | " << interfaceName
            << " | " << testSize
            << " | " << std::fixed << std::setprecision(2) << rtt
            << " | " << std::fixed << std::setprecision(2) << throughput
            << " | " << errors
            << " |\n";
    }

    return 0;
}