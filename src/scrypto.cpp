#include <cstdint>
#include <string>
#include <vector>
#include <iomanip>
#include <fstream>
#include <sstream>
#include <chrono>
#include <random>
#include <algorithm>
#include <filesystem>
#include <scrypto.hpp>
#include <openssl/evp.h>
#include <bcrypt/BCrypt.hpp>

std::string scrypto::sha256hash(const std::string& data) {
    std::string ret{};

    EVP_MD_CTX* context = EVP_MD_CTX_new();

    if (context != nullptr) {
        if (EVP_DigestInit_ex(context, EVP_sha256(), nullptr)) {
            if (EVP_DigestUpdate(context, data.c_str(), data.length())) {
                unsigned char hash[EVP_MAX_MD_SIZE];
                unsigned int len{0};

                if (EVP_DigestFinal_ex(context, hash, &len)) {
                    std::stringstream ss{};

                    for (unsigned int i{0}; i < len; ++i) {
                        ss << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(hash[i]);
                    }

                    ret = ss.str();
                }
            }
        }

        EVP_MD_CTX_free(context);
    }

    return ret;
}

std::string scrypto::sha256hash_file(const std::string& path) {
    if (!std::filesystem::is_regular_file(path)) {
        return "";
    }
    std::ifstream file{path};
    return sha256hash(std::string{std::istreambuf_iterator<char>{file}, std::istreambuf_iterator<char>{}});
}

std::string scrypto::password_hash(const std::string& password) {
    return BCrypt::generateHash(password);
}

bool scrypto::password_verify(const std::string& password, const std::string& hash) {
    return BCrypt::validatePassword(password, hash);
}

std::string scrypto::generate_random_string(const int length) {
    static constexpr char charset[] =
        "0123456789"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
        "abcdefghijklmnopqrstuvwxyz";

    static constexpr size_t charset_size = sizeof(charset) - 1;
    static std::random_device rd;
    static std::mt19937 generator(rd());

    std::uniform_int_distribution<> distribution(0, charset_size - 1);

    std::string str(length, 0);

    std::generate_n(str.begin(), length, [&distribution]() { return charset[distribution(generator)]; });

    return str;
}

int64_t scrypto::return_unix_timestamp() {
    return std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
}

int64_t scrypto::convert_date_to_unix_millis(const std::string& date) {
    std::tm tm{};
    std::istringstream ss{date};
    ss >> std::get_time(&tm, "%Y-%m-%d");

    if (ss.fail()) {
        throw std::invalid_argument("Invalid date: " + date);
    }

    return std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::seconds(std::mktime(&tm))).count();
}

std::string scrypto::generate_key(const std::vector<std::string>& strings) {
    std::string str{};
    for (auto& it : strings) {
        str += it;
    }

    std::string key = sha256hash(str + std::to_string(scrypto::return_unix_timestamp()) + scrypto::generate_random_string());

    // equivalent to boost::erase_all()
    for (auto& it : key) {
        if (it == '\'' || it == '\"') {
            key.erase(std::remove(key.begin(), key.end(), it), key.end());
        }
    }

    return key;
}

std::string scrypto::remove_non_ascii(const std::string& str) {
    std::string ret{str};

    ret.erase(std::remove_if(ret.begin(), ret.end(), [](char c) { return !isprint(static_cast<unsigned char>(c)); }), ret.end());

    return ret;
}

std::string scrypto::remove_all_not(const std::string& str, const std::string& list) {
    std::string ret{str};

    ret.erase(std::remove_if(ret.begin(), ret.end(), [&list](char c) { return list.find(c) == std::string::npos; }), ret.end());

    return ret;
}
