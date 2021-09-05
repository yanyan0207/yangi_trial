#include <fstream>
#include <iostream>
#include <sstream>

#include "../util.h"
#include "./read_csa.h"

class SingleBoard {
    friend int main(int, char **);

   public:
    enum Teban { White = -1, Black = 1 };

    enum Koma {
        No = 0,
        Fu = 1,
        Kyou = 2,
        Kei = 3,
        Gin = 4,
        Kaku = 5,
        Hi = 6,
        Kin = 7,
        Ou = 8,
        To = 9,
        Narikyou = 10,
        Narikei = 11,
        Narigin = 12,
        Uma = 13,
        Ryuu = 14
    };

    typedef char Point;

    typedef struct THand {
        Point from;
        Point to;
        char koma;
        Teban teban() const { return koma > 0 ? Black : White; }
    } Hand;

    typedef struct THandExtraInfo {
        char capture;
        bool nari;
    } HandExtraInfo;

    typedef struct TKyokumen {
        char board[9 * 9];
        char mochigoma_list[2][7];
        bool IsSame(const TKyokumen &k) const {
            return memcmp(board, k.board, sizeof(board)) == 0 &&
                   memcmp(mochigoma_list, k.mochigoma_list,
                          sizeof(mochigoma_list)) == 0;
        }
    } Kyokumen;

    constexpr static Point KOMADAI = 81;
    const static std::map<char, Teban> TEBAN_CONVERTER;
    const static std::map<std::string, Koma> KOMA_CONVERTER;

    static char StringToKomaOnMasu(const std::string &str);
    static void StringToKyokumen(const std::string &str, Kyokumen *kyokumen);
    static std::string KyokumenToString(const Kyokumen &kyokumen);
    static Point StringToPoint(const std::string &str_point);
    static Hand StringToHand(const std::string &str_hand);

    void Init(const std::string &initial_pos);
    void AppendHand(const std::string &str_hand) {
        hand_list.push_back(StringToHand(str_hand));
    }
    void SetHandList(const std::vector<std::string> &str_hand_list) {
        for (const std::string &str_hand : str_hand_list) AppendHand(str_hand);
    }
    int Move();

   private:
    Kyokumen kyokumen;
    std::vector<Hand> hand_list;
    std::vector<HandExtraInfo> hand_extra_info_list;
    int tesuu;
};

const std::map<char, SingleBoard::Teban> SingleBoard::TEBAN_CONVERTER = {
    {'+', SingleBoard::Black}, {'-', SingleBoard::White}};

const std::map<std::string, SingleBoard::Koma> SingleBoard::KOMA_CONVERTER = {
    {"FU", SingleBoard::Fu},      {"KY", SingleBoard::Kyou},
    {"KE", SingleBoard::Kei},     {"GI", SingleBoard::Gin},
    {"KA", SingleBoard::Kaku},    {"HI", SingleBoard::Hi},
    {"KI", SingleBoard::Kin},     {"OU", SingleBoard::Ou},
    {"TO", SingleBoard::To},      {"NY", SingleBoard::Narikyou},
    {"NK", SingleBoard::Narikei}, {"NG", SingleBoard::Narigin},
    {"UM", SingleBoard::Uma},     {"RY", SingleBoard::Ryuu}};

static void Throw(const std::string &error) { throw CsaException("", error); }

void SingleBoard::Init(const std::string &initial_pos) {
    StringToKyokumen(initial_pos, &kyokumen);
    hand_list.clear();
    tesuu = 0;
}

char SingleBoard::StringToKomaOnMasu(const std::string &str) {
    assert(str.size() == 3);
    if (str == " * ") {
        return 0;
    } else {
        Teban teban = TEBAN_CONVERTER.at(str.at(0));
        Koma koma = KOMA_CONVERTER.at(str.substr(1));
        return koma * teban;
    }
}

void SingleBoard::StringToKyokumen(const std::string &str, Kyokumen *kyokumen) {
    memset(kyokumen, 0, sizeof(str));

    auto lines = Util::Split(str, '\n');
    if (lines.back().empty()) lines.pop_back();
    if (lines.size() < 9 || lines.size() > 11) {
        Throw("board size error");
    }

    // ボードの読み込み
    for (int row = 0; row < 9; row++) {
        const std::string &line = lines[row];
        if (line.length() != 29) Throw("line size error");
        if (!Util::StartWith(line, 'P' + std::to_string(row + 1)))
            Throw("line hedder error");
        for (int col = 0; col < 9; col++) {
            kyokumen->board[row * 9 + col] =
                StringToKomaOnMasu(line.substr(2 + col * 3, 3));
        }
    }

    // 持ち駒の読み込み
    for (int i = 9; i < lines.size(); i++) {
        const std::string &line = lines[i];
        if (line[0] != 'P') Throw("Mochigoma NOT P" + str);
        Teban teban = TEBAN_CONVERTER.at(line[1]);
        for (int j = 0; j < (line.size() - 2) / 4; j++) {
            const std::string &mochikoma_str = line.substr(2 + j * 4, 4);
            if (!Util::StartWith(mochikoma_str, "00"))
                Throw("Mochikoma 00 error");
            Koma koma = KOMA_CONVERTER.at(mochikoma_str.substr(2));
            if (koma > Kin) Throw("Mochikoma large num error");
            kyokumen->mochigoma_list[(teban == Black ? 0 : 1)][koma - 1]++;
        }
    }
}

std::string SingleBoard::KyokumenToString(const Kyokumen &kyokumen) {
    std::stringstream ss;
    for (int row = 0; row < 9; row++) {
        ss << "P" << row + 1;
        for (int col = 0; col < 9; col++) {
            char koma = kyokumen.board[row * 9 + col];
            if (koma == 0) {
                ss << " * ";
            } else {
                ss << (koma > 0 ? '+' : '-');
                std::string koma_str;
                for (const auto &pair : SingleBoard::KOMA_CONVERTER) {
                    if (pair.second == abs(koma)) {
                        ss << pair.first;
                        break;
                    }
                }
            }
        }
        ss << std::endl;
    }
    std::vector<char> no_koma(7);
    for (int teban = 0; teban < 2; teban++) {
        if (memcmp(kyokumen.mochigoma_list[teban], &no_koma[0], 7) == 0)
            continue;
        ss << "P" << (teban ? '-' : '+');
        for (const auto &pair : SingleBoard::KOMA_CONVERTER) {
            if (pair.second >= SingleBoard::Ou) continue;
            for (int num = 0;
                 num < kyokumen.mochigoma_list[teban][pair.second - 1]; num++) {
                ss << "00" << pair.first;
            }
        }
        ss << std::endl;
    }
    return ss.str();
}

SingleBoard::Point SingleBoard::StringToPoint(const std::string &str_point) {
    if (str_point.size() != 2) Throw("point size error");

    if (str_point == "00") {
        return KOMADAI;
    }

    int col = '9' - str_point.at(0);
    int row = str_point.at(1) - '1';

    if (col < 0 || col >= 9 || row < 0 || row >= 9) Throw("point error");

    return row * 9 + col;
}

SingleBoard::Hand SingleBoard::StringToHand(const std::string &str_hand) {
    if (str_hand.size() != 7) Throw("hand size error");
    Hand hand;
    Teban teban = TEBAN_CONVERTER.at(str_hand.at(0));
    hand.from = StringToPoint(str_hand.substr(1, 2));
    hand.to = StringToPoint(str_hand.substr(3, 2));
    if (hand.to == KOMADAI) Throw("dst KOMADA error");
    hand.koma = KOMA_CONVERTER.at(str_hand.substr(5, 2)) * teban;
    return hand;
}

int SingleBoard::Move() {
    if (tesuu >= hand_list.size()) return 0;

    const Hand &hand = hand_list.at(tesuu);

    // 追加情報
    assert(hand_extra_info_list.size() >= tesuu);
    if (hand_extra_info_list.size() == tesuu) {
        HandExtraInfo extra_info;
        // TODO IllegalCheck
        // 成り判定
        extra_info.nari = kyokumen.board[hand.from] != hand.koma;
        // 駒どり
        extra_info.capture = kyokumen.board[hand.to];
        hand_extra_info_list.push_back(extra_info);
    }

    const HandExtraInfo &info = hand_extra_info_list.at(tesuu);

    if (hand.from == KOMADAI) {
        kyokumen.board[hand.to] = hand.koma;
        // 駒台から減らす
        kyokumen.mochigoma_list[hand.teban() == Black ? 0 : 1]
                               [abs(hand.koma) - 1]--;

    } else {
        kyokumen.board[hand.from] = 0;
        kyokumen.board[hand.to] = hand.koma;
        // 駒を取った場合は駒台に追加
        if (info.capture) {
            kyokumen.mochigoma_list[hand.teban() == Black ? 0 : 1]
                                   [abs(info.capture) % 8 - 1]++;
        }
    }
#if 0
    int koma_num = 0;
    for (int i = 0; i < 81; i++)
        if (kyokumen.board[i]) koma_num++;
    for (int i = 0; i < 2; i++)
        for (int j = 0; j < 7; j++) koma_num += kyokumen.mochigoma_list[i][j];
    std::cout << koma_num << std::endl;
    assert(koma_num == 40);
#endif

    tesuu++;
    return 1;
}

int main(int argc, char **argv) {
    if (argc < 1) {
        std::cerr << "no file" << std::endl;
        return -1;
    }

    std::ifstream csv(argv[1]);
    if (!csv) {
        throw CsaException(argv[1], "cannot read");
    }
    std::ofstream out("output_feature.csv");
    out << "filename"
        << ","
        << "tesuu";
    for (int row = 9; row > 0; row--) {
        for (int col = 1; col <= 9; col++) {
            out << ","
                << "Point_" << col << row;
        }
    }
    for (int i = 0; i < 2; i++) {
        for (const auto &pair : SingleBoard::KOMA_CONVERTER) {
            if (pair.second == SingleBoard::Ou) break;
            out << "," << (i == 0 ? "Black_" : "White_") << pair.first;
        }
    }
    out << std::endl;

    std::string line;
    // ヘッダ読み飛ばし
    std::getline(csv, line);

    while (std::getline(csv, line)) {
        std::string file_name = Util::Split(line, ',')[1];

        std::istringstream ss(file_name);
        //    file_name = file_name.substr(1, file_name.length() - 2);
        std::filesystem::path p;
        ss >> p;
        std::filesystem::path file_path = "/Users/yanyano0207/Downloads/2020/";
        file_path += p;
        std::cout << "filename:" << file_path.c_str() << std::endl;
        CsaReader reader;
        reader.ReadCsa(file_path.c_str());

        SingleBoard board;
        board.Init(reader.initial_pos);
        board.SetHandList(reader.move_list);

        // 手番を進める
        for (int tesuu = 0; tesuu < reader.move_list.size(); tesuu++) {
#if 0
            std::cout << tesuu + 1 << std::endl;
            std::cout << board.KyokumenToString(board.kyokumen) << std::endl;
            std::cout << reader.move_list[tesuu] << std::endl;
#endif
            out << file_name;
            out << "," << tesuu + 1;

            // 局面を出力
            for (int point = 0; point < 9 * 9; point++)
                out << "," << (int)board.kyokumen.board[point];
            // 持ち駒を出力
            for (int teban = 0; teban < 2; teban++)
                for (int kind = 0; kind < 7; kind++)
                    out << "," << (int)board.kyokumen.board[kind];

            if (board.Move() == 0) {
                Throw("move cant error");
            }
            out << std::endl;
        }

        SingleBoard::Kyokumen end_kyokumen;
        board.StringToKyokumen(reader.end_pos, &end_kyokumen);
        if (!board.kyokumen.IsSame(end_kyokumen)) {
            std::cerr << reader.end_pos << std::endl;
            std::cerr << board.KyokumenToString(board.kyokumen) << std::endl;
            Throw("end kyokumen differnt");
        }
    }
}
