#include <so_5/all.hpp>
#include <random>
#include <unordered_map>
#include <vector>
#include <cmath>

class raft_server final : public so_5::agent_t {
public:
    struct change_state : public so_5::signal_t{};

    struct heartbeat : public so_5::signal_t{};

    struct RequestVote {
        std::string name;
    };

    struct AppendEntry {
        int term;
        int status{0};
    };

    struct SetCluster {
        std::unordered_map<std::string, so_5::mbox_t> mboxes;
    };

    raft_server(context_t ctx, std::string name) : so_5::agent_t{std::move(ctx)}, m_name{name} {
        server_state.activate();
    }

    void so_define_agent() override {
        std::random_device dev;
        std::mt19937 rng(dev());
        std::uniform_int_distribution dist(150, 300);
        m_election_timeout = dist(rng);
        std::cout << m_election_timeout << std::endl;

        server_state.event([this](mhood_t<raft_server::SetCluster> sc) {
            m_mboxes = sc->mboxes;
        });

        candidate.event([this](mhood_t<raft_server::change_state>) {
            std::random_device dev;
            std::mt19937 rng(dev());
            std::uniform_int_distribution dist(150, 300);
            m_election_timeout = dist(rng);
            std::cout << m_election_timeout << std::endl;

            std::cout << "candidate" << std::endl;
            ++m_term;

            //refresh of candidate state
            candidate.time_limit(std::chrono::milliseconds{m_election_timeout}, candidate);
            for (auto el : m_mboxes) {
                std::cout << el.first << std::endl;
                so_5::send<raft_server::RequestVote>(el.second, m_name);
            }

        })
        .event(&raft_server::candidate_request_vote_handler);

        follower.event([this](mhood_t<raft_server::change_state>) {
            follower.time_limit(std::chrono::milliseconds{m_election_timeout}, candidate);
            std::cout << "follower" << std::endl;
        })
        .event(&raft_server::heartbeat_handler)
        .event(&raft_server::candidate_request_vote_handler);

        leader.event([this](mhood_t<raft_server::change_state>) {

            std::cout << m_name << ": leader" << std::endl;
            //timer wird bei neuaufruf zurückgesetzt
            leader.activate();
            
            for (int i{0}; i < 100; i++) {
                std::this_thread::sleep_for(std::chrono::milliseconds{50});
                for (auto el : m_mboxes) {
                    if (el.first != m_name)
                        so_5::send<raft_server::AppendEntry>(el.second, m_term, 0);
                }
            }

            /*while (true) {
                std::this_thread::sleep_for(std::chrono::milliseconds{50});
                for (auto el : m_mboxes) {
                    so_5::send<raft_server::AppendEntry>(el.second, m_term, 0);
                }
            }*/
        })
        .event(&raft_server::heartbeat_handler);

        candidate.on_enter([this] {
            m_vote_cnt = 0;
            std::cout << "start of candidate" << std::endl;
            so_5::send<raft_server::change_state>(*this);
        });

        follower.on_enter([this] {
            std::cout << "start of follower" << std::endl;
            m_vote_cnt = 0;
            so_5::send<raft_server::change_state>(*this);
        });

        leader.on_enter([this] {
            std::cout << "start of leader" << std::endl;
            m_vote_cnt = 0;
            so_5::send<raft_server::change_state>(*this);
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
    std::string m_name;
    std::unordered_map<std::string, so_5::mbox_t> m_mboxes;

    void heartbeat_handler(mhood_t<raft_server::AppendEntry> ae) {
        if (ae->status == 0) {
            follower.activate();
            follower.time_limit(std::chrono::milliseconds{m_election_timeout}, candidate);
        }
    }

    void candidate_request_vote_handler(mhood_t<raft_server::RequestVote> rv) {

        if (rv->name == m_name) {
            ++m_vote_cnt;
        } else {
            for (auto el : m_mboxes) {
                if (el.first == rv->name) {
                    so_5::send<raft_server::RequestVote>(el.second, el.first);
                    break;
                }
            }
        }

        if (m_vote_cnt > ceil(m_mboxes.size() / 2.0)) {
            m_vote_cnt = 0;
            leader.activate();
        }
        std::cout << m_name << ": " << m_vote_cnt << std::endl;
    }
};