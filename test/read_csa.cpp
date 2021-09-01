#include <string>
#include <fstream>
#include <iostream>
#include <vector>
#include <map>
#include <tuple>
#include <sstream>

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

bool startWith(const std::string &str,const std::string &prefix) {
    return str.substr(0,prefix.length()) == prefix;
}

std::vector<std::string> split(const std::string &str,const char seperator) {
    std::vector<std::string> ret;
    int nextpos,pos = 0;
    while((nextpos = str.find(seperator,pos+1)) > 0) {
        ret.push_back(str.substr(pos+1,nextpos-pos));
        pos = nextpos;
    }
    ret.push_back(str.substr(pos));
    return ret;

}

std::tuple<std::string,std::vector<std::string>,std::string,std::vector<std::string>> read_csa(const std::string file_name) {
    std::cout << file_name << std::endl;
    std::ifstream ifs(file_name);

    std::vector<std::string> lines;
    std::string line;
    while(std::getline(ifs,line)) {
        lines.push_back(line);
    }

    std::string version = lines[0];
    std::string black_player = lines[1];
    std::string white_player = lines[2];
    std::cout << "black_player:" << black_player << std::endl;
    std::cout << "white_player:" << white_player << std::endl;

    // 平手確認
    int line_idx = 2;
    std::string initial_pos;
    while(++line_idx < lines.size()) {
        line = lines[line_idx];
        if (line[0] == '+') {
            break;
        } else {
            if (line[0] == 'P') {
                initial_pos += line + "\n";
            }
        }
    }
    if (initial_pos != hirate_initial_pos) {
        std::cout << initial_pos << std::endl;
        std::cout << hirate_initial_pos << std::endl;
    }

    double black_rate = -1;
    double white_rate = -1;
    if (line == "+") {
        while(++line_idx < lines.size()) {
            line = lines[line_idx];
            if (line[0] == '+') {
                break;
            }
            else if(startWith(line,"'black_rate:")){
                black_rate =  std::atof(line.substr(line.find_last_of(":") + 1).c_str());
            }
            else if(startWith(line,"'white_rate:")) {
                white_rate =  std::atof(line.substr(line.find_last_of(":") + 1).c_str());
            }
        }
    }
    std::cout << "black_rate:" << black_rate << std::endl;
    std::cout << "white_rate:" << white_rate << std::endl;

    line_idx--;
    std::vector<std::string> move_list;
    bool black = true;
    std::string last_command; 
    while(++line_idx < lines.size()) {
        line = lines[line_idx];
        // 先手番
        if(black && line[0] == '+') {
            move_list.push_back(line);
            black = false;
        }
        // 後手番
        else if(!black && line[0] == '-') {
            move_list.push_back(line);
            black = true;
        }
        // 終了コマンド
        else if(line[0] == '%') {
            last_command = line;
            break;
        }
    }

    std::cout << move_list.size() << " move:" << last_command << std::endl;

    // footer
    std::string end_pos;
    std::vector<std::string> summary;
    while(++line_idx < lines.size()) {
        line = lines[line_idx];
        if (line.substr(0,2) == "'P") {
            end_pos += line.substr(1) + "\n";
        }
        else if (startWith(line,"'summary:")) {
            summary = split(line,':');
        }
    }
    for (const auto &str : summary) {
        std::cout << str << " ";
    }
    std::cout << std::endl;
    return std::make_tuple(initial_pos,move_list,end_pos,summary);
}

std::string narikoma(const std::string &koma) {
    std::map<std::string,std::string> pair = {
        {"FU","TO"},
        {"KY","NY"},
        {"KE","NK"},
        {"GI","NG"},
        {"KA","UM"},
        {"HI","RY"},
    };
    return pair.count(koma) ? pair[koma] : "";
}

int main(int argc,char **argv) {
    for (int i = 1;i<argc;i++) {
        auto csa = read_csa(argv[i]);
        const std::string &initial_pos = std::get<0>(csa);
        const auto &move_list = std::get<1>(csa);
        const std::string &end_pos = std::get<2>(csa);
        const auto &summary = std::get<3>(csa);

        if(initial_pos != hirate_initial_pos) {
            std::cerr << "initial_pos is not hirate" << std::endl; 
        }

        std::string board[9][9];
        for(int i = 0;i<9;i++) {
            for(int j = 0;j<9;j++) {
                board[i][j] = initial_pos.substr(i*30+2+j*3,3);
            }
        }
        bool black = true;
        for (const auto &move : move_list) {
            std::string teban = move.substr(0,1);
            unsigned int src_col = '9' - move[1];
            unsigned int src_row = move[2] - '1';
            unsigned int dst_col = '9' - move[3];
            unsigned int dst_row = move[4] - '1';
            std::string koma = move.substr(5,2);
            if ((black ? "+" : "-") != teban || move.size() != 7 || 
                (src_col != 9 && (src_row >= 9 || src_col >= 9)) || dst_row >= 9 || dst_col >= 9) {
                std::cerr << "move error:" << move << std::endl;
                return - 1;
            }
            if (src_col == 9) {
                board[dst_row][dst_col] = (black ? "+" : "-") + koma;
            }
            else {
                if(board[src_row][src_col] != ((black ? "+" : "-") + koma) && 
                board[src_row][src_col][0] + narikoma(board[src_row][src_col].substr(1)) != ((black ? "+" : "-") + koma)) {
                    for (int i = 0;i<9;i++) {
                        for (int j = 0;j<9;j++) {
                            std::cout << board[i][j];
                        }
                        std::cout << std::endl;
                    }
                    std::cerr << "move koma error:" << src_row << src_col << move << koma << std::endl;
                    return - 1;
                
                }
                else {
                    board[dst_row][dst_col] = (black ? "+" : "-") + koma;
                    board[src_row][src_col] = " * ";
                }
            }
           black = !black;

        }

        std::stringstream last_board_ss;
        for (int i = 0;i<9;i++) {
            last_board_ss << "P" << i+1;
            for (int j = 0;j<9;j++) {
                last_board_ss << board[i][j];
            }
            last_board_ss << std::endl;
        }
        std::string last_board = last_board_ss.str();

        if (last_board != end_pos.substr(0,270)) {
            std::cout << last_board << std::endl;
            std::cout << end_pos.substr(0,270) << std::endl;
            return -1;
        }

    }
}
