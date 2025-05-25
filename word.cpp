#include "word.h"

#include <ostream>

word::Word::Word(std::vector<code_point>&& word): word_(std::move(word)) {}
word::Word::Word() {}

[[nodiscard]] std::string word::Word::to_utf8() const {
  std::string result;

  for (const auto& code_point : word_) {
    if (code_point <= 0x7F) {  // 1 байт (ASCII)
      result += static_cast<char>(code_point);
    }
    else if (code_point <= 0x7FF) {  // 2 байта
      result += static_cast<char>(0xC0 | (code_point >> 6));
      result += static_cast<char>(0x80 | (code_point & 0x3F));
    }
    else if (code_point <= 0xFFFF) {  // 3 байта
      result += static_cast<char>(0xE0 | (code_point >> 12));
      result += static_cast<char>(0x80 | ((code_point >> 6) & 0x3F));
      result += static_cast<char>(0x80 | (code_point & 0x3F));
    }
    else if (code_point <= 0x10FFFF) {  // 4 байта
      result += static_cast<char>(0xF0 | (code_point >> 18));
      result += static_cast<char>(0x80 | ((code_point >> 12) & 0x3F));
      result += static_cast<char>(0x80 | ((code_point >> 6) & 0x3F));
      result += static_cast<char>(0x80 | (code_point & 0x3F));
    }
  }

  return result;
}

std::ostream& word::operator<<(std::ostream& os, const word::Word& word) {
  auto result = word.to_utf8();
  std::ranges::copy(result, std::ostreambuf_iterator(os));
  return os;
}

[[nodiscard]] bool word::Word::empty() const noexcept { return word_.empty(); }

[[nodiscard]] size_t word::Word::size() const noexcept { return word_.size(); }