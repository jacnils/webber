#include <webber.hpp>
#include <db_abstract.hpp>
#include <nlohmann/json.hpp>
#include <scrypto.hpp>

std::string webber::upload_file(database& db, const webber::FileConstruct& c) {
    if (!db.good()) {
        throw std::runtime_error{"Database is not good."};
    }
    if (c.path.empty() || c.name.empty()) {
        throw std::runtime_error{"File or name is empty."};
    }
    if (!std::filesystem::is_regular_file(c.path)) {
        throw std::runtime_error{"File is not a regular file."};
    }

    nlohmann::json json;

    json["filename"] = c.name;
    json["username"] = c.username;
    json["ip_address"] = c.ip_address;
    json["user_agent"] = c.user_agent;
    json["uploaded_at"] = scrypto::return_unix_timestamp();
    json["downloads"] = 0;
    json["downloaders"] = nlohmann::json::array(); /* combine username, ip address, user agent and timestamp */
    json["require_admin"] = c.require_admin;
    json["require_login"] = c.require_login;

    const auto check_for_dup = [](database& db, const std::string& key) -> bool {
        if (is_page(db, key)) {
            return true;
        }

        for (const auto& it : db.query("SELECT * FROM files WHERE file_path = ?;", key)) {
            if (it.empty()) {
                return false;
            }

            return true;
        }

        return false;
    };

    std::string file_key = scrypto::generate_random_string(16);
    std::string key = scrypto::generate_random_string(16);

    std::filesystem::remove(webber::settings.data_directory + "/" + key);

    std::filesystem::path dir{webber::settings.data_directory + "/" + key};
    if (!std::filesystem::is_directory(dir)) {
        std::filesystem::create_directories(dir);
    }

    dir += "/" + file_key;
    std::filesystem::rename(c.path, dir);
    if (!std::filesystem::is_regular_file(dir)) {
        throw std::runtime_error{"Failed to move file."};
    }

    json["path"] = dir;
    json["size"] = std::filesystem::file_size(dir);

    if (std::filesystem::file_size(dir) <= webber::settings.max_file_size_hash) {
        json["sha256"] = scrypto::sha256hash_file(dir);
    }

    if (check_for_dup(db, c.virtual_path)) {
#ifdef WEBBER_DEBUG
        logger.write_to_log(limhamn::logger::type::error, "Duplicate file.\n");
        logger.write_to_log(limhamn::logger::type::notice, "File path: " + c.virtual_path + "\n");
#endif
        throw std::runtime_error{"Duplicate file."};
    }

    // insert into the files table
    if (!db.exec("INSERT INTO files (file_path, json) VALUES (?, ?);", c.virtual_path, json.dump())) {
        throw std::runtime_error{"Error inserting into the files table."};
    }

    return file_key;
}

bool webber::is_file(database& db, const std::string& file_path) {
    if (!db.good()) {
        throw std::runtime_error{"Database is not good."};
    }
    if (file_path.empty()) {
        throw std::runtime_error{"File key is empty."};
    }

    for (const auto& it : db.query("SELECT * FROM files WHERE file_path = ?;", file_path)) {
        if (it.empty()) {
            return false;
        }

        return true;
    }

    return false;
}

void webber::remove_file(database& db, const std::string& file_path) {
    if (!db.good()) {
        throw std::runtime_error{"Database is not good."};
    }
    if (file_path.empty()) {
        throw std::runtime_error{"File path is empty."};
    }

    const auto query = db.query("SELECT * FROM files WHERE file_path = ?;", file_path);
    if (query.empty() || !query.at(0).contains("json")) {
        throw std::runtime_error{"Query is empty or JSON not found."};
    }

    const auto json = nlohmann::json::parse(query.at(0).at("json"));
    if (json.contains("path") && json.at("path").is_string()) {
        std::filesystem::remove(json.at("path").get<std::string>());
    }

    if (db.exec("DELETE FROM files WHERE file_path = ?;", file_path) == false) {
        throw std::runtime_error{"Error deleting from the files table."};
    }
}

void webber::update_file(database& db, const std::string& file_path, const FileConstruct& c) {
    remove_file(db, file_path);
    upload_file(db, c);
}

webber::RetrievedFile webber::download_file(database& db, const webber::UserProperties& prop, const std::string& file_path) {
    if (!db.good()) {
        throw std::runtime_error{"Database is not good."};
    }
    if (prop.ip_address.empty() || prop.user_agent.empty() || file_path.empty()) {
        throw std::runtime_error{"IP address, user agent, or file key is empty."};
    }

    nlohmann::json downloaders;
    downloaders["username"] = prop.username;
    downloaders["ip_address"] = prop.ip_address;
    downloaders["user_agent"] = prop.user_agent;
    downloaders["timestamp"] = scrypto::return_unix_timestamp();

    if (prop.username.empty()) {
        downloaders["username"] = "_nouser_";
    }

    /* select all matching file_key */
    const auto query = db.query("SELECT * FROM files WHERE file_path = ?;", file_path);
    if (query.empty()) {
        throw std::runtime_error{"Query is empty."};
    }

    // get json
    nlohmann::json json;
    try {
        json = nlohmann::json::parse(query.at(0).at("json"));
    } catch (const std::exception&) {
        throw std::runtime_error{"Error parsing JSON."};
    }

    if (json.find("downloads") != json.end() && json.at("downloads").is_number()) {
        json["downloads"] = json.at("downloads").get<int>() + 1;
    }
    if (json.find("downloaders") != json.end() && json.at("downloaders").is_array()) {
        json["downloaders"].push_back(downloaders);
    }
    if (json.find("filename") == json.end() || !json.at("filename").is_string()) {
        throw std::runtime_error{"Filename not found."};
    }

    webber::RetrievedFile f;
    if (json.find("path") == json.end() || !json.at("path").is_string()) {
        throw std::runtime_error{"Path not found."};
    }
    if (!std::filesystem::is_regular_file(json.at("path").get<std::string>())) {
        throw std::runtime_error{"File is not a regular file."};
    }

    f.name = json.at("filename").get<std::string>();
    f.path = json.at("path").get<std::string>();
    if (json.contains("require_admin") && json.at("require_admin").is_boolean()) f.require_admin = json.at("require_admin").get<bool>();
    if (json.contains("require_login") && json.at("require_login").is_boolean()) f.require_login = json.at("require_login").get<bool>();

    // reinsert into the files table
    if (!db.exec("UPDATE files SET json = ? WHERE file_path = ?;", json.dump(), file_path)) {
        throw std::runtime_error{"Error updating the files table."};
    }

    return f;
}