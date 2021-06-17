#ifndef BF_BLOOM_FILTER_BASIC_HPP
#define BF_BLOOM_FILTER_BASIC_HPP

#include <bf/bloom_filter.hpp>
#include <bf/hash.hpp>
#include <random>

namespace bf {
/// The basic Bloom filter.
///
/// @note This Bloom filter does not use partitioning because it results in
/// slightly worse performance because partitioned Bloom filters tend to have
/// more 1s than non-partitioned filters.
class basic_bloom_filter : public bloom_filter {
 public:
  static size_t m(double fp, size_t capacity);

  static size_t k(size_t cells, size_t capacity);

  basic_bloom_filter(size_t numberOfHashFunctions, size_t cells, bool partition = false);

  basic_bloom_filter(std::string filename,
                     bool& hasKzandcanonicalvalues,
                     unsigned long long& K,
                     unsigned long long& z,
                     bool& canonical,
                     bool partition = false);

  basic_bloom_filter(basic_bloom_filter&&);

  using bloom_filter::add;
  using bloom_filter::lookup;

  virtual void add(object const& o) override;
  virtual size_t lookup(object const& o) const override;

  /// Swaps two basic Bloom filters.
  /// @param other The other basic Bloom filter.
  void swap(basic_bloom_filter& other);

  /// Returns the underlying storage of the Bloom filter.
  std::vector<bool> const& storage() const;

  /// Returns the hasher of the Bloom filter.
  hasher const& hasher_function() const;

  size_t getNumberOfHashFunctions() const;

  /// Saves the Bloom filter in a file named filename.
  void save(const std::string& filename, const unsigned long long& K,
            const unsigned long long& z, const bool& canonical);

 private:
  void writeUUID(std::ofstream& fout);
  hasher hasher_;
  std::vector<bool> bits_;
  bool partition_;
  std::string uuid_2_0_0 = "93d4c313-eed5-434e-bddd-34bd2ba23a12";
  std::string uuid_3_0_0 = "c625b08b-0a6c-4fda-82b6-2e213f4c04f1";
  size_t numberOfHashFunctions_ = 1;
};

basic_bloom_filter make_filter(double fp, size_t capacity);
basic_bloom_filter* make_filter_ptr(double fp, size_t capacity);
}  // namespace bf

#endif
