#include "bf/all.hpp"
#include "test.hpp"

using namespace bf;

TEST(bloom_filter_basic) {
  const size_t numberOfHashFunctions = 3;
  basic_bloom_filter bf(numberOfHashFunctions, 10000);
  bf.add("foo");
  bf.add("bar");
  bf.add("baz");
  bf.add('c');
  bf.add(4.2);
  bf.add(4711ULL);
  // True-positives
  CHECK_EQUAL(bf.lookup("foo"), 1u);
  CHECK_EQUAL(bf.lookup("bar"), 1u);
  CHECK_EQUAL(bf.lookup("baz"), 1u);
  CHECK_EQUAL(bf.lookup(4.2), 1u);
  CHECK_EQUAL(bf.lookup('c'), 1u);
  CHECK_EQUAL(bf.lookup(4711ULL), 1u);
  // OK
  // True-negatives
  CHECK_EQUAL(bf.lookup("qux"), 0u);
  CHECK_EQUAL(bf.lookup("graunt"), 0u);
  CHECK_EQUAL(bf.lookup(3.1415), 0u);
  // False-positives
  // CHECK_EQUAL(bf.lookup("corge"), 1u);
  // CHECK_EQUAL(bf.lookup('a'), 1u);

  std::string filemane = "test.bin";
  const unsigned long long K = 31;
  const unsigned long long z = 3;
  const bool canonical = false;

  unsigned long long Kinfilter;
  unsigned long long zinfilter;
  bool canonicalinfilter;
  bf.save(filemane, K, z, canonical);
  bool hasKandzvalue;
  basic_bloom_filter loaded(filemane, hasKandzvalue, Kinfilter, zinfilter, canonicalinfilter);
  CHECK_EQUAL(K, Kinfilter);
  CHECK_EQUAL(z, zinfilter);
  CHECK_EQUAL(numberOfHashFunctions, loaded.getNumberOfHashFunctions());
  CHECK_EQUAL(canonical, canonicalinfilter);
  CHECK_EQUAL(hasKandzvalue, true);
  CHECK_EQUAL(loaded.storage() == bf.storage(), true);  //TODO: why don't this work with true sometimes ?
}
