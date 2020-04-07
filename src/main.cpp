#include <iostream>
#include <so_5/all.hpp>
#include "server.h"

int main() {
    so_5::launch([](so_5::environment_t& env) {
        so_5::mbox_t m;
        env.introduce_coop([&](so_5::coop_t& coop) {
            auto test = coop.make_agent<raft_server>();
            m = test->so_direct_mbox();
        });

        so_5::send<raft_server::change_state>(m);
    });
    return 0;
}