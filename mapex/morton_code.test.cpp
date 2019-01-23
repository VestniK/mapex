#include <QtTest/QtTest>

#include <mapex/morton_code.hpp>

namespace {

// clang-format off
constexpr uint32_t values[] = {
    0b0000'0000, 0b0000'0001, 0b0000'0010, 0b0000'0011, 0b0000'0100, 0b0000'0101, 0b0000'0110, 0b0000'0111,
    0b0000'1000, 0b0000'1001, 0b0000'1010, 0b0000'1011, 0b0000'1100, 0b0000'1101, 0b0000'1110, 0b0000'1111
};
constexpr uint64_t interleaved[] = {
    0b0000'0000, 0b0000'0001, 0b0000'0100, 0b0000'0101, 0b0001'0000, 0b0001'0001, 0b0001'0100, 0b0001'0101,
    0b0100'0000, 0b0100'0001, 0b0100'0100, 0b0100'0101, 0b0101'0000, 0b0101'0001, 0b0101'0100, 0b0101'0101
};
// clang-format on
static_assert(std::size(values) == std::size(interleaved));
constexpr size_t group_sz = 4;

} // namespace

class morton_code_tests : public QObject {
  Q_OBJECT
private:
  void all_4bit_groups() {
    QTest::addColumn<uint32_t>("group_pos");
    QTest::addColumn<size_t>("group_item");

    for (uint32_t group_pos = 0; group_pos < 32 / group_sz; ++group_pos) {
      for (size_t pos = 0; pos < std::size(values); ++pos) {
        QTest::addRow("interleave %x", static_cast<unsigned>(values[pos] << (group_sz * group_pos)))
            << group_pos << pos;
      }
    }
  }

private slots:
  void encode_and_decode_point_is_identity_data() {
      QTest::addColumn<uint32_t>("x");
      QTest::addColumn<uint32_t>("y");

      std::uniform_int_distribution<uint32_t> dist;
      for (int i = 0; i < 100; ++i) {
        const uint32_t x = dist(rnd_engine_);
        const uint32_t y = dist(rnd_engine_);
        QTest::addRow("{x: %llu, x: %llu}", static_cast<unsigned long long>(x), static_cast<unsigned long long>(y)) << x << y;
      }
  }
  void encode_and_decode_point_is_identity() {
      QFETCH(uint32_t, x);
      QFETCH(uint32_t, y);
      QCOMPARE(from_morton(morton_code({x, y})), (point{x, y}));
  }

  void groups_of_4_bits_interleaved_correctly_data() { all_4bit_groups(); }
  void groups_of_4_bits_interleaved_correctly() {
    QFETCH(uint32_t, group_pos);
    QFETCH(size_t, group_item);
    QCOMPARE(interleave(values[group_item] << (group_sz * group_pos)),
        interleaved[group_item] << (2 * group_sz * group_pos));
  }

  void groups_of_4_bits_deinterleaved_correctly_data() { all_4bit_groups(); }
  void groups_of_4_bits_deinterleaved_correctly() {
    QFETCH(uint32_t, group_pos);
    QFETCH(size_t, group_item);
    QCOMPARE(deinterleave(interleaved[group_item] << (2 * group_sz * group_pos)),
        values[group_item] << (group_sz * group_pos));
  }

private:
  std::default_random_engine rnd_engine_;
};

QTEST_MAIN(morton_code_tests)
#include "morton_code.test.moc"
