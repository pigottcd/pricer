#ifndef POOL_ALLOCATOR_H
#define POOL_ALLOCATOR_H

#include <algorithm>
#include <cstddef>
#include <cstdlib>
#include <new>
#include <span>

template<typename T, std::size_t BlockSize, std::size_t BlockCount, std::size_t Alignment = alignof(T)>
class PoolAllocator
{
public:
  static_assert(BlockSize % Alignment == 0, "BlockSize must be a multiple of Alignment");

  using value_type = T;

  template<typename U>
  struct rebind
  {
    using other = PoolAllocator<U, BlockSize, BlockCount, Alignment>;
  };

  PoolAllocator() = default;


private:
  struct Pool
  {
    Pool()
    {
      constexpr auto size = BlockSize * BlockCount;
      auto* data = static_cast<std::byte*>(std::aligned_alloc(Alignment, size));
      if (data == nullptr) { throw std::bad_alloc(); }
      data_ = std::span<std::byte>(data, size);

      constexpr auto ledgerSize = (BlockCount + 7) / 8;
      auto* ledgerData = static_cast<std::byte*>(std::malloc(ledgerSize));
      if (ledgerData == nullptr) {
        std::free(data_.data());
        throw std::bad_alloc();
      }
      ledger_ = std::span<std::byte>(ledgerData, ledgerSize);
      std::ranges::fill(ledger_, std::byte{ 0 });
    }

    ~Pool()
    {
      std::free(data_.data());
      std::free(ledger_.data());
    }

    Pool(const Pool&) = delete;
    Pool(Pool&&) = delete;
    auto operator=(const Pool&) -> Pool& = delete;
    auto operator=(Pool&&) -> Pool& = delete;

    [[nodiscard]] auto allocate(std::size_t bytes) noexcept -> void*
    {
      const auto blocksNeeded = (bytes + BlockSize - 1) / BlockSize;

      auto index = findContiguousBlocks(blocksNeeded);
      if (index == BlockCount) { return nullptr; }

      setBlocksInUse(index, blocksNeeded);
      return &data_[index * BlockSize];
    }

    auto deallocate(T* p, std::size_t n) noexcept -> void
    {
      const auto* bytePtr = reinterpret_cast<const std::byte*>(p);
      const std::size_t offset = bytePtr - data_.data();
      const std::size_t index = offset / BlockSize;
      const std::size_t blocksNeeded = (n * sizeof(T) + BlockSize - 1) / BlockSize;
      setBlocksFree(index, blocksNeeded);
    }

    // Finds n free contiguous blocks in the ledger and returns the first block's index or BlockCount on failure
    [[nodiscard]] auto findContiguousBlocks(std::size_t n) const noexcept -> std::size_t
    {
      std::size_t contiguous = 0;
      for (std::size_t i = 0; i < BlockCount; ++i) {
        if (!isBlockInUse(i)) {
          if (++contiguous == n) { return i - n + 1; }
        } else {
          contiguous = 0;
        }
      }
      return BlockCount;
    }

    // Marks n blocks in the ledger as "in-use" starting at 'index'
    auto setBlocksInUse(std::size_t index, std::size_t n) noexcept -> void
    {
      for (std::size_t i = 0; i < n; ++i) {
        ledger_[(index + i) / 8] =
          static_cast<std::byte>(std::to_integer<unsigned char>(ledger_[(index + i) / 8]) | (1 << ((index + i) % 8)));
      }
    }

    // Marks n blocks in the ledger as "free" starting at 'index';
    auto setBlocksFree(std::size_t index, std::size_t n) noexcept -> void
    {
      for (std::size_t i = 0; i < n; ++i) {
        ledger_[(index + i) / 8] =
          static_cast<std::byte>(std::to_integer<unsigned char>(ledger_[(index + i) / 8]) & ~(1 << ((index + i) % 8)));
      }
    }

    [[nodiscard]] auto isBlockInUse(std::size_t index) const noexcept -> bool
    {
      return std::to_integer<unsigned char>(ledger_[index / 8]) & (1 << (index % 8));
    }

    std::span<std::byte> data_;
    std::span<std::byte> ledger_;
  };

  static auto getInstance() -> Pool&
  {
    static thread_local Pool pool;
    return pool;
  }

public:
  [[nodiscard]] auto allocate(std::size_t n) noexcept -> T*
  {
    const auto bytes = n * sizeof(T);
    void* ptr = getInstance().allocate(bytes);
    return reinterpret_cast<T*>(ptr);
  }

  auto deallocate(T* p, std::size_t n) noexcept -> void { getInstance().deallocate(reinterpret_cast<void*>(p), n); }
};

#endif
