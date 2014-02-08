/* 
 * File:   ClientProcessor.h
 * Author: oriona
 *
 * Created on 7 luty 2014, 21:16
 */

#ifndef CLIENTPROCESSOR_H
#define	CLIENTPROCESSOR_H

class CClientProcessor{
    
private:
    int clientSock;
    int User(char *clientMessage, int read_size);
    int Pass(char *clientMessage, int read_size);
    int List(char *clientMessage, int read_size);
    int Retr(char *clientMessage, int read_size);
    int Quit(char *clientMessage, int read_size);
    
public:
    CClientProcessor(int clientS) : clientSock(clientS) {};
    int Response(char* type);
    int ProcessMessage(char *clientMessage, int read_size);
};

#endif	/* CLIENTPROCESSOR_H */

