#include "test.hpp"

#include "bf/all.hpp"

using namespace bf;

TEST(bloom_filter_basic) {
  basic_bloom_filter bf(0.8, 10000);
  bf.add("foo");
  bf.add("bar");
  bf.add("baz");
  bf.add('c');
  bf.add(4.2);
  bf.add(4711ULL);
  std::cout << bf.storage().size() << std::endl;
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

  // another filter
  basic_bloom_filter obf(0.8, 100);
  // obf.swap(bf);

  // CHECK_EQUAL(obf.lookup("foo"), 1u);

  // Make bf using another filter's storage
  hasher h = obf.hasher_function();
  std::vector<bool> b = obf.storage();
  basic_bloom_filter obfc(h, b);
  CHECK_EQUAL(obfc.storage() == b, true);
  // CHECK_EQUAL(obfc.lookup("foo"), 1u);

  std::string filemane = "test.bin";
  const unsigned long long K = 31;
  const unsigned long long z = 3;
  const bool canonical = false;

  unsigned long long Kinfilter;
  unsigned long long zinfilter;
  bool canonicalinfilter;
  bf.save(filemane, K, z, canonical);
  bool hasKandzvalue;
  basic_bloom_filter loaded(make_hasher(1), filemane, hasKandzvalue, Kinfilter,
                            zinfilter, canonicalinfilter);
  CHECK_EQUAL(K, Kinfilter);
  CHECK_EQUAL(z, zinfilter);
  CHECK_EQUAL(canonical, canonicalinfilter);
  CHECK_EQUAL(hasKandzvalue, true);

  CHECK_EQUAL(loaded.storage() == bf.storage(), true);
}

// TEST(bloom_filter_spectral_rm) {
//   auto h1 = make_hasher(3, 0);
//   auto h2 = make_hasher(3, 1);
//   spectral_rm_bloom_filter bf(std::move(h1), 5, 2, std::move(h2), 4, 2);
//   bf.add("foo");
//   CHECK_EQUAL(bf.lookup("foo"), 1u);
//   // TODO: port old unit tests and double-check the implementation.

//   //// For "bar", all hash functions return the same position, the we have
//   //// necessarily a recurring minimum (RM). Thus we do not look in the
//   second
//   //// core and return 2, although the correct count would be 1.
//   // b.add("bar"); // 2 0 1 0 0 and 0 0
//   // CHECK(b.count("bar") == 2);
//   // CHECK(to_string(b) == "010000100000000\n0000");
//   //// For "foo", we encounter a unique minimum in the first core, but since
//   //// all positions for "foo" are zero in the second core, we return the
//   //// mimimum of the first, which is 1.
//   // CHECK(b.count("foo") == 1);
//   //// After increasing the counters for "foo", we find that it (still) has a
//   //// unique minimum in in the first core. Hence we add its minimum to the
//   //// second core.
//   // b.add("foo"); // 3 0 2 0 0 and 2 2
//   // CHECK(b.count("foo") == 2);
//   // CHECK(to_string(b) == "110000010000000\n0101");
//   //// The "blue fish" causes some trouble: because its insertion yields a
//   //// unique minimum, we go into the second bitvector. There, we find that
//   it
//   //// hashes to the same positions as foo, wich has a counter of 2. Because
//   it
//   //// appears to exist there, we have to increment its counters. This
//   falsely
//   //// bumps up the counter of "blue fish" to 3.
//   // b.add("blue fish"); // 3 0 3 0 1 and 3 3
//   // CHECK(b.count("blue fish") == 3);
//   // CHECK(to_string(b) == "110000110000100\n1111");
//   //// Since the "blue fish" has (still) a unique minimum after removing it
//   one
//   //// time, we look in the second core and find it to be present there.
//   //// Hence we decrement the counters in the second core.
//   // b.remove("blue fish"); // 3 0 2 0 0 and 2 2
//   // CHECK(b.count("blue fish") == 2);
//   // CHECK(to_string(b) == "110000010000000\n0101");
//   // b.remove("blue fish");
//   // CHECK(b.count("blue fish") == 1); // 3 0 1 0 0 and 1 1
//   //// Let's look at "foo". This fellow has now a unique minimum. Since it
//   has
//   //// a unique minimum after the removal, we also decrement the counter in
//   the
//   //// second core.
//   // b.remove("foo"); // 2 0 0 0 0 and 0 0
//   // CHECK(b.count("foo") == 0);
//   // CHECK(to_string(b) == "010000000000000\n0000");
//   //// Alas, we violated Claim 1 in Section 2.2 in the paper! The spectral
//   //// Bloom filter returns a count of 0 for "foo", although it should be 1.
//   //// Thus, the frequency estimate is no longer a lower bound. This occurs
//   //// presumably due to the fact that we remove "blue fish" twice although
//   we
//   //// added it only once.
// }
