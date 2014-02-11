/* 
 * File:   ClientProcessor.h
 * Author: oriona
 *
 * Created on 7 luty 2014, 21:16
 */

#ifndef CLIENTPROCESSOR_H
#define	CLIENTPROCESSOR_H
#include <dirent.h>
class CClientProcessor{
    
private:
    int clientSock;
	DIR* userDirectory;
	char userDirectoryName[256];
	int serverState; //0 - auth; 1 - trans; 2 - update; TODO make it into an enum
    void WriteToSocket(char* serverMessage);
    void User(char* clientMessage, int read_size);
    void Pass(char* clientMessage, int read_size);
    void Stat(char* clientMessage, int read_size);
	void List(char* clientMessage, int read_size);
	void Retr(char* clientMessage, int read_size);
	void Quit(char* clientMessage, int read_size);
        void Dele(char* clientMessage, int read_size);
        void Top(char* clientMessage, int read_size);
        
    
public:
    CClientProcessor(int clientS) : clientSock(clientS), serverState(0) {};
    int ProcessMessage(char *clientMessage, int read_size);
};

#endif	/* CLIENTPROCESSOR_H */

