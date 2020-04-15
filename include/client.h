#pragma once

#include <so_5/all.hpp>
#include <unordered_map>
#include <random>
#include "server.h"

struct SetMBox {
    so_5::mbox_t mbox;
};

class Client final : public so_5::agent_t {
public:
    Client(context_t ctx) : so_5::agent_t{std::move(ctx)} {}

    void so_define_agent() override {
        sending
        .event(&Client::response_handler)
        .event(&Client::set_cluster_handler)
        .event(&Client::set_mbox_handler);

        off.activate();

        off.time_limit(std::chrono::milliseconds{1000}, on);
        on.time_limit(std::chrono::milliseconds{1000}, off);

        on.on_enter([this] {
            if (m_cluster_leader == "" && m_mboxes.size() > 0) {
                std::random_device dev;
                std::mt19937 rng(dev());
                std::uniform_int_distribution dist(0, (int)m_mboxes.size());
                int idx = dist(rng);
                
                int cnt{0};

                for (auto el : m_mboxes) {
                    if (cnt == idx) {
                        std::cout << "Client: selected " << el.first << std::endl;
                        so_5::send<ClientRequest>(el.second, m_mbox, "hello server");
                        break;
                    }
                    cnt++;
                }
            } else if (m_cluster_leader != "" && m_mboxes.size() > 0) {
                so_5::send<ClientRequest>(m_mboxes[m_cluster_leader], m_mbox, "hello server2");
            }
        });
    }

private:
    state_t sending{this},
    on{substate_of(sending)},
    off{initial_substate_of(sending)};

    so_5::mbox_t m_mbox;
    std::string m_cluster_leader{""};
    std::unordered_map<std::string, so_5::mbox_t> m_mboxes;

    void set_mbox_handler(mhood_t<SetMBox> sm) {
        std::cout << "its here" << std::endl;
        m_mbox = sm->mbox;
    }

    void set_cluster_handler(mhood_t<SetCluster> sc) {
        m_mboxes = sc->mboxes;
    };

    void response_handler(mhood_t<ServerResponse> sr) {
        std::cout << "hmm" << std::endl;
        if (sr->status == 0) {
            m_cluster_leader = sr->leader;
        } else {
            std::cout << "Client: " << sr->resp << std::endl;
        }
    }
};