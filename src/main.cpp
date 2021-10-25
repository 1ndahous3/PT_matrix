#define _CRT_SECURE_NO_WARNINGS

#include <iostream>
#include <fstream>
#include <iomanip>

#include <thread>
#include <vector>

#include <string>

#include "logger.h"
#include "thread_pool.h"

#define NATURAL_NUMBER_MAX_DIGITS  20                              // 18446744073709551615
#define ELEMENT_WIDTH              (NATURAL_NUMBER_MAX_DIGITS + 1) // + ' ' or '\n'


static const char g_usage_msg[] = R"(
Usage: tool <src> <dst>

<src> - file with matrix in strict text format:
3 45 5\n
24 5 4\n
4 55 3

<dst> - file for transposed matrix:
                   3                   24                    4\n
                  45                    5                   55\n
                   5                    4                    3\n
)";

logger_t g_logger;
std::mutex g_logger_mutex; // for logging from threads

// парсим кол во элементов на примере первой строки
size_t parse_rows_count(std::ifstream& ifs) {

    auto eof = std::istreambuf_iterator<char>();

    size_t count = 0;

    for (std::istreambuf_iterator<char> it(ifs); it != eof && *it != '\n'; it++) {
        if (*it == ' ') {
            count++;
        }
    }

    return count + 1;
}

size_t get_lines_count(std::ifstream& ifs) {
    return (size_t)std::count(std::istreambuf_iterator<char>(ifs), std::istreambuf_iterator<char>(), '\n') + 1;
}

void line_worker(size_t line,
                 size_t rows, size_t lines,
                 const std::string& in_file, const std::string& out_file,
                 size_t offset, std::atomic<bool>& need_stop) noexcept {

    std::ifstream ifs(in_file);
    ifs.seekg(offset, std::ios::beg);

    std::ofstream ofs(out_file, std::ios_base::in | std::ios_base::out | std::ios_base::binary | std::ios_base::ate);

    if (!ofs.good()) {

        std::lock_guard g(g_logger_mutex);
        g_logger.log_error() << "unable to open output file" << std::endl;

        need_stop = true;
        return;
    }

    uint64_t elem;

    for (size_t i = 0; i < rows; i++) {

        if (need_stop) {
            return;
        }

        ifs >> elem;

        if (ifs.fail()) {

            std::lock_guard g(g_logger_mutex);
            g_logger.log_error() << "invalid uint64_t elem, line = " << line << ", elem = " << i << std::endl;

            need_stop = true;
            return;
        }

        if (ifs.bad()) {

            std::lock_guard g(g_logger_mutex);
            g_logger.log_error() << "unable to get elem, line = " << line << ", elem = " << i << std::endl;

            need_stop = true;
            return;
        }

        if (ifs.eof() && i != rows - 1) {

            std::lock_guard g(g_logger_mutex);
            g_logger.log_error() << "invalid content, line = " << line << ", elem = " << i << std::endl;

            need_stop = true;
            return;
        }

        ofs.seekp(ELEMENT_WIDTH * (lines * i + line));
        ofs << std::setfill(' ') << std::setw(NATURAL_NUMBER_MAX_DIGITS) << elem << ((line == lines - 1) ? '\n' : ' ');
    }

    {
        std::string extra_data;
        std::getline(ifs, extra_data);

        if (ifs.bad()) {
            std::lock_guard g(g_logger_mutex);
            g_logger.log_error() << "unable to check end elems, line = " << line << std::endl;

            need_stop = true;
            return;
        }

        if (!extra_data.empty()) {

            std::lock_guard g(g_logger_mutex);
            g_logger.log_error() << "extra symbol on line = " << line << std::endl;

            need_stop = true;
            return;
        }
    }

    ofs.close();
}

int main(int argc, char **argv) {
   
    if (argc != 3) {
        std::cerr << g_usage_msg << std::endl;
        return 1;
    }

    if (!g_logger.init("error.log")) {
        g_logger.log_warn() << "unble to open log file" << std::endl;
    }

    std::ifstream ifs(argv[1]);

    if (!ifs) {
        g_logger.log_error() << "unable to open source file, error = " << std::strerror(errno) << std::endl;
        return 1;
    }

    std::atomic<bool> need_stop = false;

    // проход для вычисления размеров матрицы
    // валидация параметров будет происходить лениво, т.е. на этапе транпонирования

    auto rows = parse_rows_count(ifs);
    auto lines = get_lines_count(ifs);

    // вычисляем размер выходного файла и сразу создаем болванку для многопоточной записи

    std::ofstream ofs(argv[2], std::ios::binary | std::ios::out);

    ofs.seekp(ELEMENT_WIDTH * rows * lines - 1);
    ofs.write("", 1); // zeroed file
    ofs.close();

    auto eof = std::istreambuf_iterator<char>();

    ifs.seekg(0, std::ios::beg);
    std::istreambuf_iterator<char> it(ifs);

    size_t offset = 0;

    const std::string in_file = argv[1];
    const std::string out_file = argv[2];

    {
        thread_pool_t thread_pool(std::thread::hardware_concurrency() ? std::thread::hardware_concurrency() : 2);

        for (size_t i = 0; i < lines; i++) {

            thread_pool.push_task([=, &out_file, &in_file, &need_stop]() {
                line_worker(i, rows, lines, std::ref(in_file), std::ref(out_file), offset, std::ref(need_stop));
            });

            for (; it != eof && *it != '\n'; it++) {
                offset++;
            }

            if (it != eof) {
                offset++;
                it++;
            }
        }
    }

    return need_stop ? 1 : 0;
}
