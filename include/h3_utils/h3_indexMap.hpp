#ifndef BG_H3_INDEXMAP_HPP
#define BG_H3_INDEXMAP_HPP

#include <memory>

#include <h3/h3api.h>

template <typename T, int N>
struct H3Cell {
    using ChildCell = H3Cell<T, N+1>;

    using resolution_bit_offset = 42;
    using resolution_bit_mask = 0b1111 << resolution_bit_offset;
    using local_index_bit_offset = 3*(15-N);
    using local_index_bit_mask = 0b111 << local_index_bit_offset;
    std::array<std::uniq_ptr<ChildCell>, 7> childs;
    T value;

    T & operator[](const H3Index index) {
        if((index & resolution_bit_mask) >> resolution_bit_offset == N) return value;
        std::uniq_ptr<ChildCell> & child = childs[(index & local_index_bit_mask) >> local_index_bit_offset];
        if(child == nullptr)
            child = std::make_unique<ChildCell>();
        return child->find(index);
    }
};

template <typename T>
struct H3Cell<T, 15> {
    T value;

    T & operator[](const H3Index) { return value; }
};


template <typename T>
class H3IndexMap {
private:
    using ChildCell = H3Cell<T, 0>;
    using bit_offset = 35;
    using bit_mask = 0b1111111 << bit_offset;

    std::array<std::uniq_ptr<ChildCell>, 122> first_childs;

public:
    void set(const H3Index index, T value) {

    }

    T & operator[](const H3Index index) {
        return first_childs[(index & bit_mask) >> bit_offset].find(index);
    }
};

#endif // BG_H3_INDEXMAP_HPP