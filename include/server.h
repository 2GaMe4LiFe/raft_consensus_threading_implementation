#include <so_5/all.hpp>

class raft_server final : public so_5::agent_t {
public:
    struct change_state : public so_5::signal_t{};
    raft_server(context_t ctx) : so_5::agent_t{std::move(ctx)} {
        server_state.activate();

        candidate.event([this](mhood_t<change_state>) {
            std::cout << "candidate" << std::endl;
            follower.activate();
            so_5::send<raft_server::change_state>(*this);
        });
        follower.event([this](mhood_t<change_state>) {
            std::cout << "follower" << std::endl;
        });
        leader.event([this](mhood_t<change_state>) {
            std::cout << "leader" << std::endl;
        });
    }
private:
    state_t server_state{this},
    candidate{initial_substate_of(server_state)},
    follower{substate_of(server_state)},
    leader{substate_of(server_state)};
};