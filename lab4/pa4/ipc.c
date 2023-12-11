#include "ipc.h"
#include "processes.h"
#include "lamport.h"
#include <errno.h>

int send(void *self, local_id dst, const Message *msg) {
  Process *this = (Process *)self;
  local_id src = this->id;
  ssize_t msg_len = sizeof(MessageHeader) + msg->s_header.s_payload_len;
  ssize_t ret = write(this->pipes[dst][src]->fw, msg, msg_len);
  if (ret == msg_len) {
    // printf("Send message successfull from %d pipe to %d: %s\n", src, dst,
    // msg->s_payload);
    return 0;
  } else {
    printf("Can't send message from %d pipe to %d: return %ld, expect %ld\n",
           src, dst, ret, msg_len);
    perror("Can't send message");
    return -1;
  }
}

int send_multicast(void *self, const Message *msg) {
  Process *this = (Process *)self;
  local_id src = this->id;
  for (int i = 0; i < this->num_of_processes; i++) {
    if (i != src) {
      if (send(this, i, msg)) {
        printf("Can't send multicast message from %d\n", src);
      }
    }
  }
  return 0;
}

int receive(void *self, local_id from, Message *msg) {
  Process *this = (Process *)self;
  int listened_pipe = this->pipes[this->id][from]->fr;
  uint8_t is_read_header = 0;
  ssize_t bytes_to_read = sizeof(MessageHeader);
  while (1) {
    ssize_t read_count = 0;
    if (!is_read_header) {
      read_count = read(listened_pipe,
                        &msg->s_header + sizeof(MessageHeader) - bytes_to_read,
                        bytes_to_read);
      if (read_count == bytes_to_read) {
        is_read_header = 1;
        bytes_to_read = msg->s_header.s_payload_len;
        // printf("Read header from %d pipe to %d successfull: type %d,
        // size_payload %d\n", from, this->id, msg->s_header.s_type,
        // msg->s_header.s_payload_len);
      } else if (read_count > 0) {
        bytes_to_read -= read_count;
      } else if (errno != EAGAIN) {
        printf("Can't read from %d pipe to %d\n", from, this->id);
        perror("Can't receive header");
        return -1;
      }
    }
    if (is_read_header) {
      read_count =
          read(listened_pipe,
               msg->s_payload + msg->s_header.s_payload_len - bytes_to_read,
               bytes_to_read);
      if (read_count == bytes_to_read) {
        msg->s_payload[msg->s_header.s_payload_len] = 0;
        // printf("Receive message from %d pipe to %d successfull: %s\n", from,
        // this->id, msg->s_payload);
        return 0;
      } else if (read_count > 0) {
        bytes_to_read -= read_count;
      } else if (errno != EAGAIN) {
        printf("Can't read from %d pipe to %d\n", from, this->id);
        perror("Can't receive payload");
        return -1;
      }
    }
  }
}

int receive_any(void *self, Message *msg) {
  Process *this = (Process *)self;
  uint8_t is_chose_pipe = 0;
  uint8_t is_read_header = 0;
  local_id from = -1;
  ssize_t bytes_to_read = sizeof(MessageHeader);
  while (1) {
    // printf("DEBUG %i: try to read_any\n", this->id);
    ssize_t read_count = 0;
    if (!is_chose_pipe) {
      for (int i = 0; i < this->num_of_processes; i++) {
        if (i != this->id) {
          // printf("DEBUG %i: do read from %i\n", this->id, i);
          read_count = read(this->pipes[this->id][i]->fr, &msg->s_header,
                            sizeof(MessageHeader));
          // printf("DEBUG %i: do read from %i bytes=%i\n", this->id, i, read_count);
          if (read_count == bytes_to_read) {
            // printf("DEBUG %i: header from proc %i\n", this->id, i);
            is_chose_pipe = 1;
            from = i;
            is_read_header = 1;
            bytes_to_read = msg->s_header.s_payload_len;
            break;
          } else if (read_count > 0) {
            // printf("DEBUG %i: read %i bytes from proc %i\n", this->id, read_count, i);
            is_chose_pipe = 1;
            from = i;
            bytes_to_read -= read_count;
            // printf("DEBUG %i: is_chouse_pipe=%i from=%i bytes_to_read=%i is_read_header=%i\n", this->id, is_chose_pipe, from, bytes_to_read, is_read_header);
            break;
          } else if (errno != EAGAIN) {
            printf("Can't read any to %d pipe\n", this->id);
            return -1;
          }
        }
      }
    }
    // printf("DEBUG %i: is_chose_pipe=%i is_read_header=%i\n", this->id, is_chose_pipe, is_read_header);
    if (is_chose_pipe && !is_read_header) {
      // printf("DEBUG %i: read choose pipe and didn't finished read\n", this->id);
      read_count = read(this->pipes[this->id][from]->fr,
                        &msg->s_header + sizeof(MessageHeader) - bytes_to_read, bytes_to_read);
      if (read_count == bytes_to_read) {
        // printf("DEBUG %i: finished reading header\n", this->id);
        is_read_header = 1;
        bytes_to_read = msg->s_header.s_payload_len;
      } else if (read_count > 0) {
        // printf("DEBUG %i: didn't finished reading header\n", this->id);
        bytes_to_read -= read_count;
      } else if (errno != EAGAIN) {
        printf("Can't read from %d pipe to %d\n", from, this->id);
        return -1;
      }
    }
    if (is_chose_pipe && is_read_header) {
      read_count =
          read(this->pipes[this->id][from]->fr,
               msg->s_payload + msg->s_header.s_payload_len - bytes_to_read,
               bytes_to_read);
      if (read_count == bytes_to_read) {
        msg->s_payload[msg->s_header.s_payload_len] = 0;
        // printf("DEBUG %i: Receive message from %d pipe to %d successfull\n", this->id, from, this->id);
        return 0;
      } else if (read_count > 0) {
        bytes_to_read -= read_count;
      } else if (errno != EAGAIN) {
        printf("Can't read from %d pipe to %d\n", from, this->id);
        return -1;
      }
    }
  }
}
