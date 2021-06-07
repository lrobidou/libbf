#include <bf/bloom_filter/basic.hpp>

#include <cassert>
#include <cmath>
#include <fstream>
#include <iostream>

namespace hidden_bf {

inline bool file_exists(const std::string& name) {
  std::ifstream f(name.c_str());
  return f.good();
}

int getVersionOfIndex(std::string filename) {
  std::string uuid1 = "93d4c313-eed5-434e-bddd-34bd2ba23a12";
  std::ifstream fin(filename, std::ios::out | std::ofstream::binary);
  char ct;
  for (char& c : uuid1) {
    fin.read((char*)&ct, sizeof(unsigned char));
    if (c != ct) {
      return 0; // not our UUID, so the version of this index is 0
    }
  }
  return 1; // we have our UUID at the beginning of the file
}

std::vector<bool> loadBoolVectorFromDisk1(std::string filename,
                                          unsigned long long& K,
                                          unsigned long long& z,
                                          bool& canonical) {
  if (!file_exists(filename)) {
    std::cerr << "The filename " << filename
              << " that is supposed to be used to construct the Bloom filter "
                 "does not exist."
              << std::endl;
    exit(1);
  }
  std::ifstream fin(filename, std::ios::out | std::ofstream::binary);

  // read UUID
  std::string uuid1 = "93d4c313-eed5-434e-bddd-34bd2ba23a12";
  char ct;
  for (char& c : uuid1) {
    fin.read((char*)&ct, sizeof(unsigned char));
    if (c != ct) {
      std::cerr << "Tried to use loadBoolVectorFromDisk1 on an index that is "
                   "not of version 1."
                << std::endl;
      exit(1);
    }
  }
  // read K
  fin.read(reinterpret_cast<char*>(&K), sizeof(K));
  // read z
  fin.read(reinterpret_cast<char*>(&z), sizeof(z));
  // read canonical
  fin.read(reinterpret_cast<char*>(&canonical), sizeof(canonical));
  // read the vector
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

std::vector<bool> loadBoolVectorFromDisk(std::string filename) {
  if (!file_exists(filename)) {
    std::cerr << "The filename " << filename
              << " that is supposed to be used to construct the Bloom filter "
                 "does not exist."
              << std::endl;
    exit(1);
  }
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

basic_bloom_filter::basic_bloom_filter(hasher h, std::string filename,
                                       bool& hasKzandcanonicalvalues,
                                       unsigned long long& K,
                                       unsigned long long& z, bool& canonical)
    : hasher_(std::move(h)) {

  if (hidden_bf::getVersionOfIndex(filename) == 0) {
    bits_ = hidden_bf::loadBoolVectorFromDisk(filename);
    K = 0;
    z = 0;
    canonical = false;
    hasKzandcanonicalvalues = false;
  } else {
    hasKzandcanonicalvalues = true;
    bits_ = hidden_bf::loadBoolVectorFromDisk1(filename, K, z, canonical);
  }
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

void basic_bloom_filter::save(const std::string& filename,
                              const unsigned long long& K,
                              const unsigned long long& z,
                              const bool& canonical) {
  std::ofstream fout(filename, std::ios::out | std::ofstream::binary);
  // write the UUID
  std::string myuuid = "93d4c313-eed5-434e-bddd-34bd2ba23a12";
  for (char& c : myuuid) {
    fout.write((const char*)&c, sizeof(unsigned char));
  }
  // write k
  fout.write(reinterpret_cast<const char*>(&K), sizeof(unsigned long long));
  // write z
  fout.write(reinterpret_cast<const char*>(&z), sizeof(unsigned long long));
  // write canonical
  fout.write(reinterpret_cast<const char*>(&canonical), sizeof(bool));
  // write the vector
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
