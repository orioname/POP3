#include "clientProcessor.h"
#include "global.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <iostream>

using namespace std;

//TODO mailbox

int CClientProcessor::ProcessMessage(char* clientMessage, int read_size) {

    int responseType = 502; // command not implemented
    int length = strlen(clientMessage);
	clientMessage[length-1] = '\0';
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
		WriteToSocket("-ERR User not provided\r\n"); //actually user not provided
		return;
	}
	char* username = clientMessage + 5; //cut USER[sp]
	//WriteToSocket(username);
	char path[512];
	path[0] = '\0';
	strcat(path, "./");
	strcat(path, username);
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
			int length = strlen(passwordBuffer);
			passwordBuffer[length-1] = '\0';
			fclose(passwordFile);
			rewinddir(userDirectory);
			//TODO what to do with fileEntity?
			//free(fileEntity);
			if (strcmp(password, passwordBuffer) == 0)
			{
				serverState = 1;
				WriteToSocket("+OK User authenticated\r\n");
				return;
			}
			else
			{
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
		//free(fileEntity);
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
		//free(fileEntity);
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
	for (int i = 0; i < strlen(clientMessage); i++)
	{
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
			char lineBuffer[1024];
			while (fgets(lineBuffer, 1024, messageFile))
			{
				WriteToSocket(lineBuffer);
			}
		}
		//free(fileEntity);
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
void CClientProcessor::Top(char* clientMessage, int read_size)
{
	if (serverState != 1)
	{
		WriteToSocket("-ERR Not in TRANSACTION state\r\n");
		return;
	}
	char mId[256];
	mId[0] = '\0';
	char amount[256];
	amount[256] = '\0';
	int i = 0;
	char* ptr = clientMessage + 4;
	while(*ptr != ' ')
	{
		mId[i] = *ptr;
		i++;
		ptr++;
	}
	ptr++;
	mId[i] = '\0';
	i = 0;
	while(*ptr != '\r' && *ptr != '\n')
	{
		amount[i] = *ptr;
		i++;
		ptr++;
	}
	amount[i] = '\0';
	dirent* fileEntity = NULL;
	bool found = false;
	while (fileEntity = readdir(userDirectory))
	{
		if (strcmp(fileEntity->d_name, mId) == 0)
		{
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
			while (fgets(lineBuffer, 1024, messageFile) && i > 0)
			{
				WriteToSocket(lineBuffer);
				i--; 
			}
		}
		//free(fileEntity);
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
void CClientProcessor::Dele(char* clientMessage, int read_size)
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
	for (int i = 0; i < strlen(clientMessage); i++)
	{
		if (clientMessage[i] == '\r' || clientMessage[i] == '\n')
			clientMessage[i] = '\0';
	}
	char* messageId = clientMessage + 5;
	dirent* fileEntity = NULL;
	bool found = false;
	while (fileEntity = readdir(userDirectory))
	{
		if (strcmp(fileEntity->d_name, messageId) == 0)
		{
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
	if (!found)
	{
		WriteToSocket("-ERR Message not found\r\n");
	}
}
void CClientProcessor::Quit(char* clientMessage, int read_size)
{
	if (serverState == 1) serverState = 2;
	WriteToSocket("+OK See you soon\r\n");
	if (serverState == 2)
	{
		dirent* fileEntity = NULL;
		while (fileEntity = readdir(userDirectory))
		{
			if (strstr(fileEntity->d_name, "d") != NULL)
			{
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
