#pragma once

#include <so_5/all.hpp>
#include <random>
#include <unordered_map>
#include <vector>
#include <cmath>

struct ClientRequest {
    so_5::mbox_t inbox;
    std::string cmd;
};

struct ServerResponse {
    int status; // 0 or 1 if 0 then request answered by follower
    std::string resp;
    std::string leader;
};

struct SetCluster {
    std::unordered_map<std::string, so_5::mbox_t> mboxes;
};

class raft_server final : public so_5::agent_t {
public:
    struct change_state : public so_5::signal_t{};

    struct heartbeat : public so_5::signal_t{};

    struct RequestVote {
        std::string name;
        int term;
    };

    struct AppendEntry {
        std::string leader_name;
        int term;
        int status{0};
    };

    raft_server(context_t ctx, std::string name) 
        : so_5::agent_t{std::move(ctx)}, m_name{name} {
        server_state.activate();
    }

    void so_define_agent() override {
        std::random_device dev;
        std::mt19937 rng(dev());
        std::uniform_int_distribution dist(150, 300);
        m_election_timeout = dist(rng);

        //**********************************************************************
        // this event is used to get the current list of servers
        // must be done before the first leader election
        //**********************************************************************
        server_state.event([this](mhood_t<SetCluster> sc) {
            m_mboxes = sc->mboxes;
        });


        candidate.on_enter([this] {
            m_vote_cnt = 0;
            ++m_term;
            m_is_candidate = true;
            
            std::random_device dev;
            std::mt19937 rng(dev());
            std::uniform_int_distribution dist(150, 300);
            m_election_timeout = dist(rng);
            
            std::cout << "start of candidate" << std::endl;
            m_request_vote_thread = std::thread([this]{send_request_vote();});
            
            so_5::send<raft_server::change_state>(*this);
        })
        .event([this](mhood_t<raft_server::change_state>) {

            std::cout << "candidate" << std::endl;

            //**********************************************************************
            //transition to candidate state if the election timeout kicks in
            //**********************************************************************
            /*candidate.time_limit(std::chrono::milliseconds{m_election_timeout},
              candidate);
            */
            //******************************************************************
            //sends RequestVote to every server in the cluster. Even itself.
            //the vote count gets incremented in the vote handler.
            //******************************************************************
            /*for (auto el : m_mboxes) {
                so_5::send<raft_server::RequestVote>(el.second, m_name);
            }*/

        })
        .event(&raft_server::request_vote_handler)
        .event(&raft_server::heartbeat_handler)
        .on_exit([this] {
            m_is_candidate = false;
            m_request_vote_thread.join();
        });


        follower.on_enter([this] {
            std::cout << "start of follower" << std::endl;
            m_vote_cnt = 0;
            so_5::send<raft_server::change_state>(*this);
        })
        .event([this](mhood_t<raft_server::change_state>) {
            follower.time_limit(std::chrono::milliseconds{m_election_timeout},
              candidate);
            std::cout << "follower" << std::endl;
        })
        .event(&raft_server::heartbeat_handler)
        .event(&raft_server::request_vote_handler)
        .event(&raft_server::follower_client_request_handler);


        leader.on_enter([this] {
            std::cout << "start of leader" << std::endl;
            m_is_leader = true;
            m_vote_cnt = 0;
            so_5::send<raft_server::change_state>(*this);
        })
        .event([this](mhood_t<raft_server::change_state>) {
            std::cout << m_name << ": leader" << std::endl;
            m_heartbeat_thread = std::thread([this]{send_heartbeat();});
        })
        .event(&raft_server::heartbeat_handler)
        .event(&raft_server::leader_client_request_handler)
        .on_exit([this] {
            m_is_leader = false;
            m_heartbeat_thread.join();
        });
    }
private:

    state_t server_state{this},
    candidate{substate_of(server_state)},
    follower{initial_substate_of(server_state)},
    leader{substate_of(server_state)};

    int m_term{0};
    int m_heartbeat_freq;
    int m_election_timeout;
    int m_vote_cnt{0};
    bool m_is_leader{};
    bool m_is_candidate{};
    std::thread m_heartbeat_thread;
    std::thread m_request_vote_thread;
    std::string m_name;
    std::string m_curr_leader{};
    std::unordered_map<std::string, so_5::mbox_t> m_mboxes;

    void send_request_vote() {
        while (m_is_candidate) {
            for (auto el : m_mboxes) {
                so_5::send<raft_server::RequestVote>(el.second, m_name);
            }
            std::this_thread::sleep_for(std::chrono::milliseconds{m_election_timeout});
        }
    }

    void send_heartbeat() {
        while (m_is_leader) {
            for (auto el : m_mboxes) {
                if (el.first != m_name)
                    so_5::send<raft_server::AppendEntry>(el.second, m_name, m_term, 0);
            }
            std::this_thread::sleep_for(std::chrono::milliseconds{50});
        }
    }

    void heartbeat_handler(mhood_t<raft_server::AppendEntry> ae) {
        if (ae->status == 0 && ae->term >= m_term) {
            follower.activate();
            m_vote_cnt = 0;
            m_term = ae->term;
            m_curr_leader = ae->leader_name;
            follower.time_limit(std::chrono::milliseconds{m_election_timeout}, candidate);
        }
    }

    void request_vote_handler(mhood_t<raft_server::RequestVote> rv) {

        if (rv->name == m_name) {
            ++m_vote_cnt;
        } else {
            if (rv->term >= m_term)
                so_5::send<raft_server::RequestVote>(m_mboxes[rv->name], rv->name, rv->term);
        }

        if (m_vote_cnt > ceil(m_mboxes.size() / 2.0)) {
            m_vote_cnt = 0;
            leader.activate();
        }
    }

    void leader_client_request_handler(mhood_t<ClientRequest> cr) {
        std::cout << "youre right im the leader" << std::endl;
        so_5::send<ServerResponse>(cr->inbox, 1, cr->cmd, m_name);
        
    }

    void follower_client_request_handler(mhood_t<ClientRequest> cr) {
        std::cout << "im not the leader" << std::endl;
        so_5::send<ServerResponse>(cr->inbox, 0, "", m_curr_leader);
    }
};