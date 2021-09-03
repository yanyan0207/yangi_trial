#include "./util.h"

std::vector<std::string> Util::Split(const std::string &str,
                                     const char seperator) {
    std::vector<std::string> ret;
    int nextpos, pos = -1;
    while ((nextpos = str.find(seperator, pos + 1)) != std::string::npos) {
        ret.push_back(str.substr(pos + 1, nextpos - 1 - pos));
        pos = nextpos;
    }
    ret.push_back(str.substr(pos + 1));
    return ret;
}

bool Util::StartWith(const std::string &str, const std::string &prefix) {
    return str.substr(0, prefix.length()) == prefix;
}

bool Util::EndtWith(const std::string &str, const std::string &postfix) {
    return str.substr(str.length() - postfix.length()) == postfix;
}
