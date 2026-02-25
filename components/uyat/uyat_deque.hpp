#pragma once

#include "sma_stl.hpp"
#include <deque>
#include <vector>

namespace esphome::uyat
{

static constexpr const std::size_t MAX_DEQUE_BUFFER_SIZE = 1024u * 2u;
static constexpr const std::size_t MAX_DEQUE_BUFFER_SLOTS = 200;

struct DequeMemoryPool
{
   DequeMemoryPool(const std::size_t max_buffer_size, const std::size_t max_buffer_chunks):
   buffer_(max_buffer_size),
   allocator_(buffer_, max_buffer_chunks)
   {}

   static sma::StaticMemoryAllocator& get_sma()
   {
      static DequeMemoryPool instance(MAX_DEQUE_BUFFER_SIZE, MAX_DEQUE_BUFFER_SLOTS);
      return instance.allocator_;
   }

private:

   std::vector<uint8_t> buffer_;
   sma::StaticMemoryAllocator allocator_;
};

struct StaticDeque
{
   using underlying_type_t = std::deque<uint8_t, sma::STLAllocator<uint8_t, DequeMemoryPool>>;

   struct DequeView
   {
      inline StaticDeque::underlying_type_t::const_iterator cbegin() const
      {
         if (size_ == 0)
         {
            return buffer_.cend();
         }
         return buffer_.cbegin() + offset_;
      }

      inline StaticDeque::underlying_type_t::const_iterator cend() const
      {
         if (size_ == 0)
         {
            return buffer_.cend();
         }
         return buffer_.cbegin() + offset_ + size_;
      }

      inline uint8_t byte_at(const std::size_t idx) const {
         return buffer_[offset_ + idx];
      }

      DequeView create_view(const std::size_t offset, const std::size_t size) const
      {
         if ((offset >= size_) || ((offset + size) > size_))
         {
            return DequeView{buffer_, 0, 0};
         }
         return DequeView{buffer_, offset + offset_, size};
      }

      const StaticDeque::underlying_type_t& buffer_;
      const std::size_t offset_; // relative to the parent
      const std::size_t size_;
   };


   DequeView create_view() const
   {
      return DequeView{buffer_, 0, buffer_.size()};
   }
   DequeView create_view(const std::size_t offset, const std::size_t size) const
   {
      if ((offset >= buffer_.size()) || ((offset + size) > buffer_.size()))
      {
         return DequeView{buffer_, 0, 0};
      }
      return DequeView{buffer_, offset, size};
   }

   underlying_type_t buffer_;
};

}
