#pragma once

#include "array_ptr.h"

#include <algorithm>
#include <cassert>
#include <initializer_list>
#include <iterator>
#include <stdexcept>
#include <utility>

struct ReserveProxyObj {
    ReserveProxyObj() = default;
    ReserveProxyObj(size_t capacity) : capacity_(capacity) {
    }
    
    size_t capacity_ = 0;
};

template <typename Type>
class SimpleVector {
public:
    using Iterator = Type*;
    using ConstIterator = const Type*;

    SimpleVector() noexcept = default;

    explicit SimpleVector(size_t size) 
        : items_(new Type[size]), size_(size), capacity_(size) {
        std::fill(begin(), end(), Type());
    }

    SimpleVector(size_t size, const Type& value)
        : items_(new Type[size]), size_(size), capacity_(size) {
        std::fill(begin(), end(), value);
    }

    SimpleVector(std::initializer_list<Type> init) 
        : items_(new Type[init.size()]), size_(init.size()), capacity_(init.size()) {
        std::copy(init.begin(), init.end(), begin());
    }
    
    SimpleVector(const SimpleVector& other) 
        : items_(new Type[other.capacity_]), size_(other.size_), capacity_(other.capacity_) {
        std::copy(other.begin(), other.end(), begin());
    }
    
    SimpleVector(SimpleVector&& other) 
        : items_(std::move(other.items_))
        , size_(std::exchange(other.size_, 0))
        , capacity_(std::exchange(other.capacity_, 0)) {
    }
    
    SimpleVector(ReserveProxyObj obj) 
        : items_(new Type[obj.capacity_]), size_(0), capacity_(obj.capacity_) {
    }

    SimpleVector& operator=(const SimpleVector& rhs) {
        if (this != &rhs) {
            ArrayPtr<Type> other(rhs.capacity_);
            items_.swap(other);
            std::copy(rhs.begin(), rhs.end(), begin());
            size_ = rhs.size_;
            capacity_ = rhs.capacity_; 
        }
        return *this;
    }
    
    SimpleVector& operator=(SimpleVector&& rhs) {
        if (this != &rhs) {
            items_ = std::move(rhs.items_);
            size_ = std::exchange(rhs.size_, 0);
            capacity_ = std::exchange(rhs.capacity_, 0);
        }
        return *this;
    }
    
    Iterator begin() noexcept {
        return items_.Get();
    }

    Iterator end() noexcept {
        return items_.Get() + size_;
    }

    ConstIterator begin() const noexcept {
        return items_.Get();
    }

    ConstIterator end() const noexcept {
        return items_.Get() + size_;
    }

    ConstIterator cbegin() const noexcept {
        return items_.Get();
    }

    ConstIterator cend() const noexcept {
        return items_.Get() + size_;
    }

    size_t GetSize() const noexcept {
        return size_;
    }

    size_t GetCapacity() const noexcept {
        return capacity_;
    }

    bool IsEmpty() const noexcept {
        return size_ == 0;
    }

    Type& operator[](size_t index) noexcept {
        assert(index < size_);
        return items_[index];
    }

    const Type& operator[](size_t index) const noexcept {
        assert(index < size_);
        return items_[index];
    }

    Type& At(size_t index) {
        if (index >= size_) {
            throw std::out_of_range("Index is out of range");
        }
        return items_[index];
    }

    const Type& At(size_t index) const {
        if (index >= size_) {
            throw std::out_of_range("Index is out of range");
        }
        return items_[index];
    }

    void Clear() noexcept {
        size_ = 0;
    }
    
    void Resize(size_t new_size) {
        if (new_size <= size_) {
            size_ = new_size;
        } else if (new_size <= capacity_) {
            for (auto it = end(); it != end() + (new_size - size_); ++it) {
                *it = std::move(Type());
            }
            size_ = new_size;
        } else {
            size_t new_capacity = std::max(new_size, 2 * capacity_);
            ArrayPtr<Type> new_items(new_capacity);
            std::move(begin(), end(), new_items.Get());
            for (auto it = new_items.Get() + size_; it != new_items.Get() + new_size; ++it) {
                *it = std::move(Type());
            }
            items_.swap(new_items);
            size_ = new_size;
            capacity_ = new_capacity;
        }
    }

    void PushBack(const Type& item) {
        Resize(size_ + 1);
        items_[size_ - 1] = item;
    }
    
    void PushBack(Type&& item) {
        Resize(size_ + 1);
        items_[size_ - 1] = std::move(item);
    }

    Iterator Insert(ConstIterator pos, const Type& value) {
        assert(pos >= cbegin() && pos <= cend());
        
        size_t index = std::distance(cbegin(), pos);
        Resize(size_ + 1);
        Iterator new_pos = &items_[index];
        std::copy_backward(new_pos, begin() + (size_ - 1), end());
        *new_pos = value;
        return new_pos;
    }
    
    Iterator Insert(ConstIterator pos, Type&& value) {
        assert(pos >= cbegin() && pos <= cend());
    
        size_t index = std::distance(cbegin(), pos);
        Resize(size_ + 1);
        Iterator new_pos = &items_[index];
        std::move_backward(new_pos, begin() + (size_ - 1), end());
        *new_pos = std::move(value);
        return new_pos;
    }

    void PopBack() noexcept {
        if (size_ > 0) {
            --size_;
        }
    }

    Iterator Erase(ConstIterator pos) {
        assert(pos >= cbegin() && pos < cend());
    
        size_t index = std::distance(cbegin(), pos);
        std::move(begin() + index + 1, end(), const_cast<Type*>(pos));
        --size_;
        return Iterator{&items_[index]};
    }

    void swap(SimpleVector& other) noexcept {
        items_.swap(other.items_);
        std::swap(size_, other.size_);
        std::swap(capacity_, other.capacity_);
    }
    
    void Reserve(size_t new_capacity) {
        if (new_capacity > capacity_) {
            ArrayPtr<Type> new_items(new_capacity);
            std::copy(begin(), end(), new_items.Get());
            items_.swap(new_items);
            capacity_ = new_capacity;
        }
    }
    
private:
    ArrayPtr<Type> items_;
    size_t size_ = 0;
    size_t capacity_ = 0;
};

ReserveProxyObj Reserve(size_t capacity_to_reserve) {
    return ReserveProxyObj(capacity_to_reserve);
}

template <typename Type>
inline bool operator==(const SimpleVector<Type>& lhs, const SimpleVector<Type>& rhs) {
    return std::equal(lhs.begin(), lhs.end(), rhs.begin(), rhs.end());
}

template <typename Type>
inline bool operator!=(const SimpleVector<Type>& lhs, const SimpleVector<Type>& rhs) {
    return !(lhs == rhs);
}

template <typename Type>
inline bool operator<(const SimpleVector<Type>& lhs, const SimpleVector<Type>& rhs) {
    return std::lexicographical_compare(lhs.begin(), lhs.end(), rhs.begin(), rhs.end());
}

template <typename Type>
inline bool operator<=(const SimpleVector<Type>& lhs, const SimpleVector<Type>& rhs) {
    return !(rhs < lhs);
}

template <typename Type>
inline bool operator>(const SimpleVector<Type>& lhs, const SimpleVector<Type>& rhs) {
    return rhs < lhs;
}

template <typename Type>
inline bool operator>=(const SimpleVector<Type>& lhs, const SimpleVector<Type>& rhs) {
    return !(lhs < rhs);
} 