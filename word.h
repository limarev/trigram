#pragma once

#include <vector>

namespace word {
struct Word {
  using code_point = uint32_t;

  Word();
  Word(std::vector<code_point>&& word);

  auto operator<=>(const Word &) const = default;

  template<typename OutputIterator>
  [[nodiscard]] auto copy(OutputIterator result) const {
    return std::ranges::copy(word_, result);
  }

  [[nodiscard]] std::string to_utf8() const;
  [[nodiscard]] bool empty() const noexcept;
  [[nodiscard]] size_t size() const noexcept;
  // Доступ к code points
  [[nodiscard]] const code_point& operator[](size_t pos) const { return word_[pos]; }

private:
  alignas(64) std::vector<code_point> word_;
};

std::ostream& operator<<(std::ostream& os, const Word& word);

} // namespace word