#include <so_5/all.hpp>

class raft_server final : public so_5::agent_t {
public:
    struct change_state : public so_5::signal_t{};

    struct RequestVote {
        std::string name;
    };

    raft_server(context_t ctx) : so_5::agent_t{std::move(ctx)} {}

    void so_define_agent() override {

        server_state.event(&raft_server::request_handler);

        server_state.activate();

        candidate.event([this](mhood_t<change_state>) {
            std::cout << "candidate" << std::endl;
            leader.time_limit(std::chrono::milliseconds{500}, candidate);
        })
        .event(&raft_server::request_handler);

        follower.event([this](mhood_t<change_state>) {
            std::cout << "follower" << std::endl;
            leader.activate();
            so_5::send<raft_server::change_state>(*this);
            leader.time_limit(std::chrono::milliseconds{500}, candidate);
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
        candidate.on_enter([] {std::cout << "start of candidate" << std::endl;});
    }
private:
    state_t server_state{this},
    candidate{substate_of(server_state)},
    follower{initial_substate_of(server_state)},
    leader{substate_of(server_state)};

    void request_handler(mhood_t<RequestVote> rv) {
        std::cout << rv->name << std::endl;
    }
};