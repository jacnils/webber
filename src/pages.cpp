#include <webber.hpp>
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
    }

    const std::string& username = input_json.at("username");
    const std::string& password = input_json.at("password");
    std::string email{};

    if (input_json.find("email") == input_json.end() || !input_json.at("email").is_string()) {
        response.http_status = 400;
        nlohmann::json json;
        json["error"] = "WEBBER_MISSING_EMAIL";
        json["error_str"] = "Email missing.";
        response.body = json.dump();
        return response;
    }

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