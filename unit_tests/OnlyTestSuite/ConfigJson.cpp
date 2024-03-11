#include "gtest/gtest.h"

#include "internal/ConfigJson.hpp"

namespace ModbusTechnologyAdapterTests::ConfigJsonTests {

using namespace Technology_Adapter::Modbus::Config;

struct FloatDecodePoint {
  std::vector<uint16_t> register_values;
  double expected_outcome;
};

struct ConfigJsonTests : public testing::Test {
  static void checkDecoder(
      json const& json, std::vector<FloatDecodePoint> const& points_to_check) {

    auto typed_decoder = DecoderOfJson(json);
    EXPECT_EQ(typed_decoder.return_type, Information_Model::DataType::DOUBLE);
    for (auto const& point : points_to_check) {
      auto outcome =
          std::get<double>(typed_decoder.decoder(point.register_values));
      EXPECT_EQ(outcome, point.expected_outcome);
    }
  }
};

// NOLINTBEGIN(readability-magic-numbers)

TEST_F(ConfigJsonTests, unsignedLinearDecoder) {
  checkDecoder(
      {
          {"type", "linear"},
          {"signed", false},
          {"factor", 3},
          {"offset", 2.5},
      },
      {
          {{}, 2.5},
          {{0}, 2.5},
          {{1}, 5.5},
          {{65535}, 196607.5},
          {{0, 1}, 196610.5},
          {{65535, 65535}, 12884901887.5},
          {{0, 0, 1}, 12884901890.5},
      });
}

TEST_F(ConfigJsonTests, signedLinearDecoder) {
  checkDecoder(
      {
          {"type", "linear"},
          {"signed", true},
          {"factor", 3},
          {"offset", 2.5},
      },
      {
          {{}, 2.5},
          {{0}, 2.5},
          {{1}, 5.5},
          {{65535}, -0.5},
          {{65535, 0}, 196607.5},
          {{0, 1}, 196610.5},
          {{65535, 65535}, -0.5},
          {{0, 0, 1}, 12884901890.5},
      });
}

TEST_F(ConfigJsonTests, defaultLinearDecoder) {
  checkDecoder(
      {
          {"type", "linear"},
      },
      {
          {{}, 0},
          {{0}, 0},
          {{1}, 1},
          {{65535}, 65535},
          {{0, 1}, 65536},
          {{65535, 65535}, 4294967295},
          {{0, 0, 1}, 4294967296},
      });
}

TEST_F(ConfigJsonTests, floatDecoder) {
  checkDecoder(
      {
          {"type", "float"},
      },
      {
          {{0, 0x4228}, 42},
          {{0xb438, 0x4996}, 1234567},
          {{0xb43f, 0xc996}, -1234567.875},
          {{0xaaab, 0x3eaa}, ((float)1.0) / 3}, // `b` because it is rounded up
          {{0, 0, 0, 0x4045}, 42},
          {{0x0400, 0xe1f6, 0xfee0, 0xc206}, -12345678910.751953125},
          {{0x5555, 0x5555, 0x5555, 0x3fd5}, 1.0 / 3},
      });
}

TEST_F(ConfigJsonTests, unsignedMantissaExponentDecoder) {
  checkDecoder(
      {
          {"type", "mantissa/exponent"},
          {"base", 10},
      },
      {
          {{0}, 0},
          {{42}, 0},
          {{0, 3}, 3},
          {{1, 3}, 30},
          {{65535, 30}, 3},
          {{1, 3, 0}, 30},
          {{1, 0, 1}, 655360},
          {{3, 2, 1}, 65538000},
      });
}

TEST_F(ConfigJsonTests, signedMantissaExponentDecoder) {
  checkDecoder(
      {
          {"type", "mantissa/exponent"},
          {"base", 0.5},
          {"signed", true}
      },
      {
          {{0}, 0},
          {{42}, 0},
          {{0, 3}, 3},
          {{1, 3}, 1.5},
          {{65535, 3}, 6},
          {{1, 3, 0}, 1.5},
          {{1, 0, 1}, 32768},
          {{3, 2, 1}, 8192.25},
          {{1, 65535}, -0.5},
          {{1, 65535, 0}, 32767.5},
      });
}

// NOLINTEND(readability-magic-numbers)

} // namespace ModbusTechnologyAdapterTests::ConfigJsonTests
