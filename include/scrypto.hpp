#pragma once

#include <string>
#include <vector>

/**
 * @brief  Namespace that contains various functions for cryptographic operations.
 */
namespace scrypto {
    /**
     * @brief  Function that hashes a string using SHA256.
     * @param  data Data to hash.
     * @return Returns the resulting hash in the form of a string.
     */
    std::string sha256hash(const std::string& data);
    /**
     * @brief  Function that hashes a file using SHA256.
     * @param  path Path to the file to hash.
     * @return Returns the resulting hash in the form of a string.
     */
    std::string sha256hash_file(const std::string& path);
    /**
     * @brief  Function that hashes a string using BCrypt.
     * @param  data Data to hash.
     * @return Returns the resulting hash in the form of a string.
     */
    std::string password_hash(const std::string& data);
    /**
     * @brief  Function that verifies a string against a BCrypt hash.
     * @param  data Data to verify.
     * @param  hash Hash to verify against.
     * @return Returns whether the data matches the hash.
     */
    bool password_verify(const std::string& data, const std::string& hash);
    /**
     * @brief  Function that generates a random string.
     * @param  length Length of the generated string.
     * @return Returns the generated string.
     */
    std::string generate_random_string(const int length = 256);
    /**
     * @brief  Function that returns the amount of time in milliseconds since the Unix epoch.
     * @return Returns the amount of milliseconds in the form of a 64 bit integer.
     */
    int64_t return_unix_timestamp();
    /**
     * @brief  Converts a YYYY-MM-DD format date into Unix millis.
     * @return int64_t
     */
    int64_t convert_date_to_unix_millis(const std::string& date);
    /**
     * @brief  Function that generates a key from a username and password based on various factors.
     * @param  strings Strings to use when generating.
     * @return Returns the generated key in the form of a string.
     */
    std::string generate_key(const std::vector<std::string>& strings);
    /**
     * @brief  Function that removes all non-ASCII characters from a string.
     * @param  str String to remove non-ASCII characters from.
     * @return Returns the string with all non-ASCII characters removed.
     */
    std::string remove_non_ascii(const std::string& str);
    /**
     * @brief  Function that removes all characters from a string that are not in a list.
     * @param  str String to remove characters from.
     * @param  list List of characters to keep.
     * @return Returns the string with all characters not in the list removed.
     */
    std::string remove_all_not(const std::string& str, const std::string& list);
}
