#include <so_5/all.hpp>
#include <random>
#include <vector>

class raft_server final : public so_5::agent_t {
public:
    struct change_state : public so_5::signal_t{};

    struct RequestVote {
        std::string name;
    };

    struct AppendEntry {
        int term;
    };

    raft_server(context_t ctx) : so_5::agent_t{std::move(ctx)} {
        server_state.activate();
    }

    void so_define_agent() override {
        std::random_device dev;
        std::mt19937 rng(dev());
        std::uniform_int_distribution dist(150, 300);
        m_election_timeout = dist(rng);
        std::cout << m_election_timeout << std::endl;

        //server_state.event(&raft_server::request_handler);

        candidate.event([this](mhood_t<change_state>) {
            std::random_device dev;
            std::mt19937 rng(dev());
            std::uniform_int_distribution dist(150, 300);
            m_election_timeout = dist(rng);
            std::cout << m_election_timeout << std::endl;

            std::cout << "candidate" << std::endl;


        })
        .event(&raft_server::request_handler);

        follower.event([this](mhood_t<change_state>) {
            follower.time_limit(std::chrono::milliseconds{m_election_timeout}, candidate);

            std::cout << "follower" << std::endl;
        })
        .event(&raft_server::request_handler);

        leader.event([this](mhood_t<change_state>) {

            std::cout << "leader" << std::endl;
            //timer wird bei neuaufruf zurückgesetzt
            leader.time_limit(std::chrono::milliseconds{100}, candidate);
            std::this_thread::sleep_for(std::chrono::milliseconds{200});
            leader.activate();
            so_5::send<raft_server::change_state>(*this);
        });
        candidate.on_enter([this] {
            std::cout << "start of candidate" << std::endl;
            so_5::send<change_state>(*this);
        });
    }
private:
    

    state_t server_state{this},
    candidate{substate_of(server_state)},
    follower{initial_substate_of(server_state)},
    leader{substate_of(server_state)};

    int m_term;
    int m_heartbeat_freq;
    int m_election_timeout;
    std::vector<so_5::mbox_t> m_mboxes;

    void heartbeat_handler(mhood_t<AppendEntry> ae) {
        so_5::send<raft_server::change_state>(*this);
    }

    void request_handler(mhood_t<RequestVote> rv) {
        std::cout << rv->name << std::endl;
    }
};