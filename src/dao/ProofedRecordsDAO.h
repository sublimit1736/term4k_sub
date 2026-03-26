#ifndef TERM4K_PROOFEDRECORDSDAO_H
#define TERM4K_PROOFEDRECORDSDAO_H

#include <cstdint>
#include <string>
#include <vector>

// Anti-tamper score database DAO (all-static utility class).
// Uses chained hashes to protect historical records from modification.
class ProofedRecordsDAO {
public:
    /**
     * Sets the directory for records.db, proof.db, and key.bin.
     * Passing empty string or "." restores default behavior (current working directory, useful in tests).
     * In production installation, call this before any other ProofedRecordsDAO method.
     * This method is idempotent and safe to call multiple times.
     */
    static void setDataDir(const std::string &dir);

    // Appends one score record.
    // Recommended format: uid chartId songName username score accuracy timestamp
    static bool addRecord(const std::string &serial_record);

    // Inserts after the last verified record; following records remain unverified.
    static bool addAfterVerified(const std::string &serial_record);

    // Inserts after the last verified record and optionally overwrites the first unverified record.
    static bool coverAfterVerified(const std::string &serial_record);

    // Removes all records from the earliest unverified record to the end.
    static bool cleanNotVerified();

    // Verifies integrity of the whole hash chain.
    static bool verifyChain();

    // Reads all score records (decrypted plaintext).
    static std::vector<std::string> readAllRecord();

    // Reads records in time order and truncates at the first verification failure.
    // Returns only the continuous verified prefix.
    static std::vector<std::string> readVerifiedRecord();

    // Returns the number of continuously verified records from the beginning.
    static size_t verifiedRecordCount();

    // Reads all records by username (without chain verification).
    static std::vector<std::string> readAllRecordByUser(const std::string &username);

    // Reads verified records by username (chain-truncate first, then filter).
    static std::vector<std::string> readVerifiedRecordByUser(const std::string &username);

    // Reads all records by chart ID (without chain verification).
    static std::vector<std::string> readAllRecordByChart(const std::string &chartId);

    // Reads verified records by chart ID (chain-truncate first, then filter).
    static std::vector<std::string> readVerifiedRecordByChart(const std::string &chartId);

    // Reads all records by username and chart ID (without chain verification).
    static std::vector<std::string> readAllRecordByUserAndChart(const std::string &username,
                                                                const std::string &chartId
        );

    // Reads verified records by username and chart ID (chain-truncate first, then filter).
    static std::vector<std::string> readVerifiedRecordByUserAndChart(const std::string &username,
                                                                     const std::string &chartId
        );

    // Reads all records by UID (without chain verification).
    static std::vector<std::string> readAllRecordByUID(const std::string &uid);

    // Reads verified records by UID.
    static std::vector<std::string> readVerifiedRecordByUID(const std::string &uid);

    // Reads all records by UID and chart ID (without chain verification).
    static std::vector<std::string> readAllRecordByUIDAndChart(const std::string &uid,
                                                               const std::string &chartId
        );

    // Reads verified records by UID and chart ID.
    static std::vector<std::string> readVerifiedRecordByUIDAndChart(const std::string &uid,
                                                                    const std::string &chartId
        );

private:
    // Score record file path.
    static std::string recordList;
    // Hash-chain proof file path.
    static std::string proofList;

    // Reads raw proof file bytes.
    static std::vector<uint8_t> readRawData();

    // Extracts the last hash value from proof bytes.
    static std::vector<uint8_t> extractLastHash(const std::vector<uint8_t> &data);

    // Extracts idx-th whitespace-separated field from a record string (0-based).
    static std::string getField(const std::string &record, size_t idx);
};

#endif // TERM4K_PROOFEDRECORDSDAO_H