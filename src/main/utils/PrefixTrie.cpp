#include "PrefixTrie.h"

#include <algorithm>
#include <cctype>

std::size_t PrefixTrie::commonPrefixLength(const std::string &a, const std::string &b) {
    const std::size_t limit = std::min(a.size(), b.size());
    std::size_t i           = 0;
    while (i < limit && a[i] == b[i]){
        ++i;
    }
    return i;
}

std::string PrefixTrie::normalize(const std::string &text) {
    std::string out;
    out.reserve(text.size());
    for (const unsigned char ch: text){
        out.push_back(static_cast<char>(std::tolower(ch)));
    }
    return out;
}

void PrefixTrie::collectIndices(const Node* node, std::vector<std::size_t> &out) {
    if (node == nullptr) return;
    out.insert(out.end(), node->indices.begin(), node->indices.end());
    for (const auto &[_, child]: node->children){
        collectIndices(child.get(), out);
    }
}

const PrefixTrie::Node *PrefixTrie::descendFrom(const Node* startNode, const std::string &suffix) {
    const Node* node = startNode;
    for (const char ch: suffix){
        if (node == nullptr) return nullptr;
        auto it = node->children.find(ch);
        if (it == node->children.end()) return nullptr;
        node = it->second.get();
    }
    return node;
}

const PrefixTrie::Node *PrefixTrie::findNodeByPrefix(const std::string &normalizedPrefix) const {
    return descendFrom(&rootNode, normalizedPrefix);
}

void PrefixTrie::insert(const std::string &word, std::size_t index) {
    const std::string key = normalize(word);
    Node* node            = &rootNode;
    for (const char ch: key){
        auto &child = node->children[ch];
        if (!child){
            child         = std::make_unique<Node>();
            child->parent = node;
        }
        node = child.get();
    }
    node->indices.push_back(index);
}

std::vector<std::size_t> PrefixTrie::searchByPrefix(const std::string &prefix) const {
    SearchCursor cursor;
    return searchByPrefixIncremental(prefix, cursor);
}

std::vector<std::size_t> PrefixTrie::searchByPrefixIncremental(const std::string &prefix, SearchCursor &cursor) const {
    const std::string normalizedTarget = normalize(prefix);

    const Node* node = nullptr;
    std::string previousPrefix;
    if (cursor.generation == generation){
        previousPrefix = cursor.normalizedPrefix;
        node           = static_cast<const Node *>(cursor.node);
    }
    else{
        previousPrefix.clear();
        node = &rootNode;
    }

    if (node == nullptr){
        node = findNodeByPrefix(previousPrefix);
    }

    const std::size_t lcpLength = commonPrefixLength(previousPrefix, normalizedTarget);

    if (node != nullptr && lcpLength < previousPrefix.size()){
        std::size_t backtrackSteps = previousPrefix.size() - lcpLength;
        while (backtrackSteps > 0 && node != nullptr){
            node = node->parent;
            --backtrackSteps;
        }
    }

    if (node == nullptr){
        node = findNodeByPrefix(normalizedTarget.substr(0, lcpLength));
    }

    const std::string suffix = normalizedTarget.substr(lcpLength);
    node                     = descendFrom(node, suffix);

    cursor.normalizedPrefix = normalizedTarget;
    cursor.node             = node;
    cursor.generation       = generation;

    if (node == nullptr) return {};

    std::vector<std::size_t> result;
    collectIndices(node, result);
    std::sort(result.begin(), result.end());
    result.erase(std::unique(result.begin(), result.end()), result.end());
    return result;
}

void PrefixTrie::clear() {
    rootNode = Node{};
    ++generation;
}
