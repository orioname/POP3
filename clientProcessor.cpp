#include "clientProcessor.h"
#include "global.h"
#include <stdio.h>
#include <string.h>
#include <iostream>

using namespace std;

//TODO mailbox

int CClientProcessor::ProcessMessage(char* clientMessage, int read_size) {

    int responseType = 502; // command not implemented

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
    } else if (strcasecmp(command, "QUIT") == 0) {

        Quit(clientMessage, read_size);
		return -1;
    }
	return 0;
}
void CClientProcessor::WriteToSocket(char* serverMessage)
{
    write(clientSock, serverMessage, (int) strlen(serverMessage));
}
void CClientProcessor::User(char* clientMessage, int read_size) {
    if (serverState != 0)
	{
		WriteToSocket("-ERR Not in AUTH state\r\n");
		return;
	}
	if (read_size == 4)
	{
		WriteToSocket("-ERR Could not find user\r\n"); //actually user not provided
		return;
	}
	char* username = clientMessage + 5; //cut USER[sp]
	userDirectory = opendir(username); //TODO "./" + username?
	strcpy(userDirectoryName, username);
	if (userDirectory == NULL)
		WriteToSocket("-ERR Could not find user\r\n");
	else
		WriteToSocket("+OK User set\r\n");
}

//TODO maybe just make it a stub for now?
void CClientProcessor::Pass(char* clientMessage, int read_size) {

    if (serverState != 0)
	{
		WriteToSocket("-ERR Not in AUTH state\r\n");
		return;
	}
	if (read_size == 4)
	{
		WriteToSocket("-ERR Could not find user\r\n"); //actually user not provided
		return;
	}
	if (userDirectory == NULL)
	{
		WriteToSocket("-ERR USER not set\r\n");
		return;
	}
	char* password = clientMessage + 5; //cut PASS[sp]
	dirent* fileEntity = NULL;
	while (fileEntity = readdir(userDirectory))
	{
		if (strcmp(fileEntity->d_name, "pass") == 0)
		{
			char filenameBuffer[1024];
			char passwordBuffer[1024];
			filenameBuffer[0] = '\0';
			strcat(filenameBuffer, userDirectoryName);
			strcat(filenameBuffer, "/");
			strcat(filenameBuffer, fileEntity->d_name);
			FILE* passwordFile = fopen(filenameBuffer, "r");
			//assert(passwordFile != NULL)
			fgets(passwordBuffer, 1024, passwordFile);
			fclose(passwordFile);
			rewinddir(userDirectory);
			//TODO what to do with fileEntity?
			free(fileEntity);
			if (strcmp(password, passwordBuffer) == 0)
			{
				serverState = 1;
				WriteToSocket("+OK User authenticated\r\n");
				return;
			}
			else
			{
				WriteToSocket("-ERR Wrong password\r\n");
				return;
			}
		}
		//TODO again, what do we do here?
		free(fileEntity);
		fileEntity = NULL;
	}
	rewinddir(userDirectory);
	WriteToSocket("-ERR Password file not found\r\n");
}

void CClientProcessor::Stat(char* clientMessage, int read_size) {

    if (serverState != 1)
	{
		WriteToSocket("-ERR Not in TRANSACTION state\r\n");
		return;
	}
	dirent* fileEntity = NULL;
	int totalMessages = 0;
	int totalOctets = 0;
	while (fileEntity = readdir(userDirectory))
	{
		if (strcmp(fileEntity->d_name, ".") != 0 && strcmp(fileEntity->d_name, "..") != 0 && strcmp(fileEntity->d_name, "pass") != 0)
		{
			totalMessages++;
			totalOctets += fileEntity->d_reclen;
		}
		free(fileEntity);
		fileEntity = NULL;
	}
	rewinddir(userDirectory);
	char responseBuffer[128];
	sprintf(responseBuffer, "+OK %d %d\r\n", totalMessages, totalOctets);
	WriteToSocket(responseBuffer);
}

//TODO implement LIST x
void CClientProcessor::List(char* clientMessage, int read_size)
{
	int index = 0;
	if (serverState != 1)
	{
		WriteToSocket("-ERR Not in TRANSACTION state\r\n");
		return;
	}
	WriteToSocket("+OK List follows\r\n");
	dirent* fileEntity = NULL;
	char linebuffer[128];
	while (fileEntity = readdir(userDirectory))
	{
		if (strcmp(fileEntity->d_name, ".") != 0 && strcmp(fileEntity->d_name, "..") != 0 && strcmp(fileEntity->d_name, "pass") != 0)
		{
			sprintf(linebuffer, "%s %d\r\n", fileEntity->d_name, fileEntity->d_reclen);
			WriteToSocket(linebuffer);
		}
		free(fileEntity);
		fileEntity = NULL;
	}
	rewinddir(userDirectory);
	WriteToSocket(".\r\n");
}

void CClientProcessor::Retr(char* clientMessage, int read_size)
{
	if (serverState != 1)
	{
		WriteToSocket("-ERR Not in TRANSACTION state\r\n");
		return;
	}
	if (read_size == 4)
	{
		WriteToSocket("-ERR Message not found\r\n"); //actually mid not provided
		return;
	}
	char* messageId = clientMessage + 5;
	dirent* fileEntity = NULL;
	bool found = false;
	while (fileEntity = readdir(userDirectory))
	{
		if (strcmp(fileEntity->d_name, messageId) == 0)
		{
			found = true;
			WriteToSocket("+OK Message follows\r\n");
			char filenameBuffer[1024];
			filenameBuffer[0] = '\0';
			strcat(filenameBuffer, userDirectoryName);
			strcat(filenameBuffer, "/");
			strcat(filenameBuffer, fileEntity->d_name);
			FILE* messageFile = fopen(filenameBuffer, "r");
			//assert(passwordFile != NULL)
			char lineBuffer[1024];
			while (fgets(lineBuffer, 1024, messageFile))
			{
				WriteToSocket(lineBuffer);
			}
		}
		free(fileEntity);
		fileEntity = NULL;
	}
	rewinddir(userDirectory);
	if (found)
	{
		WriteToSocket("\r\n.\r\n");
	}
	else
	{
		WriteToSocket("-ERR Message not found\r\n");
	}
}
void CClientProcessor::Quit(char* clientMessage, int read_size)
{
	if (serverState == 1) serverState = 2;
	WriteToSocket("+OK See you soon\r\n");
	//TODO remove mails if serverState == 2
}