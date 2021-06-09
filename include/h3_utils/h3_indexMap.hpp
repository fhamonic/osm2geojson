#ifndef BG_H3_INDEXMAP_HPP
#define BG_H3_INDEXMAP_HPP

#include <memory>

#include <h3/h3api.h>

template <typename T, int N>
struct H3Cell {
    using ChildCell = H3Cell<T, N+1>;

    static constexpr int resolution_bit_offset = 52;
    static constexpr H3Index resolution_bit_mask = static_cast<H3Index>(0b1111) << resolution_bit_offset;
    static constexpr int local_index_bit_offset = 3*(15-N);
    static constexpr H3Index local_index_bit_mask = static_cast<H3Index>(0b111) << local_index_bit_offset;

    std::array<std::unique_ptr<ChildCell>, 7> childs;
    T value;

    T & operator[](const H3Index index) {
        if((index & resolution_bit_mask) >> resolution_bit_offset == N) return value;
        std::unique_ptr<ChildCell> & child = childs[(index & local_index_bit_mask) >> local_index_bit_offset];
        if(child == nullptr)
            child = std::make_unique<ChildCell>();
        return child->operator[](index);
    }

    bool contains(const H3Index index) const {
        if((index & resolution_bit_mask) >> resolution_bit_offset == N) return true;
        const std::unique_ptr<ChildCell> & child = childs[(index & local_index_bit_mask) >> local_index_bit_offset];
        if(child == nullptr)
            return false;
        return child->contains(index);
    }

    size_t count_leafs() {
        size_t sum = 0;
        for(const auto & child : childs) {
            if(child == nullptr) continue;
            sum += child->count_leafs();
        }
        return (sum == 0) ? 1 : sum;
    }
};
template <typename T>
struct H3Cell<T, 15> {
    T value;
    T & operator[](const H3Index) { return value; }
    bool contains(const H3Index) const { return true; }
    size_t count_leafs() { return 1; }
};


template <typename T>
class H3IndexMap {
private:
    using ChildCell = H3Cell<T, 1>;

    static constexpr int base_index_bit_offset = 45;
    static constexpr H3Index base_index_bit_mask = static_cast<H3Index>(0b1111111) << base_index_bit_offset;

    std::array<std::unique_ptr<ChildCell>, 122> first_childs;

public:
    T & operator[](const H3Index index) {
        std::unique_ptr<ChildCell> & child = first_childs[(index & base_index_bit_mask) >> base_index_bit_offset];
        if(child == nullptr)
            child = std::make_unique<ChildCell>();
        return child->operator[](index);
    }

    bool contains(const H3Index index) {
        const std::unique_ptr<ChildCell> & child = first_childs[(index & base_index_bit_mask) >> base_index_bit_offset];
        if(child == nullptr)
            return false;
        return child->contains(index);
    }

    size_t count_leafs() {
        size_t sum = 0;
        for(const auto & child : first_childs) {
            if(child == nullptr) continue;
            sum += child->count_leafs();
        }
        return sum;
    }
};

#endif // BG_H3_INDEXMAP_HPP