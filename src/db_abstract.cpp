#include <sstream>
#include <webber.hpp>
#include <db_abstract.hpp>
#include <limhamn/http/http_server.hpp>
#include <yaml-cpp/yaml.h>
#include <nlohmann/json.hpp>

void webber::setup_database(database& database) {
    std::string primary = "id INTEGER PRIMARY KEY";

    // if database is postgresql, use SERIAL
    if (webber::settings.enabled_database) {
        primary = "id SERIAL PRIMARY KEY";
    }

    // users -- the user table
    // id: the user id
    // username: the username of the user
    // password: the password of the user
    // key: the key, stored in the database and in the user's cookie, used to authenticate the user
    // email: the email of the user
    // created_at: the time the user was created
    // updated_at: the time the user was last updated
    // ip_address: the ip address of the user
    // user_agent: the user agent of the user
    // user_type: 0 = User, 1 = Administrator
    // json: the json of the user
    if (!database.exec("CREATE TABLE IF NOT EXISTS users (" + primary + ", username TEXT NOT NULL, password TEXT NOT NULL, key TEXT NOT NULL, email TEXT NOT NULL, created_at bigint NOT NULL, updated_at bigint NOT NULL, ip_address TEXT NOT NULL, user_agent TEXT NOT NULL, user_type bigint NOT NULL, json TEXT NOT NULL);")) {
        throw std::runtime_error{"Error creating the users table."};
    }

    // pages -- the page table
    // id: the url id
    // location: the location of the page
    // json: json data
    if (!database.exec("CREATE TABLE IF NOT EXISTS pages (" + primary + ", location TEXT NOT NULL, json TEXT NOT NULL);")) {
        throw std::runtime_error{"Error creating the pages table."};
    }

    // files -- the file table
    // id: the file id
    // file_id: identifier of the file, used to retrieve the file
    // json: the json of the file (including actual path)
    if (!database.exec("CREATE TABLE IF NOT EXISTS files (" + primary + ", file_id TEXT NOT NULL, json TEXT NOT NULL);")) {
        throw std::runtime_error{"Error creating the files table."};
    }
}

std::string webber::get_json_from_table(database& db, const std::string& table, const std::string& key, const std::string& value) {
    if (!db.good()) {
        throw std::runtime_error{"Database is not good."};
    }
    if (table.empty() || key.empty() || value.empty()) {
        throw std::runtime_error{"Table, key, or value is empty."};
    }

    const auto& query = db.query("SELECT json FROM ? WHERE ? = ?;", table, key, value);
    if (query.empty()) {
        throw std::runtime_error{"Query is empty."};
    }

    for (const auto& it : query) {
        if (!it.contains("json")) {
            throw std::runtime_error{"JSON not found."};
        }

        return it.at("json");
    }

    throw std::runtime_error{"JSON not found."};
}

bool webber::set_json_in_table(database& db, const std::string& table, const std::string& key, const std::string& value, const std::string& json) {
    if (!db.good() || table.empty() || key.empty() || value.empty() || json.empty()) {
        return false;
    }

    return db.exec("UPDATE ? SET json = ? WHERE ? = ?;", table, json, key, value);
}