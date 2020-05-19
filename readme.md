#   cppext

cppext is an open-source, header-only extension library for C++.

##  Usage

  Just add `#include <chesham/cppext/cppext.hpp>` in your source code and `using namespace chesham`.

  Then you can go with those extension methods such as replace, split for `std::string`.

##  Samples

  For samples, please visit the source files under `cppext/test/` folder.

### `sequence_equal`

Test whether the elements in two ranges are equal sequentially, and the size of two ranges might be different.

```cpp
auto s1 = { 10, 20, 33, 50, 230, 70 };
auto s2 = { 10, 20, 33, 50, 230, 70 };
Assert::IsTrue(cppext::sequence_equal(begin(s1), end(s1), begin(s2), end(s2)));

auto s3 = { 10, 20, 33, 50, 70 };
Assert::IsFalse(cppext::sequence_equal(begin(s1), end(s1), begin(s3), end(s3)));

auto str1 = "hello world"s;
auto str2 = "hello world"s;
Assert::IsTrue(cppext::sequence_equal(str1.begin(), str1.end(), str2.begin(), str2.end()));
```

### `string_exted::replace`

  Replace substring with string.

  ```cpp
  auto src = "hello world heworldllo"s;
  auto expect = "hello c++ hec++llo"s;
  auto actual = cppext::ext(src).replace("world", "c++");
  Assert::AreEqual(expect, (decltype(expect))actual);
  ```

  With limits.

  ```cpp
  auto src = "hello world heworldllo"s;
  auto expect = "hello c++ heworldllo"s;
  auto actual = cppext::ext(src).replace("world", "c++", 1);
  Assert::AreEqual(expect, (decltype(expect))actual);
  ```

  With abstract data even integer.

  ```cpp
  auto src = "hello world heworldllo"s;
  auto expect = "hello 123 he123llo"s;
  auto actual = cppext::ext(src).replace("world", 123);
  Assert::AreEqual(expect, (decltype(expect))actual);
  ```

### `string_exted::split`

  Split string into vector with specific delimiter.

  ```cpp
  auto target = ",1,,2,3,4,5,6,";
  auto expect = vector<string>{ "", "1", "", "2", "3", "4", "5", "6", "" };
  auto actual = cppext::ext(target).split(",");
  Assert::IsTrue(cppext::sequence_equal(expect.begin(), expect.end(), actual.begin(), actual.end()));
  ```

  With limits.

  ```cpp
  auto target = ",1,,2,3,4,5,6,";
  auto expect = vector<string>{ "", "1", "" };
  auto actual = cppext::ext(target).split(",", false, 3);
  Assert::IsTrue(cppext::sequence_equal(expect.begin(), expect.end(), actual.begin(), actual.end()));
  ```

  Skip while string is empty.

  ```cpp
  auto target = ",1,,2,3,4,5,6,";
  auto expect = vector<string>{ "1", "2", "3", "4", "5", "6" };
  auto actual = cppext::ext(target).split(",", true);
  Assert::IsTrue(cppext::sequence_equal(expect.begin(), expect.end(), actual.begin(), actual.end()));
  ```
