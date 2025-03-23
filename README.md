# webber

Dynamic website generator written in JavaScript (frontend) and C++ (backend).

## Dependencies

- npm
- uglify-js (optional, only needed for production)
- CMake
- C++23 compiler
- Boost (beast, asio, system) - for networking
- OpenSSL - SSL/TLS and general cryptography
- yaml-cpp - for configuration files
- SQLite3 - for database (optional, if PostgreSQL is enabled)
- PostgreSQL - for database (optional, if SQLite3 is enabled)
- iconv - for character encoding (probably already installed)
- nlohmann-json - for JSON parsing
- bcrypt (included as a submodule) - more cryptography, hashing passwords

## Licensing

See LICENSE for more information. webber is not open source software. Dependencies are subject to their own licenses.

Copyright (C) 2025 Jacob Nilsson <contact@jacobnilsson.com>