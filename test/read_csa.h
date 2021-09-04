#pragma once

#include <map>
#include <string>
#include <vector>

class CsaException : public std::runtime_error {
   public:
    explicit CsaException(const std::string file_name, const std::string reason)
        : std::runtime_error(file_name + " " + reason) {}
};

class CsaReader {
    friend int main(int, char **);

   public:
    void ReadCsa(const std::string &file_name);

   protected:
    int ReadExpectedLines(int line_idx, const std::string &prefix,
                          void (*fpfunc)(CsaReader *reader,
                                         const std::string &line),
                          const std::string &end_prefix = "");
    void ReadSettingLine(const std::string &line);
    void ReadStartTime(const std::string &line);
    void ReadRatingLine(const std::string &line);
    int ReadHeader();
    int ReadMoveList(int line_idx);
    void ReadFooter(int linc_idx);

   private:
    std::vector<std::string> lines;
    std::string file_name;
    std::string version;
    std::string black_player;
    std::string white_player;
    std::map<std::string, std::string> settings;
    int max_moves = -1;
    std::string initial_pos;
    std::string start_time;
    float black_rate = -1;
    float white_rate = -1;
    std::vector<std::string> move_list;
    std::string last_command;
    std::string end_pos;
    std::string summary;
    std::string end_time;
    std::map<std::string, int> mochigoma[2];
};
