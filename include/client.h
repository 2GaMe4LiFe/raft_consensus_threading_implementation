#pragma once

#include <so_5/all.hpp>
#include <unordered_map>
#include "server.h"

class Client final : public so_5::agent_t {
public:
    Client(context_t ctx) : so_5::agent_t{std::move(ctx)} {}

    void so_define_agent() override {
        so_subscribe_self()
        .event(&Client::response_handler)
        .event(&Client::set_cluster_handler);
    }
private:
    std::string cluster_leader;
    std::unordered_map<std::string, so_5::mbox_t> m_mboxes;

    void set_cluster_handler(mhood_t<SetCluster> sc) {
        m_mboxes = sc->mboxes;
        std::cout << "works" << std::endl;
    };

    void response_handler(mhood_t<ServerResponse> sr) {
        std::cout << "Client: " << sr->resp << std::endl;
    }
};