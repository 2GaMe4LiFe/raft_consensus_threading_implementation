#pragma once

#include <string>
#include <sstream>
#include <vector>
#include <functional>
#include "utils.h"

class Calc {
public:
    Calc() : m_result{0} {}
    Calc(long long result) : m_result{result} {}

    void parse(std::string cmd) {
        std::vector<std::string> tokens;

        std::stringstream ss(cmd);
        std::string token;
        while (std::getline(ss, token, ';')) {
            tokens.push_back(token);
        }

        if (tokens.size() > 2)
            return;

        switch (utils::hash(tokens[0].c_str())) {
        case utils::hash("ADD"):
            add(std::stoll(tokens[1]));
            break;
        case utils::hash("SUB"):
            sub(std::stoll(tokens[1]));
            break;
        case utils::hash("MUL"):
            mul(std::stoll(tokens[1]));
            break;
        case utils::hash("DIV"):
            div(std::stoll(tokens[1]));
            break;
        default:
            break;
        }
    }

    long long get_result() {
        return m_result;
    }

private:
    long long m_result;

    void add(long long value) {
        m_result += value;
    }

    void sub(long long value) {
        m_result -= value;
    }

    void mul(long long value) {
        m_result *= value;
    }

    void div(long long value) {
        if (value != 0)
            m_result /= value;
    }
};