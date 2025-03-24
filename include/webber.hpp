#pragma once

#include <limhamn/logger/logger.hpp>
#include <limhamn/http/http_server.hpp>
#include <db_abstract.hpp>

namespace webber {
    struct Settings {
#ifndef WEBBER_DEBUG
        std::string access_file{"/var/log/webber/access.log"};
        std::string warning_file{"/var/log/webber/warning.log"};
        std::string error_file{"/var/log/webber/error.log"};
        std::string notice_file{"/var/log/webber/notice.log"};
        bool output_to_std{false};
        bool halt_on_error{false};
        std::string sqlite_database_file{"/var/db/webber/webber.db"};
        std::string session_directory{"/var/lib/webber/sessions"};
        std::string data_directory{"/var/lib/webber/data"};
        std::string temp_directory{"/var/tmp/webber"};
        std::vector<std::pair<std::string, std::string>> custom_paths{};
        int64_t max_request_size{250 * 1024 * 1024}; // 250mb
        std::string site_url{"https://jacobnilsson.com"};
#else
        std::string access_file{"./access.log"};
        std::string warning_file{"./warning.log"};
        std::string error_file{"./error.log"};
        std::string notice_file{"./notice.log"};
        std::string sqlite_database_file{"./webber-debug.db"};
        bool output_to_std{true};
        bool halt_on_error{false};
        std::string session_directory{"./sessions"};
        std::string data_directory{"./data"};
        std::string temp_directory{"./tmp"};
        std::vector<std::pair<std::string, std::string>> custom_paths{};
        int64_t max_request_size{1024 * 1024 * 1024};
        std::string site_url{"http://localhost:8080"};
#endif
        int port{8081};
        bool log_access_to_file{true};
        bool log_warning_to_file{true};
        bool log_error_to_file{true};
        bool log_notice_to_file{true};
        std::size_t password_min_length{8};
        std::size_t password_max_length{64};
        std::size_t username_min_length{3};
        std::size_t username_max_length{32};
        std::string allowed_characters{"abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789"};
        bool allow_all_characters{false};
        std::string session_cookie_name{"webber_session"};
        bool preview_files{true};
        std::string psql_username{"postgres"};
        std::string psql_password{"postgrespasswordhere"};
        std::string psql_database{"webber"};
        std::string psql_host{"localhost"};
        int psql_port{5432};
        bool enabled_database{false}; // false = sqlite, true = postgres
        bool trust_x_forwarded_for{false};
        int rate_limit{100};
        std::vector<std::string> blacklisted_ips{};
        std::vector<std::string> whitelisted_ips{"127.0.0.1"};
        int64_t max_file_size_hash{1024 * 1024 * 1024};
    };

    enum class UserType : int {
        Undefined = -1,
        User = 0,
        Administrator = 1,
    };

    enum class AccountCreationStatus {
        Success,
        Failure,
        UsernameExists,
        UsernameTooShort,
        UsernameTooLong,
        PasswordTooShort,
        PasswordTooLong,
        InvalidUsername,
        InvalidPassword,
        InvalidEmail,
        EmailExists,
    };

    enum class LoginStatus {
        Success,
        Failure,
        Inactive,
        InvalidUsername,
        InvalidPassword,
        Banned,
    };

    struct FileConstruct {
        std::string path{};
        std::string name{};
        std::string username{};
        std::string ip_address{};
        std::string user_agent{};
    };

    struct RetrievedFile {
        std::string path{};
        std::string name{};
    };

    struct UserProperties {
        std::string username{}; /* only filled in if cookie is valid */
        std::string ip_address{};
        std::string user_agent{};
    };

    inline limhamn::logger::logger logger{};
    inline Settings settings{};
    inline bool fatal{false};
    inline bool needs_setup{false};

    void print_help();
    Settings load_settings(const std::string&);
    std::string get_default_config();
    void prepare_wd();
    void clean_data();
    void server_init();
    std::string open_file(const std::string&);
    void setup_database(database&);

    UserType get_user_type(database&, const std::string&);
    std::pair<LoginStatus, std::string> try_login(database&, const std::string&, const std::string&,
        const std::string&, const std::string&, limhamn::http::server::response&);
    AccountCreationStatus make_account(database&, const std::string&, const std::string&, const std::string&, const std::string&, const std::string&, UserType);
    std::string get_email_from_username(database&, const std::string&);
    std::string get_username_from_email(database&, const std::string&);
    void insert_into_user_table(database&, const std::string&, const std::string&,
        const std::string&, const std::string&, int64_t, int64_t, const std::string&,
        const std::string&, UserType, const std::string&);

    std::string get_json_from_table(database&, const std::string&, const std::string&, const std::string&);
    bool set_json_in_table(database&, const std::string&, const std::string&, const std::string&, const std::string&);

    bool is_file(database&, const std::string&);
    RetrievedFile download_file(database&, const UserProperties&, const std::string&);
    std::string upload_file(database&, const FileConstruct&);

    std::pair<bool, std::string> is_logged_in(const limhamn::http::server::request&, database&);

    limhamn::http::server::response get_stylesheet(const limhamn::http::server::request&, database&);
    limhamn::http::server::response get_script(const limhamn::http::server::request&, database&);
    limhamn::http::server::response get_index_page(const limhamn::http::server::request&, database&);
    limhamn::http::server::response get_setup_page(const limhamn::http::server::request&, database&);
    limhamn::http::server::response get_api_try_login(const limhamn::http::server::request&, database&);
    limhamn::http::server::response get_api_try_register(const limhamn::http::server::request&, database&);
    limhamn::http::server::response get_api_try_setup(const limhamn::http::server::request&, database&);
    limhamn::http::server::response get_api_get_settings(const limhamn::http::server::request&, database&);
    limhamn::http::server::response get_api_update_settings(const limhamn::http::server::request&, database&);
}