#pragma once

#include <iostream>
#include <fstream>
#include <vector>
#include <tuple>
#include <sstream>

namespace utils {
    std::uint64_t constexpr mix(char m, std::uint64_t s) {
        return ((s<<7) + ~(s>>3)) + ~m;
    }

    uint64_t constexpr hash(const char* m) {
        return (*m) ? mix(*m,hash(m+1)) : 0;
    }

    std::vector<std::tuple<int,int,std::string>> read_log_from(std::string filename) {
        std::vector<std::tuple<int,int, std::string>> cont;
        
        std::ifstream is{filename};
        std::string line;
        if (is.good()) {
            while (std::getline(is, line)) {
                std::stringstream ss{line};
                std::string tmp;
                std::string var1;
                std::string var2;
                std::string var3;

                std::getline(ss, tmp, ';');
                var1 = tmp;
                std::getline(ss, tmp, ';');
                var2 = tmp;
                std::getline(ss, tmp);
                var3 = tmp;

                cont.push_back(std::tuple<int,int,std::string>(
                    std::stoi(var1), std::stoi(var2), var3));
            }
        }        

        return cont;
    }

    void write_log_to(std::string filename,
        std::vector<std::tuple<int,int,std::string>> log) {
        std::ofstream os{filename};
        if (os.good()) {
            for (auto el : log) {
                os << std::to_string(std::get<0>(el)) << ";"
                    << std::to_string(std::get<1>(el))
                    << ";" << std::get<2>(el);
            }
        }
    }
}