#include <iostream>
#include <so_5/all.hpp>
#include <vector>
#include <unordered_map>
#include "server.h"
#include "client.h"

using namespace std;

int main() {
    so_5::launch([](so_5::environment_t& env) {
        so_5::mbox_t client_inbox;

        auto client_disp = so_5::disp::thread_pool::make_dispatcher(env, 1);
        auto server_disp = so_5::disp::thread_pool::make_dispatcher(env, 6);

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

            client_inbox = coop.make_agent<Client>()->so_direct_mbox();
        });

        so_5::send<SetCluster>(client_inbox, mboxes);
        so_5::send<SetMBox>(client_inbox, client_inbox);

        for (auto el : mboxes) {
            so_5::send<SetCluster>(el.second, mboxes);
            so_5::send<raft_server::change_state>(el.second);
        }

        
        this_thread::sleep_for(chrono::milliseconds{2000});
        so_5::send<ClientRequest>(mboxes["server2"], client_inbox, "hello world");
        std::cout << "???" << std::endl;
    });
    return 0;
}