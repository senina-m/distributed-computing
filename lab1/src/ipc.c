#include "ipc.h"
#include "processes.h"

int send(void* self, local_id dst, const Message* msg) {
	Process* this = (Process*) self;
	local_id src = this->id;
	size_t msg_len = sizeof(MessageHeader) + msg->s_header.s_payload_len;
	if (write(this->pipes[src][dst]->fw, msg, msg_len) == msg_len) {
		printf("Send message successfull from %d pipe to %d", src, dst);
		return 0;
	} else {
		printf("Can't send message from %d pipe to %d: return %ld", src, dst, msg_len);
		return -1;
	}	
}

int send_multicast(void* self, const Message* msg) {
	Process* this = (Process*) self;
	local_id src = this->id;
	Pipe** pipe_to_send = this->pipes[src];
	for (int i = 0; i < this->num_of_processes; i++) {
		if (i != src) {
			if (send(this, i, msg)) {
					printf("Can't send multicast message from %d", src);
			}
		}
	}
}
