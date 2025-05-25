#pragma once

// uu - Unicode Utilities
namespace uu {

using Byte = std::bitset<8>;

/**
 * Функция проверяет что первые старшие биты байта равны target
 *
 * @tparam N Количество первых старших бит (должно быть меньше 8)
 * @param byte Входной байт
 * @param target Референсное значение для проверки
 * @return True, если N старших бит равны target, иначе false
 */
template <size_t N>
constexpr bool СheckFirstNMostSignBits(Byte byte, unsigned long target) {
  static_assert(byte.size() > N);

  byte >>= byte.size() - N;
  return byte.to_ulong() == target;
}

/**
 *
 * @param byte Байт для проверки старших битов
 *
 * Функция проверяет старшие биты байта и определяет длину символа, закодированного в utf-8
 *
 * @note В UTF-8 длина символа определяется по старшим битам первого байта:
 *
 * 1. Одиночный байт (ASCII):\n
 *    Если первый бит 0, это ASCII-символ (1 байт).\n
 *    Формат: 0xxxxxxx\n
 *    Пример: A → 01000001 (0x41)\n
 *
 * 2. Многобайтовые символы:\n
 *    - 2 байта: Первые 3 бита 110\n
 *      Формат: 110xxxxx 10xxxxxx\n
 *      Пример: русская "р" → 11010000 10110000 (0xD0 0xB0)\n
 *    - 3 байта: Первые 4 бита 1110\n
 *      Формат: 1110xxxx 10xxxxxx 10xxxxxx\n
 *      Пример: "€" → 11100010 10000010 10101100 (0xE2 0x82 0xAC)\n
 *    - 4 байта: Первые 5 бита 11110\n
 *      Формат: 11110xxx 10xxxxxx 10xxxxxx 10xxxxxx\n
 *      Пример: "𐍈" → 11110000 10010000 10001101 10001000 (0xF0 0x90 0x8D 0x88)\n
 *
 * @return Длина символа, закодированного в utf-8
 */
__attribute__((always_inline)) inline short get_utf8_char_len1(Byte byte) {

  if (СheckFirstNMostSignBits<1>(byte, 0)) [[likely]] {  // 0xxxxxxx
    return 1;
  }
  if (СheckFirstNMostSignBits<3>(byte, 0b110)) [[likely]] {  // 110xxxxx
    return 2;
  }
  if (СheckFirstNMostSignBits<4>(byte, 0b1110)) {  // 1110xxxx
    return 3;
  }
  if (СheckFirstNMostSignBits<5>(byte, 0b11110)) {  // 11110xxx
    return 4;
  }

  return 1;  // Некорректный UTF-8, обрабатываем как 1 байт
}

__attribute__((always_inline)) inline short get_utf8_char_len(uint8_t byte) {
  if ((byte & 0x80) == 0x00) [[likely]] {  // 0xxxxxxx
    return 1;
  }
  if ((byte & 0xE0) == 0xC0) [[likely]] {  // 110xxxxx
    return 2;
  }
  if ((byte & 0xF0) == 0xE0) {             // 1110xxxx
    return 3;
  }
  if ((byte & 0xF8) == 0xF0) {             // 11110xxx
    return 4;
  }
  return 1;  // Некорректный UTF-8, обрабатываем как 1 байт
}

using UnicodeCodePoint = uint32_t;
/**
 * @brief Groups UTF-8 encoded characters based on a predicate and converts groups using a converter
 *
 * Processes a sequence of UTF-8 bytes, grouping consecutive code points that satisfy the predicate.
 * Each group is then converted using the provided converter and written to the output iterator.
 *
 * @tparam InputIterator Iterator type for input UTF-8 byte sequence (must meet InputIterator requirements)
 * @tparam OutputIterator Iterator type for output (must meet OutputIterator requirements)
 * @tparam GroupInclusionPredicate Callable type that takes UnicodeCodePoint and returns bool
 * @tparam Converter Callable type that converts vector<UnicodeCodePoint> to output value type
 *
 * @param[in] first Iterator to the start of UTF-8 byte sequence
 * @param[in] last Iterator to the end of UTF-8 byte sequence
 * @param[out] result Output iterator where converted groups will be stored
 * @param[in] pred Predicate determining group inclusion (returns true if code point belongs to current group)
 * @param[in] convert Converter function that transforms code point groups to output type
 *
 * @pre InputIterator must dereference to byte-like type (char, uint8_t, etc.)
 * @pre GroupInclusionPredicate must satisfy std::predicate<UnicodeCodePoint> concept
 * @pre Converter must accept vector<UnicodeCodePoint> and return type compatible with OutputIterator's value_type
 *
 * @note The function handles UTF-8 sequences of 1-4 bytes length
 * @note Empty groups are not output
 * @note Any trailing group will be output even without a terminating non-matching character
 *
 * @throws None (no-throw guarantee if converter and predicate don't throw)
 *
 * Example usage:
 * @code
 * auto is_cyrillic = [](uint32_t cp) { return cp >= 0x0400 && cp <= 0x04FF; };
 * auto to_utf8 = [](const auto& group) { <convert to string> };
 * std::vector<uint8_t> input = {0xD0, 0x9F, 0xD1, 0x80, 0x41}; // "ПрA"
 * std::vector<std::string> output;
 * group_if(input.begin(), input.end(), back_inserter(output), is_cyrillic, to_utf8);
 * // output contains ["Пр", "A"]
 * @endcode
 */
template<typename InputIterator, typename OutputIterator, typename GroupInclusionPredicate, typename Converter>
void group_if(InputIterator first, InputIterator last, OutputIterator result, GroupInclusionPredicate pred, Converter convert) {
  using CodePointGroup = std::vector<UnicodeCodePoint>;
  /*
  static_assert(requires (Converter conv, CodePointGroup group, GroupInclusionPredicate pred, UnicodeCodePoint point)
  {
    { conv(group) } -> std::same_as<typename std::iterator_traits<OutputIterator>::value_type>;
    { pred(point) } -> std::same_as<bool>;
  });
  */
  // static_assert(std::regular<typename std::iterator_traits<OutputIterator>::value_type>);
  // static_assert(std::predicate<GroupInclusionPredicate, UnicodeCodePoint>);

  CodePointGroup char_group;
  char_group.reserve(32);
  for (; first != last; ++first) {
    const auto char_length = get_utf8_char_len(*first);
    auto code_point = static_cast<UnicodeCodePoint>(*first);;

    switch (char_length) {
      [[likely]] case 1: {
        break;
      }
      [[likely]] case 2: {
        auto a = *first++;
        auto b = *first;
        code_point = (a & 0x1F) << 6 | (b & 0x3F);
        break;
      }
      case 3: {
        auto a = *first++;
        auto b = *first++;
        auto c = *first;
        code_point = (a& 0x1F) << 12 | (b & 0x3F) << 6 | (c& 0x3F);
        break;
      }
      case 4: {
        auto a = *first++;
        auto b = *first++;
        auto c = *first++;
        auto d = *first;
        code_point = (a & 0x1F) << 18 | (b & 0x3F) << 12 | (c & 0x3F) << 6 | (d & 0x3F);
        break;
      }
      default: {}
    }

    if (pred(code_point)) {
      char_group.push_back(code_point);
    } else {
      if (size(char_group) > 32) { std::cout << "Size: " << size(char_group) << '\n'; }
      CodePointGroup empty_char_group; empty_char_group.reserve(32);
      *result = convert(std::exchange(char_group, std::move(empty_char_group)));
      ++result;
    }
  }
}

} // namespace uu