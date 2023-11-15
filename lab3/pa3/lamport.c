
#include "lamport.h"

timestamp_t now_time = 0;

timestamp_t get_lamport_time(){
    return now_time;
}

timestamp_t update_lamport_time(timestamp_t others_time){
    if (others_time > now_time) now_time = others_time;
    return ++now_time;
}

timestamp_t inc_lamport_time(){
    now_time++;
    return now_time;
}

int lamport_send(Process* this, local_id dst, Message * msg){
    msg->s_header.s_local_time = inc_lamport_time();
    return send(this, dst, msg);
}

int lamport_send_multicast(Process*this, Message *msg) {
    msg->s_header.s_local_time = inc_lamport_time();
    return send_multicast(this, msg);

}

int lamport_receive(Process*this, local_id from, Message *msg) {

    int ret = receive(this, from, msg);
    msg->s_header.s_local_time = update_lamport_time(msg->s_header.s_local_time);
    return ret;
}

int lamport_receive_any(Process*this, Message *msg) {

    int ret = receive_any(this, msg);
    msg->s_header.s_local_time = update_lamport_time(msg->s_header.s_local_time);
    return ret;

}
