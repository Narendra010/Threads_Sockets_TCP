#include <iostream>
#include <vector>
#include <windows.h>
#include <string>
using namespace std;


// Base class managing thread implementation for different functions
class ThreadManager {
private:
    HANDLE hThreads[2];

protected:
    HANDLE hEvent;


    //Constructor
    ThreadManager() {
        try {
            hEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
            if (hEvent == NULL) {
                throw "Failed to create event!";
                CloseHandle(hEvent);
            }
        }
        catch (const char* errorMessage) {
            cout << errorMessage << endl;
            return; // Return control to main() or handle the error as needed
        }
    }


    //Destructor
    ~ThreadManager() {
        CloseHandle(hEvent);
    }


    //Read thread initializer function
    static DWORD WINAPI ReadThread(LPVOID param) { 
        ThreadManager* pManager = reinterpret_cast<ThreadManager*>(param);
        return pManager->Read();
    }


    //Write thread initializer function
    static DWORD WINAPI WriteThread(LPVOID param) {
        ThreadManager* pManager = reinterpret_cast<ThreadManager*>(param);
        return pManager->Write();
    }

    //Virtual functions to be defined in the inherited classes 
    //for thread implementation 
    virtual DWORD Read() = 0;
    virtual DWORD Write() = 0;

public:

    // Function to create read write threads
    void StartReadWriteThreads() {
        hThreads[0] = CreateThread(NULL, 0, WriteThread, this, 0, NULL);
        if (hThreads[0] == NULL) {
            std::cout << "Error creating write thread" << std::endl;
            return;
        }

        hThreads[1] = CreateThread(NULL, 0, ReadThread, this, 0, NULL);
        if (hThreads[1] == NULL) {
            std::cout << "Error creating read thread" << std::endl;
            return;
        }

        WaitForMultipleObjects(2, hThreads, TRUE, INFINITE);

        CloseHandle(hThreads[0]);
        CloseHandle(hThreads[1]);
    }
};


// Inherited class to manage file read write operations using threads
class FileOperations : public ThreadManager {
private:

    string m_strFilename;

    // override Read method to read the data from the file
    DWORD Read() override {

        // Waiting for write function to generate event of its completion
        WaitForSingleObject(hEvent, INFINITE);

        HANDLE hFile = CreateFileA(m_strFilename.c_str(), GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
        if (hFile == INVALID_HANDLE_VALUE) {
            cout << "Error opening file for reading" << endl;
            return 1;
        }

        DWORD dwBytesRead = 0;
        char cBuffer[1024] = { 0 };
        if (ReadFile(hFile, cBuffer, sizeof(cBuffer), &dwBytesRead, NULL) && dwBytesRead > 0) {
            cBuffer[dwBytesRead] = '\0'; // null terminate
            cout << "Data read: " << cBuffer << endl;
        }
        else {
            cout << "\n Unable to read the data ";
        }

        CloseHandle(hFile);

        cout << "Reading done!" << endl;
        return 0;
    }


    // override Write method to write the data to the file
    DWORD Write() override {

        HANDLE hFile = CreateFileA(m_strFilename.c_str(), GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
        if (hFile == INVALID_HANDLE_VALUE) {
            cout << "Error opening file for writing" << endl;
            return 1;
        }
        string sValue;
        DWORD dwBytesWritten;
        for (int i = 1; i <= 10; i++) {

            sValue = to_string(int(pow(i, 2))) + "\n";

            WriteFile(hFile, sValue.c_str(), sValue.length(), &dwBytesWritten, NULL);

            cout << "Writing: " << sValue << endl;
        }

        CloseHandle(hFile);

        cout << "Writing done!" << endl;
        SetEvent(hEvent);
        return 0; // Replace with your return value
    }

public:
    FileOperations(const string& filename) {
        m_strFilename = filename;
    }

    ~FileOperations() = default; // Destructor should automatically call the base destructor
};


// Another inherited class to implement read write operation 
// on vectors using threads
class DataOperations : public ThreadManager {
private:
    vector<int> m_vnData;

    // override Read method to read the data from the vector
    DWORD Read() override {
        WaitForSingleObject(hEvent, INFINITE);
        for (int i = 0; i < m_vnData.size(); i++) {
            cout << endl << "Data read : " << m_vnData[i];
        }
        cout << endl << "Reading done!!" << endl;
        return 0;
    }

    // override Write method to write data in the vector
    DWORD Write() override {
        for (int i = 0; i < m_vnData.size(); i++) {
            m_vnData[i] = pow((i + 1), 2);
            cout << endl << "Writing : " << m_vnData[i];
        }
        cout << endl << "Writing done!!" << endl;
        SetEvent(hEvent);
        return 0;
    }

public:

    //Constructor
    DataOperations() {
        m_vnData.resize(10, 0);
    }

    //Destructor
    ~DataOperations() = default; // Destructor should automatically call the base destructor
};


// Inherited class to implement TCP Socket communication using threads.
class SocketManager : public ThreadManager {

private:

    char m_cBuffer[1024] = { 0 };

    DWORD Read() override {

        WSADATA wsaData;
        SOCKET sockClient;
        struct sockaddr_in serverAddr;

        // Initialize Winsock
        if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
            cout << " Startup failed!" << endl;
            return 1;
        }

        // Create socket
        if ((sockClient = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) == INVALID_SOCKET) {
            cout << " Socket creation failed!" << endl;
            WSACleanup();
            return 1;
        }

        // Server address struct
        serverAddr.sin_family = AF_INET;
        serverAddr.sin_addr.s_addr = inet_addr("127.0.0.1"); // Change to server IP
        serverAddr.sin_port = htons(5150); // Change to server port

        cout << "\n Client trying to connect server  ";

        // Connect to server
        if (connect(sockClient, (sockaddr*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR) {
            cout << " Connection failed." << endl;
            closesocket(sockClient);
            WSACleanup();
            return 1;
        }

        cout << "\n port number: " << serverAddr.sin_port;
        cout << " Connected to server!" << endl;

        // Read data from server
        while (int bytesReceived = recv(sockClient, m_cBuffer, sizeof(m_cBuffer), 0)) {
            if (bytesReceived == SOCKET_ERROR) {
                cout << "\n Receive failed." << endl;
            }
        }

        // Close socket
        closesocket(sockClient);
        SetEvent(hEvent);
        return 0;
    }

    // FUnction to print data read from a server using the Read() function above
    DWORD Write() override {
        WaitForSingleObject(hEvent, INFINITE);
        cout << "\n Data received from the server is : " << m_cBuffer << endl;
        return 0;
    }

public:
    //Constructor
    SocketManager() {
#define _WINSOCK_DEPRECATED_NO_WARNINGS
#pragma comment(lib, "Ws2_32.lib")

    }

    //Destructor
    ~SocketManager() {
        WSACleanup();
    }
};



int main(int argc, char* argv[]) {
    // Menu driven to provide all the functions to user
    do{
        cout << "\n *******Thread Implementation Functions******" << endl;
        cout << "\n\t  f : File ";            // For file operation
        cout << "\n\t  v : Vector ";          // For vector operation
        cout << "\n\t  s : Socket ";          // For reading from server
        cout << "\n\t  q : Exit " << endl;    // Exit from loop
        char cChoice;
        cout << "\n  Enter your choice : ";
        cin >> cChoice;
        
        switch(cChoice) {
            case 'f': {
                string strFilename;
                cout << "Enter File Name: ";
                cin >> strFilename;
                FileOperations fileObj(strFilename);
                fileObj.StartReadWriteThreads();
            }break;

            case 'v': {
                int nManagers;
                cout << "\n  Enter number of objects for thread: ";
                cin >> nManagers;
                vector<DataOperations> managers(nManagers);
                for (int i = 0; i < nManagers; ++i) {
                    managers[i].StartReadWriteThreads();
                }
            }break;

            case 's': {
                SocketManager sm;
                sm.StartReadWriteThreads();
            }break;

            case 'q': return 1;
            default: cout << "Invalid choice!";

        }
    }while (TRUE);
    return 0;
}



// IF WSASTRATUP NEEDED IN CONSTRUCTOR  

/*
SocketManager() {
        WSADATA wsaData;
        if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
            cout << "WSAStartup failed!" << endl;
            isWSAInitialized = false; // Set the flag to indicate failure
        } else {
            isWSAInitialized = true; // Set the flag to indicate successful initialization
            // Other initialization code if needed
        }
    }

    ~SocketManager() {
        if (isWSAInitialized) {
            WSACleanup();
        }
    }

    bool IsWSAInitializedSuccessfully() const {
        return isWSAInitialized;
    }

// MODIFICATIONS IN MAIN - 

SocketManager sm;
    if (!sm.IsWSAInitializedSuccessfully()) {
        // Handle the case when WSA initialization fails
        cout << "WSA initialization failed. Exiting." << endl;
        return 1; // Or any other suitable action
    }

    // Proceed with other operations using SocketManager...

    */