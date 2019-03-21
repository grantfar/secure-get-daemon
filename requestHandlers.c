/*
//CS-5750-100 - Secure Software Development
//Program 2: getd
//Authors:
//  Grant Farnsworth
//  Joshua Sziede
*/
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include "requests.h"
#include "requestHandlers.h"
#include "requestVerify.h"
#include <string.h>

char pathName[4097];
FILE *ptr == NULL;
int eof = -1;

//establish session between get and getd with a unique session id.
int MessageType0Handler(MessageType0 * messageType0, State * state)
{   
    //if a message type 0 was not the first message type received
    if (state->lastSent != '5' || state->lastSent != '2')
    {
        return -2;
    }
    //check contents of message to ensure it is secure
    int ver = type0Ver(messageType0);
    //the length of the unique session id string
    int sessionIdLength = 128;
    //if the message is in proper form
    if (ver == 0)
    {
        //store the username so we can later check if the requested file belongs to the user
        state->userName = messageType0->distinguishedName;
        state->sessionId = malloc(sizeof(char) * 129);
        //generates random string using the chars found in possibleChars and assigns the string to sessionId
        char possibleChars[] = "0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ";
        while (sessionIdLength > 0)
        {
            size_t index = (double) rand() / RAND_MAX * (sizeof possibleChars - 1);
            state->sessionId[sessionIdLength] = possibleChars[index];
            sessionIdLength = sessionIdLength - 1;
        }
        state->sessionId[129] = '\0';
        state->lastRecieved = '0';
        return (sizeof(int) + (sizeof(char) * 129));
    }
    //could return different negative value based on ver 
    else
    {
        return -1;
    }
    
}

//request from get to receive a file from getd
int MessageType3Handler(MessageType3 * messageType3, State * state, void * outMessage)
{
    //the requested file to be sent from getd
    static FILE * sendFile;
    //check if message is in proper form
    int ver = type3Ver(messageType3);

    //if getd was not expecting a message of type 3
    if(state.lastSent != '1' || ver > 0 || eof >= 0)
    {
        return -1;
    }

    //copies the received path name into the global path name so it can be used for multiple functions
    strncpy(pathName, messageType3->pathName, messageType3->pathLength);
    state->lastRecieved = '3';
    return 0;
}

//ackowledgement from get that it received a type 4 message from getd
int MessageType6Handler(MessageType6 * messageType6, State * state, void * outMessage)
{
    int ver = type6Ver(messageType6, state);
    //type 6 messages are acks of types 4 and 5 messages
    if (state->lastSent != '4' || state->lastSent != '5')
    {
        return -1;
    }

    if (ver == 0)
    {
        //if end of file was reached, terminate connection with a type 5 message
        state->lastRecieved = '6';
        if (eof == 1)
        {
            return 5;
        }
        //else continue copying the file contents
        else
        {
            return 4;
        }
        
    }
    //if the type 6 message was malformed
    else
    {
        return -1;
    }
}

//builds a type 1 message given a string containing the error message
MessageType1 MessageType1Builder(int messageSize)
{
    //build the header
    Header t1Header;
    t1Header.messageType = '1';
    t1Header.messageLength = messageSize;

    MessageType1 t1;
    t1.header = t1Header;
    //gets the length of the session id since outSize is sidLength + sizeof(int)
    t1.sidLength = messageSize - sizeof(int);
    //copy session id generated from the type 0 handler into the type 1 message
    strncpy(t1.sessionId, state.sessionId, sidLength);
    return t1;
}

//builds a type 2 error message given a string containing the error message
MessageType2 MessageType2Builder(char *errorMsg)
{
    //gets the length of the error message
    int errorMsgLen = strlen(errorMsg);

    //message header
    Header t2Head;
    t2Head.messageLength = errorMsgLen + sizeof(int) + 1;
    t2Head.messageType = '2';

    MessageType2 t2;
    t2.header = t2Head;
    t2.msgLength = errorMsgLen;
    strncpy(t2.errorMessage, errorMsg, errorMsgLen);
    t2.errorMessage[errorMsgLen + 1] = '\0';
    return t2;
}

//builds a type 4 message by opening a file and copying its contents
MessageType4 MessageType4Builder()
{
    //if a file has not been opened
    if (ptr == NULL)
    {
        //attempt to open file
        ptr = fopen(pathName, "r");
        if (ptr == NULL) {
            return -1;
        }
    }

    //stores one character at a time to be read from the file
    char ch;
    //counts how many characters have been read from the file
    int index = 0;
    //stores the copied contents of the file
    char buffer[4096];
    //determines whether or not to keep looping during the file copy
    int loop = 1;
    //set end of file to 0 to indicate a file has been opened but not fully read
    eof = 0;
    //each loop copies one character from the file until end of file or buffer is full
    while (loop)
    {
        //gets the current character from the file
        ch = fgetc(ptr);
        //if end of file
        if (ch == EOF)
        {
            //set eof bit and end the loop
            eof = 1;
            loop = 0;
        //else if buffer is full, stop the loop
        } else if (index == 4095) {
            loop = 0;
        }
        //store the file character into the buffer
        buffer[index] = ch;
    }

    //gets length of the session id string
    int sidLen = strnlen(state->sessionId, 128);

    //create header for message type 4
    Header t4Head;
    t4Head.messageLength = index + sidLen + (sizeof(int) * 2) + 1;
    t4Head.messageType = '4';

    //build type 4 message
    MessageType4 t4;
    t4.header = t4Head;
    t4.sidLength = sidLen;
    strncpy(t4.sessionId, state->sessionId, sidLen);
    t4.sessionId[sidLen + 1] = '\0';
    t4.contentLength = index;
    strncpy(t4.contentBuffer, buffer, index);
    return t4;
}

//builds a type 5 message given the connection state for the session id
MessageType5 MessageType5Builder(State * state)
{
    //gets the length of the error message
    int sidLen = strnlen(state->sessionId, 129);

    //message header
    Header t5Head;
    t5Head.messageLength = sidLen + sizeof(int) + 1;
    t5Head.messageType = '5';
    
    MessageType5 t5;
    t5.header = t5Head;
    t5.sidLength = sidLen;
    strncpy(t5.sessionId, state->sessionId, sidLen);
    t5.sessionId[sidLen + 1] = '\0';
    return t5;
}
