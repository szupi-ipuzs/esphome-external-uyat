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
   struct Stats
   {
      std::size_t peak_allocated_size = 0;
      std::size_t peak_occupied_free_slots = 0;
      std::size_t peak_occupied_used_slots = 0;
      std::size_t smallest_max_chunk = 0;
   };

   explicit StaticMemoryAllocator(std::span<uint8_t> buffer, const std::size_t max_slots):
   buffer_(buffer),
   occupied_slots_(max_slots),
   free_slots_(max_slots + 1u)
   {
      free_slots_[0] = Slot{.used = true, .offset = 0, .size = buffer.size()};
   }

   StaticMemoryAllocator(const StaticMemoryAllocator&) = delete;
   StaticMemoryAllocator& operator=(const StaticMemoryAllocator&) = delete;

   StaticMemoryAllocator(StaticMemoryAllocator&&) = default;
   StaticMemoryAllocator& operator=(StaticMemoryAllocator&&) = default;

   const Stats& get_stats() const
   {
      return stats_;
   }

   std::size_t max_size() const
   {
      std::size_t result = 0u;

      for (auto & slot: free_slots_)
      {
         if ((slot.used) && (slot.size > result))
         {
            result = slot.size;
         }
      }

      return result;
   }

   std::size_t total_occupied() const
   {
      std::size_t result = 0u;
      for (auto & slot: occupied_slots_)
      {
         if (slot.used)
         {
            result += slot.size;
         }
      }
      return result;
   }

   std::size_t total_free() const
   {
      std::size_t result = 0u;
      for (auto & slot: free_slots_)
      {
         if (slot.used)
         {
            result += slot.size;
         }
      }
      return result;
   }

   uint8_t* allocate(const std::size_t size)
   {
      auto new_slot = find_empty_slot(occupied_slots_);
      if (!new_slot)
      {
         return nullptr;
      }

      auto selected_free_slot_index = select_free_slot_index(size);
      if (!selected_free_slot_index)
      {
         return nullptr;
      }

      auto & selected_free_slot = free_slots_[*selected_free_slot_index];
      auto & occupied_slot = occupied_slots_[*new_slot];
      occupied_slot = Slot{.used = true, .offset = selected_free_slot.offset, .size = size};
      selected_free_slot.offset += size;
      selected_free_slot.size -= size;
      if (selected_free_slot.size == 0)
      {
         selected_free_slot.used = 0;
      }
      update_stats();
      return &buffer_[occupied_slot.offset];
   }

   void free(uint8_t* ptr)
   {
      if ((ptr < &buffer_[0]) || (ptr >= &buffer_[buffer_.size()]))
      {
         // buffer out of range
         return;
      }
      // find slot by pointer
      // todo: binary search should be faster over bigger number of slots (only sorted!)
      auto occupied_idx = find_occupied_slot_by_ptr(ptr);
      if (!occupied_idx)
      {
         return;
      }

      // move to the free slots
      auto new_slot = find_empty_slot(free_slots_);
      free_slots_[*new_slot] = std::move(occupied_slots_[*occupied_idx]);
      occupied_slots_[*occupied_idx].used = false;

      // merge free slots if needed
      merge_free_slots();

      update_stats();
   }

private:

   struct Slot
   {
      bool used = false;
      std::size_t offset = 0;
      std::size_t size = 0;
   };

   bool is_slot_buffer(const Slot& slot, const uint8_t* ptr) const
   {
      if (!slot.used)
      {
         return false;
      }

      return (&buffer_[slot.offset] == ptr);
   }

   bool is_valid_slot(const Slot& slot) const
   {
      return (slot.offset + slot.size) <= buffer_.size();
   }

   std::optional<std::size_t> find_empty_slot(const std::vector<Slot>& slots) const
   {
      for (std::size_t idx = 0; idx < slots.size(); ++idx)
      {
         if (!slots[idx].used)
         {
            return idx;
         }
      }

      return std::nullopt;
   }

   bool slots_overlap(const Slot& slot1, const Slot& slot2) const
   {
      if ((slot2.offset >= slot1.offset) && (slot2.offset < (slot1.offset + slot1.size)))
      {
         return true;
      }
      if ((slot1.offset >= slot2.offset) && (slot1.offset < (slot2.offset + slot2.size)))
      {
         return true;
      }
      if ((slot1.offset == slot2.offset) && (slot1.size == slot2.size))
      {
         return true;
      }
      return false;
   }

   std::optional<std::size_t> select_free_slot_index(const std::size_t size) const
   {
      std::optional<std::size_t> result;
      for (std::size_t idx = 0; idx < free_slots_.size(); ++idx)
      {
         auto & slot = free_slots_[idx];
         if ((slot.used) && (slot.size >= size))
         {
            if (slot.size == size)
            {
               return idx;
            }
            if ((!result) || (slot.size < free_slots_[*result].size))
            {
               result = idx;
            }
         }
      }

      return result;
   }

   std::optional<std::size_t> find_occupied_slot_by_ptr(const uint8_t* ptr) const
   {
      for (std::size_t idx = 0; idx < occupied_slots_.size(); ++idx)
      {
         auto & slot = occupied_slots_[idx];
         if (is_slot_buffer(slot, ptr))
         {
            return idx;
         }
      }
      return std::nullopt;
   }

   void merge_free_slots()
   {
      // sort first
      std::sort(free_slots_.begin(), free_slots_.end(), [](const auto& lhs, const auto& rhs){
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
      while (current_idx < free_slots_.size())
      {
         auto & current = free_slots_[current_idx];
         if (current.used)
         {
            std::size_t next_idx = current_idx + 1u;
            while (next_idx < free_slots_.size())
            {
               auto & next = free_slots_[next_idx];
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

   std::size_t get_total_used_slots(const std::vector<Slot>& slots) const
   {
      std::size_t result = 0;

      for (auto & slot: slots)
      {
         if (slot.used)
         {
            ++result;
         }
      }

      return result;
   }

   void update_stats()
   {
      {
         const auto allocated_size = total_occupied();
         if (stats_.peak_allocated_size < allocated_size)
         {
            stats_.peak_allocated_size = allocated_size;
         }
      }

      {
         const auto occupied_slots = get_total_used_slots(occupied_slots_);
         if (stats_.peak_occupied_used_slots < occupied_slots)
         {
            stats_.peak_occupied_used_slots = occupied_slots;
         }
      }

      {
         const auto free_slots = get_total_used_slots(free_slots_);
         if (stats_.peak_occupied_free_slots < free_slots)
         {
            stats_.peak_occupied_free_slots = free_slots;
         }
      }

      {
         const auto max_chunk = max_size();
         if ((0u == stats_.smallest_max_chunk) || (stats_.smallest_max_chunk > max_chunk))
         {
            stats_.smallest_max_chunk = max_chunk;
         }
      }
   }

   std::span<uint8_t> buffer_;

   // used memory slots
   std::vector<Slot> occupied_slots_;
   std::vector<Slot> free_slots_;

   Stats stats_{};
};

}
