#pragma once

#include <chrono>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <optional>
#include <random>
#include <string>

namespace test_support {
    class ScopedEnvVar {
    public:
        ScopedEnvVar(std::string key, std::optional<std::string> value) : key_(std::move(key)) {
            const char* current = std::getenv(key_.c_str());
            if (current != nullptr){
                oldValue_ = std::string(current);
            }

            if (value.has_value()){
                setenv(key_.c_str(), value->c_str(), 1);
            }
            else{
                unsetenv(key_.c_str());
            }
        }

        ~ScopedEnvVar() {
            if (oldValue_.has_value()){
                setenv(key_.c_str(), oldValue_->c_str(), 1);
            }
            else{
                unsetenv(key_.c_str());
            }
        }

    private:
        std::string key_;
        std::optional<std::string> oldValue_;
    };

    class TempDir {
    public:
        explicit TempDir(const std::string &prefix) {
            const auto now = static_cast<unsigned long long>(
                std::chrono::high_resolution_clock::now().time_since_epoch().count());
            std::mt19937_64 rng(now ^ 0x9e3779b97f4a7c15ULL);
            const auto suffix = static_cast<unsigned long long>(rng());

            path_ = std::filesystem::temp_directory_path() /
                    (prefix + "_" + std::to_string(now) + "_" + std::to_string(suffix));
            std::filesystem::create_directories(path_);
        }

        ~TempDir() {
            std::error_code ec;
            std::filesystem::remove_all(path_, ec);
        }

        const std::filesystem::path &path() const {
            return path_;
        }

    private:
        std::filesystem::path path_;
    };

    inline void writeTextFile(const std::filesystem::path &path, const std::string &content) {
        std::filesystem::create_directories(path.parent_path());
        std::ofstream out(path, std::ios::trunc);
        out << content;
    }
} // namespace test_support