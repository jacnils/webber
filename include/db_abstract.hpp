#pragma once

#include <limhamn/database/database.hpp>
#include <functional>

namespace webber {
        class database {
#if WEBBER_ENABLE_SQLITE
        limhamn::database::sqlite3_database sqlite{};
#define SQLITE_HANDLE this->sqlite
#endif
#if WEBBER_ENABLE_POSTGRESQL
        limhamn::database::postgresql_database postgres{};
#define POSTGRES_HANDLE this->postgres
#endif
#ifndef SQLITE_HANDLE
#define SQLITE_HANDLE this->postgres
#endif
#ifndef POSTGRES_HANDLE
#define POSTGRES_HANDLE this->sqlite
#endif

        bool enabled_type = false; // false = sqlite, true = postgres
        std::function<void(const std::string&, bool)> callback;
    public:
        explicit database(bool type, const std::function<void(const std::string&, bool)>& callback) : enabled_type(type), callback(callback) {}
        std::vector<std::unordered_map<std::string, std::string>> query(const std::string& query) {
            if (!this->enabled_type) {
                if (this->callback) callback(query, enabled_type);
                return SQLITE_HANDLE.query(query);
            }

            if (this->callback) callback(query, enabled_type);
            return POSTGRES_HANDLE.query(query);
        }
        bool exec(const std::string& query) {
            if (!this->enabled_type) {
                if (this->callback) callback(query, enabled_type);
                return SQLITE_HANDLE.exec(query);
            }
            if (this->callback) callback(query, enabled_type);
            return POSTGRES_HANDLE.exec(query);
        }
        template <typename... Args>
        std::vector<std::unordered_map<std::string, std::string>> query(const std::string& query, Args... args) {
            if (!this->enabled_type) {
                if (this->callback) callback(query, enabled_type);
                return SQLITE_HANDLE.query(query, args...);
            }
            if (this->callback) callback(query, enabled_type);
            return POSTGRES_HANDLE.query(query, args...);
        }
        template <typename... Args>
        bool exec(const std::string& query, Args... args) {
            if (!this->enabled_type) {
                if (this->callback) callback(query, enabled_type);
                return SQLITE_HANDLE.exec(query, args...);
            }
            if (this->callback) callback(query, enabled_type);
            return POSTGRES_HANDLE.exec(query, args...);
        }
        [[nodiscard]] bool good() const {
            return this->enabled_type ? POSTGRES_HANDLE.good() : SQLITE_HANDLE.good();
        }
#if WEBBER_ENABLE_SQLITE
        limhamn::database::sqlite3_database& get_sqlite() {
            return this->sqlite;
        }
#endif
#if WEBBER_ENABLE_POSTGRESQL
        limhamn::database::postgresql_database& get_postgres() {
            return this->postgres;
        }
#endif
    };
}