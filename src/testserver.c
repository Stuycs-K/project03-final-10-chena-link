#include "testserver.h"

void test_main() {
    Server *server = server_new(111);
    server_run(server);
}