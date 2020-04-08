#include <iostream>
#include <so_5/all.hpp>
#include "server.h"

int main() {
    so_5::launch([](so_5::environment_t& env) {
        so_5::mbox_t m;
        so_5::mbox_t m2;

        env.introduce_coop([&](so_5::coop_t& coop) {
            auto test = coop.make_agent<raft_server>();
            m = test->so_direct_mbox();
            auto test2 = coop.make_agent<raft_server>();
            m2 = test2->so_direct_mbox();
        });

        //so_5::send<raft_server::change_state>(m);
        so_5::send<raft_server::RequestVote>(m, "test1");
        so_5::send<raft_server::RequestVote>(m2, "test2");
        so_5::send<raft_server::RequestVote>(m2, "test3");
        so_5::send<raft_server::RequestVote>(m, "test4");
    });
    return 0;
}