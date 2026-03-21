#pragma once

#include <vector>
#include <functional>
#include <stdexcept>

template <typename T>
struct Tree {
    T data;
    std::vector<Tree<T>> children;
};

template <typename T>
T& searchTree(Tree<T>& tree, std::function<bool(T&)> predicate) {
    if (predicate(tree.data)) {
        return tree.data;
    }
    for (auto& child : tree.children) {
        try {
            return searchTree(child, predicate);
        } catch (const std::runtime_error&) {
            continue;
        }
    }
    throw std::runtime_error("Element not found in tree");
}
