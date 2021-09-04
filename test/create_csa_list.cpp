#include <filesystem>
#include <fstream>
#include <iostream>

#include "../util.h"
#include "./read_csa.h"

int main(int argc, char **argv) {
    std::cout << "argc:" << argc << std::endl;

    std::vector<std::string> file_list;
    for (int i = 1; i < argc; i++) {
        const char *file = argv[i];
        if (std::filesystem::exists(file)) {
            if (std::filesystem::is_directory(file)) {
                for (const auto &f : std::filesystem::directory_iterator(file))
                    file_list.push_back(f.path().c_str());
            } else {
                file_list.push_back(file);
            }
        }
    }

    int file_num = 0;
    int ok_num = 0;

    std::ofstream ofs("output.csv");
    ofs << "No,file_name,start_time,end_time"
           ",black_player,white_player"
           ",black_rate,white_rate"
           ",move_num,last_command,summary"
        << std::endl;
    for (const std::string &file : file_list) {
        CsaReader reader;
        if (Util::EndWith(file, ".csa")) {
            file_num++;
            try {
                reader.ReadCsa(file);
                ok_num++;
                ofs << ok_num;
                ofs << "," << std::filesystem::path(file).filename();
                ofs << "," << reader.start_time;
                ofs << "," << reader.end_time;
                ofs << "," << reader.black_player;
                ofs << "," << reader.white_player;
                ofs << "," << reader.black_rate;
                ofs << "," << reader.white_rate;
                ofs << "," << reader.move_list.size();
                ofs << "," << reader.last_command;
                ofs << "," << reader.summary;
                ofs << std::endl;
            } catch (const CsaException &e) {
                std::cerr << e.what() << std::endl;
            } catch (const std::runtime_error &e) {
                std::cerr << e.what() << std::endl;
            } catch (const std::out_of_range &e) {
                std::cerr << "out_of_range:" << file << ":" << e.what()
                          << std::endl;
            } catch (...) {
                std::cerr << "unexpected error:" << file << std::endl;
            }
        }
    }
    std::cout << "file_num:" << file_num << std::endl;
    std::cout << "ok_num:" << ok_num << std::endl;
    std::cout << "error_num:" << file_num - ok_num << std::endl;
}
