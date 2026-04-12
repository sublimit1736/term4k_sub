#pragma once

#include <cstdint>
#include <string>
#include <vector>

class LiteDBUtils {
public:
    static void setKeyFile(const std::string &path);

    static bool ensureKey();

    static std::vector<uint8_t> keyBytes();

    static std::vector<uint8_t> sha256(const std::vector<uint8_t> &data);

    static std::vector<uint8_t> slowHash(const std::vector<uint8_t> &data);

    static void xorObfuscate(std::vector<uint8_t> &data);

    static std::vector<uint8_t> aesEncrypt(const std::vector<uint8_t> &plaintext);

    static std::vector<uint8_t> aesDecrypt(const std::vector<uint8_t> &ciphertext);

    static std::string hexEncode(const std::vector<uint8_t> &data);

    static std::vector<uint8_t> hexDecode(const std::string &hex);

private:
    static void generateKey(size_t length = 32);

    static bool loadKey();

    static void saveKey();

    static std::string keyFile;
    static std::vector<uint8_t> key;
};


