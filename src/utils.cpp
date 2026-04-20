#include <sstream>
#include <string>
#include <vector>

std::vector<std::string> split(const std::string& str, char delimiter) {
    std::vector<std::string> tokens;
    std::istringstream sstr(str);

    while (!sstr.eof()) {
        std::string token;
        std::getline(sstr, token, delimiter);
        if (token.empty()) {
            continue;
        }
        tokens.push_back(token);
    }
    return tokens;
}
