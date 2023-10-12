#include "ipc.h"
#include "processes.h"

int send(void* self, local_id dst, const Message* msg) {
	Process* this = (Process*) self;
	local_id src = this->id;
	size_t msg_len = sizeof(MessageHeader) + msg->s_header.s_payload_len;
	if (write(this->pipes[src][dst].fw, msg, msg_len) == msg_len) {
		return 0;
	} else {
		return -1;
	}	
}
