#include "./read_csa.h"

#include <string.h>

#include <filesystem>
#include <fstream>
#include <iostream>
#include <map>
#include <sstream>
#include <string>
#include <tuple>
#include <vector>

#include "../util.h"

static std::string hirate_initial_pos =
    "P1-KY-KE-GI-KI-OU-KI-GI-KE-KY\n"
    "P2 * -HI *  *  *  *  * -KA * \n"
    "P3-FU-FU-FU-FU-FU-FU-FU-FU-FU\n"
    "P4 *  *  *  *  *  *  *  *  * \n"
    "P5 *  *  *  *  *  *  *  *  * \n"
    "P6 *  *  *  *  *  *  *  *  * \n"
    "P7+FU+FU+FU+FU+FU+FU+FU+FU+FU\n"
    "P8 * +KA *  *  *  *  * +HI * \n"
    "P9+KY+KE+GI+KI+OU+KI+GI+KE+KY\n";

static std::string narikoma(const std::string &koma) {
    std::map<std::string, std::string> pair = {
        {"FU", "TO"}, {"KY", "NY"}, {"KE", "NK"},
        {"GI", "NG"}, {"KA", "UM"}, {"HI", "RY"},
    };
    return pair.count(koma) ? pair[koma] : "";
}

static std::string narimotokoma(const std::string &koma) {
    std::map<std::string, std::string> pair = {
        {"TO", "FU"}, {"NY", "KY"}, {"NK", "KE"},
        {"NG", "GI"}, {"UM", "KA"}, {"RY", "HI"},
    };
    return pair.count(koma) ? pair[koma] : koma;
}

static std::string boardToString(const std::string *board) {
    std::string ret;
    for (int row = 0; row < 9; row++) {
        for (int col = 0; col < 9; col++) {
            ret += board[row * 9 + col];
        }
        ret += "\n";
    }
    return ret;
}

int CsaReader::ReadExpectedLines(int line_idx, const std::string &prefix,
                                 void (*fpfunc)(CsaReader *reader,
                                                const std::string &line),
                                 const std::string &end_prefix) {
    std::string line;
    while (true) {
        line = lines.at(line_idx);
        if (Util::StartWith(line, prefix)) {
            (*fpfunc)(this, line);
        } else {
            break;
        }
        line_idx++;
    }

    if (end_prefix != "" && !Util::StartWith(line, end_prefix)) {
        throw CsaException(file_name, "unexpected line:" + line);
    } else {
        return line_idx;
    }
}

void CsaReader::ReadSettingLine(const std::string &line) {
    const auto &tmp = Util::Split(line.substr(1), ':');
    if (tmp.size() != 2) {
        throw CsaException(file_name, "setting error:" + line);
    }
    settings[tmp[0]] = tmp[1];
    if (tmp[0] == "Max_Moves") {
        max_moves = std::atoi(settings["Max_Moves"].c_str());
    }
}

void CsaReader::ReadStartTime(const std::string &line) {
    if (Util::StartWith(line, "$START_TIME:")) {
        start_time = line.substr(strlen("$START_TIME:"));
    }
}

void CsaReader::ReadRatingLine(const std::string &line) {
    const auto &tmp = Util::Split(line.substr(1), ':');
    if (tmp.size() != 3) {
        throw CsaException(file_name, "ReadRatingLine error:" + line);
    }
    if (tmp[0] == "black_rate") {
        black_rate = std::atof(tmp[2].c_str());
    } else if (tmp[0] == "white_rate") {
        white_rate = std::atof(tmp[2].c_str());
    }
}

int CsaReader::ReadHeader() {
    // ヘッダ部分読み込み
    version = lines[0];
    black_player = lines[1];
    white_player = lines[2];

    // Version確認
    if (version != "V2") {
        throw CsaException(file_name, "version error:" + version);
    }

    // プレーヤー
    if (!Util::StartWith(black_player, "N+")) {
        throw CsaException(file_name, "black_player error:" + black_player);
    }
    if (!Util::StartWith(white_player, "N-")) {
        throw CsaException(file_name, "white_player error:" + white_player);
    }

    black_player = black_player.substr(2);
    white_player = white_player.substr(2);

    // 設定読みこみ
    int line_idx = ReadExpectedLines(
        3, "'",
        [](CsaReader *reader, const std::string &line) {
            reader->ReadSettingLine(line);
        },
        "$");

    // 開始時間取得
    line_idx = ReadExpectedLines(
        line_idx, "$",
        [](CsaReader *reader, const std::string &line) {
            reader->ReadStartTime(line);
        },
        "P");

    // 初期配置確認
    line_idx = ReadExpectedLines(
        line_idx, "P",
        [](CsaReader *reader, const std::string &line) {
            reader->initial_pos += line + "\n";
        },
        "+");

    if (initial_pos != hirate_initial_pos) {
        throw CsaException(file_name, "initial pos error\n" + initial_pos);
    }

    // レート確認
    line_idx = ReadExpectedLines(
        line_idx + 1, "'",
        [](CsaReader *reader, const std::string &line) {
            reader->ReadRatingLine(line);
        },
        "+");

    return line_idx;
}

int CsaReader::ReadMoveList(int line_idx) {
    std::string board[9][9];
    for (int i = 0; i < 9; i++) {
        for (int j = 0; j < 9; j++) {
            board[i][j] = initial_pos.substr(i * 30 + 2 + j * 3, 3);
        }
    }
    bool black = true;
    int tesuu = 0;

    while (true) {
        std::string line = lines.at(line_idx++);
        // コメントは削除
        const char teban = line.at(0);
        if (teban == '-' || teban == '+') {
            move_list.push_back(line);
        }

        // 終了コマンド
        if (line[0] == '%') {
            last_command = Util::Split(line, ',')[0];
            break;
        }
        if (move_list.size() == max_moves) {
            break;
        }
    }

    // 手がないときはエラー
    if (move_list.empty()) {
        throw CsaException(file_name, "No Move");
    }

    // 千日手の場合は最後の手を削除
    if (last_command == "SENNICHITE") {
        move_list.pop_back();
    }

    for (int i = 0; i < move_list.size(); i++) {
        std::string line = move_list[i];

        // コメントは削除
        const char teban = line.at(0);
        std::string move = line.substr(1);
        if (move.size() > 6) {
            int pos = move.find('\'');
            if (pos != std::string::npos) {
                move = move.substr(0, move.find_last_not_of(" ", pos - 1) + 1);
            }
        }
        // サイズ確認
        if (move.size() != 6)
            throw CsaException(file_name,
                               "move size eror:" + std::to_string(i));

        // 手番確認
        if ((black && teban != '+') || (!black && teban != '-'))
            throw CsaException(file_name, "move teban eror:" + line);

        //
        int src_col = '9' - move[0];
        int src_row = move[1] - '1';
        int dst_col = '9' - move[2];
        int dst_row = move[3] - '1';
        std::string koma = move.substr(4);

        if (src_col == 9 && src_row == -1) {
            // 持っているか確認
            if (mochigoma[black ? 0 : 1][koma] == 0) {
                throw new CsaException(
                    file_name, "no koma:" + line + ":" + std::to_string(teban));
            }

            // 持ち駒
            board[dst_row][dst_col] = (black ? "+" : "-") + koma;
            mochigoma[black ? 0 : 1][koma]--;
        } else {
            // けたチェック
            auto check = [](int a) -> bool { return a < 0 || a >= 9; };
            if (check(src_col) || check(src_row) || check(dst_col) ||
                check(dst_row)) {
                throw CsaException(file_name, "koma error:" + line);
            }

            // 元の駒が同じかチェック
            const std::string &src = board[src_row][src_col];
            if (teban + koma != src && teban + narimotokoma(koma) != src) {
                throw CsaException(file_name,
                                   "src koma error:" + line + "\n" +
                                       boardToString((std::string *)board));
            }

            // コマを撮る
            if (board[dst_row][dst_col] == " * ")
                ;
            else if (board[dst_row][dst_col][0] == (black ? '-' : '+')) {
                mochigoma[black ? 0 : 1]
                         [narimotokoma(board[dst_row][dst_col].substr(1))]++;
            } else
                throw CsaException(file_name, "move to my pos:" + line + ":" +
                                                  std::to_string(teban));

            // コマを移動
            board[dst_row][dst_col] = teban + koma;
            board[src_row][src_col] = " * ";
        }
        black = !black;
    }
    end_pos = boardToString((std::string *)board);
    return line_idx;
}

void CsaReader::ReadFooter(int line_idx) {
    std::vector<std::string> Plist;

    while (line_idx < lines.size()) {
        const std::string &line = lines.at(line_idx++);

        // 最後の局面
        if (Util::StartWith(line, "'P")) {
            Plist.push_back(line.substr(1));
        }

        // サマリー
        if (Util::StartWith(line, "'summary:")) {
            summary = line.substr(strlen("'summary:"));
        }

        // 終了日時
        if (Util::StartWith(line, "'$END_TIME:")) {
            end_time = line.substr(strlen("'$END_TIME:"));
        }
    }

    if (Plist.size() < 9 || Plist.size() > 11) {
        throw CsaException(file_name,
                           "Plist size:" + std::to_string(Plist.size()));
    }
    if (summary.empty()) {
        throw CsaException(file_name, "no summary");
    }
    if (end_time.empty()) {
        throw CsaException(file_name, "no end_time");
    }

    // 最後の局面
    std::string end;
    for (int i = 0; i < 9; i++) {
        end += Plist[i].substr(2) + "\n";
    }

    if (end_pos != end) {
        throw CsaException(file_name, "end pos error\n" + end_pos + "\n" + end);
    }

    // 持ち駒の確認
    std::map<std::string, int> end_mochigoma[2];
    for (int i = 9; i < Plist.size(); i++) {
        std::string line = Plist[i];
        int teban = line.at(1) == '+' ? 0 : 1;
        for (int j = 0; j < ((line.size() - 2) / 4); j++)
            end_mochigoma[teban][line.substr(2 + j * 4 + 2, 2)]++;
    }
    for (int i = 0; i < 2; i++) {
        for (const std::string &koma : std::vector<std::string>{
                 "FU", "KY", "KE", "GI", "KI", "KA", "HI"}) {
            if (mochigoma[i][koma] != end_mochigoma[i][koma])
                throw CsaException(file_name, "mochigoma:" + koma);
        }
    }

    end_pos = "";
    for (const std::string &line : Plist) {
        end_pos += line + "\n";
    }
}

void CsaReader::ReadCsa(const std::string &file) {
    file_name = file;
    // ファイルオープン
    std::ifstream ifs(file_name);
    if (!ifs) {
        throw CsaException(file_name, " cannot open");
    }

    // 行の読み込み
    std::string line;
    while (std::getline(ifs, line)) {
        if (line.size() == 0) {
            throw CsaException(file_name, "void line error");
        }
        lines.push_back(line);
    }

    // ヘッダ読みこみ
    int line_idx = ReadHeader();

    // 手の読み込み
    line_idx = ReadMoveList(line_idx);

    // フッターの読みこみ
    ReadFooter(line_idx);
}
