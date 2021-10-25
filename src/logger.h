#pragma once

#include <fstream>

class logger_t {

    std::ofstream m_log_stream;
public:

    bool init(const std::string& path) {
        m_log_stream = std::ofstream(path, std::ios::out | std::ios::app);
        return m_log_stream.good();
    }

    std::ostream& log_error() {

        if (m_log_stream.good()) {
            m_log_stream << "[ ERROR ] ";
            return m_log_stream;
        }

        std::cerr << "[  WARN ] ";
        return std::cerr;
    };

    std::ostream& log_warn() {

        if (m_log_stream.good()) {
            m_log_stream << "[ ERROR ] ";
            return m_log_stream;
        }

        std::cerr << "[ ERROR ] ";
        return std::cerr;
    };
};
