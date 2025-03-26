#include <webber.hpp>
#include <prebuilt.hpp>
#include <limhamn/http/http_server.hpp>
#include <nlohmann/json.hpp>

limhamn::http::server::response webber::get_index_page(const limhamn::http::server::request& request, database& db) {
    return {
        .http_status = 200,
        .content_type = "text/html",
        .body = open_file(settings.data_directory + "/index.html"),
    };
}

limhamn::http::server::response webber::get_stylesheet(const limhamn::http::server::request& request, database& db) {
    return {
        .http_status = 200,
        .content_type = "text/css",
        .body = open_file(settings.data_directory + "/style.css"),
    };
}

limhamn::http::server::response webber::get_script(const limhamn::http::server::request& request, database& db) {
    limhamn::http::server::response response;

    response.content_type = "text/javascript";
    response.http_status = 200;

    // TODO: Just like the name, this function is UGLY AS FUCK, and does not belong anywhere near
    // a project like this. But I simply cannot be bothered to write a JS minifier myself, nor
    // am I aware of any C++ library for doing such a thing, and I am therefore just going to call uglifyjs.
    const auto uglify_file = [](const std::string& path) -> std::string {
        static const std::string temp_file = settings.temp_directory + "/ff_temp.js";
        if (std::filesystem::is_regular_file(temp_file)) {
            return temp_file;
        }
        if (std::system("which uglifyjs > /dev/null") != 0) {
            return path;
        }

        std::filesystem::remove(temp_file);
        std::filesystem::copy_file(path, temp_file);

        // run uglifyjs on the file
        std::string command = "uglifyjs " + temp_file + " -o " + temp_file;
        std::system(command.c_str());

        return temp_file;
    };

#if WEBBER_DEBUG
    response.body = open_file(settings.data_directory + "/script.js");
#else
    std::string path = uglify_file(settings.data_directory + "/script.js");
    response.body = open_file(path);
#endif

    return response;
}

limhamn::http::server::response webber::get_setup_page(const limhamn::http::server::request& request, database& db) {
    return {
        .http_status = 200,
        .content_type = "text/html",
        .body = open_file(settings.data_directory + "/setup.html"),
    };
}

limhamn::http::server::response webber::get_api_try_register(const limhamn::http::server::request& request, database& db) {
    limhamn::http::server::response response{};
    response.content_type = "application/json";

#if WEBBER_DEBUG
    logger.write_to_log(limhamn::logger::type::notice, "Attempting to register.\n");
    logger.write_to_log(limhamn::logger::type::notice, "Request body: " + request.body + "\n");
#endif

    if (request.method != "POST") {
        nlohmann::json json;
        json["error"] = "WEBBER_METHOD_NOT_ALLOWED";
        json["error_str"] = "Method not allowed.";
        response.body = json.dump();
        response.http_status = 405;
        return response;
    }

    nlohmann::json input_json;
    try {
        input_json = nlohmann::json::parse(request.body);
    } catch (const std::exception&) {
        response.http_status = 400;
        nlohmann::json json;
        json["error"] = "WEBBER_INVALID_JSON";
        json["error_str"] = "Invalid JSON.";
        response.body = json.dump();
        return response;
    }

    if (input_json.find("username") == input_json.end() || !input_json.at("username").is_string()) {
        response.http_status = 400;
        nlohmann::json json;
        json["error"] = "WEBBER_MISSING_USERNAME";
        json["error_str"] = "Username missing.";
        response.body = json.dump();
        return response;
    } else if (input_json.find("password") == input_json.end() || !input_json.at("password").is_string()) {
        response.http_status = 400;
        nlohmann::json json;
        json["error"] = "WEBBER_MISSING_PASSWORD";
        json["error_str"] = "Password missing.";
        response.body = json.dump();
        return response;
    } else if (input_json.find("email") == input_json.end() || !input_json.at("email").is_string()) {
        response.http_status = 400;
        nlohmann::json json;
        json["error"] = "WEBBER_MISSING_EMAIL";
        json["error_str"] = "Email missing.";
        response.body = json.dump();
        return response;
    }

    const std::string& username = input_json.at("username");
    const std::string& password = input_json.at("password");
    const std::string& email = input_json.at("email");
    const std::string& ip_address = request.ip_address;
    const std::string& user_agent = request.user_agent;

    AccountCreationStatus status = webber::make_account(
            db,
            username,
            password,
            email,
            ip_address,
            user_agent,
            UserType::User
    );

    if (status == AccountCreationStatus::Success) {
        response.http_status = 204;
    } else {
        static const std::unordered_map<AccountCreationStatus, std::pair<std::string, std::string>> map{
            {AccountCreationStatus::Failure, {"WEBBER_FAILURE", "Failure."}},
            {AccountCreationStatus::UsernameExists, {"WEBBER_USERNAME_EXISTS", "Username exists."}},
            {AccountCreationStatus::UsernameTooShort, {"WEBBER_USERNAME_TOO_SHORT", "Username too short."}},
            {AccountCreationStatus::UsernameTooLong, {"WEBBER_USERNAME_TOO_LONG", "Username too long."}},
            {AccountCreationStatus::PasswordTooShort, {"WEBBER_PASSWORD_TOO_SHORT", "Password too short."}},
            {AccountCreationStatus::PasswordTooLong, {"WEBBER_PASSWORD_TOO_LONG", "Password too long."}},
            {AccountCreationStatus::InvalidUsername, {"WEBBER_INVALID_USERNAME", "Invalid username."}},
            {AccountCreationStatus::InvalidPassword, {"WEBBER_INVALID_PASSWORD", "Invalid password."}},
            {AccountCreationStatus::InvalidEmail, {"WEBBER_INVALID_EMAIL", "Invalid email."}},
            {AccountCreationStatus::EmailExists, {"WEBBER_EMAIL_EXISTS", "Email exists."}},
        };

        if (!map.contains(status)) {
            nlohmann::json json;
            json["error"] = "WEBBER_UNKNOWN_ERROR";
            json["error_str"] = "Unknown error.";
            response.body = json.dump();
            response.http_status = 500;
            return response;
        }

        nlohmann::json json;
        json["error"] = map.at(status).first;
        json["error_str"] = map.at(status).second;
        response.body = json.dump();
        response.http_status = 400;

        return response;
    }

    return response;
}

limhamn::http::server::response webber::get_api_try_login(const limhamn::http::server::request& request, database& db) {
    limhamn::http::server::response response{};
    response.content_type = "application/json";

    if (request.method != "POST") {
        response.http_status = 405;
        nlohmann::json json;
        json["error"] = "WEBBER_METHOD_NOT_ALLOWED";
        json["error_str"] = "Method not allowed.";
        response.body = json.dump();
        return response;
    }

    nlohmann::json input_json;
    try {
       input_json = nlohmann::json::parse(request.body);
    } catch (const std::exception&) {
        response.content_type = "application/json";
        response.http_status = 400;

        return response;
    }

#if WEBBER_DEBUG
    logger.write_to_log(limhamn::logger::type::notice, "Attempting to login.\n");
    logger.write_to_log(limhamn::logger::type::notice, "Request body: " + request.body + "\n");
#endif

    std::string username{};

    if (input_json.find("password") == input_json.end() || !input_json.at("password").is_string()) {
        nlohmann::json json;
        json["error"] = "WEBBER_MISSING_PASSWORD";
        json["error_str"] = "Password missing.";
        response.body = json.dump();
        response.http_status = 400;
        return response;
    }

    if (input_json.find("username") != input_json.end() && input_json.at("username").is_string()) {
        username = input_json.at("username").get<std::string>();
    } else if (input_json.find("email") != input_json.end() && input_json.at("email").is_string()) {
        try {
            username = get_username_from_email(db, input_json.at("email").get<std::string>());
        } catch (const std::exception&) {
            nlohmann::json json;
            json["error"] = "WEBBER_INVALID_EMAIL";
            json["error_str"] = "Invalid email.";
            response.body = json.dump();
            response.http_status = 400;
            return response;
        }
    } else {
        nlohmann::json json;
        json["error"] = "WEBBER_MISSING_USERNAME";
        json["error_str"] = "Username missing.";
        response.body = json.dump();
        response.http_status = 400;
        return response;
    }

    const std::string& password = input_json.at("password").get<std::string>();
    const std::string& ip_address = request.ip_address;
    const std::string& user_agent = request.user_agent;

    std::pair<LoginStatus, std::string> status = webber::try_login(
            db,
            username,
            password,
            ip_address,
            user_agent,
            response
    );

    if (status.first == LoginStatus::Success) {
        response.http_status = 200;
        nlohmann::json json;
        json["username"] = username;
        json["key"] = status.second;
        response.body = json.dump();
        return response;
    } else {
        const std::unordered_map<LoginStatus, std::pair<std::string, std::string>> error_map{
            {LoginStatus::Failure, {"WEBBER_FAILURE", "Failure."}},
            {LoginStatus::InvalidUsername, {"WEBBER_INVALID_USERNAME", "Invalid username."}},
            {LoginStatus::InvalidPassword, {"WEBBER_INVALID_PASSWORD", "Invalid password."}},
            {LoginStatus::Inactive, {"WEBBER_NOT_ACTIVATED", "Account not activated. Check your email!"}},
            {LoginStatus::Banned, {"WEBBER_BANNED", "Banned."}},
        };

        if (!error_map.contains(status.first)) {
            nlohmann::json json;
            json["error"] = "WEBBER_UNKNOWN_ERROR";
            json["error_str"] = "Unknown error.";
            response.body = json.dump();
            response.http_status = 500;
            return response;
        }

        nlohmann::json json;
        json["error"] = error_map.at(status.first).first;
        json["error_str"] = error_map.at(status.first).second;
        response.body = json.dump();
        response.http_status = 400;
    }

    return response;
}

limhamn::http::server::response webber::get_api_try_setup(const limhamn::http::server::request& request, database& db) {
    limhamn::http::server::response response{};
    response.content_type = "application/json";

    if (request.method != "POST") {
        response.http_status = 405;
        nlohmann::json json;
        json["error"] = "WEBBER_METHOD_NOT_ALLOWED";
        json["error_str"] = "Method not allowed.";
        response.body = json.dump();
        return response;
    }
    if (request.body.empty()) {
        response.http_status = 400;
        nlohmann::json json;
        json["error"] = "WEBBER_NO_BODY";
        json["error_str"] = "No body.";
        response.body = json.dump();
        return response;
    }

    nlohmann::json input_json;
    try {
        input_json = nlohmann::json::parse(request.body);
    } catch (const std::exception&) {
        response.http_status = 400;
        nlohmann::json json;
        json["error"] = "WEBBER_INVALID_JSON";
        json["error_str"] = "Invalid JSON.";
        response.body = json.dump();
        return response;
    }

    if (input_json.find("username") == input_json.end() || !input_json.at("username").is_string()) {
        nlohmann::json json;
        json["error"] = "WEBBER_NO_USERNAME";
        json["error_str"] = "No username.";
        response.body = json.dump();
        return response;
    }
    if (input_json.find("password") == input_json.end() || !input_json.at("password").is_string()) {
        nlohmann::json json;
        json["error"] = "WEBBER_NO_PASSWORD";
        json["error_str"] = "No password.";
        response.body = json.dump();
        return response;
    }
    if (input_json.find("email") == input_json.end() || !input_json.at("email").is_string()) {
        nlohmann::json json;
        json["error"] = "WEBBER_NO_EMAIL";
        json["error_str"] = "No email.";
        response.body = json.dump();
        return response;
    }

    const std::string& username = input_json.at("username").get<std::string>();
    const std::string& password = input_json.at("password").get<std::string>();
    const std::string& ip_address = request.ip_address;
    const std::string& user_agent = request.user_agent;
    const std::string& email = input_json.at("email").get<std::string>();

    AccountCreationStatus status = webber::make_account(
            db,
            username,
            password,
            email,
            ip_address,
            user_agent,
            UserType::Administrator
    );

    if (status == AccountCreationStatus::Success) {
        response.http_status = 204;
        return response;
    } else {
        static const std::unordered_map<AccountCreationStatus, std::pair<std::string, std::string>> error_map{
            {AccountCreationStatus::Failure, {"WEBBER_FAILURE", "Failure."}},
            {AccountCreationStatus::UsernameExists, {"WEBBER_USERNAME_EXISTS", "Username exists."}},
            {AccountCreationStatus::UsernameTooShort, {"WEBBER_USERNAME_TOO_SHORT", "Username too short."}},
            {AccountCreationStatus::UsernameTooLong, {"WEBBER_USERNAME_TOO_LONG", "Username too long."}},
            {AccountCreationStatus::PasswordTooShort, {"WEBBER_PASSWORD_TOO_SHORT", "Password too short."}},
            {AccountCreationStatus::PasswordTooLong, {"WEBBER_PASSWORD_TOO_LONG", "Password too long."}},
            {AccountCreationStatus::InvalidUsername, {"WEBBER_INVALID_USERNAME", "Invalid username."}},
            {AccountCreationStatus::InvalidPassword, {"WEBBER_INVALID_PASSWORD", "Invalid password."}},
            {AccountCreationStatus::InvalidEmail, {"WEBBER_INVALID_EMAIL", "Invalid email."}},
            {AccountCreationStatus::EmailExists, {"WEBBER_EMAIL_EXISTS", "Email exists."}},
        };

        if (error_map.contains(status)) {
            nlohmann::json json;
            json["error"] = error_map.at(status).first;
            json["error_str"] = error_map.at(status).second;
            response.body = json.dump();
        } else {
            nlohmann::json json;
            json["error"] = "WEBBER_UNKNOWN_ERROR";
            json["error_str"] = "Unknown error.";
            response.body = json.dump();
        }

        response.http_status = 400;
        return response;
    }
}

// TODO: Implement the following functions
limhamn::http::server::response webber::get_api_get_settings(const limhamn::http::server::request& request, database& db) {
    limhamn::http::server::response response{};

    try {
        nlohmann::json file_json = nlohmann::json::parse(open_file(settings.data_directory + "/settings.json"));
        nlohmann::json default_json = nlohmann::json::parse(default_settings);
        for (const auto& it : default_json.items()) {
            if (!file_json.contains(it.key())) {
                file_json[it.key()] = it.value();
            }
        }
        response.body = file_json.dump();
        response.http_status = 200;
    } catch (const std::exception&) {
        nlohmann::json json;
        json["error"] = "WEBBER_FAILURE";
        json["error_str"] = "Failed to get settings.";
        response.body = json.dump();
        response.http_status = 400;
    }

    return response;
}

limhamn::http::server::response webber::get_api_update_settings(const limhamn::http::server::request& request, database& db) {
    limhamn::http::server::response response{};

    const auto user = is_logged_in(request, db);
    if (!user.first || user.second.empty()) {
        nlohmann::json json;
        json["error"] = "WEBBER_NOT_LOGGED_IN";
        json["error_str"] = "Not logged in.";
        response.body = json.dump();
        response.http_status = 400;
        return response;
    }
    if (get_user_type(db, user.second) != UserType::Administrator) {
        nlohmann::json json;
        json["error"] = "WEBBER_NOT_ADMIN";
        json["error_str"] = "Not an administrator.";
        response.body = json.dump();
        response.http_status = 400;
        return response;
    }

    try {
        nlohmann::json input_json = nlohmann::json::parse(request.body);
        nlohmann::json file_json = nlohmann::json::parse(open_file(settings.data_directory + "/settings.json"));
        for (const auto& it : input_json.items()) {
            if (it.key() == "username" || it.key() == "key") {
                continue;
            }

            if (it.key().empty() == false && it.value().empty() == true) {
                file_json.erase(it.key());
            }

            file_json[it.key()] = it.value();
        }

        std::ofstream file(settings.data_directory + "/settings.json", std::ios::trunc);
        file << file_json.dump();
        file.close();

        return {
            .http_status = 204,
        };
    } catch (const std::exception&) {
        nlohmann::json json;
        json["error"] = "WEBBER_FAILURE";
        json["error_str"] = "Failed to update settings.";
        response.body = json.dump();
        response.http_status = 400;
        return response;
    }
}

limhamn::http::server::response webber::get_api_get_page(const limhamn::http::server::request& request, database& db) {
    limhamn::http::server::response response{};

    bool json_requested = false;

    nlohmann::json json;
    try {
        json = nlohmann::json::parse(request.body);
    } catch (const std::exception&) {
        nlohmann::json return_json;
        response.http_status = 400;
        return_json["error"] = "WEBBER_INVALID_JSON";
        return_json["error_str"] = "Invalid JSON.";
        response.body = return_json.dump();
        return response;
    }

    // allows us to write, for example, a great admin panel and access tons of information from JS
    if (json.contains("json") && json.at("json").is_boolean()) {
        json_requested = json.at("json").get<bool>();
    }

    const auto stat = is_logged_in(request, db);
    bool is_admin = false;
    if (json_requested) {
        if (!stat.first || stat.second.empty()) {
            response.http_status = 400;
            nlohmann::json json;
            json["error"] = "WEBBER_INVALID_CREDS";
            json["error_str"] = "Invalid credentials.";
            response.body = json.dump();
            return response;
        }

        is_admin = get_user_type(db, stat.second) == UserType::Administrator;
    }

    if (json.contains("page") == false || json.at("page").is_string() == false) {
        nlohmann::json return_json;
        response.http_status = 400;
        return_json["error"] = "WEBBER_MISSING_PAGE";
        return_json["error_str"] = "Page missing.";
        response.body = return_json.dump();
        return response;
    }

    const std::string page = json.at("page").get<std::string>();
    if (!is_page(db, page)) {
        nlohmann::json return_json;
        response.http_status = 400;
        return_json["error"] = "WEBBER_PAGE_NOT_FOUND";
        return_json["error_str"] = "Page not found.";
        response.body = return_json.dump();
        return response;
    }

    try {
        const RetrievedPage ret = download_page(db,
            { .ip_address = request.ip_address, .user_agent = request.user_agent, .username = stat.second},
            page, (json_requested && is_admin));

        if (ret.json.empty() == false) {
            response.content_type = "application/json";
            response.http_status = 200;
            response.body = ret.json;

            return response;
        } else {
            response.content_type = "application/json";
            response.http_status = 200;
            nlohmann::json response_json;

            if (!ret.input_content.empty()) response_json["input_content"] = ret.input_content;
            if (!ret.output_content.empty()) response_json["output_content"] = ret.output_content;
            if (!ret.input_content_type.empty()) response_json["input_content_type"] = ret.input_content_type;
            if (!ret.output_content_type.empty()) response_json["output_content_type"] = ret.output_content_type;

            response.body = response_json.dump();
            return response;
        }
    } catch (const std::exception&) {
        nlohmann::json return_json;
        response.http_status = 400;
        return_json["error"] = "WEBBER_FAILURE";
        return_json["error_str"] = "Failed to get page.";
        response.body = return_json.dump();
        return response;
    }
}

limhamn::http::server::response webber::get_api_update_page(const limhamn::http::server::request& request, database& db) {
    limhamn::http::server::response response{};

    const auto stat = is_logged_in(request, db);
    if (!stat.first || stat.second.empty()) {
        response.http_status = 400;
        nlohmann::json json;
        json["error"] = "WEBBER_INVALID_CREDS";
        json["error_str"] = "Invalid credentials.";
        response.body = json.dump();
        return response;
    }
    if (get_user_type(db, stat.second) != UserType::Administrator) {
        response.http_status = 400;
        nlohmann::json json;
        json["error"] = "WEBBER_NOT_ADMIN";
        json["error_str"] = "Not an administrator.";
        response.body = json.dump();
        return response;
    }

    nlohmann::json json;
    try {
        json = nlohmann::json::parse(request.body);
    } catch (const std::exception&) {
        nlohmann::json return_json;
        response.http_status = 400;
        return_json["error"] = "WEBBER_INVALID_JSON";
        return_json["error_str"] = "Invalid JSON.";
        response.body = return_json.dump();
        return response;
    }

    if (json.contains("page") == false || json.at("page").is_string() == false) {
        nlohmann::json return_json;
        response.http_status = 400;
        return_json["error"] = "WEBBER_MISSING_PAGE";
        return_json["error_str"] = "Page missing.";
        response.body = return_json.dump();
        return response;
    }

    const std::string page = json.at("page").get<std::string>();

    if (!is_page(db, page)) {
        nlohmann::json return_json;
        response.http_status = 400;
        return_json["error"] = "WEBBER_PAGE_NOT_FOUND";
        return_json["error_str"] = "Page not found.";
        response.body = return_json.dump();
        return response;
    }

    PageConstruct c;
    c.virtual_path = page;
    c.ip_address = request.ip_address;
    c.user_agent = request.user_agent;
    c.username = stat.second;
    if (json.contains("markdown_content") && json.at("markdown_content").is_string() == true) {
        c.markdown_content = json.at("markdown_content").get<std::string>();
    } else if (json.contains("html_content") && json.at("html_content").is_string() == true) {
        c.html_content = json.at("html_content").get<std::string>();
    } else {
        nlohmann::json return_json;
        response.http_status = 400;
        return_json["error"] = "WEBBER_MISSING_CONTENT";
        return_json["error_str"] = "Content missing.";
        response.body = return_json.dump();
        return response;
    }

    try {
        update_page(db, c);
        response.http_status = 204;
        return response;
    } catch (const std::exception&) {
        nlohmann::json return_json;
        response.http_status = 400;
        return_json["error"] = "WEBBER_FAILURE";
        return_json["error_str"] = "Failed to get page.";
        response.body = return_json.dump();
        return response;
    }
}

limhamn::http::server::response webber::get_api_delete_page(const limhamn::http::server::request& request, database& db) {
    limhamn::http::server::response response{};

    const auto stat = is_logged_in(request, db);
    if (!stat.first || stat.second.empty()) {
        response.http_status = 400;
        nlohmann::json json;
        json["error"] = "WEBBER_INVALID_CREDS";
        json["error_str"] = "Invalid credentials.";
        response.body = json.dump();
        return response;
    }
    if (get_user_type(db, stat.second) != UserType::Administrator) {
        response.http_status = 400;
        nlohmann::json json;
        json["error"] = "WEBBER_NOT_ADMIN";
        json["error_str"] = "Not an administrator.";
        response.body = json.dump();
        return response;
    }

    nlohmann::json json;
    try {
        json = nlohmann::json::parse(request.body);
    } catch (const std::exception&) {
        nlohmann::json return_json;
        response.http_status = 400;
        return_json["error"] = "WEBBER_INVALID_JSON";
        return_json["error_str"] = "Invalid JSON.";
        response.body = return_json.dump();
        return response;
    }

    if (json.contains("page") == false || json.at("page").is_string() == false) {
        nlohmann::json return_json;
        response.http_status = 400;
        return_json["error"] = "WEBBER_MISSING_PAGE";
        return_json["error_str"] = "Page missing.";
        response.body = return_json.dump();
        return response;
    }

    const std::string page = json.at("page").get<std::string>();

    if (!is_page(db, page)) {
        nlohmann::json return_json;
        response.http_status = 400;
        return_json["error"] = "WEBBER_PAGE_NOT_FOUND";
        return_json["error_str"] = "Page not found.";
        response.body = return_json.dump();
        return response;
    }

    try {
        remove_page(db, page);
        response.http_status = 204;
        return response;
    } catch (const std::exception&) {
        nlohmann::json return_json;
        response.http_status = 400;
        return_json["error"] = "WEBBER_FAILURE";
        return_json["error_str"] = "Failed to delete page.";
        response.body = return_json.dump();
        return response;
    }
}

limhamn::http::server::response webber::get_api_create_page(const limhamn::http::server::request& request, database& db) {
    limhamn::http::server::response response{};

    const auto stat = is_logged_in(request, db);
    if (!stat.first || stat.second.empty()) {
        response.http_status = 400;
        nlohmann::json json;
        json["error"] = "WEBBER_INVALID_CREDS";
        json["error_str"] = "Invalid credentials.";
        response.body = json.dump();
        return response;
    }
    if (get_user_type(db, stat.second) != UserType::Administrator) {
        response.http_status = 400;
        nlohmann::json json;
        json["error"] = "WEBBER_NOT_ADMIN";
        json["error_str"] = "Not an administrator.";
        response.body = json.dump();
        return response;
    }

    nlohmann::json json;
    try {
        json = nlohmann::json::parse(request.body);
    } catch (const std::exception&) {
        nlohmann::json return_json;
        response.http_status = 400;
        return_json["error"] = "WEBBER_INVALID_JSON";
        return_json["error_str"] = "Invalid JSON.";
        response.body = return_json.dump();
        return response;
    }

    if (json.contains("page") == false || json.at("page").is_string() == false) {
        nlohmann::json return_json;
        response.http_status = 400;
        return_json["error"] = "WEBBER_MISSING_PAGE";
        return_json["error_str"] = "Page missing.";
        response.body = return_json.dump();
        return response;
    }

    const std::string page = json.at("page").get<std::string>();

    if (is_page(db, page)) {
        nlohmann::json return_json;
        response.http_status = 400;
        return_json["error"] = "WEBBER_PAGE_FOUND";
        return_json["error_str"] = "Page already exists.";
        response.body = return_json.dump();
        return response;
    }

    PageConstruct c;
    c.virtual_path = page;
    c.ip_address = request.ip_address;
    c.user_agent = request.user_agent;
    c.username = stat.second;
    if (json.contains("markdown_content") && json.at("markdown_content").is_string() == true) {
        c.markdown_content = json.at("markdown_content").get<std::string>();
    } else if (json.contains("html_content") && json.at("html_content").is_string() == true) {
        c.html_content = json.at("html_content").get<std::string>();
    } else {
        nlohmann::json return_json;
        response.http_status = 400;
        return_json["error"] = "WEBBER_MISSING_CONTENT";
        return_json["error_str"] = "Content missing.";
        response.body = return_json.dump();
        return response;
    }

    try {
        upload_page(db, c);
        response.http_status = 204;
        return response;
    } catch (const std::exception&) {
        nlohmann::json return_json;
        response.http_status = 400;
        return_json["error"] = "WEBBER_FAILURE";
        return_json["error_str"] = "Failed to get page.";
        response.body = return_json.dump();
        return response;
    }
}

limhamn::http::server::response webber::get_api_user_exists(const limhamn::http::server::request& request, database& db) {
    limhamn::http::server::response response{};

    try {
        nlohmann::json input_json = nlohmann::json::parse(request.body);

        if (input_json.contains("username") == false || input_json.at("username").is_string() == false) {
            nlohmann::json json;
            json["error"] = "WEBBER_MISSING_USERNAME";
            json["error_str"] = "Username missing.";
            response.body = json.dump();
            response.http_status = 400;
            return response;
        }

        nlohmann::json json;
        json["status"] = is_user(db, input_json.at("username").get<std::string>());
        response.body = json.dump();
        response.http_status = 200;
    } catch (const std::exception&) {
        nlohmann::json json;
        json["error"] = "WEBBER_FAILURE";
        json["error_str"] = "Failed to check if user exists.";
        response.body = json.dump();
        response.http_status = 400;
    }

    return response;
}

limhamn::http::server::response webber::get_api_upload_file(const limhamn::http::server::request& request, database& database) {
    limhamn::http::server::response response{};

    if (request.body.empty()) {
        response.http_status = 400;
        nlohmann::json json;
        json["error"] = "WEBBER_NO_BODY";
        json["error_str"] = "No body.";
        response.body = json.dump();
        return response;
    }

    if (request.method != "POST") {
        response.http_status = 405;
        nlohmann::json json;
        json["error"] = "WEBBER_METHOD_NOT_ALLOWED";
        json["error_str"] = "Method not allowed.";
        response.body = json.dump();
        return response;
    }

    const UploadStatus status = upload_file(request, database);

    if (status == UploadStatus::Success) {
        response.http_status = 204;
        return response;
    } else {
        static const std::unordered_map<UploadStatus, std::pair<std::string, std::string>> status_map{
            {UploadStatus::NoFile, {"WEBBER_NO_FILE", "No file was specified."}},
            {UploadStatus::Failure, {"WEBBER_FAILURE", "Failed to upload the file."}},
            {UploadStatus::TooLarge, {"WEBBER_TOO_LARGE", "The file was too large."}},
            {UploadStatus::InvalidCreds, {"WEBBER_INVALID_CREDS", "Invalid credentials."}},
        };

        nlohmann::json json;
        if (status_map.contains(status)) {
            json["error"] = status_map.at(status).first;
            json["error_str"] = status_map.at(status).second;
        } else {
            json["error"] = "WEBBER_UNKNOWN_ERROR";
            json["error_str"] = "Unknown error.";
        }

        response.content_type = "application/json";
        response.http_status = 400;
        response.body = json.dump();
    }

    return response;
}

limhamn::http::server::response webber::get_api_delete_file(const limhamn::http::server::request& request, database& database) {
    limhamn::http::server::response response{};

    if (request.body.empty()) {
        response.http_status = 400;
        nlohmann::json json;
        json["error"] = "WEBBER_NO_BODY";
        json["error_str"] = "No body.";
        response.body = json.dump();
        return response;
    }

    const auto stat = is_logged_in(request, database);
    if (!stat.first) {
        response.http_status = 400;
        nlohmann::json json;
        json["error"] = "WEBBER_INVALID_CREDS";
        json["error_str"] = "Invalid credentials.";
        response.body = json.dump();
        return response;
    }
    if (get_user_type(database, stat.second) != UserType::Administrator) {
        response.http_status = 400;
        nlohmann::json json;
        json["error"] = "WEBBER_INVALID_CREDS";
        json["error_str"] = "Invalid credentials.";
        response.body = json.dump();
        return response;
    }

    nlohmann::json input_json;
    try {
        input_json = nlohmann::json::parse(request.body);
    } catch (const std::exception&) {
        response.http_status = 400;
        nlohmann::json json;
        json["error"] = "WEBBER_INVALID_JSON";
        json["error_str"] = "Invalid JSON.";
        response.body = json.dump();
        return response;
    }

    if (input_json.contains("endpoint") == false || input_json.at("endpoint").is_string() == false) {
        response.http_status = 400;
        nlohmann::json json;
        json["error"] = "WEBBER_NO_ENDPOINT";
        json["error_str"] = "No endpoint.";
        response.body = json.dump();
        return response;
    }
    if (!is_file(database, input_json.at("endpoint").get<std::string>())) {
        response.http_status = 400;
        nlohmann::json json;
        json["error"] = "WEBBER_NO_FILE";
        json["error_str"] = "Not a file.";
        response.body = json.dump();
        return response;
    }

    remove_file(database, input_json.at("endpoint").get<std::string>());

    return response;
}