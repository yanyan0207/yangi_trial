#include <fstream>
#include <iostream>
#include <map>
#include <sstream>
#include <string>
#include <tuple>
#include <vector>

typedef struct {
    std::string last_command;
    std::map<std::string, std::string> settings;
    std::vector<std::string> move_list;
    std::string initial_pos;
    std::string end_pos;
    std::string summary;

} CsaData;

std::string hirate_initial_pos =
    "P1-KY-KE-GI-KI-OU-KI-GI-KE-KY\n"
    "P2 * -HI *  *  *  *  * -KA * \n"
    "P3-FU-FU-FU-FU-FU-FU-FU-FU-FU\n"
    "P4 *  *  *  *  *  *  *  *  * \n"
    "P5 *  *  *  *  *  *  *  *  * \n"
    "P6 *  *  *  *  *  *  *  *  * \n"
    "P7+FU+FU+FU+FU+FU+FU+FU+FU+FU\n"
    "P8 * +KA *  *  *  *  * +HI * \n"
    "P9+KY+KE+GI+KI+OU+KI+GI+KE+KY\n";

bool startWith(const std::string &str, const std::string &prefix) {
    return str.substr(0, prefix.length()) == prefix;
}

std::vector<std::string> split(const std::string &str, const char seperator) {
    std::vector<std::string> ret;
    int nextpos, pos = -1;
    std::cout << str << std::endl;
    while ((nextpos = str.find(seperator, pos + 1)) != std::string::npos) {
        std::cout << "pos:" << pos << " nextpos:" << nextpos << std::endl;
        ret.push_back(str.substr(pos + 1, nextpos - 1 - pos));
        pos = nextpos;
    }
    ret.push_back(str.substr(pos + 1));
    return ret;
}

CsaData read_csa(const std::string file_name) {
    std::cout << std::endl << file_name << std::endl;
    std::ifstream ifs(file_name);

    std::vector<std::string> lines;
    std::string line;
    if (!ifs) {
        throw std::runtime_error(file_name + " cannot open");
    }
    while (std::getline(ifs, line)) {
        if (line.size() == 0) {
            throw new std::runtime_error("void line error");
        }
        lines.push_back(line);
    }
    std::cout << "line_num" << lines.size() << std::endl;

    std::string version = lines[0];
    std::string black_player = lines[1];
    std::string white_player = lines[2];
    std::cout << "black_player:" << black_player << std::endl;
    std::cout << "white_player:" << white_player << std::endl;

    // 平手確認
    int line_idx = 2;
    std::string initial_pos;
    std::map<std::string, std::string> settings;
    while (++line_idx < lines.size()) {
        line = lines[line_idx];
        if (line[0] == '+') {
            break;
        } else {
            if (line[0] == 'P') {
                initial_pos += line + "\n";
            } else if (line[0] == '\'') {
                const auto &tmp = split(line.substr(1), ':');
                if (tmp.size() != 2) {
                    throw std::runtime_error("unexpected settings:" + line);
                }
                settings[tmp[0]] = tmp[1];
            }
        }
    }

    int max_moves = -1;
    if (settings.count("Max_Moves")) {
        max_moves = std::atoi(settings["Max_Moves"].c_str());
        std::cout << "max_moves:" << max_moves << std::endl;
    }
    std::cout << "settings" << std::endl;
    for (const auto &pair : settings) {
        std::cout << "\t" << pair.first << " " << pair.second << std::endl;
    }

    if (initial_pos != hirate_initial_pos) {
        std::cout << initial_pos << std::endl;
        std::cout << hirate_initial_pos << std::endl;
    }

    double black_rate = -1;
    double white_rate = -1;
    if (line == "+") {
        while (++line_idx < lines.size()) {
            line = lines[line_idx];
            if (line[0] == '+' || line[0] == '%') {
                break;
            } else if (startWith(line, "'black_rate:")) {
                black_rate =
                    std::atof(line.substr(line.find_last_of(":") + 1).c_str());
            } else if (startWith(line, "'white_rate:")) {
                white_rate =
                    std::atof(line.substr(line.find_last_of(":") + 1).c_str());
            }
        }
    }
    std::cout << "black_rate:" << black_rate << std::endl;
    std::cout << "white_rate:" << white_rate << std::endl;

    line_idx--;
    std::vector<std::string> move_list;
    bool black = true;
    std::string last_command;
    while (++line_idx < lines.size()) {
        line = lines[line_idx];
        // 先手番
        if (black && line[0] == '+') {
            // コメント削除
            int pos = line.find("'");
            if (pos != std::string::npos) {
                line = line.substr(0, line.find_last_not_of(" ", pos - 1) + 1);
            }
            move_list.push_back(line);
            black = false;
        }
        // 後手番
        else if (!black && line[0] == '-') {
            // コメント削除
            int pos = line.find("'");
            if (pos != std::string::npos) {
                line = line.substr(0, line.find_last_not_of(" ", pos - 1) + 1);
            }
            move_list.push_back(line);
            black = true;
        }

        // 終了コマンド
        if (line[0] == '%') {
            last_command = line;
            break;
        }
        if (move_list.size() == max_moves) {
            break;
        }
    }

    std::cout << move_list.size() << " move:" << last_command << std::endl;
    std::cout << "last_command:" << last_command << std::endl;
    // footer
    std::string end_pos;
    std::string summary;
    while (++line_idx < lines.size()) {
        line = lines[line_idx];
        if (last_command.size() == 0 && line[0] == '%') {
            last_command = line;
        }
        if (line.substr(0, 2) == "'P") {
            end_pos += line.substr(1) + "\n";
        } else if (startWith(line, "'summary:")) {
            summary = line;
        }
    }
    for (const auto &str : summary) {
        std::cout << str << " ";
    }
    std::cout << std::endl;
    std::cout << "read_csa finish" << std::endl;

    CsaData data;
    data.last_command = last_command;
    data.move_list = move_list;
    data.initial_pos = initial_pos;
    data.end_pos = end_pos;
    data.settings = settings;
    data.summary = summary;

    return data;
}

std::string narikoma(const std::string &koma) {
    std::map<std::string, std::string> pair = {
        {"FU", "TO"}, {"KY", "NY"}, {"KE", "NK"},
        {"GI", "NG"}, {"KA", "UM"}, {"HI", "RY"},
    };
    return pair.count(koma) ? pair[koma] : "";
}

int main(int argc, char **argv) {
    for (int fileno = 1; fileno < argc; fileno++) {
        std::cout << "No." << fileno << std::endl;
        auto csa = read_csa(argv[fileno]);
        const std::string &initial_pos = csa.initial_pos;
        const auto &move_list = csa.move_list;
        const std::string &end_pos = csa.end_pos;
        const std::string &last_command = csa.last_command;
        const auto &settings = csa.settings;

        if (initial_pos != hirate_initial_pos) {
            std::cerr << "initial_pos is not hirate" << std::endl;
        }

        std::string board[9][9];
        for (int i = 0; i < 9; i++) {
            for (int j = 0; j < 9; j++) {
                board[i][j] = initial_pos.substr(i * 30 + 2 + j * 3, 3);
            }
        }
        bool black = true;
        int last_num = move_list.size();
        if (last_command == "%SENNICHITE") {
            last_num--;
        }
        for (int tesuu = 0; tesuu < last_num; tesuu++) {
            const auto &move = move_list[tesuu];
            // std::cout << move << std::endl;
            std::string teban = move.substr(0, 1);
            unsigned int src_col = '9' - move[1];
            unsigned int src_row = move[2] - '1';
            unsigned int dst_col = '9' - move[3];
            unsigned int dst_row = move[4] - '1';
            std::string koma = move.substr(5, 2);
            if ((black ? "+" : "-") != teban || move.size() != 7 ||
                (src_col != 9 && (src_row >= 9 || src_col >= 9)) ||
                dst_row >= 9 || dst_col >= 9) {
                std::cerr << "move error:'" << move << "'" << std::endl;
                return -1;
            }
            if (src_col == 9) {
                board[dst_row][dst_col] = (black ? "+" : "-") + koma;
            } else {
                if (board[src_row][src_col] != ((black ? "+" : "-") + koma) &&
                    board[src_row][src_col][0] +
                            narikoma(board[src_row][src_col].substr(1)) !=
                        ((black ? "+" : "-") + koma)) {
                    for (int i = 0; i < 9; i++) {
                        for (int j = 0; j < 9; j++) {
                            std::cout << board[i][j];
                        }
                        std::cout << std::endl;
                    }
                    std::cerr << "move koma error:" << src_row << src_col
                              << move << koma << std::endl;
                    return -1;

                } else {
                    board[dst_row][dst_col] = (black ? "+" : "-") + koma;
                    board[src_row][src_col] = " * ";
                }
            }
            black = !black;
        }

        std::stringstream last_board_ss;
        for (int i = 0; i < 9; i++) {
            last_board_ss << "P" << i + 1;
            for (int j = 0; j < 9; j++) {
                last_board_ss << board[i][j];
            }
            last_board_ss << std::endl;
        }
        std::string last_board = last_board_ss.str();

        if (last_board != end_pos.substr(0, 270)) {
            std::cout << "last_board" << std::endl;
            std::cout << last_board << std::endl;

            std::cout << "end_pos" << std::endl;
            std::cout << end_pos.substr(0, 270) << std::endl;
            return -1;
        }
    }
}
