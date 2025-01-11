#include "baseclient.h"

BaseClient *base_client_new() {
    BaseClient *this = malloc(sizeof(BaseClient));
    this->client_id = -1;
    this->from_server_fd = -1;
    this->to_server_fd = -1;

    this->recv_queue = net_event_queue_new();
    this->send_queue = net_event_queue_new();

    return this;
}