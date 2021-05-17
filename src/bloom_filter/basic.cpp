#include <bf/bloom_filter/basic.hpp>

#include <cassert>
#include <cmath>
#include <fstream>
#include <iostream>

namespace hidden_bf {
std::vector<bool> loadBoolVectorFromDisk(std::string filename) {

  std::ifstream fin(filename, std::ios::out | std::ofstream::binary);

  std::vector<bool> x;
  std::vector<bool>::size_type n;
  fin.read((char*)&n, sizeof(std::vector<bool>::size_type));
  x.resize(n);
  for (std::vector<bool>::size_type i = 0; i < n;) {
    unsigned char aggr;
    fin.read((char*)&aggr, sizeof(unsigned char));
    for (unsigned char mask = 1; mask > 0 && i < n; ++i, mask <<= 1)
      x.at(i) = aggr & mask;
  }
  return x;
}

} // namespace hidden_bf

namespace bf {

size_t basic_bloom_filter::m(double fp, size_t capacity) {
  auto ln2 = std::log(2);
  return std::ceil(-(capacity * std::log(fp) / ln2 / ln2));
}

size_t basic_bloom_filter::k(size_t cells, size_t capacity) {
  auto frac = static_cast<double>(cells) / static_cast<double>(capacity);
  return std::ceil(frac * std::log(2));
}

basic_bloom_filter::basic_bloom_filter(hasher h, size_t cells, bool partition)
    : hasher_(std::move(h)), bits_(cells), partition_(partition) {
}

basic_bloom_filter::basic_bloom_filter(double fp, size_t capacity, size_t seed,
                                       bool double_hashing, bool partition)
    : partition_(partition) {
  auto required_cells = m(fp, capacity);
  auto optimal_k = k(required_cells, capacity);
  if (partition_)
    required_cells += optimal_k - required_cells % optimal_k;
  bits_.resize(required_cells, false);
  hasher_ = make_hasher(optimal_k, seed, double_hashing);
}

basic_bloom_filter::basic_bloom_filter(hasher h, std::vector<bool> b)
    : hasher_(std::move(h)), bits_(std::move(b)) {
}

basic_bloom_filter::basic_bloom_filter(hasher h, std::string filename)
    : hasher_(std::move(h)),
      bits_(hidden_bf::loadBoolVectorFromDisk(filename)) {
}

basic_bloom_filter::basic_bloom_filter(basic_bloom_filter&& other)
    : hasher_(std::move(other.hasher_)), bits_(std::move(other.bits_)) {
}

void basic_bloom_filter::add(object const& o) {
  auto digests = hasher_(o);
  if (partition_) {
    assert(bits_.size() % digests.size() == 0);

    auto parts = bits_.size() / digests.size();
    for (size_t i = 0; i < digests.size(); ++i) {
      bits_[i * parts + (digests[i] % parts)] = true;
    }

  } else {
    for (auto d : digests)
      bits_[d % bits_.size()] = true;
  }
}

size_t basic_bloom_filter::lookup(object const& o) const {
  auto digests = hasher_(o);
  if (partition_) {
    assert(bits_.size() % digests.size() == 0);
    auto parts = bits_.size() / digests.size();
    for (size_t i = 0; i < digests.size(); ++i) {
      if (!bits_[i * parts + (digests[i] % parts)])
        return 0;
    }

  } else {
    for (auto d : digests)
      if (!bits_[d % bits_.size()])
        return 0;
  }

  return 1;
}

void basic_bloom_filter::swap(basic_bloom_filter& other) {
  using std::swap;
  swap(hasher_, other.hasher_);
  swap(bits_, other.bits_);
}

std::vector<bool> const& basic_bloom_filter::storage() const {
  return bits_;
}
hasher const& basic_bloom_filter::hasher_function() const {
  return hasher_;
}

void basic_bloom_filter::save(std::string filename) {
  std::ofstream fout(filename, std::ios::out | std::ofstream::binary);
  std::vector<bool>::size_type n = bits_.size();
  fout.write((const char*)&n, sizeof(std::vector<bool>::size_type));
  for (std::vector<bool>::size_type i = 0; i < n;) {
    unsigned char aggr = 0;
    for (unsigned char mask = 1; mask > 0 && i < n; ++i, mask <<= 1)
      if (bits_.at(i))
        aggr |= mask;
    fout.write((const char*)&aggr, sizeof(unsigned char));
  }
  fout.flush();
  fout.close();
}

} // namespace bf
