#include "catch_amalgamated.hpp"

#include "utils/LiteDBUtils.h"
#include "TestSupport.h"

#include <cstdint>

using namespace test_support;

TEST_CASE (
"LiteDBUtils hex encode/decode roundtrip and invalid input"
,
"[utils][LiteDBUtils]"
)
 {
    const std::vector<uint8_t> bytes = {0x00, 0x0F, 0xA5, 0xFF};
    const std::string hex = LiteDBUtils::hexEncode(bytes);

    REQUIRE(hex == "000fa5ff");
    REQUIRE(LiteDBUtils::hexDecode(hex) == bytes);
    REQUIRE(LiteDBUtils::hexDecode("zz").empty());
    REQUIRE(LiteDBUtils::hexDecode("0f1") == std::vector<uint8_t>{0x0f});
}

TEST_CASE (
"LiteDBUtils hash outputs are deterministic with SHA-256 length"
,
"[utils][LiteDBUtils]"
)
 {
    const std::vector<uint8_t> input = {'t', 'e', 'r', 'm', '4', 'k'};

    const auto shaA = LiteDBUtils::sha256(input);
    const auto shaB = LiteDBUtils::sha256(input);
    REQUIRE(shaA == shaB);
    REQUIRE(shaA.size() == 32);

    const auto slowA = LiteDBUtils::slowHash(input);
    const auto slowB = LiteDBUtils::slowHash(input);
    REQUIRE(slowA == slowB);
    REQUIRE(slowA.size() == 32);
}

TEST_CASE (
"LiteDBUtils key setup, xor obfuscation and AES roundtrip"
,
"[utils][LiteDBUtils]"
)
 {
    TempDir temp("term4k_litedb");
    const auto keyPath = (temp.path() / "key.bin").string();

    LiteDBUtils::setKeyFile(keyPath);
    REQUIRE(LiteDBUtils::ensureKey());
    REQUIRE_FALSE(LiteDBUtils::keyBytes().empty());

    std::vector<uint8_t> payload = {1, 2, 3, 4, 5, 200};
    const auto original = payload;

    LiteDBUtils::xorObfuscate(payload);
    REQUIRE(payload != original);
    LiteDBUtils::xorObfuscate(payload);
    REQUIRE(payload == original);

    const auto encrypted = LiteDBUtils::aesEncrypt(original);
    REQUIRE_FALSE(encrypted.empty());

    const auto decrypted = LiteDBUtils::aesDecrypt(encrypted);
    REQUIRE(decrypted == original);
}