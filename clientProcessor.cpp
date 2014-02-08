#include "clientProcessor.h"
#include "global.h"
#include <stdio.h>
#include <string.h>
#include <iostream>
#include <sstream>

using namespace std;

//TODO mailbox

int CClientProcessor::ProcessMessage(char* clientMessage, int read_size) {

    int responseType = 0; // default negative response

    char command[5] = "comm";

    if (read_size >= 4)
        strncpy(command, clientMessage, 4);

    if (strcasecmp(command, "USER") == 0) {

        responseType = User(clientMessage, read_size);

    } else if (strcasecmp(command, "PASS") == 0) {

        responseType = Pass(clientMessage, read_size);

    } else if (strcasecmp(command, "LIST") == 0) {

        responseType = List(clientMessage, read_size);

    } else if (strcasecmp(command, "QUIT") == 0) {

        responseType = Quit(clientMessage, read_size);

    }
    ostringstream ss;
    ss << responseType;
    string rt = ss.str();
    char *response = new char[rt.length() + 1];
    strcpy(response, rt.c_str());
    
    return Response(response);
}

int CClientProcessor::Response(char* type) {

    int write_size = (int) strlen(type);

    write(clientSock, type, write_size);

    if ((int)type == -1)
        return -1;

    return 0;
}

int CClientProcessor::User(char* clientMessage, int read_size) {

    //TODO implementation
    return 1; //default positive
}

int CClientProcessor::List(char* clientMessage, int read_size) {

    //TODO implementation
    return 1; //default positive
}

int CClientProcessor::Pass(char* clientMessage, int read_size) {

    //TODO implementation
    return 1; //default positive
}

int CClientProcessor::Retr(char* clientMessage, int read_size) {

    //TODO implementation
    return 1; //default positive
}

int CClientProcessor::Quit(char* clientMessage, int read_size) {

    //TODO implementation
    return -1; //quit
}
