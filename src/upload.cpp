#include <webber.hpp>
#include <db_abstract.hpp>
#include <limhamn/http/http_utils.hpp>
#include <nlohmann/json.hpp>

webber::UploadStatus webber::upload_file(const limhamn::http::server::request& req, database& db) {
    std::string json{};
    std::string file_endpoint{};
    std::string file_name{};
    std::string file_path{};

#ifdef WEBBER_DEBUG
    logger.write_to_log(limhamn::logger::type::notice, "Attempting to upload a file.\n");
#endif

    const auto file_handles = limhamn::http::utils::parse_multipart_form_file(req.raw_body, settings.temp_directory + "/%f-%h-%r");
    for (const auto& it : file_handles) {
#ifdef WEBBER_DEBUG
        logger.write_to_log(limhamn::logger::type::notice, "File name: " + it.filename + ", Name: " + it.name + "\n");
#endif
        if (it.name == "json") {
            json = open_file(it.path);
#ifdef WEBBER_DEBUG
            logger.write_to_log(limhamn::logger::type::notice, "Got JSON\n");
#endif
        } else {
            file_path = it.path;
            file_name = it.filename;
            break;
        }
    }

    if (json.empty() || file_path.empty()) {
        return UploadStatus::Failure;
    }
    if (is_file(db, file_path) || is_page(db, file_path)) {
        return UploadStatus::Failure;
    }

    const auto stat = is_logged_in(req, db, json);
    if (!stat.first) {
        return UploadStatus::InvalidCreds;
    }
    if (stat.second.empty()) {
        return UploadStatus::InvalidCreds;
    }
    if (get_user_type(db, stat.second) != UserType::Administrator) {
        return UploadStatus::InvalidCreds;
    }

    nlohmann::json recv_json{};
    try {
        recv_json = nlohmann::json::parse(json);
    } catch (const std::exception&) {
#ifdef WEBBER_DEBUG
        logger.write_to_log(limhamn::logger::type::error, "Failed to parse JSON.\n");
#endif
        return UploadStatus::Failure;
    }

    if (recv_json.contains("endpoint") && recv_json.at("endpoint").is_string()) {
        file_endpoint = recv_json.at("endpoint").get<std::string>();
    } else {
#ifdef WEBBER_DEBUG
        logger.write_to_log(limhamn::logger::type::error, "No endpoint.\n");
#endif
        return UploadStatus::Failure;
    }

    if (recv_json.contains("name") && recv_json.at("name").is_string()) {
        file_name = recv_json.at("name").get<std::string>();
    }

    if (file_path.empty() || file_name.empty()) {
#ifdef WEBBER_DEBUG
        logger.write_to_log(limhamn::logger::type::error, "File path or name is empty.\n");
        // write to log
        logger.write_to_log(limhamn::logger::type::notice, "File path: " + file_path + ", File name: " + file_name + "\n");
#endif
        return UploadStatus::Failure;
    }

    bool require_admin{false};
    bool require_login{false};

    if (recv_json.contains("require_admin") && recv_json.at("require_admin").is_boolean()) {
        require_admin = recv_json.at("require_admin").get<bool>();
    }
    if (recv_json.contains("require_login") && recv_json.at("require_login").is_boolean()) {
        require_login = recv_json.at("require_login").get<bool>();
    }

    if (file_endpoint.front() != '/') {
        file_endpoint = "/" + file_endpoint;
    }

    try {
        upload_file(db, FileConstruct{
            .virtual_path = file_endpoint,
            .path = file_path,
            .name = file_name,
            .username = stat.second,
            .ip_address = req.ip_address,
            .user_agent = req.user_agent,
            .require_admin = require_admin,
            .require_login = require_login,
        });
    } catch (const std::exception&) {
        return UploadStatus::Failure;
    }

    return UploadStatus::Success;
}