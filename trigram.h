#pragma once

#include <string_view>

namespace trigram {
  struct TextVector {};
  TextVector generate_trigrams(std::string_view text);
}