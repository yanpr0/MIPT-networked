#pragma once

#include <cstddef>
#include <cstring>
#include <stdexcept>

class Bitstream
{
public:
    Bitstream(void* data, std::size_t size) : data_{static_cast<std::byte*>(data)}, size_{size} {}

    Bitstream(std::size_t size) : data_{new std::byte[size]}, size_{size}, alloc_{true} {}

    ~Bitstream() {
        if (alloc_)
        {
            delete[] data_;
        }
    }

    void reset() noexcept { offset_ = 0; }

    std::byte* data() const noexcept { return data_; }

    std::size_t size() const noexcept { return size_; }

    std::size_t offset() const noexcept { return offset_; }

    template <typename T>
    void write(const T& val)
    {
        if (size_ - offset_ < sizeof(T))
        {
            throw std::length_error("insufficient buffer size");
        }
        std::memcpy(data_ + offset_, &val, sizeof(T));
        offset_ += sizeof(T);
    }

    template <typename T>
    void read(T& val)
    {
        if (size_ - offset_ < sizeof(T))
        {
            throw std::length_error("insufficient buffer size");
        }
        std::memcpy(&val, data_ + offset_, sizeof(T));
        offset_ += sizeof(T);
    }

    void ignore(std::size_t size)
    {
        if (size_ - offset_ < size)
        {
            throw std::length_error("insufficient buffer size");
        }
        offset_ += size;
    }

private:
    std::byte* data_;
    std::size_t size_;
    std::size_t offset_ = 0;
    bool alloc_ = false;
};

