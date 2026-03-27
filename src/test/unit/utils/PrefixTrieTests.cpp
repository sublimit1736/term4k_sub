#include "catch_amalgamated.hpp"

#include "utils/PrefixTrie.h"

TEST_CASE("PrefixTrie supports case-insensitive prefix search", "[utils][PrefixTrie]") {
    PrefixTrie trie;

    trie.insert("Hello", 3);
    trie.insert("helicopter", 1);
    trie.insert("hero", 2);

    const auto result = trie.searchByPrefix("HE");
    REQUIRE(result == std::vector<std::size_t>{1, 2, 3});
}

TEST_CASE("PrefixTrie removes duplicate indices and clears correctly", "[utils][PrefixTrie]") {
    PrefixTrie trie;

    trie.insert("term", 4);
    trie.insert("terminal", 4);
    trie.insert("team", 7);

    REQUIRE(trie.searchByPrefix("te") == std::vector<std::size_t>{4, 7});

    trie.clear();
    REQUIRE(trie.searchByPrefix("te").empty());
}

TEST_CASE("PrefixTrie handles empty or missing prefixes", "[utils][PrefixTrie]") {
    PrefixTrie trie;
    trie.insert("Alpha", 9);
    trie.insert("Beta", 1);

    REQUIRE(trie.searchByPrefix("") == std::vector<std::size_t>{1, 9});
    REQUIRE(trie.searchByPrefix("zzz").empty());
}

TEST_CASE("PrefixTrie incremental search reuses cursor across extend and backtrack", "[utils][PrefixTrie]") {
    PrefixTrie trie;
    trie.insert("star", 0);
    trie.insert("start", 1);
    trie.insert("stack", 2);
    trie.insert("stone", 3);

    PrefixTrie::SearchCursor cursor;

    REQUIRE(trie.searchByPrefixIncremental("sta", cursor) == std::vector<std::size_t>{0, 1, 2});
    REQUIRE(trie.searchByPrefixIncremental("star", cursor) == std::vector<std::size_t>{0, 1});
    REQUIRE(trie.searchByPrefixIncremental("st", cursor) == std::vector<std::size_t>{0, 1, 2, 3});
    REQUIRE(trie.searchByPrefixIncremental("sto", cursor) == std::vector<std::size_t>{3});
    REQUIRE(trie.searchByPrefixIncremental("x", cursor).empty());
    REQUIRE(trie.searchByPrefixIncremental("st", cursor) == std::vector<std::size_t>{0, 1, 2, 3});
}


