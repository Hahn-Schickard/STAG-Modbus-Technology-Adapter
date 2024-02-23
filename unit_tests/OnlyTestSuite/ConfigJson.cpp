#include "gtest/gtest.h"

#include "internal/ConfigJson.hpp"

namespace ModbusTechnologyAdapterTests::ConfigJsonTests {

using namespace Technology_Adapter::Modbus::Config;

struct FloatDecodePoint {
  std::vector<uint16_t> register_values;
  double expected_outcome;
};

struct ConfigJsonTests : public testing::Test {
  void checkDecoder(json const& json,
      std::vector<FloatDecodePoint> points_to_check) {

    auto typed_decoder = DecoderOfJson(json);
    EXPECT_EQ(typed_decoder.return_type, Information_Model::DataType::DOUBLE);
    for (auto const& point : points_to_check) {
      auto outcome =
        std::get<double>(typed_decoder.decoder(point.register_values));
      EXPECT_EQ(outcome, point.expected_outcome);
    }
  }
};

TEST_F(ConfigJsonTests, linearDecoder) {
  checkDecoder(
    {
      {"type","linear"},
      {"factor",3},
      {"offset",2.5},
    },
    {
      {{}, 2.5},
      {{0}, 2.5},
      {{1}, 5.5},
      {{65535}, 196607.5},
      {{0,1}, 196610.5},
      {{65535,65535}, 12884901887.5},
      {{0,0,1}, 12884901890.5},
    });
}

} // namespace ModbusTechnologyAdapterTests::ConfigJsonTests
