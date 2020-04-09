#include <iostream>
#include <so_5/all.hpp>
#include <vector>
#include <unordered_map>
#include "server.h"

using namespace std;

int main() {
    so_5::launch([](so_5::environment_t& env) {
        so_5::mbox_t m;
        so_5::mbox_t m2;
        unordered_map<string, so_5::mbox_t> mboxes;
        env.introduce_coop([&](so_5::coop_t& coop) {
            auto server1 = coop.make_agent<raft_server>("server1");
            mboxes["server1"] = server1->so_direct_mbox();
            auto server2 = coop.make_agent<raft_server>("server2");
            mboxes["server2"] = server2->so_direct_mbox();
            auto server3 = coop.make_agent<raft_server>("server3");
            mboxes["server3"] = server3->so_direct_mbox();
            auto server4 = coop.make_agent<raft_server>("server4");
            mboxes["server4"] = server4->so_direct_mbox();
            auto server5 = coop.make_agent<raft_server>("server5");
            mboxes["server5"] = server5->so_direct_mbox();
        });


        for (auto el : mboxes) {
            so_5::send<raft_server::SetCluster>(el.second, mboxes);
            so_5::send<raft_server::change_state>(el.second);
        }
    });
    return 0;
}