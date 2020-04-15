#pragma once

#include <so_5/all.hpp>
#include "server.h"

class Client final : public so_5::agent_t {
public:
    Client(context_t ctx, so_5::mbox_t inbox) : so_5::agent_t{std::move(ctx)},
        m_inbox{inbox} {}

    void so_define_agent() override {
        so_subscribe(m_inbox).event(&Client::response_handler);
    }
private:
    so_5::mbox_t m_inbox;

    void response_handler(mhood_t<ServerResponse> sr) {
        std::cout << "Client: " << sr->resp << std::endl;
    }
};