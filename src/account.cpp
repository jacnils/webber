#include <ranges>
#include <webber.hpp>
#include <db_abstract.hpp>
#include <nlohmann/json.hpp>
#include <scrypto.hpp>

webber::UserType webber::get_user_type(database& database, const std::string& username) {
    for (const auto& it : database.query("SELECT user_type FROM users WHERE username = ?;", username)) {
        if (it.empty()) {
            return webber::UserType::Undefined;
        }

        if (it.at("user_type") == "0") {
            return webber::UserType::User;
        } else if (it.at("user_type") == "1") {
            return webber::UserType::Administrator;
        } else {
            return webber::UserType::Undefined;
        }
    }

    return webber::UserType::Undefined;
}

bool webber::is_user(database& database, const std::string& username) {
    for (const auto& it : database.query("SELECT * FROM users WHERE username = ?;", username)) {
        if (it.empty()) {
            return !get_username_from_email(database, username).empty();
        }

        return true;
    }

    return !get_username_from_email(database, username).empty();
}

std::string webber::get_email_from_username(database& database, const std::string& username) {
    for (const auto& it : database.query("SELECT email FROM users WHERE username = ?;", username)) {
        if (it.empty()) {
            return "";
        }

        return it.at("email");
    }

    return "";
}

std::string webber::get_username_from_email(database& database, const std::string& email) {
    for (const auto& it : database.query("SELECT username FROM users WHERE email = ?;", email)) {
        if (it.empty()) {
            return "";
        }

        return it.at("username");
    }

    return "";
}

// Warning: This function does not check credentials, nor does it check if the user already exists, or if the values are valid or even safe.
void webber::insert_into_user_table(database& database, const std::string& username, const std::string& password,
        const std::string& key, const std::string& email, const int64_t created_at, const int64_t updated_at, const std::string& ip_address,
        const std::string& user_agent, UserType user_type, const std::string& json) {

    std::string ua{};
    for (auto& c : user_agent) {
        if (c >= 32 && c < 127) {
            ua += c;
        }
    }

#if FF_DEBUG
    logger.write_to_log(limhamn::logger::type::notice, "Inserting into the users table.\n");
    logger.write_to_log(limhamn::logger::type::notice, "Username: " + username + ", Password: " + password + ", Key: " + key + ", Email: " + email + ", Created at: " + std::to_string(created_at) + ", Updated at: " + std::to_string(updated_at) + ", IP Address: " + ip_address + ", User Agent: " + ua + ", User Type: " + std::to_string(static_cast<int>(user_type)) + ", JSON: " + json + "\n");
#endif

    if (!database.exec("INSERT INTO users (username, password, key, email, created_at, updated_at, ip_address, user_agent, user_type, json) VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?);",
                username,
                password,
                key,
                email,
                created_at,
                updated_at,
                ip_address,
                ua,
                static_cast<int>(user_type),
                json)) {
        throw std::runtime_error{"insert_into_user_table(): Error inserting into the users table."};
    }
}

std::pair<bool, std::string> webber::is_logged_in(const limhamn::http::server::request& request, database& db, const std::string& _json ) {
    std::string username{};
    std::string key{};
    if (request.session.contains("username")) {
        username = request.session.at("username");
    }
    if (request.session.contains("key")) {
        key = request.session.at("key");
    }
    for (auto& it : db.query("SELECT * FROM users WHERE username = ? AND key = ?;", username, key)) {
        if (it.empty()) {
            return {false, {}};
        }
        return {true, username};
    }

    try {
        nlohmann::json json = nlohmann::json::parse(_json.empty() ? request.body : _json);

        if (!json.contains("username") || !json.at("username").is_string()) {
            return {false, {}};
        }
        if (!json.contains("key") || !json.at("key").is_string()) {
            return {false, {}};
        }

        const std::string& username = json.at("username").get<std::string>();
        const std::string& key = json.at("key").get<std::string>();
        for (auto& it : db.query("SELECT * FROM users WHERE username = ? AND key = ?;", username, key)) {
            if (it.empty()) {
                return {false, {}};
            }
            return {true, username};
        }
    } catch (const std::exception&) {}

    return {false, {}};
}

std::pair<webber::LoginStatus, std::string> webber::try_login(database& database, const std::string& username, const std::string& password,
        const std::string& ip_address, const std::string& user_agent, limhamn::http::server::response& response) {

#if FF_DEBUG
    logger.write_to_log(limhamn::logger::type::notice, "Attempting to login.\n");
    logger.write_to_log(limhamn::logger::type::notice, "Username: " + base_username + ", Password: " + base_password + "\n");
#endif

    for (const auto& it : database.query("SELECT * FROM users WHERE username = ?;", username)) {
        if (it.empty()) {
            return {webber::LoginStatus::InvalidUsername, {}};
        }

        if (!scrypto::password_verify(password, it.at("password"))) {
            return {webber::LoginStatus::InvalidPassword, {}};
        }

        const int64_t last_login{scrypto::return_unix_timestamp()};
        std::string key{scrypto::generate_key({password})};

        if (!database.exec("UPDATE users SET updated_at = ?, ip_address = ?, user_agent = ?, key = ? WHERE username = ?;", last_login, ip_address, user_agent, key, username)) {
            return {webber::LoginStatus::Failure, {}};
        }

        response.session["username"] = username;
        response.session["key"] = key;

        response.cookies.push_back({"username", username, .path = "/"});

        limhamn::http::server::cookie c;

        UserType type = webber::get_user_type(database, username);
        int user_type{0};

        if (type == UserType::Administrator) {
            user_type = 1;
        }

        response.cookies.push_back({"user_type", std::to_string(user_type)});

        return {webber::LoginStatus::Success, key};
    }

    return {webber::LoginStatus::InvalidUsername, {}};
}

// Warning: This function does not check credentials or anything. That should be done before calling this function.
// If such case is not taken, it may be possible to create an account with elevated privileges.
webber::AccountCreationStatus webber::make_account(database& database, const std::string& username, const std::string& password,
        const std::string& email, const std::string& ip_address, const std::string& user_agent, UserType user_type) {
    const std::string hashed_password{scrypto::password_hash(password)};
    const std::string key{scrypto::generate_key({password})};
    const int64_t current_time{scrypto::return_unix_timestamp()};

    nlohmann::json json;
    json["uploads"] = 0;

    for (auto& it : database.query("SELECT username FROM users WHERE username = ?;", username)) {
        if (!it.empty()) {
            return webber::AccountCreationStatus::UsernameExists;
        }
    }
    for (const auto& it : database.query("SELECT email FROM users WHERE email = ?;", email)) {
        if (!it.empty()) {
            return webber::AccountCreationStatus::EmailExists;
        }
    }

    if (username.empty()) {
        return webber::AccountCreationStatus::InvalidUsername;
    } else if (username.size() < webber::settings.username_min_length) {
        return webber::AccountCreationStatus::UsernameTooShort;
    } else if (username.size() > webber::settings.username_max_length) {
        return webber::AccountCreationStatus::UsernameTooLong;
    }

    if (email.empty() || email.find('@') == std::string::npos || email.find('.') == std::string::npos) {
        return webber::AccountCreationStatus::InvalidEmail;
    }

    // invalid chars that may never be allowed: < > " ' / \ | ? *
    for (auto& c : username) {
        static const std::vector<char> invalid_chars{'<', '>', '"', '\'', '/', '\\', '|', '?', '*'};
        if (std::ranges::find(invalid_chars.begin(), invalid_chars.end(), c) != invalid_chars.end()) {
            return webber::AccountCreationStatus::InvalidUsername;
        }
    }
    for (auto& c : email) {
        static const std::vector<char> invalid_chars{'<', '>', '"', '\'', '/', '\\', '|', '?', '*'};
        if (std::ranges::find(invalid_chars.begin(), invalid_chars.end(), c) != invalid_chars.end()) {
            return webber::AccountCreationStatus::InvalidEmail;
        }
    }

    for (auto& c : username) {
        if (std::ranges::find(webber::settings.allowed_characters.begin(), webber::settings.allowed_characters.end(), c) == webber::settings.allowed_characters.end() &&
                webber::settings.allow_all_characters == false) {
            return webber::AccountCreationStatus::InvalidUsername;
        }
    }

    if (password.empty() || password.size() < webber::settings.password_min_length) {
        return AccountCreationStatus::PasswordTooShort;
    } else if (password.size() > webber::settings.password_max_length) {
        return AccountCreationStatus::PasswordTooLong;
    }

    insert_into_user_table(
            database,
            username,
            hashed_password,
            key,
            email,
            current_time,
            current_time,
            ip_address,
            user_agent,
            user_type,
            json.dump()
    );

    if (needs_setup) {
        needs_setup = false;
    }

    return webber::AccountCreationStatus::Success;
}