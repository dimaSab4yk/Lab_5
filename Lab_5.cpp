#include <iostream>
#include <fstream>
#include <string>
#include <winsock2.h>
#include <ws2tcpip.h>

#pragma comment(lib, "ws2_32.lib")

#define SERVER_PORT 8080

using namespace std;

void sendHttpResponse(SOCKET client, const string& content, const string& status = "200 OK")
{
    string responseHeader = "HTTP/1.1 " + status + "\r\n";
    responseHeader += "Content-Type: text/html\r\n";
    responseHeader += "Content-Length: " + to_string(content.length()) + "\r\n";
    responseHeader += "\r\n";

    send(client, responseHeader.c_str(), responseHeader.length(), 0);
    send(client, content.c_str(), content.length(), 0);
}

void processClient(SOCKET client)
{
    const int bufferSize = 4096;
    char buffer[bufferSize] = { 0 };

    int bytesReceived = recv(client, buffer, bufferSize, 0);

    if (bytesReceived <= 0) 
    {
        closesocket(client);
        return;
    }

    string request(buffer);
    size_t pos = request.find("\r\n");

    if (pos == string::npos) 
    {
        closesocket(client);
        return;
    }

    string requestLine = request.substr(0, pos);
    cout << "Request: " << requestLine << endl;

    size_t firstSpace = requestLine.find(' ');
    size_t secondSpace = requestLine.find(' ', firstSpace + 1);

    if (firstSpace == string::npos || secondSpace == string::npos) 
    {
        closesocket(client);
        return;
    }

    string path = requestLine.substr(firstSpace + 1, secondSpace - firstSpace - 1);

    if (path == "/") 
    {
        path = "/index.html";
    }

    string fileName = path.substr(1);

    ifstream file(fileName, ios::binary);

    if (file) 
    {
        string content((istreambuf_iterator<char>(file)), istreambuf_iterator<char>());
        sendHttpResponse(client, content, "200 OK");
    }
    else 
    {
        string notFoundPage = "<html><head><title>404 Not Found</title></head>"
            "<body><h1>404 Not Found</h1><p>The requested page was not found.</p></body></html>";
        sendHttpResponse(client, notFoundPage, "404 Not Found");
    }

    closesocket(client);
}

int main()
{
    WSADATA wsaData;

    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) 
    {
        cerr << "WSAStartup failed.\n";
        return 1;
    }

    SOCKET listeningSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

    if (listeningSocket == INVALID_SOCKET) 
    {
        cerr << "Socket creation failed.\n";
        WSACleanup();
        return 1;
    }

    sockaddr_in serverAddress{};
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_addr.s_addr = htonl(INADDR_ANY);
    serverAddress.sin_port = htons(SERVER_PORT);

    if (bind(listeningSocket, (sockaddr*)&serverAddress, sizeof(serverAddress)) == SOCKET_ERROR) 
    {
        cerr << "Binding failed.\n";
        closesocket(listeningSocket);
        WSACleanup();
        return 1;
    }

    if (listen(listeningSocket, SOMAXCONN) == SOCKET_ERROR) 
    {
        cerr << "Listening failed.\n";
        closesocket(listeningSocket);
        WSACleanup();
        return 1;
    }

    cout << "Server is running on port " << SERVER_PORT << "...\n";

    while (true)
    {
        sockaddr_in clientAddress{};
        int clientSize = sizeof(clientAddress);
        SOCKET clientSocket = accept(listeningSocket, (sockaddr*)&clientAddress, &clientSize);

        if (clientSocket == INVALID_SOCKET) 
        {
            cerr << "Accepting connection failed.\n";
            continue;
        }

        char clientIP[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &clientAddress.sin_addr, clientIP, INET_ADDRSTRLEN);
        cout << "New connection from " << clientIP << ":" << ntohs(clientAddress.sin_port) << "\n";

        processClient(clientSocket);
    }

    closesocket(listeningSocket);
    WSACleanup();

    return 0;
}

