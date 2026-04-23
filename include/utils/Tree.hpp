#pragma once

#include <vector>
#include <optional>
#include <functional>
#include <stdexcept>

template <typename T>
struct Tree {
    T data;
    std::vector<Tree<T>> children;
};

template <typename T>
std::optional<T> searchTree(Tree<T>& tree, std::function<bool(T&)> predicate) {
    if (predicate(tree.data)) {
        return tree.data;
    }
    for (auto& child : tree.children) {
        auto result = searchTree(child, predicate);
        if (result.has_value()) {
            return result;
        }
    }
    return std::nullopt;
}
