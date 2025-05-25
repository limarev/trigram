#include "CLI11.hpp"
#include "group_if.h"

#ifdef LF
#include "lfqueue.h"
#else
#include "tsqueue.h"
#endif

#include "util.h"
#include "word.h"
#include <algorithm>
#include <bitset>
#include <cctype>
#include <fstream>
#include <iostream>
#include <string>
#include <unordered_set>
#include <vector>
#include <stack>
#include <ranges>
#include <thread>
#include <iomanip>


// Функция для проверки, является ли символ UTF-8 строчной буквой (русской или английской)
bool islower_utf8(const std::string& s, size_t pos) {
    if (pos >= s.size()) return false;

    unsigned char c = s[pos];

    // Английские строчные буквы
    if (c >= 'a' && c <= 'z') return true;

    // Русские строчные буквы UTF-8 (2 байта)
    if ((c & 0xE0) == 0xD0 && pos + 1 < s.size()) {
        unsigned char c2 = s[pos + 1];
        // Диапазон русских строчных букв в UTF-8 (D0 B0..D0 BF, D1 80..D1 8F)
        return (c == 0xD0 && c2 >= 0xB0 && c2 <= 0xBF) ||
               (c == 0xD1 && c2 >= 0x80 && c2 <= 0x8F);
    }

    return false;
}

// Функция для приведения символа UTF-8 к нижнему регистру (русский и английский)
void tolower_utf8(std::string& s, size_t pos) {
    if (pos >= s.size()) return;

    unsigned char c = s[pos];

    // Английские заглавные буквы
    if (c >= 'A' && c <= 'Z') {
        s[pos] = c + 32;
        return;
    }

    // Русские заглавные буквы UTF-8 (2 байта)
    if ((c & 0xE0) == 0xD0 && pos + 1 < s.size()) {
        unsigned char c2 = s[pos + 1];
        // Диапазон русских заглавных букв в UTF-8 (D0 90..D0 9F, D0 81, D1 80..D1 8F)
        if (c == 0xD0) {
            if (c2 >= 0x90 && c2 <= 0x9F) {
                s[pos + 1] = c2 + 0x20; // Сдвиг к строчным
            } else if (c2 == 0x81) { // Буква Ё
                s[pos] = 0xD1;
                s[pos + 1] = 0x91;
            }
        } else if (c == 0xD1 && c2 >= 0x80 && c2 <= 0x8F) {
            if (c2 == 0x80) { // Буква Ё (заглавная)
                s[pos] = 0xD0;
                s[pos + 1] = 0x81;
            } else {
                s[pos + 1] = c2 + 0x20; // Сдвиг к строчным
            }
        }
    }
}

void encode_utf8(uint32_t code_point, std::string& out) {
  if (code_point <= 0x7F) {  // 1 байт
    out.push_back(static_cast<char>(code_point));
  } else if (code_point <= 0x7FF) {  // 2 байта
    out.push_back(static_cast<char>(0xC0 | ((code_point >> 6) & 0x1F)));
    out.push_back(static_cast<char>(0x80 | (code_point & 0x3F)));
  } else if (code_point <= 0xFFFF) {  // 3 байта
    out.push_back(static_cast<char>(0xE0 | ((code_point >> 12) & 0x0F)));
    out.push_back(static_cast<char>(0x80 | ((code_point >> 6) & 0x3F)));
    out.push_back(static_cast<char>(0x80 | (code_point & 0x3F)));
  } else if (code_point <= 0x10FFFF) {  // 4 байта
    out.push_back(static_cast<char>(0xF0 | ((code_point >> 18) & 0x07)));
    out.push_back(static_cast<char>(0x80 | ((code_point >> 12) & 0x3F)));
    out.push_back(static_cast<char>(0x80 | ((code_point >> 6) & 0x3F)));
    out.push_back(static_cast<char>(0x80 | (code_point & 0x3F)));
  }
  // Игнорируем некорректные символы (по желанию можно добавить обработку ошибок)
}


struct Trigram {
  auto operator<=>(const Trigram &) const = default;
  std::string to_utf8() const {
    std::string result;

    // Извлекаем 3 символа (21 бит каждый)
    uint32_t char1 = (value >> 42) & 0x1FFFFF;  // Старшие 21 бит
    uint32_t char2 = (value >> 21) & 0x1FFFFF;  // Средние 21 бит
    uint32_t char3 = value & 0x1FFFFF;          // Младшие 21 бит

    // Кодируем каждый символ в UTF-8 и добавляем в строку
    encode_utf8(char1, result);
    encode_utf8(char2, result);
    encode_utf8(char3, result);

    return result;
  }
  alignas(64) uint64_t value;
};

#ifdef LF
using Queue = lf::lf_queue<Trigram>;
#else
using Queue = ts::TSQueue<Trigram>;
#endif

// Функция для извлечения триграмм из слова
using Trigrams = std::vector<Trigram>;
Trigrams generate_trigrams(word::Word&& word) {
  assert(not word.empty());

  switch (auto size = word.size(); size) {
    case 1: { // [_][w0][_]
      auto trigram_value = (static_cast<uint64_t>(0x20) << 42) | 0x20;
      trigram_value |= static_cast<uint64_t>(word[0]) << 21;
      Trigrams result; result.reserve(1);result.emplace_back(trigram_value);
      return result;
    }
    case 2: { // [w0][w1][_]
      auto trigram_value = static_cast<uint64_t>(0x20);
      trigram_value |= static_cast<uint64_t>(word[0]) << 42;
      trigram_value |= static_cast<uint64_t>(word[1]) << 21;
      Trigrams result; result.reserve(1);result.emplace_back(trigram_value);
      return result;
    }
    default: {
      Trigrams result; result.reserve(size);
      for (size_t i = 0; i < size - 2; ++i) {
        auto trigram_value = static_cast<uint64_t>(word[i]);
        trigram_value |= static_cast<uint64_t>(word[i+1]) << 42;
        trigram_value |= static_cast<uint64_t>(word[i+2]) << 21;
        result.emplace_back(trigram_value);
      }
      {
        auto trigram_value = static_cast<uint64_t>(0x20);
        trigram_value |= static_cast<uint64_t>(word[size-1]) << 42;
        trigram_value |= static_cast<uint64_t>(word[size-2]) << 21;
        result.emplace_back(trigram_value);
      }
      {
        auto trigram_value = static_cast<uint64_t>(0x20) << 42;
        trigram_value |= static_cast<uint64_t>(word[0]) << 21;
        trigram_value |= static_cast<uint64_t>(word[1]);
        result.emplace_back(trigram_value);
      }
      return result;
    }
  }

  return {};
}

constexpr bool is_russian(uint32_t code_point) {
  // Русские буквы в Unicode
  return (code_point >= 0x0410 && code_point <= 0x044F) ||  // А-я (без Ё/ё)
         (code_point == 0x0451) ||                         // ё
         (code_point == 0x0401);                           // Ё
}

template<>
Trigram ts::Limiter<Trigram>() {
  return Trigram{};
}

void produce(Queue &q, std::vector<word::Word>&& words) {
    // Генерируем триграммы для каждого слова
    for (;not empty(words); words.pop_back()) {
      auto word = std::move(words.back());
      if (std::empty(word)) { continue; }

      auto trigrams = generate_trigrams(std::move(word));

      while (not empty(trigrams)) {
        q.push(std::move(trigrams.back())); trigrams.pop_back();
      }
    }

    q.push(ts::Limiter<Trigram>());
}

void consume(Queue &q, std::unordered_map<uint64_t, int> &result) {
  Trigram value;
  while (q.wait_and_pop(value)) {
    result[value.value]++;
  }
}

int main(int argc, char** argv) {
  CLI::App app{"File reader with size output"};

  std::string file_path;
  app.add_option("file", file_path, "Path to the text file")
      ->required()
      ->check(CLI::ExistingFile);

  try {
    CLI11_PARSE(app, argc, argv);

    // Открываем файл в бинарном режиме для точного определения размера
    std::ifstream file(file_path, std::ios::binary | std::ios::in);
    if (!file.is_open()) {
      throw std::runtime_error("Failed to open file: " + file_path);
    }

    // Получаем размер файла
    auto file_size = std::filesystem::file_size(file_path);

    // Читаем содержимое файла (опционально)
    std::string input; input.resize(file_size);
    if (!file.read(input.data(), file_size)) {
      throw std::runtime_error("Failed to read file: " + file_path);
    }

    // Выводим размер файла
    std::cout << "File size: " << file_size << " bytes\n";
    Timer t; t.start();
    // Разбиваем строку на слова
    std::vector<word::Word> words;
    words.reserve(265535);

    uu::group_if(cbegin(input), cend(input), std::back_inserter(words),
      [](auto&& rune) { return std::isalpha(rune) || is_russian(rune); },
      [](auto word){ return word::Word{std::move(word)}; });

    word::Word empty_word {};
    auto&& t1 = std::erase(words, empty_word);

    std::unordered_map<uint64_t, int> result;

#define parallel1
#ifdef parallel
    Queue queue;

    std::thread producer(produce, std::ref(queue), std::move(words));
    std::thread consumer(consume, std::ref(queue), std::ref(result));

    producer.join();
    consumer.join();
#else
    for (;not empty(words); words.pop_back()) {
      auto word = std::move(words.back());
      // if (std::empty(word)) { continue; }

      auto trigrams = generate_trigrams(std::move(word));
      for (auto &[value] : trigrams) {
        result[value]++;
      }
      /*
      while (not empty(trigrams)) {
        auto value = std::move(trigrams.back()); trigrams.pop_back();
        result[value.value]++;
      }*/
    }
#endif
    t.stop();
    std::cout << '\n';
    std::cout << "Time: " << t.elapsed_ms() << '\n';

    /*
    for (const auto& [k, v] : result) {
      std::cout << std::quoted(k.to_utf8()) << ": " << v << "\n";
    }*/


  } catch (const std::exception& e) {
    std::cerr << "Error: " << e.what() << std::endl;
    return 1;
  }

  return 0;
}
