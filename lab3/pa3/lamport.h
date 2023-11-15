#ifndef __IFMO_DISTRIBUTED_CLASS_LAMPORT_TIME__H
#define __IFMO_DISTRIBUTED_CLASS_LAMPORT_TIME__H

#include "banking.h"
#include "processes.h"

timestamp_t get_lamport_time();

timestamp_t update_lamport_time(timestamp_t others_time);

timestamp_t inc_lamport_time();

int lamport_send(Process* this, local_id dst, Message * msg);
int lamport_send_multicast(Process* this, Message * msg);
int lamport_receive(Process* this, local_id from, Message * msg);
int lamport_receive_any(Process* this, Message * msg);
#endif
