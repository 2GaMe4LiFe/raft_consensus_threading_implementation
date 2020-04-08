#include <iostream>
#include <so_5/all.hpp>
#include <vector>
#include "server.h"

using namespace std;

int main() {
    so_5::launch([](so_5::environment_t& env) {
        so_5::mbox_t m;
        so_5::mbox_t m2;
        vector<so_5::mbox_t> mboxes;
        env.introduce_coop([&](so_5::coop_t& coop) {
            auto test = coop.make_agent<raft_server>();
            mboxes.push_back(test->so_direct_mbox());
            auto test2 = coop.make_agent<raft_server>();
            mboxes.push_back(test2->so_direct_mbox());
        });


        for (auto el : mboxes) {
            so_5::send<raft_server::SetCluster>(el, mboxes);
            so_5::send<raft_server::change_state>(el);
        }
    });
    return 0;
}