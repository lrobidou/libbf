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

std::string getUUID(std::string filename, std::size_t length) {
  std::ifstream fin(filename, std::ios::out | std::ofstream::binary);
  char ct;
  std::string uuid = "";
  for (std::size_t i = 0; i < length; i++) {
    fin.read((char*)&ct, sizeof(unsigned char));
    uuid += ct;
  }
  return uuid;
}

void skipChar(std::ifstream& fin, std::size_t length) {
  char ct;
  std::string uuid = "";
  for (std::size_t i = 0; i < length; i++) {
    fin.read((char*)&ct, sizeof(unsigned char));
  }
}

std::vector<bool> loadBoolVectorFromDisk(std::ifstream& fin) {
  std::vector<bool> v;
  std::vector<bool>::size_type n;
  fin.read((char*)&n, sizeof(std::vector<bool>::size_type));
  v.resize(n);
  for (std::vector<bool>::size_type i = 0; i < n;) {
    unsigned char aggr;
    fin.read((char*)&aggr, sizeof(unsigned char));
    for (unsigned char mask = 1; mask > 0 && i < n; ++i, mask <<= 1)
      v.at(i) = aggr & mask;
  }
  return v;
}

}  // namespace hidden_bf

namespace bf {
basic_bloom_filter make_filter(double fp, size_t capacity) {
  size_t required_cells = basic_bloom_filter::m(fp, capacity);
  size_t optimal_k = basic_bloom_filter::k(required_cells, capacity);
  return basic_bloom_filter(optimal_k, required_cells);
}

basic_bloom_filter* make_filter_ptr(double fp, size_t capacity) {
  size_t required_cells = basic_bloom_filter::m(fp, capacity);
  size_t optimal_k = basic_bloom_filter::k(required_cells, capacity);
  return new basic_bloom_filter(optimal_k, required_cells);
}

size_t basic_bloom_filter::m(double fp, size_t capacity) {
  auto ln2 = std::log(2);
  return std::ceil(-(capacity * std::log(fp) / ln2 / ln2));
}

size_t basic_bloom_filter::k(size_t cells, size_t capacity) {
  auto frac = static_cast<double>(cells) / static_cast<double>(capacity);
  return std::ceil(frac * std::log(2));
}

basic_bloom_filter::basic_bloom_filter(size_t numberOfHashFunctions, size_t cells, bool partition)
    : hasher_(make_hasher(numberOfHashFunctions)), bits_(cells), partition_(partition) {
  numberOfHashFunctions_ = numberOfHashFunctions;
}

basic_bloom_filter::basic_bloom_filter(std::string filename,
                                       bool& hasKzandcanonicalvalues,
                                       unsigned long long& K,
                                       unsigned long long& z,
                                       bool& canonical,
                                       bool partition) {
  partition_ = partition;

  if (!hidden_bf::file_exists(filename)) {
    std::cerr << "The filename "
              << filename
              << " that is supposed to be used to construct the Bloom filter"
              << " does not exist."
              << std::endl;
    exit(1);
  }

  std::size_t sizeOfUuid = uuid_2_0_0.length();
  std::string uuid = hidden_bf::getUUID(filename, sizeOfUuid);
  std::ifstream fin(filename, std::ios::out | std::ofstream::binary);
  if (uuid == uuid_3_0_0) {
    hidden_bf::skipChar(fin, sizeOfUuid);                                                        // skip first char
    fin.read(reinterpret_cast<char*>(&K), sizeof(K));                                            // read K
    fin.read(reinterpret_cast<char*>(&z), sizeof(z));                                            // read z
    fin.read(reinterpret_cast<char*>(&canonical), sizeof(canonical));                            // read canonical
    fin.read(reinterpret_cast<char*>(&numberOfHashFunctions_), sizeof(numberOfHashFunctions_));  // read canonical
    bits_ = hidden_bf::loadBoolVectorFromDisk(fin);
  } else if (uuid == uuid_2_0_0) {
    hidden_bf::skipChar(fin, sizeOfUuid);
    fin.read(reinterpret_cast<char*>(&K), sizeof(K));                  // read K
    fin.read(reinterpret_cast<char*>(&z), sizeof(z));                  // read z
    fin.read(reinterpret_cast<char*>(&canonical), sizeof(canonical));  // read canonical
    numberOfHashFunctions_ = 1;
    bits_ = hidden_bf::loadBoolVectorFromDisk(fin);
  } else {
    hasKzandcanonicalvalues = false;
    K = 0;
    z = 0;
    canonical = false;
    numberOfHashFunctions_ = 1;
    bits_ = hidden_bf::loadBoolVectorFromDisk(fin);
  }
  hasher_ = make_hasher(numberOfHashFunctions_);
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

size_t basic_bloom_filter::getNumberOfHashFunctions() const {
  return numberOfHashFunctions_;
}

void basic_bloom_filter::save(const std::string& filename,
                              const unsigned long long& K,
                              const unsigned long long& z,
                              const bool& canonical) {
  std::ofstream fout(filename, std::ios::out | std::ofstream::binary);
  // write the UUID
  for (char& c : uuid_3_0_0) {
    fout.write((const char*)&c, sizeof(unsigned char));
  }
  // write k
  fout.write(reinterpret_cast<const char*>(&K), sizeof(K));
  // write z
  fout.write(reinterpret_cast<const char*>(&z), sizeof(z));
  // write canonical
  fout.write(reinterpret_cast<const char*>(&canonical), sizeof(canonical));
  // TODO write the number of hash function
  fout.write(reinterpret_cast<const char*>(&numberOfHashFunctions_), sizeof(numberOfHashFunctions_));
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
}  // namespace bf
