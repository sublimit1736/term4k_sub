#include "ProofedRecordsDAO.h"
#include "services/I18nService.h"
#include "utils/ErrorNotifier.h"
#include "utils/LiteDBUtils.h"

#include <fstream>
#include <sstream>

namespace {
    std::vector<uint8_t> buildGenesisData() {
        std::vector<uint8_t> data = {
            'G', 'e', 'n', 'e', 'S', 'i', 's', '1', '7', '9', '2', '*', '2', '4', '?', '1', '~'
        };
        auto key = LiteDBUtils::keyBytes();
        data.insert(data.end(), key.begin(), key.end());
        return data;
    }

    std::vector<std::string> readNonEmptyLines(const std::string &path) {
        std::vector<std::string> lines;
        std::ifstream f(path);
        std::string line;
        while (std::getline(f, line)){
            if (!line.empty()) lines.push_back(line);
        }
        return lines;
    }

    std::vector<std::string> decryptRecordLines(const std::vector<std::string> &recLines) {
        std::vector<std::string> records;
        records.reserve(recLines.size());
        for (const auto &line: recLines){
            std::vector<uint8_t> encrypted = LiteDBUtils::hexDecode(line);
            std::vector<uint8_t> decrypted = LiteDBUtils::aesDecrypt(encrypted);
            if (decrypted.empty()) continue;
            LiteDBUtils::xorObfuscate(decrypted);
            records.emplace_back(decrypted.begin(), decrypted.end());
        }
        return records;
    }

    bool writeRecordLines(const std::string &path, const std::vector<std::string> &plainRecords) {
        std::ofstream out(path, std::ios::trunc);
        if (!out){
            ErrorNotifier::notify("ProofedRecordsDAO::writeRecordLines",
                                  I18nService::instance().get("error.file_open_failed") + ": " + path);
            return false;
        }
        for (const auto &record: plainRecords){
            std::vector<uint8_t> bytes(record.begin(), record.end());
            LiteDBUtils::xorObfuscate(bytes);
            bytes = LiteDBUtils::aesEncrypt(bytes);
            out << LiteDBUtils::hexEncode(bytes) << "\n";
        }
        return true;
    }

    bool writeProofLines(const std::string &path, const std::vector<std::string> &plainRecords) {
        std::ofstream out(path, std::ios::trunc);
        if (!out){
            ErrorNotifier::notify("ProofedRecordsDAO::writeProofLines",
                                  I18nService::instance().get("error.file_open_failed") + ": " + path);
            return false;
        }
        std::vector<uint8_t> prevHash = LiteDBUtils::slowHash(buildGenesisData());
        for (const auto &record: plainRecords){
            std::vector<uint8_t> recordBytes(record.begin(), record.end());
            std::vector<uint8_t> recordHash = LiteDBUtils::sha256(recordBytes);
            std::vector<uint8_t> combined   = prevHash;
            combined.insert(combined.end(), recordHash.begin(), recordHash.end());
            const std::vector<uint8_t> expectedHash = LiteDBUtils::slowHash(combined);
            out << LiteDBUtils::hexEncode(expectedHash) << "\n";
            prevHash = expectedHash;
        }
        return true;
    }

    size_t verifiedPrefixCount(const std::vector<std::string> &recLines, const std::vector<std::string> &hashLines) {
        std::vector<uint8_t> prevHash = LiteDBUtils::slowHash(buildGenesisData());
        const size_t limit            = std::min(recLines.size(), hashLines.size());
        size_t verifiedCount          = 0;
        for (size_t i = 0; i < limit; ++i){
            std::vector<uint8_t> encrypted = LiteDBUtils::hexDecode(recLines[i]);
            std::vector<uint8_t> decrypted = LiteDBUtils::aesDecrypt(encrypted);
            if (decrypted.empty()) break;
            LiteDBUtils::xorObfuscate(decrypted);

            std::vector<uint8_t> recordHash = LiteDBUtils::sha256(decrypted);
            std::vector<uint8_t> combined   = prevHash;
            combined.insert(combined.end(), recordHash.begin(), recordHash.end());
            const std::vector<uint8_t> expectedHash = LiteDBUtils::slowHash(combined);
            const std::vector<uint8_t> storedHash   = LiteDBUtils::hexDecode(hashLines[i]);
            if (expectedHash != storedHash) break;
            ++verifiedCount;
            prevHash = expectedHash;
        }
        return verifiedCount;
    }

    bool isUIDFormatRecord(const std::string &record) {
        std::istringstream iss(record);
        std::vector<std::string> fields;
        std::string token;
        while (iss >> token) fields.push_back(token);
        if (fields.size() < 7) return false;
        const std::string &uid = fields[0];
        return !uid.empty() && uid.find_first_not_of("0123456789") == std::string::npos;
    }

    size_t chartFieldIndex(const std::string &record) {
        return isUIDFormatRecord(record) ? 1 : 0;
    }

    size_t userFieldIndex(const std::string &record) {
        return isUIDFormatRecord(record) ? 3 : 2;
    }

    size_t uidFieldIndex(const std::string &record) {
        return isUIDFormatRecord(record) ? 0 : static_cast<size_t>(-1);
    }
}

// Static member initialization.
std::string ProofedRecordsDAO::recordList = "records.db";
std::string ProofedRecordsDAO::proofList  = "proof.db";

// Sets data directory (redirects records.db / proof.db / key.bin).
// Empty string or "." restores default current-directory behavior (useful for tests).
void ProofedRecordsDAO::setDataDir(const std::string &dir) {
    if (dir.empty() || dir == "."){
        recordList = "records.db";
        proofList  = "proof.db";
        LiteDBUtils::setKeyFile("key.bin");
        return;
    }
    std::string d = dir;
    if (d.back() != '/' && d.back() != '\\') d += '/';
    recordList = d + "records.db";
    proofList  = d + "proof.db";
    LiteDBUtils::setKeyFile(d + "key.bin");
}

// Reads full proof file as raw bytes.
std::vector<uint8_t> ProofedRecordsDAO::readRawData() {
    std::ifstream f(proofList);
    if (!f) return {};
    return std::vector<uint8_t>(std::istreambuf_iterator<char>(f),
                                std::istreambuf_iterator<char>());
}

// Extracts the last hash value from proof bytes (hex-decoded).
std::vector<uint8_t> ProofedRecordsDAO::extractLastHash(const std::vector<uint8_t> &data) {
    std::string content(data.begin(), data.end());
    std::istringstream iss(content);
    std::string line, lastLine;
    while (std::getline(iss, line)){
        if (!line.empty()) lastLine = line;
    }
    if (lastLine.empty()) return {};
    return LiteDBUtils::hexDecode(lastLine);
}

// Appends one record: verify chain -> hash chain update -> encrypt -> write.
bool ProofedRecordsDAO::addRecord(const std::string &serial_record) {
    if (!LiteDBUtils::ensureKey()){
        ErrorNotifier::notify("ProofedRecordsDAO::addRecord",
                              I18nService::instance().get("error.encryption_key_ensure_failed"));
        return false;
    }

    // Read existing proof data and verify chain if proof exists.
    auto data = readRawData();
    std::vector<uint8_t> prevHash;

    if (!data.empty()){
        if (!ProofedRecordsDAO::verifyChain()){
            ErrorNotifier::notify("ProofedRecordsDAO::addRecord",
                                  I18nService::instance().get("error.database_integrity_check_failed"));
            return false;
        }
        prevHash = extractLastHash(data);
    }
    else{
        // Genesis hash: slowHash(seed + key)
        prevHash = LiteDBUtils::slowHash(buildGenesisData());
    }

    // Compute SHA-256 for current plaintext record.
    const std::vector<uint8_t> recordBytes(serial_record.begin(), serial_record.end());
    const auto recordHash = LiteDBUtils::sha256(recordBytes);

    // Chained hash: slowHash(prevHash + recordHash)
    std::vector<uint8_t> chainInput = prevHash;
    chainInput.insert(chainInput.end(), recordHash.begin(), recordHash.end());
    std::vector<uint8_t> newHash = LiteDBUtils::slowHash(chainInput);

    // Encrypt record: XOR obfuscation + AES-256-CBC.
    std::vector<uint8_t> plain(serial_record.begin(), serial_record.end());
    LiteDBUtils::xorObfuscate(plain);
    plain = LiteDBUtils::aesEncrypt(plain);

    // Append one encrypted line and one proof-hash line (hex encoded).
    {
        std::ofstream recFile(recordList, std::ios::app);
        if (!recFile){
            ErrorNotifier::notify("ProofedRecordsDAO::addRecord",
                                  I18nService::instance().get("error.record_file_open_failed") + ": " + recordList);
            return false;
        }
        recFile << LiteDBUtils::hexEncode(plain) << "\n";
    }

    {
        std::ofstream proofFile(proofList, std::ios::app);
        if (!proofFile){
            ErrorNotifier::notify("ProofedRecordsDAO::addRecord",
                                  I18nService::instance().get("error.proof_file_open_failed") + ": " + proofList);
            return false;
        }
        proofFile << LiteDBUtils::hexEncode(newHash) << "\n";
    }

    return true;
}

// Reads all records (decrypted plaintext list).
std::vector<std::string> ProofedRecordsDAO::readAllRecord() {
    if (!LiteDBUtils::ensureKey()){
        ErrorNotifier::notify("ProofedRecordsDAO::readAllRecord",
                              I18nService::instance().get("error.encryption_key_ensure_failed"));
        return {};
    }

    std::vector<std::string> records;
    std::ifstream recFile(recordList);
    if (!recFile){
        ErrorNotifier::notify("ProofedRecordsDAO::readAllRecord",
                              I18nService::instance().get("error.record_file_open_failed") + ": " + recordList);
        return {};
    }

    std::string line;
    while (std::getline(recFile, line)){
        if (line.empty()) continue;
        std::vector<uint8_t> encrypted = LiteDBUtils::hexDecode(line);
        std::vector<uint8_t> decrypted = LiteDBUtils::aesDecrypt(encrypted);
        if (decrypted.empty()) continue;
        LiteDBUtils::xorObfuscate(decrypted);
        records.emplace_back(decrypted.begin(), decrypted.end());
    }
    return records;
}

// Verifies integrity of the full hash chain.
bool ProofedRecordsDAO::verifyChain() {
    if (!LiteDBUtils::ensureKey()){
        ErrorNotifier::notify("ProofedRecordsDAO::verifyChain",
                              I18nService::instance().get("error.encryption_key_ensure_failed"));
        return false;
    }

    // Read all record lines and hash lines.
    auto records = readAllRecord();
    auto hashes  = readNonEmptyLines(proofList);

    // Record count and hash count must match.
    if (records.size() != hashes.size()) return false;

    // Verify sequentially from genesis hash.
    std::vector<uint8_t> prevHash = LiteDBUtils::slowHash(buildGenesisData());

    for (size_t i = 0; i < records.size(); ++i){
        const std::vector<uint8_t> recordBytes(records[i].begin(), records[i].end());

        // Recompute expected hash
        const auto recordHash           = LiteDBUtils::sha256(recordBytes);
        std::vector<uint8_t> chainInput = prevHash;
        chainInput.insert(chainInput.end(), recordHash.begin(), recordHash.end());
        std::vector<uint8_t> expectedHash = LiteDBUtils::slowHash(chainInput);

        // Compare expected hash with stored hash.
        if (expectedHash != LiteDBUtils::hexDecode(hashes[i])) return false;

        prevHash = expectedHash;
    }

    return true;
}

// Reads records in time order and truncates at first verification failure,
// returning only the continuous verified prefix.
std::vector<std::string> ProofedRecordsDAO::readVerifiedRecord() {
    if (!LiteDBUtils::ensureKey()){
        ErrorNotifier::notify("ProofedRecordsDAO::readVerifiedRecord",
                              I18nService::instance().get("error.encryption_key_ensure_failed"));
        return {};
    }

    // Read all record lines and hash lines.
    auto records = readAllRecord();
    auto hashes  = readNonEmptyLines(proofList);

    // Verify line-by-line from genesis hash.
    std::vector<uint8_t> prevHash = LiteDBUtils::slowHash(buildGenesisData());

    std::vector<std::string> result;
    size_t limit = std::min(records.size(), hashes.size());
    for (size_t i = 0; i < limit; ++i){
        // Recompute expected chained hash.
        const std::vector<uint8_t> recordBytes(records[i].begin(), records[i].end());
        std::vector<uint8_t> recordHash = LiteDBUtils::sha256(recordBytes);
        std::vector<uint8_t> chainInput = prevHash;
        chainInput.insert(chainInput.end(), recordHash.begin(), recordHash.end());
        std::vector<uint8_t> expectedHash = LiteDBUtils::slowHash(chainInput);

        // Stored hash mismatch means chain break; truncate here.
        if (expectedHash != LiteDBUtils::hexDecode(hashes[i])) break;

        result.push_back(records[i]);
        prevHash = expectedHash;
    }
    return result;
}

// Returns the number of continuously verified records from earliest record.
size_t ProofedRecordsDAO::verifiedRecordCount() {
    return readVerifiedRecord().size();
}

// Extracts idx-th whitespace-separated field from record string (0-based).
// Returns empty string when idx is out of range.
std::string ProofedRecordsDAO::getField(const std::string &record, size_t idx) {
    std::istringstream iss(record);
    std::string token;
    for (size_t i = 0; i <= idx; ++i){
        if (!(iss >> token)) return {};
    }
    return token;
}

// Reads all records by username (without chain verification).
std::vector<std::string> ProofedRecordsDAO::readAllRecordByUser(const std::string &username) {
    std::vector<std::string> result;
    for (const auto &rec: readAllRecord()){
        if (getField(rec, userFieldIndex(rec)) == username) result.push_back(rec);
    }
    return result;
}

// Reads verified records by username (truncate first, then filter).
std::vector<std::string> ProofedRecordsDAO::readVerifiedRecordByUser(const std::string &username) {
    std::vector<std::string> result;
    for (const auto &rec: readVerifiedRecord()){
        if (getField(rec, userFieldIndex(rec)) == username) result.push_back(rec);
    }
    return result;
}

// Reads all records by chart ID (without chain verification).
std::vector<std::string> ProofedRecordsDAO::readAllRecordByChart(const std::string &chartID) {
    std::vector<std::string> result;
    for (const auto &rec: readAllRecord()){
        if (getField(rec, chartFieldIndex(rec)) == chartID) result.push_back(rec);
    }
    return result;
}

// Reads verified records by chart ID (truncate first, then filter).
std::vector<std::string> ProofedRecordsDAO::readVerifiedRecordByChart(const std::string &chartID) {
    std::vector<std::string> result;
    for (const auto &rec: readVerifiedRecord()){
        if (getField(rec, chartFieldIndex(rec)) == chartID) result.push_back(rec);
    }
    return result;
}

// Reads all records by username and chart ID (without chain verification).
std::vector<std::string> ProofedRecordsDAO::readAllRecordByUserAndChart(const std::string &username,
                                                                        const std::string &chartID
    ) {
    std::vector<std::string> result;
    for (const auto &rec: readAllRecord()){
        if (getField(rec, userFieldIndex(rec)) == username && getField(rec, chartFieldIndex(rec)) == chartID)
            result.
                push_back(rec);
    }
    return result;
}

// Reads verified records by username and chart ID (truncate first, then filter).
std::vector<std::string> ProofedRecordsDAO::readVerifiedRecordByUserAndChart(const std::string &username,
                                                                             const std::string &chartID
    ) {
    std::vector<std::string> result;
    for (const auto &rec: readVerifiedRecord()){
        if (getField(rec, userFieldIndex(rec)) == username && getField(rec, chartFieldIndex(rec)) == chartID)
            result.
                push_back(rec);
    }
    return result;
}

std::vector<std::string> ProofedRecordsDAO::readAllRecordByUID(const std::string &uid) {
    std::vector<std::string> result;
    for (const auto &rec: readAllRecord()){
        const auto idx = uidFieldIndex(rec);
        if (idx == static_cast<size_t>(-1)) continue;
        if (getField(rec, idx) == uid) result.push_back(rec);
    }
    return result;
}

std::vector<std::string> ProofedRecordsDAO::readVerifiedRecordByUID(const std::string &uid) {
    std::vector<std::string> result;
    for (const auto &rec: readVerifiedRecord()){
        const auto idx = uidFieldIndex(rec);
        if (idx == static_cast<size_t>(-1)) continue;
        if (getField(rec, idx) == uid) result.push_back(rec);
    }
    return result;
}

std::vector<std::string> ProofedRecordsDAO::readAllRecordByUIDAndChart(const std::string &uid,
                                                                       const std::string &chartId
    ) {
    std::vector<std::string> result;
    for (const auto &rec: readAllRecord()){
        const auto uidIdx = uidFieldIndex(rec);
        if (uidIdx == static_cast<size_t>(-1)) continue;
        if (getField(rec, uidIdx) == uid && getField(rec, chartFieldIndex(rec)) == chartId){
            result.push_back(rec);
        }
    }
    return result;
}

std::vector<std::string> ProofedRecordsDAO::readVerifiedRecordByUIDAndChart(const std::string &uid,
                                                                            const std::string &chartId
    ) {
    std::vector<std::string> result;
    for (const auto &rec: readVerifiedRecord()){
        const auto uidIdx = uidFieldIndex(rec);
        if (uidIdx == static_cast<size_t>(-1)) continue;
        if (getField(rec, uidIdx) == uid && getField(rec, chartFieldIndex(rec)) == chartId){
            result.push_back(rec);
        }
    }
    return result;
}

bool ProofedRecordsDAO::addAfterVerified(const std::string &serial_record) {
    if (!LiteDBUtils::ensureKey()){
        ErrorNotifier::notify("ProofedRecordsDAO::addAfterVerified",
                              I18nService::instance().get("error.encryption_key_ensure_failed"));
        return false;
    }
    const auto recLines        = readNonEmptyLines(recordList);
    const auto hashLines       = readNonEmptyLines(proofList);
    const size_t verifiedCount = verifiedPrefixCount(recLines, hashLines);

    auto plainRecords    = decryptRecordLines(recLines);
    const auto insertPos = std::min(verifiedCount, plainRecords.size());
    plainRecords.insert(plainRecords.begin() + static_cast<std::ptrdiff_t>(insertPos), serial_record);
    if (!writeRecordLines(recordList, plainRecords)) return false;
    return writeProofLines(proofList, plainRecords);
}

bool ProofedRecordsDAO::coverAfterVerified(const std::string &serial_record) {
    if (!LiteDBUtils::ensureKey()){
        ErrorNotifier::notify("ProofedRecordsDAO::coverAfterVerified",
                              I18nService::instance().get("error.encryption_key_ensure_failed"));
        return false;
    }
    const auto recLines        = readNonEmptyLines(recordList);
    const auto hashLines       = readNonEmptyLines(proofList);
    const size_t verifiedCount = verifiedPrefixCount(recLines, hashLines);

    auto plainRecords    = decryptRecordLines(recLines);
    const auto insertPos = std::min(verifiedCount, plainRecords.size());
    plainRecords.insert(plainRecords.begin() + static_cast<std::ptrdiff_t>(insertPos), serial_record);
    if (insertPos + 1 < plainRecords.size()){
        plainRecords.erase(plainRecords.begin() + static_cast<std::ptrdiff_t>(insertPos + 1));
    }
    if (!writeRecordLines(recordList, plainRecords)) return false;
    return writeProofLines(proofList, plainRecords);
}

bool ProofedRecordsDAO::cleanNotVerified() {
    if (!LiteDBUtils::ensureKey()){
        ErrorNotifier::notify("ProofedRecordsDAO::cleanNotVerified",
                              I18nService::instance().get("error.encryption_key_ensure_failed"));
        return false;
    }
    const auto recLines        = readNonEmptyLines(recordList);
    const auto hashLines       = readNonEmptyLines(proofList);
    const size_t verifiedCount = verifiedPrefixCount(recLines, hashLines);

    auto plainRecords = decryptRecordLines(recLines);
    if (verifiedCount < plainRecords.size()){
        plainRecords.erase(plainRecords.begin() + static_cast<std::ptrdiff_t>(verifiedCount), plainRecords.end());
    }
    if (!writeRecordLines(recordList, plainRecords)) return false;
    return writeProofLines(proofList, plainRecords);
}