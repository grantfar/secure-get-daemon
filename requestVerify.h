
#ifndef REQUESTVER
#define REQUESTVER
#include "requests.h"

#define TP0_LENMATCH 1
#define TP0_DNLEN33P 2
#define TP0_NONULL 4
#define TP0_MESSLEN 8

unsigned int type0Ver(MessageType0 * message);
unsigned int type3Ver(MessageType3 * message);
unsigned int type6Ver(MessageType6 * message);

#endif