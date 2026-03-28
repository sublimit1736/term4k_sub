#pragma once

#include <cstddef>
#include <map>
#include <memory>
#include <string>
#include <vector>

class PrefixTrie {
public:
    struct SearchCursor {
        std::string normalizedPrefix;
        const void* node       = nullptr;
        std::size_t generation = 0;
    };

    void insert(const std::string &word, std::size_t index);

    std::vector<std::size_t> searchByPrefix(const std::string &prefix) const;

    // Reuses the last cursor position by backtracking to LCP, then descends to target prefix.
    std::vector<std::size_t> searchByPrefixIncremental(const std::string &prefix, SearchCursor &cursor) const;

    void clear();

private:
    struct Node {
        Node* parent = nullptr;
        std::map<char, std::unique_ptr<Node>> children;
        std::vector<std::size_t> indices;
    };

    static std::string normalize(const std::string &text);

    static std::size_t commonPrefixLength(const std::string &a, const std::string &b);

    const Node *descendFrom(const Node* startNode, const std::string &suffix) const;

    const Node *findNodeByPrefix(const std::string &normalizedPrefix) const;

    static void collectIndices(const Node* node, std::vector<std::size_t> &out);

    Node rootNode;
    std::size_t generation = 1;
};