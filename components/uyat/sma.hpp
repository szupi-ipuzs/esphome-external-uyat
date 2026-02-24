#pragma once

#include <cstdint>
#include <functional>
#include <vector>
#include <algorithm>
#include <optional>
#include <span>

namespace sma
{

struct StaticMemoryAllocator
{
   explicit StaticMemoryAllocator(std::span<uint8_t> buffer, const std::size_t max_chunks):
   buffer_(buffer),
   occupied_chunks_(max_chunks),
   free_chunks_(max_chunks + 1u)
   {
      free_chunks_[0] = Chunk{.used = true, .offset = 0, .size = buffer.size()};
   }

   StaticMemoryAllocator(const StaticMemoryAllocator&) = delete;
   StaticMemoryAllocator& operator=(const StaticMemoryAllocator&) = delete;

   StaticMemoryAllocator(StaticMemoryAllocator&&) = default;
   StaticMemoryAllocator& operator=(StaticMemoryAllocator&&) = default;

   std::size_t max_size() const
   {
      std::size_t result = 0u;

      for (auto & chunk: free_chunks_)
      {
         if ((chunk.used) && (chunk.size > result))
         {
            result = chunk.size;
         }
      }

      return result;
   }

   std::size_t total_occupied() const
   {
      std::size_t result = 0u;
      for (auto & chunk: occupied_chunks_)
      {
         if (chunk.used)
         {
            result += chunk.size;
         }
      }
      return result;
   }

   std::size_t total_free() const
   {
      std::size_t result = 0u;
      for (auto & chunk: free_chunks_)
      {
         if (chunk.used)
         {
            result += chunk.size;
         }
      }
      return result;
   }

   uint8_t* allocate(const std::size_t size)
   {
      auto new_slot = find_empty_slot(occupied_chunks_);
      if (!new_slot)
      {
         return nullptr;
      }

      auto selected_free_chunk_index = select_free_chunk_index(size);
      if (!selected_free_chunk_index)
      {
         return nullptr;
      }

      auto & selected_free_chunk = free_chunks_[*selected_free_chunk_index];
      auto & occupied_chunk = occupied_chunks_[*new_slot];
      occupied_chunk = Chunk{.used = true, .offset = selected_free_chunk.offset, .size = size};
      selected_free_chunk.offset += size;
      selected_free_chunk.size -= size;
      if (selected_free_chunk.size == 0)
      {
         selected_free_chunk.used = 0;
      }
      return &buffer_[occupied_chunk.offset];
   }

   void free(uint8_t* ptr)
   {
      if ((ptr < &buffer_[0]) || (ptr >= &buffer_[buffer_.size()]))
      {
         // buffer out of range
         return;
      }
      // find chunk by pointer
      // todo: binary search should be faster over bigger number of chunks (only sorted!)
      auto occupied_idx = find_occupied_chunk_by_ptr(ptr);
      if (!occupied_idx)
      {
         return;
      }

      // move to the free slots
      auto new_slot = find_empty_slot(free_chunks_);
      free_chunks_[*new_slot] = std::move(occupied_chunks_[*occupied_idx]);
      occupied_chunks_[*occupied_idx].used = false;

      // merge free slots if needed
      merge_free_slots();
   }

private:

   struct Chunk
   {
      bool used = false;
      std::size_t offset = 0;
      std::size_t size = 0;
   };

   bool is_chunk_buffer(const Chunk& chunk, const uint8_t* ptr) const
   {
      if (!chunk.used)
      {
         return false;
      }

      return (&buffer_[chunk.offset] == ptr);
   }

   bool is_valid_chunk(const Chunk& chunk) const
   {
      return (chunk.offset + chunk.size) <= buffer_.size();
   }

   std::optional<std::size_t> find_empty_slot(const std::vector<Chunk>& chunks) const
   {
      for (std::size_t idx = 0; idx < chunks.size(); ++idx)
      {
         if (!chunks[idx].used)
         {
            return idx;
         }
      }

      return std::nullopt;
   }

   bool chunks_overlap(const Chunk& chunk1, const Chunk& chunk2) const
   {
      if ((chunk2.offset >= chunk1.offset) && (chunk2.offset < (chunk1.offset + chunk1.size)))
      {
         return true;
      }
      if ((chunk1.offset >= chunk2.offset) && (chunk1.offset < (chunk2.offset + chunk2.size)))
      {
         return true;
      }
      if ((chunk1.offset == chunk2.offset) && (chunk1.size == chunk2.size))
      {
         return true;
      }
      return false;
   }

   std::optional<std::size_t> select_free_chunk_index(const std::size_t size) const
   {
      std::optional<std::size_t> result;
      for (std::size_t idx = 0; idx < free_chunks_.size(); ++idx)
      {
         auto & chunk = free_chunks_[idx];
         if ((chunk.used) && (chunk.size >= size))
         {
            if (chunk.size == size)
            {
               return idx;
            }
            if ((!result) || (chunk.size < free_chunks_[*result].size))
            {
               result = idx;
            }
         }
      }

      return result;
   }

   std::optional<std::size_t> find_occupied_chunk_by_ptr(const uint8_t* ptr) const
   {
      for (std::size_t idx = 0; idx < occupied_chunks_.size(); ++idx)
      {
         auto & chunk = occupied_chunks_[idx];
         if (is_chunk_buffer(chunk, ptr))
         {
            return idx;
         }
      }
      return std::nullopt;
   }

   void merge_free_slots()
   {
      // sort first
      std::sort(free_chunks_.begin(), free_chunks_.end(), [](const auto& lhs, const auto& rhs){
         if (!lhs.used)
         {
            return false;
         }

         if (!rhs.used)
         {
            return true;
         }

         return lhs.offset < rhs.offset;
      });

      std::size_t current_idx = 0u;
      while (current_idx < free_chunks_.size())
      {
         auto & current = free_chunks_[current_idx];
         if (current.used)
         {
            std::size_t next_idx = current_idx + 1u;
            while (next_idx < free_chunks_.size())
            {
               auto & next = free_chunks_[next_idx];
               if (next.used)
               {
                  if (next.offset == (current.offset + current.size))
                  {
                     current.size += next.size;
                     next.used = false;
                  }
               }
               ++next_idx;
            }
         }
         ++current_idx;
      }
   }

   std::span<uint8_t> buffer_;

   // used memory chunks
   std::vector<Chunk> occupied_chunks_;
   std::vector<Chunk> free_chunks_;
};

}
