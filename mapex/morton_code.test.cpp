#include <QtTest/QtTest>

#include <mapex/morton_code.hpp>

namespace {

static_assert(morton::detail::load(morton::detail::load_masks[0], 2, 0b0000) == 0b0100,
    "load('100...', val) should set value when all bits are zero");
static_assert(morton::detail::load(morton::detail::load_masks[1], 5, 0b000000) == 0b001010,
    "load('011...', val) should set value when all bits are zero");
static_assert(morton::detail::load(morton::detail::load_masks[0], 3, 0b1010) == 0b1000,
    "load('100...', val) should set value when all bits are one");
static_assert(morton::detail::load(morton::detail::load_masks[1], 4, 0b010101) == 0b000101,
    "load('011...', val) should set value when all bits are one");
static_assert(morton::detail::load(morton::detail::load_masks[0], 1, 0b1101) == 0b0111,
    "load('100...', val) do not affect untoched axis");
static_assert(morton::detail::load(morton::detail::load_masks[1], 0, 0b111011) == 0b101010,
    "load('011...', val) do not affect untoched axis");

static_assert(
    morton::bigmin(0, 0, 0) == 0, "bigmin should work on single point rect in the top left of coordinate space");
static_assert(morton::bigmin(0, ~uint64_t{0}, ~uint64_t{0}) == ~uint64_t{0},
    "bigmin should work on single point rect in the bottom right of coordinate space");
static_assert(morton::bigmin(0, morton::code({1, 1}), morton::code({3, 5})) == morton::code({1, 1}),
    "bigmin should return top left of the rect for point with code below code of the rect top left");

constexpr bool is_in_rect(point pt, point rect_min, point rect_max) noexcept {
  return pt.x >= rect_min.x && pt.y >= rect_min.y && pt.x <= rect_max.x && pt.y <= rect_max.y;
}

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

  void random_rects_with_div_points(unsigned count) {
    QTest::addColumn<uint64_t>("div_pt");
    QTest::addColumn<uint64_t>("min");
    QTest::addColumn<uint64_t>("max");

    std::uniform_int_distribution<uint32_t> coord_dist;
    std::uniform_int_distribution<uint64_t> code_dist;
    for (unsigned i = 0; i < count; ++i) {
      const point min{coord_dist(rnd_engine_), coord_dist(rnd_engine_)};
      point max;
      coord_dist.param(std::uniform_int_distribution<uint32_t>::param_type{min.x});
      max.x = coord_dist(rnd_engine_);
      coord_dist.param(std::uniform_int_distribution<uint32_t>::param_type{min.y});
      max.y = coord_dist(rnd_engine_);
      code_dist.param(std::uniform_int_distribution<uint64_t>::param_type{morton::code(min), morton::code(max)});
      const uint64_t div_pt = code_dist(rnd_engine_);
      QTest::addRow("bigmin(%llX, %llX, %llX)", static_cast<unsigned long long>(div_pt),
          static_cast<unsigned long long>(morton::code(min)), static_cast<unsigned long long>(morton::code(max)))
          << div_pt << morton::code(min) << morton::code(max);
      coord_dist.param(std::uniform_int_distribution<uint32_t>::param_type{});
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
      QTest::addRow("{x: %llu, y: %llu}", static_cast<unsigned long long>(x), static_cast<unsigned long long>(y))
          << x << y;
    }
  }
  void encode_and_decode_point_is_identity() {
    QFETCH(uint32_t, x);
    QFETCH(uint32_t, y);
    QCOMPARE(morton::decode(morton::code({x, y})), (point{x, y}));
  }

  void groups_of_4_bits_interleaved_correctly_data() { all_4bit_groups(); }
  void groups_of_4_bits_interleaved_correctly() {
    QFETCH(uint32_t, group_pos);
    QFETCH(size_t, group_item);
    QCOMPARE(morton::interleave(values[group_item] << (group_sz * group_pos)),
        interleaved[group_item] << (2 * group_sz * group_pos));
  }

  void groups_of_4_bits_deinterleaved_correctly_data() { all_4bit_groups(); }
  void groups_of_4_bits_deinterleaved_correctly() {
    QFETCH(uint32_t, group_pos);
    QFETCH(size_t, group_item);
    QCOMPARE(morton::deinterleave(interleaved[group_item] << (2 * group_sz * group_pos)),
        values[group_item] << (group_sz * group_pos));
  }

  void bigmin_result_is_inside_the_rect_data() { random_rects_with_div_points(100); }
  void bigmin_result_is_inside_the_rect() {
    QFETCH(uint64_t, div_pt);
    QFETCH(uint64_t, min);
    QFETCH(uint64_t, max);
    const point bigmin = morton::decode(morton::bigmin(div_pt, min, max));
    QVERIFY(is_in_rect(bigmin, morton::decode(min), morton::decode(max)));
  }

  void no_rect_points_between_div_pt_and_bigmin_data() { random_rects_with_div_points(10); }
  void no_rect_points_between_div_pt_and_bigmin() {
    QFETCH(uint64_t, div_pt);
    QFETCH(uint64_t, min);
    QFETCH(uint64_t, max);
    const uint64_t bigmin = morton::bigmin(div_pt, min, max);
    for (uint64_t pt = div_pt; pt < bigmin; ++pt)
      QVERIFY(!is_in_rect(morton::decode(pt), morton::decode(min), morton::decode(max)));
  }

private:
  std::default_random_engine rnd_engine_;
  const point rect_min_ = {100, 500};
  const point rect_max_ = {rect_min_.x + 640, rect_min_.y + 480};
};

QTEST_MAIN(morton_code_tests)
#include "morton_code.test.moc"
