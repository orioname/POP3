#include "clientProcessor.h"
#include "global.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <iostream>
#include <vector>
#include <errno.h>
#include <sys/stat.h>

using namespace std;

//TODO mailbox

int CClientProcessor::ProcessMessage(char* clientMessage, int read_size) {

    //cout << "client message" << clientMessage << endl;
    
    int responseType = 502; // command not implemented
    int length = strlen(clientMessage);
    clientMessage[length - 1] = '\0';
    char command[5] = "comm";
    if (read_size >= 4)
        strncpy(command, clientMessage, 4);

    if (strcasecmp(command, "USER") == 0) {

        User(clientMessage, read_size);

    } else if (strcasecmp(command, "PASS") == 0) {

        Pass(clientMessage, read_size);

    } else if (strcasecmp(command, "STAT") == 0) {

        Stat(clientMessage, read_size);

    } else if (strcasecmp(command, "LIST") == 0) {

        List(clientMessage, read_size);

    } else if (strcasecmp(command, "RETR") == 0) {

        Retr(clientMessage, read_size);
        //TODO DELE command, TOP command
    } else if (strcasecmp(command, "DELE") == 0) {

        Dele(clientMessage, read_size);

    } else if (strcasecmp(command, "TOP ") == 0) {

        Top(clientMessage, read_size);

    } else if (strcasecmp(command, "QUIT") == 0) {

        Quit(clientMessage, read_size);
        return -1;
    }
    return responseType;
}

void CClientProcessor::WriteToSocket(char* serverMessage) {
    write(clientSock, serverMessage, (int) strlen(serverMessage));
}

int getdir(string dir, vector<string> &files) {
    DIR *dp;
    struct dirent *dirp;
    

    
    if ((dp = opendir(dir.c_str())) == NULL) {
                cout << "Error(" << errno << ") opening " << dir << endl;
                return errno;
    }

    
    while ((dirp = readdir(dp)) != NULL) {
        files.push_back(string(dirp->d_name));
    }

    cout << "close dir " << dir << endl; 
    closedir(dp);
 cout << "closed dir " << dir << endl; 
    return 0;
}


void CClientProcessor::User(char* clientMessage, int read_size) {
    
    cout << "Received USER"  <<  clientMessage <<endl;
   
    
    if (serverState != 0) {
        WriteToSocket("-ERR Not in AUTH state\r\n");
        return;
    }
    if (read_size == 4) {
        WriteToSocket("-ERR User not provided\r\n"); //actually user not provided
        return;
    }
        
    char* username = clientMessage + 5; //cut USER[sp]
    for (int i = 0; i < strlen(username); i++){
        if (username[i] == '\r' || username[i] == '\n'){
            username[i] = 0;
        }
            
    }
    
    
    
    //WriteToSocket(username);
    //char path[512];
    //path[0] = '\0';
    //strcat(path, "./");
    //strcat(path, username);

    cout << "Received USER "  <<  username <<endl;
    
    vector<string> files;
    vector<string> domains;
    getdir(".", files);

    int status;
    struct stat st_buf;

    for (unsigned int i = 0; i < files.size(); i++) {
        status = stat(files.at(i).c_str(), &st_buf);
        if (status != 0) {
            printf("Error, errno = %d\n", errno);
        }

        if (S_ISDIR(st_buf.st_mode)) {
            if (strstr (files.at(i).c_str(),".") == 0){
               domains.push_back(files.at(i));
            }
        }
    }
    
    
    string usrFound = "u";
    string domain = "d";
    bool found = false;
    
    cout << "seek users" <<endl;
    
    for (unsigned int i = 0; i < domains.size(); i++) {
        vector<string> users;
        string dir = string("./") + domains.at(i) + string("/");
        getdir(dir, users);
        
        cout << "loop" <<endl;
        
        for (unsigned int j = 0; j < users.size(); j++){
            if (strcmp(username, users.at(j).c_str()) == 0){
               printf("%s user found!.\n", users.at(j).c_str()); 
               //strcpy(usrFound, users.at(j).c_str());
               //strcpy(domain, domains.at(i).c_str());
               cout << "assign" <<endl;
               usrFound = users.at(j).c_str();
               domain = domains.at(i).c_str();
               found = true;
               break;
            }
        }
        if (found)
            break;
    }

    //userDirectory = opendir(username); //TODO "./" + username?
    string uDir = string("./") + string(domain) + string("/") + string(usrFound) + string("/mbox/");
    userDirectory = opendir(uDir.c_str());
    strcpy(userDirectoryName, uDir.c_str());
    if (userDirectory == NULL)
        WriteToSocket("-ERR Could not find user\r\n");
    else
        WriteToSocket("+OK User set\r\n");
}

//TODO maybe just make it a stub for now?

void CClientProcessor::Pass(char* clientMessage, int read_size) {

    cout << "Received PASS" << endl;
    
    if (serverState != 0) {
        WriteToSocket("-ERR Not in AUTH state\r\n");
        return;
    }
    if (read_size == 4) {
        WriteToSocket("-ERR Could not find user\r\n"); //actually user not provided
        return;
    }
    if (userDirectory == NULL) {
        WriteToSocket("-ERR USER not set\r\n");
        return;
    }
    char* password = clientMessage + 5; //cut PASS[sp]
        for (int i = 0; i < strlen(password); i++){
        if (password[i] == '\r' || password[i] == '\n'){
            password[i] = 0;
        }
            
    }
    dirent* fileEntity = NULL;
    while (fileEntity = readdir(userDirectory)) {
        if (strcmp(fileEntity->d_name, "pass") == 0) {
            char filenameBuffer[1024];
            char passwordBuffer[1024];
            filenameBuffer[0] = '\0';
            strcat(filenameBuffer, userDirectoryName);
            strcat(filenameBuffer, "/");
            strcat(filenameBuffer, fileEntity->d_name);
            FILE* passwordFile = fopen(filenameBuffer, "r");
            //assert(passwordFile != NULL)
            fgets(passwordBuffer, 1024, passwordFile);
            int length = strlen(passwordBuffer);
            passwordBuffer[length - 1] = '\0';
            fclose(passwordFile);
            rewinddir(userDirectory);
            //TODO what to do with fileEntity?
            //free(fileEntity);
            if (strcmp(password, passwordBuffer) == 0) {
                serverState = 1;
                WriteToSocket("+OK User authenticated\r\n");
                return;
            } else {
                //WriteToSocket(password);
                //WriteToSocket("\r\n");
                //WriteToSocket(passwordBuffer);
                //WriteToSocket("\r\n");
                WriteToSocket("-ERR Wrong password\r\n");
                return;
            }
        }
        //TODO again, what do we do here?
        //free(fileEntity);
        fileEntity = NULL;
    }
    rewinddir(userDirectory);
    WriteToSocket("-ERR Password file not found\r\n");
}

void CClientProcessor::Stat(char* clientMessage, int read_size) {

    if (serverState != 1) {
        WriteToSocket("-ERR Not in TRANSACTION state\r\n");
        return;
    }
    dirent* fileEntity = NULL;
    int totalMessages = 0;
    int totalOctets = 0;
    while (fileEntity = readdir(userDirectory)) {
        if (strcmp(fileEntity->d_name, ".") != 0 && strcmp(fileEntity->d_name, "..") != 0 && strcmp(fileEntity->d_name, "pass") != 0) {
            totalMessages++;
            totalOctets += fileEntity->d_reclen;
        }
        //free(fileEntity);
        fileEntity = NULL;
    }
    rewinddir(userDirectory);
    char responseBuffer[128];
    sprintf(responseBuffer, "+OK %d %d\r\n", totalMessages, totalOctets);
    WriteToSocket(responseBuffer);
}

//TODO implement LIST x

void CClientProcessor::List(char* clientMessage, int read_size) {
    int index = 0;
    if (serverState != 1) {
        WriteToSocket("-ERR Not in TRANSACTION state\r\n");
        return;
    }
    WriteToSocket("+OK List follows\r\n");
    dirent* fileEntity = NULL;
    char linebuffer[128];
    while (fileEntity = readdir(userDirectory)) {
        if (strcmp(fileEntity->d_name, ".") != 0 && strcmp(fileEntity->d_name, "..") != 0 && strcmp(fileEntity->d_name, "pass") != 0) {
            sprintf(linebuffer, "%s %d\r\n", fileEntity->d_name, fileEntity->d_reclen);
            WriteToSocket(linebuffer);
        }
        //free(fileEntity);
        fileEntity = NULL;
    }
    rewinddir(userDirectory);
    WriteToSocket(".\r\n");
}

void CClientProcessor::Retr(char* clientMessage, int read_size) {
    if (serverState != 1) {
        WriteToSocket("-ERR Not in TRANSACTION state\r\n");
        return;
    }
    if (read_size == 4) {
        WriteToSocket("-ERR Message not found\r\n"); //actually mid not provided
        return;
    }
    for (int i = 0; i < strlen(clientMessage); i++) {
        if (clientMessage[i] == '\r' || clientMessage[i] == '\n')
            clientMessage[i] = '\0';
    }
    char* messageId = clientMessage + 5;
    //WriteToSocket(clientMessage);
    //WriteToSocket("\r\n");
    //WriteToSocket(messageId);
    //WriteToSocket("\r\n");
    dirent* fileEntity = NULL;
    bool found = false;
    while (fileEntity = readdir(userDirectory)) {
        if (strcmp(fileEntity->d_name, messageId) == 0) {
            found = true;
            WriteToSocket("+OK Message follows\r\n");
            char filenameBuffer[1024];
            filenameBuffer[0] = '\0';
            strcat(filenameBuffer, userDirectoryName);
            strcat(filenameBuffer, "/");
            strcat(filenameBuffer, fileEntity->d_name);
            FILE* messageFile = fopen(filenameBuffer, "r");
            char lineBuffer[1024];
            while (fgets(lineBuffer, 1024, messageFile)) {
                WriteToSocket(lineBuffer);
            }
        }
        //free(fileEntity);
        fileEntity = NULL;
    }
    rewinddir(userDirectory);
    if (found) {
        WriteToSocket("\r\n.\r\n");
    } else {
        WriteToSocket("-ERR Message not found\r\n");
    }
}

void CClientProcessor::Top(char* clientMessage, int read_size) {
    if (serverState != 1) {
        WriteToSocket("-ERR Not in TRANSACTION state\r\n");
        return;
    }
    char mId[256];
    mId[0] = '\0';
    char amount[256];
    amount[256] = '\0';
    int i = 0;
    char* ptr = clientMessage + 4;
    while (*ptr != ' ') {
        mId[i] = *ptr;
        i++;
        ptr++;
    }
    ptr++;
    mId[i] = '\0';
    i = 0;
    while (*ptr != '\r' && *ptr != '\n') {
        amount[i] = *ptr;
        i++;
        ptr++;
    }
    amount[i] = '\0';
    dirent* fileEntity = NULL;
    bool found = false;
    while (fileEntity = readdir(userDirectory)) {
        if (strcmp(fileEntity->d_name, mId) == 0) {
            found = true;
            WriteToSocket("+OK Message follows\r\n");
            char filenameBuffer[1024];
            filenameBuffer[0] = '\0';
            strcat(filenameBuffer, userDirectoryName);
            strcat(filenameBuffer, "/");
            strcat(filenameBuffer, fileEntity->d_name);
            FILE* messageFile = fopen(filenameBuffer, "r");
            char lineBuffer[1024];
            int i = atoi(amount) + 5;
            while (fgets(lineBuffer, 1024, messageFile) && i > 0) {
                WriteToSocket(lineBuffer);
                i--;
            }
        }
        //free(fileEntity);
        fileEntity = NULL;
    }
    rewinddir(userDirectory);
    if (found) {
        WriteToSocket("\r\n.\r\n");
    } else {
        WriteToSocket("-ERR Message not found\r\n");
    }
}

void CClientProcessor::Dele(char* clientMessage, int read_size) {
    if (serverState != 1) {
        WriteToSocket("-ERR Not in TRANSACTION state\r\n");
        return;
    }
    if (read_size == 4) {
        WriteToSocket("-ERR Message not found\r\n"); //actually mid not provided
        return;
    }
    for (int i = 0; i < strlen(clientMessage); i++) {
        if (clientMessage[i] == '\r' || clientMessage[i] == '\n')
            clientMessage[i] = '\0';
    }
    char* messageId = clientMessage + 5;
    dirent* fileEntity = NULL;
    bool found = false;
    while (fileEntity = readdir(userDirectory)) {
        if (strcmp(fileEntity->d_name, messageId) == 0) {
            found = true;
            WriteToSocket("+OK Message marked to delete\r\n");
            char filenameBuffer[1024];
            filenameBuffer[0] = '\0';
            strcat(filenameBuffer, userDirectoryName);
            strcat(filenameBuffer, "/");
            strcat(filenameBuffer, fileEntity->d_name);
            char newFName[1024];
            strcpy(newFName, filenameBuffer);
            strcat(newFName, "d");
            rename(filenameBuffer, newFName);
        }
        //free(fileEntity);
        fileEntity = NULL;
    }
    rewinddir(userDirectory);
    if (!found) {
        WriteToSocket("-ERR Message not found\r\n");
    }
}

void CClientProcessor::Quit(char* clientMessage, int read_size) {
    if (serverState == 1) serverState = 2;
    WriteToSocket("+OK See you soon\r\n");
    if (serverState == 2) {
        dirent* fileEntity = NULL;
        while (fileEntity = readdir(userDirectory)) {
            if (strstr(fileEntity->d_name, "d") != NULL) {
                char filenameBuffer[1024];
                filenameBuffer[0] = '\0';
                strcat(filenameBuffer, userDirectoryName);
                strcat(filenameBuffer, "/");
                strcat(filenameBuffer, fileEntity->d_name);
                unlink(filenameBuffer);
            }
            //free(fileEntity);
            fileEntity = NULL;
        }
        rewinddir(userDirectory);
    }
}
