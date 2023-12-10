
#ifndef __IFMO_DISTRIBUTED_CLASS_CS_H
#define __IFMO_DISTRIBUTED_CLASS_CS_H

#include "banking.h"
#include "processes.h"
#include "ipc.h"
#include "lamport.h"

int request_cs(Process* this);
int release_cs(Process* this);

void create_message(Message *msg, MessageType type, void *contens, int len);


#endif