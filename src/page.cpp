#include <webber.hpp>
#include <db_abstract.hpp>
#include <nlohmann/json.hpp>
#include <scrypto.hpp>
#include <maddy/parser.h>

void webber::upload_page(database& db, const webber::PageConstruct& c) {
    enum class ContentType : bool {
        HTML = true,
        Markdown = false,
    };

    if (!db.good()) {
        throw std::runtime_error{"Database is not good"};
    }
    if (c.virtual_path.empty() || (c.html_content.empty() && c.markdown_content.empty())) {
        throw std::runtime_error{"Virtual path or content is empty."};
    }
    bool content_type{};
    if (c.markdown_content.empty() == false) {
        content_type = static_cast<bool>(ContentType::Markdown);
    } else if (c.html_content.empty() == false) {
        content_type = static_cast<bool>(ContentType::HTML);
    } else {
        throw std::runtime_error{"Content is empty."};
    }

    const auto parse_input = [](const std::string& input) -> std::string {
        maddy::Parser parser;
        std::istringstream stream{input};
        return parser.Parse(stream);
    };

    nlohmann::json json;
    json["username"] = c.username;
    json["ip_address"] = c.ip_address;
    json["user_agent"] = c.user_agent;
    json["uploaded_at"] = json["updated_at"] = scrypto::return_unix_timestamp();
    json["visits"] = 0;
    json["input_content_type"] = content_type ? "html" : "markdown";
    json["output_content_type"] = "html";
    json["history"] = nlohmann::json::array();
    json["input_content"] = content_type ? c.html_content : c.markdown_content;
    json["output_content"] = content_type ? c.html_content : parse_input(c.markdown_content);
    json["visitors"] = nlohmann::json::array(); /* combine username, ip address, user agent and timestamp */
    json["require_admin"] = c.require_admin;
    json["require_login"] = c.require_login;

    const auto check_for_dup = [](database& db, const std::string& key) -> bool {
        if (is_page(db, key)) {
            return true;
        }
        for (const auto& it : db.query("SELECT * FROM pages WHERE location = ?;", key)) {
            if (it.empty()) {
                return false;
            }

            return true;
        }

        return false;
    };

    if (check_for_dup(db, c.virtual_path)) {
        throw std::runtime_error{"Duplicate file."};
    }

    // insert into the pages table
    if (!db.exec("INSERT INTO pages (location, json) VALUES (?, ?);", c.virtual_path, json.dump())) {
        throw std::runtime_error{"Error inserting into the pages table."};
    }
}

bool webber::is_page(database& db, const std::string& location) {
    if (!db.good()) {
        throw std::runtime_error{"Database is not good."};
    }
    if (location.empty()) {
        throw std::runtime_error{"Location is empty."};
    }

    for (const auto& it : db.query("SELECT * FROM pages WHERE location = ?;", location)) {
        if (it.empty()) {
            return false;
        }

        return true;
    }

    return false;
}

void webber::remove_page(database& db, const std::string& location) {
    if (!db.good()) {
        throw std::runtime_error{"Database is not good."};
    }
    if (location.empty()) {
        throw std::runtime_error{"Location is empty."};
    }

    const auto query = db.query("SELECT * FROM pages WHERE location = ?;", location);
    if (query.empty() || !query.at(0).contains("json")) {
        throw std::runtime_error{"Query is empty or JSON not found."};
    }

    if (db.exec("DELETE FROM pages WHERE location = ?;", location) == false) {
        throw std::runtime_error{"Error deleting from the location table."};
    }
}

void webber::update_page(database& db, const PageConstruct& c) {
    if (!db.good()) {
        throw std::runtime_error{"Database is not good."};
    }
    if (c.virtual_path.empty()) {
        throw std::runtime_error{"Location is empty."};
    }

    if (!is_page(db, c.virtual_path)) {
        throw std::runtime_error{"Page does not exist."};
    }

    const auto query = db.query("SELECT * FROM pages WHERE location = ?;", c.virtual_path);
    if (query.empty() || !query.at(0).contains("json")) {
        throw std::runtime_error{"Query is empty or JSON not found."};
    }
    if (c.html_content.empty() && c.markdown_content.empty()) {
        throw std::runtime_error{"Content is empty."};
    }

    auto json = nlohmann::json::parse(query.at(0).at("json"));

    enum class ContentType : bool {
        HTML = true,
        Markdown = false,
    };

    if (!db.good()) {
        throw std::runtime_error{"Database is not good"};
    }
    if (c.virtual_path.empty() || (c.html_content.empty() && c.markdown_content.empty())) {
        throw std::runtime_error{"Virtual path or content is empty."};
    }
    bool content_type{};
    if (c.markdown_content.empty() == false) {
        content_type = static_cast<bool>(ContentType::Markdown);
    } else if (c.html_content.empty() == false) {
        content_type = static_cast<bool>(ContentType::HTML);
    } else {
        throw std::runtime_error{"Content is empty."};
    }

    const auto parse_input = [](const std::string& input) -> std::string {
        maddy::Parser parser;
        std::istringstream stream{input};
        return parser.Parse(stream);
    };

    // populate history
    if (json.find("history") != json.end() && json.at("history").is_array()) {
        nlohmann::json history;

        if (json.contains("updated_at")) history["updated_at"] = json.at("updated_at");
        if (json.contains("replaced_at")) history["replaced_at"] = scrypto::return_unix_timestamp();
        if (json.contains("username")) history["username"] = json.at("username");
        if (json.contains("ip_address")) history["ip_address"] = json.at("ip_address");
        if (json.contains("user_agent")) history["user_agent"] = json.at("user_agent");
        if (json.contains("replaced_by_username")) history["replaced_by_username"] = c.username;
        if (json.contains("replaced_by_ip_address")) history["replaced_by_ip_address"] = c.ip_address;
        if (json.contains("replaced_by_user_agent")) history["replaced_by_user_agent"] = c.user_agent;
        if (json.contains("input_content_type")) history["input_content_type"] = json.at("input_content_type");
        if (json.contains("output_content_type")) history["output_content_type"] = json.at("output_content_type");
        if (json.contains("input_content")) history["input_content"] = json.at("input_content");
        if (json.contains("output_content")) history["output_content"] = json.at("output_content");
        if (json.contains("visitors_at_the_time")) history["visitors_at_the_time"] = json.at("visitors");
        if (json.contains("visits_at_the_time")) history["visits_at_the_time"] = json.at("visits");
        if (json.contains("require_admin")) history["require_admin"] = json.at("require_admin");
        if (json.contains("require_login")) history["require_login"] = json.at("require_login");

        json["history"].push_back(history);
    }

    if (!c.username.empty()) json["username"] = c.username;
    if (!c.ip_address.empty()) json["ip_address"] = c.ip_address;
    if (!c.user_agent.empty()) json["user_agent"] = c.user_agent;

    json["updated_at"] = scrypto::return_unix_timestamp();
    json["require_admin"] = c.require_admin;
    json["require_login"] = c.require_login;

    json["input_content_type"] = content_type ? "html" : "markdown";
    json["output_content_type"] = "html";
    json["input_content"] = content_type ? c.html_content : c.markdown_content;
    json["output_content"] = content_type ? c.html_content : parse_input(c.markdown_content);

    if (!db.exec("UPDATE pages SET json = ? WHERE location = ?;", json.dump(), c.virtual_path)) {
        throw std::runtime_error{"Error updating the pages table."};
    }
}

webber::RetrievedPage webber::download_page(database& db, const webber::UserProperties& prop, const std::string& page, const bool get_json) {
    if (!db.good()) {
        throw std::runtime_error{"Database is not good."};
    }
    if (prop.ip_address.empty() || prop.user_agent.empty() || page.empty()) {
        throw std::runtime_error{"IP address, user agent, or file key is empty."};
    }

    nlohmann::json visitors;

    visitors["username"] = prop.username;
    visitors["ip_address"] = prop.ip_address;
    visitors["user_agent"] = prop.user_agent;
    visitors["timestamp"] = scrypto::return_unix_timestamp();

    if (prop.username.empty()) {
        visitors["username"] = "_nouser_";
    }

    /* select all matching page */
    const auto query = db.query("SELECT * FROM pages WHERE location = ?;", page);
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

    if (json.find("visits") != json.end() && json.at("visits").is_number()) {
        json["visits"] = json.at("visits").get<int>() + 1;
    }
    if (json.find("visitors") != json.end() && json.at("visitors").is_array()) {
        json["visitors"].push_back(visitors);
    }

    webber::RetrievedPage p;
    if (json.find("input_content") == json.end() || !json.at("input_content").is_string()) {
        throw std::runtime_error{"Input content not found."};
    }
    if (json.find("output_content") == json.end() || !json.at("output_content").is_string()) {
        throw std::runtime_error{"Output content not found."};
    }
    if (json.find("input_content_type") == json.end() || !json.at("input_content_type").is_string()) {
        throw std::runtime_error{"Input content type not found."};
    }
    if (json.find("output_content_type") == json.end() || !json.at("output_content_type").is_string()) {
        throw std::runtime_error{"Output content type not found."};
    }

    p.output_content = json.at("output_content").get<std::string>();
    p.input_content = json.at("input_content").get<std::string>();
    p.input_content_type = json.at("input_content_type").get<std::string>();
    p.output_content_type = json.at("output_content_type").get<std::string>();
    if (json.contains("require_admin")) p.require_admin = json.at("require_admin").get<bool>();
    if (json.contains("require_login")) p.require_login = json.at("require_login").get<bool>();

    if (get_json) {
        p.json = json.dump();
    }

    if (!db.exec("UPDATE pages SET json = ? WHERE location = ?;", json.dump(), page)) {
        throw std::runtime_error{"Error updating the pages table."};
    }

    return p;
}