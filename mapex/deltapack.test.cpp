#include <cstddef>
#include <random>
#include <set>
#include <vector>

#include <QtTest/QtTest>

#include <mapex/deltapack.hpp>

namespace {

const std::pair<uint64_t, size_t> middle_of_same_bits_range[] = {
    {42, 1},
    {437, 2},
    {0x5555, 3},
    {0x8555ab, 4},
    {0x8555ab44, 5},
    {0xa555ab44fe, 6},
    {0xa555ab44fe34, 7},
    {0xa555ab44fe345, 8},
    {0xa555ab44fe345c3, 9},
    {0xfff45fafffefffff, 10},
};

const std::pair<uint64_t, size_t> smallest_of_same_bits_range[] = {
    {0x0, 1},
    {0x80, 2},
    {0x4000, 3},
    {0x200000, 4},
    {0x10000000, 5},
    {0x800000000, 6},
    {0x40000000000, 7},
    {0x2000000000000, 8},
    {0x100000000000000, 9},
    {0x8000000000000000, 10},
};

const std::pair<uint64_t, size_t> greatest_of_same_bits_range[] = {
    {0x7f, 1},
    {0x3fff, 2},
    {0x1fffff, 3},
    {0xfffffff, 4},
    {0x7ffffffff, 5},
    {0x3ffffffffff, 6},
    {0x1ffffffffffff, 7},
    {0xffffffffffffff, 8},
    {0x7fffffffffffffff, 9},
    {0xffffffffffffffff, 10},
};

} // namespace

class deltapack_tests : public QObject {
  Q_OBJECT
private:
  void prepare_same_bits_range_dataset() {
    QTest::addColumn<uint64_t>("val");
    QTest::addColumn<size_t>("encoded_sz");

    for (const auto& [val, sz] : middle_of_same_bits_range)
      QTest::addRow("middle_of_same_bits_range/%d", static_cast<int>(sz)) << val << sz;
    for (const auto& [val, sz] : smallest_of_same_bits_range)
      QTest::addRow("smallest_of_same_bits_range/%d", static_cast<int>(sz)) << val << sz;
    for (const auto& [val, sz] : greatest_of_same_bits_range)
      QTest::addRow("greatest_of_same_bits_range/%d", static_cast<int>(sz)) << val << sz;
  }

  template <template <class...> class Collection>
  Collection<uint64_t> gen_random_sample(size_t sample_size) {
    std::uniform_int_distribution<uint64_t> dist;
    Collection<uint64_t> res;
    std::generate_n(std::inserter(res, res.end()), sample_size, [&] { return dist(rnd_engine_); });
    return res;
  }

private slots:
  void cleanup() {
    input_.clear();
    encoded_.clear();
    decoded_.clear();
  }

  void varint_encodes_value_with_proper_number_of_bytes_data() { prepare_same_bits_range_dataset(); }
  void varint_encodes_value_with_proper_number_of_bytes() {
    QFETCH(uint64_t, val);
    QFETCH(size_t, encoded_sz);
    input_ = {val};
    varint::pack(std::begin(input_), std::end(input_), std::back_inserter(encoded_));
    QCOMPARE(encoded_.size(), encoded_sz);
  }

  void varint_decodes_encoded_value_back_data() { prepare_same_bits_range_dataset(); }
  void varint_decodes_encoded_value_back() {
    QFETCH(uint64_t, val);
    input_ = {val};
    varint::pack(input_.begin(), input_.end(), std::back_inserter(encoded_));
    varint::unpack(encoded_.begin(), encoded_.end(), std::back_inserter(decoded_));
    QCOMPARE(input_, decoded_);
  }

  void varint_returns_number_of_encoded_bytes_data() { prepare_same_bits_range_dataset(); }
  void varint_returns_number_of_encoded_bytes() {
    QFETCH(uint64_t, val);
    input_ = {val};
    const size_t sz = varint::pack(input_.begin(), input_.end(), std::back_inserter(encoded_));
    QCOMPARE(sz, encoded_.size());
  }

  void varint_decodes_multiple_encoded_values_back() {
    input_ = gen_random_sample<std::vector>(0x10000);
    varint::pack(input_.begin(), input_.end(), std::back_inserter(encoded_));
    varint::unpack(encoded_.begin(), encoded_.end(), std::back_inserter(decoded_));
    QCOMPARE(decoded_, input_);
  }

  void varint_pack_multiple_returns_number_of_encoded_bytes() {
    input_ = gen_random_sample<std::vector>(0x10000);
    const size_t sz = varint::pack(input_.begin(), input_.end(), std::back_inserter(encoded_));
    QCOMPARE(sz, encoded_.size());
  }

  void unpack_packed_values_back() {
    std::multiset<uint64_t> input = gen_random_sample<std::multiset>(0x10000);
    std::vector<char> packed;
    std::multiset<uint64_t> restored;
    delta::pack(input.begin(), input.end(), std::back_inserter(packed));
    delta::unpack(packed.begin(), packed.end(), std::inserter(restored, restored.end()));
    QCOMPARE(input, restored);
  }

private:
  std::default_random_engine rnd_engine_;
  std::vector<uint64_t> input_;
  std::vector<char> encoded_;
  std::vector<uint64_t> decoded_;
};

QTEST_MAIN(deltapack_tests)
#include "deltapack.test.moc"
