#   cppext

cppext is an open-source, header-only extension library for C++.

##  Usage

  Just add `#include <chesham/cppext/cppext.hpp>` in your source code and `using namespace chesham`.

  Then you can go with those extension methods such as replace, split for `std::string`.

##  Samples

  For samples, please visit the source files under `cppext/test/` folder.

### `asynchronous HTTP client`

Provide a HTTP client object using IOCP to retrive data on Windows.

Performing HTTP GET with auto decompression.

```cpp
net::http_client cli;
// Perform and return a future object
auto task = cli.get("http://ptsv2.com/t/math6-1609257285/post");
auto& response = *task.get();
const auto& buffers = response.buffers();
const auto& headers = response.headers();
stringstream ss;
for (const auto& buffer : buffers)
    ss << string(buffer.begin(), buffer.end());
auto _ = ss.str();
// The `_` possibly is:
// HTTP/1.0 200 OK
// Content-Type: text/plain; charset=utf-8
// Vary: Accept-Encoding
// Access-Control-Allow-Origin: *
// Content-Encoding: gzip
// X-Cloud-Trace-Context: 88c4c1c072ba4ef2b7c15dd84faf22ae
// Date: Wed, 13 Jan 2021 04:35:46 GMT
// Server: Google Frontend
// Cache-Control: private
// Content-Length: 77
// 
// Thank you for this dump. I hope you have a lovely day!
```

Performing HTTP POST.

```cpp
net::http_client cli;
auto task = cli.post("http://ptsv2.com/t/math6-1609257285/post", make_shared<net::buffer_type>(net::buffer_type(5, '?')));
auto& response = *task.get();
const auto& buffers = response.buffers();
const auto& headers = response.headers();
stringstream ss;
for (const auto& buffer : buffers)
    ss << string(buffer.begin(), buffer.end());
auto _ = ss.str();
// The `_` possibly is:
// HTTP/1.0 200 OK
// Content-Type: text/plain; charset=utf-8
// Vary: Accept-Encoding
// Access-Control-Allow-Origin: *
// Content-Encoding: gzip
// X-Cloud-Trace-Context: 4ebaff822e3fa7f149dbf3f65acc92ba
// Date: Wed, 13 Jan 2021 04:40:04 GMT
// Server: Google Frontend
// Cache-Control: private
// Content-Length: 77
// 
// Thank you for this dump. I hope you have a lovely day!
```

### `asynchronous file I/O`

Read file in bytes.

```cpp
auto target = make_shared<cppext::io::async_file>(R"(Lorem Ipsum.txt)");
auto future = target->read_bytes(0, 10);
// do other tasks
Assert::AreEqual((size_t)10, future.get()->size());
```

Write string to file.

```cpp
auto target = make_shared<cppext::io::async_file>("output.txt", ios::out | ios::app);
auto data = "hello world\n"s;
auto fut = target->write_string(0, data);
Assert::AreEqual(data.size(), fut.get());
```

### `subject/observer`

  Provides a thread-safe, generic subject/observer pattern. The observer can exit at anytime.

  ```cpp
  subject sub;
  auto isEventInvoked = false;
  auto e = make_shared<decltype(sub)::event_type>([&](...) { isEventInvoked = true; });
  sub += e;
  sub.notify();
  Assert::IsTrue(isEventInvoked);
  ```

  Observer exits with auto unsubscribes. (But **not** thread-safe, please use managed observer unless you need to manage it manually)

  ```cpp
  subject sub;
  auto isEventInvoked = false;
  auto e = make_shared<decltype(sub)::event_type>([&](...) { isEventInvoked = true; });
  sub += e;
  e.reset();
  sub.notify();
  Assert::IsFalse(isEventInvoked);
  ```

  With managed observer.

  ```cpp
  subject sub;
  auto isEventInvoked = false;
  auto e = sub += [&](...) { isEventInvoked = true; };
  sub.notify();
  Assert::IsTrue(isEventInvoked);
  ```

  Exited safely with managed observer in concurrent scenario.

  ```cpp
  auto isInvoked = false;
  subject sub;
  {
      mutex mtx;
      unique_lock<mutex> l(mtx);
      condition_variable waiter;
      auto isReleased = false;
      auto suber = sub += [&](...)
      {
          unique_lock<mutex> l(mtx);
          isInvoked = true;
          waiter.notify_one();
          Logger::WriteMessage("event invoked\n");
          while (!waiter.wait_for(l, 10ms, [&] { return isReleased; }));
          Logger::WriteMessage("event completed\n");
      };
      auto task = async([&] { sub.notify(); });
      auto cleanTask = async([&, suber = move(suber)]() mutable
      {
          unique_lock<mutex> l(mtx);
          waiter.wait(l, [&] { return isInvoked; });
          isReleased = true;
          l.unlock();
          Logger::WriteMessage("subscriber exiting with auto synchronize ...\n");
          suber = nullptr;
          Logger::WriteMessage("subscriber exited\n");
      });
      l.unlock();
  }
  Assert::IsTrue(isInvoked);

  // The log after executed
  //    event invoked
  //    subscriber exiting with auto synchronize ...
  //    event completed
  //    subscriber exited
  ```

  Deal with user-defined event arguments.
  
  ```cpp
  struct my_event_args : public cppext::event_args
  {
      bool isCancelled{ false };
  };

  struct subject : public cppext::subject<cppext::event_args>
  {
      typedef cppext::subject<cppext::event_args> base;

      void notify(my_event_args& args)
      {
          decltype(base::subs) subs;
          {
              lock_guard<mutex> l(base::mtx);
              subs = base::subs;
          }
          for (auto& i : subs)
          {
              auto e = i.lock();
              if (!e)
                  continue;
              try
              {
                  (*e)(this, args);
                  if (args.isCancelled)
                      break;
              }
              catch (const exception&)
              {
              }
          }
      }
  };
  
  subject sub;
  auto isSub1Invoked = false;
  auto isSub2Invoked = false;
  auto sub1 = sub += [&](auto, auto& e)
  {
      if (dynamic_cast<const my_event_args*>(&e))
      {
          auto& args = dynamic_cast<my_event_args&>(e);
          args.isCancelled = true;
          isSub1Invoked = true;
      }
  };
  auto sub2 = sub += [&](auto, auto& e)
  {
      if (dynamic_cast<const my_event_args*>(&e))
          isSub2Invoked = true;
  };
  my_event_args args;
  sub.notify(args);
  Assert::IsTrue(isSub1Invoked);
  Assert::IsFalse(isSub2Invoked);
  Assert::IsTrue(args.isCancelled);
  ```

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
