#include <fstream>
#include <iostream>

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
        char capture;
        bool nari;
        Teban teban() const { return koma > 0 ? Black : White; }
    } Hand;

    typedef struct TKyokumen {
        char board[9 * 9];
        char mochigoma_list[7][2];
        bool IsSame(const TKyokumen &k) const {
            return memcmp(board, k.board, sizeof(board)) == 0 &&
                   memcmp(mochigoma_list, k.mochigoma_list,
                          sizeof(mochigoma_list)) == 0;
        }
    } Kyokumen;

    constexpr static Point KOMADAI = 81;
    const static std::map<char, Teban> TEBAN_CONVERTER;
    const static std::map<std::string, Koma> KOMA_CONVERTER;

    void Init(const std::string &initial_pos);
    char StringToKomaOnMasu(const std::string &str);
    void StringToKyokumen(const std::string &str, Kyokumen *kyokumen);
    Point StringToPoint(const std::string &str_point);
    Hand StringToHand(const std::string &str_hand);
    void Move(const std::string &str_hand) { Move(StringToHand(str_hand)); }
    void Move(const Hand &hand);

   private:
    Kyokumen kyokumen;
    std::vector<Hand> hand_list;
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
            kyokumen->mochigoma_list[(teban == Black ? 0 : 1)][koma]++;
        }
    }
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

    hand.nari = false;
    hand.capture = 0;
    return hand;
}

void SingleBoard::Move(const Hand &hand) {
    // TODO IllegalCheck

    Hand new_hand = hand;
    if (hand.from != KOMADAI) {
        // 成り判定
        new_hand.nari = kyokumen.board[hand.from] != hand.koma;
        // 駒どり
        char dst_koma = kyokumen.board[hand.to];
        if (dst_koma) {
            new_hand.capture = dst_koma;
            kyokumen.mochigoma_list[hand.teban() == Black ? 0 : 1]
                                   [abs(dst_koma) % 8]++;
        }

        // コマの移動
        kyokumen.board[hand.from] = 0;
        kyokumen.board[hand.to] = hand.koma;
    } else {
        // 駒うち
        kyokumen
            .mochigoma_list[hand.teban() == Black ? 0 : 1][abs(hand.koma)]--;
        kyokumen.board[hand.to] = hand.koma;
    }
    hand_list.push_back(new_hand);
}

int main(int argc, char **argv) {
    if (argc < 1) {
        std::cerr << "no file" << std::endl;
        return -1;
    }
    std::ifstream csv("output.csv");
    if (!csv) {
        throw CsaException(argv[1], "cannot read");
    }

    std::string line;
    // ヘッダ読み飛ばし
    std::getline(csv, line);

    std::getline(csv, line);
    std::string file_name = Util::Split(line, ',')[1];
    file_name = file_name.substr(1, file_name.length() - 2);

    file_name = "/Users/yanyano0207/Downloads/2020/" + file_name;
    std::cout << "filename:" << file_name << std::endl;
    CsaReader reader;
    reader.ReadCsa(file_name);

    SingleBoard board;
    board.Init(reader.initial_pos);
    for (const std::string &str_move : reader.move_list) {
        board.Move(str_move);
    }

    SingleBoard::Kyokumen end_kyokumen;
    board.StringToKyokumen(reader.end_pos, &end_kyokumen);
    if (board.kyokumen.IsSame(end_kyokumen)) {
        Throw("end kyokumen differnt");
    }
}
