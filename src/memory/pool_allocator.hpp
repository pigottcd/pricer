#ifndef POOL_ALLOCATOR_H
#define POOL_ALLOCATOR_H

#include <algorithm>
#include <bit>
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
    // not expecting to use maps or other containers that have different alignment requirements
    // but will provide a check for it.  If we hit this case, we will use the alignment of U
    static constexpr std::size_t NewAlign =
        (Alignment < alignof(U) ? alignof(U) : Alignment);
    using other = PoolAllocator<U, BlockSize, BlockCount, NewAlign>;
  };

  constexpr PoolAllocator() noexcept = default;

  template<class U, std::size_t A>
  constexpr PoolAllocator(const PoolAllocator<U, BlockSize, BlockCount, A>&) noexcept {}


private:
  struct Pool
  {
    Pool()
    {
      constexpr auto size = BlockSize * BlockCount;
#if defined(_WIN32)
      auto* data = static_cast<std::byte*>(_aligned_malloc(size, Alignment));
#else
      auto* data = static_cast<std::byte*>(std::aligned_alloc(Alignment, size));
#endif
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
#if defined(_WIN32)
      _aligned_free(data_.data());
#else
      std::free(data_.data());
#endif
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
      return std::bit_cast<void*>(&data_[index * BlockSize]);
    }

    auto deallocate(T* p, std::size_t n) noexcept -> void
    {
      const auto* bytePtr = std::bit_cast<const std::byte*>(p);
      const auto offset = static_cast<std::size_t>(bytePtr - data_.data());
      const auto index = offset / BlockSize;
      const auto blocksNeeded = (n * sizeof(T) + BlockSize - 1) / BlockSize;
      setBlocksFree(index, blocksNeeded);
    }

    // Finds n free contiguous blocks in the ledger and returns the first block's index or BlockCount on failure
    [[nodiscard]] auto findContiguousBlocks(std::size_t n) const noexcept -> std::size_t
    {
      auto contiguous = std::size_t{ 0 };
      for (std::size_t i = 0; i < BlockCount; ++i) {
        if (!isBlockInUse(i)) {
          if (++contiguous == n) { return i - n + 1; }
        } else {
          contiguous = 0;
        }
      }
      return BlockCount;
    }

    auto setBlocksInUse(std::size_t index, std::size_t n) noexcept -> void
    {
      for (auto i = std::size_t{ 0 }; i < n; ++i) {
        ledger_[(index + i) / 8] =
          static_cast<std::byte>(std::to_integer<unsigned char>(ledger_[(index + i) / 8]) | (1 << ((index + i) % 8)));
      }
    }

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

  auto deallocate(T* p, std::size_t n) noexcept -> void { getInstance().deallocate(p, n); }
};

#endif
