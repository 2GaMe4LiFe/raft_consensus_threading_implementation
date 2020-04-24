#pragma once

#include <so_5/all.hpp>
#include <random>
#include <unordered_map>
#include <vector>
#include <cmath>
#include <tuple>

#include "functionality.h"

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

        RequestVote(int term_, bool vote_granted_) : term{term_},
        vote_granted{vote_granted_} {}

        RequestVote(int term_, std::string candidate_id_, int lli_, int llt_)
        : term{term_}, candidate_id{candidate_id_}, last_log_index{lli_},
        last_log_term{llt_}, vote_granted{false} {}

        int term;
        std::string candidate_id;
        int last_log_index;
        int last_log_term;

        bool vote_granted;
    };

    struct AppendEntry {
        
        AppendEntry(int term_, bool success_) : term{term_},
        success{success_} {};

        AppendEntry(int term_, std::string leader_id_, int prev_log_index_,
        int prev_log_term_, std::vector<std::tuple<int,std::string>> entries_,
        int leader_commit_) : term{term_}, leader_id{leader_id_},
        prev_log_index{prev_log_index_}, prev_log_term{prev_log_term_},
        entries{entries_}, leader_commit{leader_commit_} {}
        
        int term;
        std::string leader_id;
        int prev_log_index;
        int prev_log_term;
        std::vector<std::tuple<int,std::string>> entries;
        int leader_commit;
        bool success;
    };

    struct AppendEntryResult {
        std::string follower_name;
        int term;
        bool success;
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
            for (auto el : sc->mboxes) {
                m_mboxes[el.first] = std::tuple<so_5::mbox_t,int>(el.second,m_log.size());
            }
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
            m_request_vote_thread.detach();
        })
        .event(&raft_server::heartbeat_handler)
        .event(&raft_server::request_vote_handler)
        .on_exit([this] {
            m_is_candidate = false;
        });


        follower.on_enter([this] {
            std::cout << m_name << " start of follower" << std::endl;
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

            for (auto el : m_mboxes) {
                std::get<1>(el.second) = m_log.size();
            }

            m_heartbeat_thread = std::thread([this]{send_heartbeat();});
            m_heartbeat_thread.detach();
            leader.time_limit(std::chrono::milliseconds{10000}, follower);
        })
        .event([this](mhood_t<raft_server::change_state>) {
            std::cout << m_name << ": leader" << std::endl;
        })
        .event(&raft_server::heartbeat_handler)
        .event(&raft_server::leader_client_request_handler)
        .event(&raft_server::consistency_check)
        .on_exit([this] {
            m_is_leader = false;
        });
    }
private:

    state_t server_state{this},
    candidate{substate_of(server_state)},
    follower{initial_substate_of(server_state)},
    leader{substate_of(server_state)};

    int m_heartbeat_freq;
    int m_election_timeout;
    int m_vote_cnt{0};
    bool m_is_leader{};
    bool m_is_candidate{};
    
    std::thread m_heartbeat_thread;
    std::thread m_request_vote_thread;

    std::string m_name;
    std::string m_curr_leader{};
    std::unordered_map<std::string, std::tuple<so_5::mbox_t, int>> m_mboxes;

    Calc m_register;

    int m_term{0};
    std::vector<std::tuple<int, std::string>> m_log;
    int m_commit_index{0};
    int m_last_applied{0};
    int m_next_index{m_commit_index +1};
    int m_match_index{0};

    void send_request_vote() {
        while (m_is_candidate) {
            std::cout << "thread is running..." << std::endl;
            for (auto el : m_mboxes) {
                if (m_log.size() != 0)
                    so_5::send<raft_server::RequestVote>(std::get<0>(el.second), m_term, m_name,
                        m_log.size(), std::get<0>(m_log[m_log.size()-1]));
                else
                    so_5::send<raft_server::RequestVote>(std::get<0>(el.second), m_term, m_name,
                        m_log.size(), 0);
            }
            std::this_thread::sleep_for(std::chrono::milliseconds{m_election_timeout});
        }
    }

    void send_heartbeat() {
        std::vector<std::tuple<int,std::string>> empty_v;
        while (m_is_leader) {
            for (auto el : m_mboxes) {
                if (el.first != m_name) {
                    if (m_log.size() != 0)
                        so_5::send<raft_server::AppendEntry>(std::get<0>(el.second), m_term, m_name,
                            ((int)m_log.size())-1, std::get<0>(m_log[m_log.size()-1]), empty_v, m_commit_index);
                    else
                        so_5::send<raft_server::AppendEntry>(std::get<0>(el.second), m_term, m_name,
                            ((int)m_log.size())-1, 0, empty_v, m_commit_index);
                }
            }
            std::this_thread::sleep_for(std::chrono::milliseconds{50});
        }
    }

    void consistency_check(mhood_t<raft_server::AppendEntryResult> aer) {
        if (!aer->success) {
            if (std::get<1>(m_mboxes[aer->follower_name])-1 >= 0) {//nextIndex
                std::vector<std::tuple<int,std::string>> empty_v;
                std::get<1>(m_mboxes[aer->follower_name])--;
                so_5::send<raft_server::AppendEntry>(std::get<0>(m_mboxes[aer->follower_name]), m_term, m_name,
                            std::get<1>(m_mboxes[aer->follower_name]),
                            std::get<0>(m_log[std::get<1>(m_mboxes[aer->follower_name])]), empty_v, m_commit_index);
            }
        } else {

            std::vector<std::tuple<int,std::string>> payload;

            for (int i{std::get<1>(m_mboxes[aer->follower_name])}; i < m_log.size(); i++) {
                payload.push_back(m_log[i]);
            }

            if (std::get<1>(m_mboxes[aer->follower_name]) < m_log.size()) {
                so_5::send<raft_server::AppendEntry>(std::get<0>(m_mboxes[aer->follower_name]), m_term, m_name,
                                std::get<1>(m_mboxes[aer->follower_name]),
                                std::get<0>(m_log[std::get<1>(m_mboxes[aer->follower_name])]), payload, m_commit_index);

                std::get<1>(m_mboxes[aer->follower_name]) = m_log.size();
            }
        }
    }

    void heartbeat_handler(mhood_t<raft_server::AppendEntry> ae) {
        if ((ae->entries).size() == 0 && ae->term >= m_term) {
            follower.activate();
            m_vote_cnt = 0;
            m_term = ae->term;
            m_curr_leader = ae->leader_id;
            
            if (ae->prev_log_index <= 0) {
                so_5::send<raft_server::AppendEntryResult>(std::get<0>(m_mboxes[m_curr_leader]), m_name, m_term, true);
            } else if (ae->prev_log_index-1 > ((int)m_log.size())-1) {
                so_5::send<raft_server::AppendEntryResult>(std::get<0>(m_mboxes[m_curr_leader]), m_name, m_term, false);
            } else if (std::get<0>(m_log[ae->prev_log_index-1]) == ae->prev_log_term) {
                so_5::send<raft_server::AppendEntryResult>(std::get<0>(m_mboxes[m_curr_leader]), m_name, m_term, true);
            } else if (ae->prev_log_index == m_last_applied) {
                so_5::send<raft_server::AppendEntryResult>(std::get<0>(m_mboxes[m_curr_leader]), m_name, m_term, true);
            } else
                so_5::send<raft_server::AppendEntryResult>(std::get<0>(m_mboxes[m_curr_leader]), m_name, m_term, false);


            follower.time_limit(std::chrono::milliseconds{m_election_timeout}, candidate);
        } else if (ae->term < m_term) { //receiver implementation #1
            so_5::send<raft_server::AppendEntryResult>(std::get<0>(m_mboxes[m_curr_leader]), m_name, m_term, false);
        } else if (ae->term >= m_term) {
            for (auto el : ae->entries) {
                m_log.push_back(el);
                m_register.parse(std::get<1>(el));
            }

            m_last_applied = m_log.size();

            utils::write_log_to(m_name + ".log", m_log);
            so_5::send<raft_server::AppendEntryResult>(std::get<0>(m_mboxes[m_curr_leader]), m_name, m_term, true);
        }
    }

    void request_vote_handler(mhood_t<raft_server::RequestVote> rv) {
        if (rv->vote_granted) {
            ++m_vote_cnt;
        } else {
            std::cout << "Voted for " << rv->candidate_id << std::endl;
            if (rv->term >= m_term)
                so_5::send<raft_server::RequestVote>(std::get<0>(m_mboxes[rv->candidate_id]), m_term, true);
            else
                so_5::send<raft_server::RequestVote>(std::get<0>(m_mboxes[rv->candidate_id]), m_term, false);

        }

        if (m_vote_cnt > ceil(m_mboxes.size() / 2.0)) {
            m_vote_cnt = 0;
            leader.activate();
        }
    }

    void leader_client_request_handler(mhood_t<ClientRequest> cr) {
        std::cout << "youre right im the leader" << std::endl;

        m_log.push_back(std::tuple<int,std::string>(m_term, cr->cmd));

        utils::write_log_to(m_name + ".log", m_log);
        m_register.parse(cr->cmd);

        so_5::send<ServerResponse>(cr->inbox, 1, m_register.get_result(), m_name);
        
    }

    void follower_client_request_handler(mhood_t<ClientRequest> cr) {
        std::cout << "im not the leader" << std::endl;
        so_5::send<ServerResponse>(cr->inbox, 0, "", m_curr_leader);
    }
};