#pragma once

#include <string>
#include <vector>

class Util {
   public:
    static std::vector<std::string> Split(const std::string &str,
                                          const char seperator);

    static bool StartWith(const std::string &str, const std::string &prefix);
    static bool EndtWith(const std::string &str, const std::string &postfix);
};
