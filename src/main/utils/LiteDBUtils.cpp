#include "LiteDBUtils.h"
#include "ErrorNotifier.h"

#include <fstream>
#include <openssl/evp.h>
#include <openssl/rand.h>
#include <openssl/sha.h>
#include <random>

std::string LiteDBUtils::keyFile = "key.bin";
std::vector<uint8_t> LiteDBUtils::key;

// Iteration count for slow hash.
static constexpr int SLOW_HASH_ITERATIONS = 10000;

void LiteDBUtils::setKeyFile(const std::string &path) {
    keyFile = path;
}

std::string LiteDBUtils::hexEncode(const std::vector<uint8_t> &data) {
    // Use a direct lookup table instead of ostringstream to avoid per-byte stream overhead.
    static constexpr char kHex[] = "0123456789abcdef";
    std::string out;
    out.reserve(data.size() * 2);
    for (const auto b: data){
        out.push_back(kHex[b >> 4]);
        out.push_back(kHex[b & 0x0F]);
    }
    return out;
}

std::vector<uint8_t> LiteDBUtils::hexDecode(const std::string &hex) {
    std::vector<uint8_t> result;
    result.reserve(hex.size() / 2);
    // Parse hex digits directly without substr/stoi to avoid per-pair heap allocations.
    // Invalid characters cause an early-return just like the original stoi-based path.
    auto isHex = [](const char c) -> bool {
        return (c >= '0' && c <= '9') || (c >= 'a' && c <= 'f') || (c >= 'A' && c <= 'F');
    };
    auto hexVal = [](const char c) -> uint8_t {
        if (c >= '0' && c <= '9') return static_cast<uint8_t>(c - '0');
        if (c >= 'a' && c <= 'f') return static_cast<uint8_t>(c - 'a' + 10);
        if (c >= 'A' && c <= 'F') return static_cast<uint8_t>(c - 'A' + 10);
        return 0;
    };
    for (size_t i = 0; i + 1 < hex.size(); i += 2){
        if (!isHex(hex[i]) || !isHex(hex[i + 1])){
            ErrorNotifier::notifyUnknown("LiteDBUtils::hexDecode");
            return {};
        }
        result.push_back(static_cast<uint8_t>(hexVal(hex[i]) << 4) | hexVal(hex[i + 1]));
    }
    return result;
}

std::vector<uint8_t> LiteDBUtils::sha256(const std::vector<uint8_t> &data) {
    std::vector<uint8_t> hash(SHA256_DIGEST_LENGTH);
    SHA256(data.data(), data.size(), hash.data());
    return hash;
}

std::vector<uint8_t> LiteDBUtils::slowHash(const std::vector<uint8_t> &data) {
    // Reuse a single temporary buffer across all iterations to avoid
    // allocating a new vector<uint8_t> on each of the 10 000 rounds.
    std::vector<uint8_t> h = sha256(data);
    std::vector<uint8_t> tmp(SHA256_DIGEST_LENGTH);
    for (int i = 1; i < SLOW_HASH_ITERATIONS; ++i){
        SHA256(h.data(), h.size(), tmp.data());
        h.swap(tmp);
    }
    return h;
}

void LiteDBUtils::xorObfuscate(std::vector<uint8_t> &data) {
    // Lightweight obfuscation only; no-op when key is not initialized.
    if (key.empty()) return;
    for (size_t i = 0; i < data.size(); ++i){
        data[i] ^= key[i % key.size()];
    }
}

void LiteDBUtils::generateKey(size_t length) {
    key.resize(length);
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution dis(0, 255);
    for (auto &byte: key){
        byte = static_cast<uint8_t>(dis(gen));
    }
}

void LiteDBUtils::saveKey() {
    std::ofstream f(keyFile, std::ios::binary | std::ios::trunc);
    f.write(reinterpret_cast<const char *>(key.data()),
            static_cast<std::streamsize>(key.size()));
}

bool LiteDBUtils::loadKey() {
    std::ifstream f(keyFile, std::ios::binary);
    if (!f) return false;
    key.assign(std::istreambuf_iterator(f),
               std::istreambuf_iterator<char>());
    return !key.empty();
}

bool LiteDBUtils::ensureKey() {
    // Reuse in-memory key first, then try loading file, then generate new key.
    if (!key.empty()) return true;
    if (loadKey()) return true;
    generateKey(32);
    saveKey();
    return true;
}

std::vector<uint8_t> LiteDBUtils::keyBytes() {
    return key;
}

std::vector<uint8_t> LiteDBUtils::aesEncrypt(const std::vector<uint8_t> &plaintext) {
    std::vector<uint8_t> iv(16);
    // Return empty buffer on failure to keep upper persistence flow exception-safe.
    if (RAND_bytes(iv.data(), static_cast<int>(iv.size())) != 1){
        return {};
    }

    EVP_CIPHER_CTX* ctx = EVP_CIPHER_CTX_new();
    if (!ctx) return {};

    std::vector<uint8_t> out(16 + plaintext.size() + 16);
    std::copy(iv.begin(), iv.end(), out.begin());

    int len = 0, finalLen = 0;
    if (EVP_EncryptInit_ex(ctx, EVP_aes_256_cbc(), nullptr, key.data(), iv.data()) != 1 ||
        EVP_EncryptUpdate(ctx, out.data() + 16, &len,
                          plaintext.data(), static_cast<int>(plaintext.size())) != 1 ||
        EVP_EncryptFinal_ex(ctx, out.data() + 16 + len, &finalLen) != 1){
        EVP_CIPHER_CTX_free(ctx);
        return {};
    }
    EVP_CIPHER_CTX_free(ctx);

    out.resize(16 + len + finalLen);
    return out;
}

std::vector<uint8_t> LiteDBUtils::aesDecrypt(const std::vector<uint8_t> &ciphertext) {
    // Ciphertext format: first 16 bytes are IV; shorter input is invalid.
    if (ciphertext.size() < 16) return {};

    const uint8_t* iv   = ciphertext.data();
    const uint8_t* data = ciphertext.data() + 16;
    int dataLen         = static_cast<int>(ciphertext.size() - 16);

    EVP_CIPHER_CTX* ctx = EVP_CIPHER_CTX_new();
    if (!ctx) return {};

    std::vector<uint8_t> out(dataLen + 16);
    int len = 0, finalLen = 0;
    if (EVP_DecryptInit_ex(ctx, EVP_aes_256_cbc(), nullptr, key.data(), iv) != 1 ||
        EVP_DecryptUpdate(ctx, out.data(), &len, data, dataLen) != 1 ||
        EVP_DecryptFinal_ex(ctx, out.data() + len, &finalLen) != 1){
        EVP_CIPHER_CTX_free(ctx);
        return {};
    }
    EVP_CIPHER_CTX_free(ctx);

    out.resize(len + finalLen);
    return out;
}