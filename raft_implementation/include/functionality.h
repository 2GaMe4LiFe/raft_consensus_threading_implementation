#pragma once

#include <string>

class Calc {
public:
    Calc() : m_result{0} {}
    Calc(long long result) : m_result{result} {}

    void parse(std::string cmd) {

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