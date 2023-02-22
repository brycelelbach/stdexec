/*
 * Copyright (c) 2023 Maikel Nadolski
 *
 * Licensed under the Apache License Version 2.0 with LLVM Exceptions
 * (the "License"); you may not use this file except in compliance with
 * the License. You may obtain a copy of the License at
 *
 *   https://llvm.org/LICENSE.txt
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <catch2/catch.hpp>
#include <exec/when_any.hpp>
#include <exec/single_thread_context.hpp>
#include <test_common/schedulers.hpp>
#include <test_common/receivers.hpp>
#include <test_common/senders.hpp>
#include <test_common/type_helpers.hpp>

namespace ex = stdexec;

TEST_CASE("when_ny returns a sender", "[adaptors][when_any]") {
  auto snd = exec::when_any_value(ex::just(3), ex::just(0.1415));
  static_assert(ex::sender<decltype(snd)>);
  (void) snd;
}

TEST_CASE("when_any with environment returns a sender", "[adaptors][when_any]") {
  auto snd = exec::when_any_value(ex::just(3), ex::just(0.1415));
  static_assert(ex::sender_in<decltype(snd), ex::empty_env>);
  (void) snd;
}

TEST_CASE("when_any simple example", "[adaptors][when_any]") {
  auto snd = exec::when_any_value(ex::just(3.0));
  auto snd1 = std::move(snd) | ex::then([](double y) { return y + 0.1415; });
  const double expected = 3.0 + 0.1415;
  auto op = ex::connect(std::move(snd1), expect_value_receiver{expected});
  ex::start(op);
}

TEST_CASE("when_any completes with only one sender", "[adaptors][when_any]") {
  ex::sender auto snd = exec::when_any_value(         //
    completes_if{false} | ex::then([] { return 1; }), //
    completes_if{true} | ex::then([] { return 42; })  //
  );
  static_assert(ex::sender<decltype(snd)>);
  wait_for_value(std::move(snd), 42);
  ex::sender auto snd2 = exec::when_any_value(        //
    completes_if{true} | ex::then([] { return 1; }),  //
    completes_if{false} | ex::then([] { return 42; }) //
  );
  //ex::__types<ex::completion_signatures_of_t<decltype(snd2)>> t;
  wait_for_value(std::move(snd2), 1);
}

TEST_CASE("when_any with move-only types", "[adaptors][when_any]") {
  ex::sender auto snd = exec::when_any_value( //
    completes_if{false} | ex::then([] { return movable(1); }),
    ex::just(movable(42))                     //
  );
  static_assert(ex::sender<decltype(snd)>);
  // ex::__types<ex::completion_signatures_of_t<decltype(snd)>> t;
  wait_for_value(std::move(snd), movable(42));
}

TEST_CASE("when_any forwards stop signal", "[adaptors][when_any]") {
  stopped_scheduler stop;
  int result = 42;
  ex::sender auto snd =
    exec::when_any_value( //
      ex::schedule(stop), //
      ex::schedule(stop)  //
      )
    | ex::then([&result] { result += 1; });
  ex::sync_wait(std::move(snd));
  static_assert(ex::sender<decltype(snd)>);
  // ex::__types<ex::completion_signatures_of_t<decltype(snd)>> t;
  REQUIRE(result == 42);
}

TEST_CASE("nested when_any is stoppable", "[adaptors][when_any]") {
  int result = 41;
  ex::sender auto snd =
    exec::when_any_value(
      exec::when_any_value(completes_if{false}, completes_if{false}),
      completes_if{false},
      ex::just(),
      completes_if{false})
    | ex::then([&result] { result += 1; });
  static_assert(ex::sender<decltype(snd)>);
  ex::sync_wait(std::move(snd));
  // ex::__types<ex::completion_signatures_of_t<decltype(snd)>> t;
  REQUIRE(result == 42);
}

TEST_CASE("stop is forwarded", "[adaptors][when_any]") {
  int result = 41;
  ex::sender auto snd = exec::when_any_value(ex::just_stopped())
                      | ex::upon_stopped([&result]() noexcept { result += 1; });
  ex::sync_wait(std::move(snd));
  static_assert(ex::sender<decltype(snd)>);
  // ex::__types<ex::completion_signatures_of_t<decltype(snd)>> t;
  REQUIRE(result == 42);
}

TEST_CASE("when_any_value is thread-safe", "[adaptors][when_any]") {
  exec::single_thread_context ctx1;
  exec::single_thread_context ctx2;
  exec::single_thread_context ctx3;

  auto sch1 = ex::schedule(ctx1.get_scheduler());
  auto sch2 = ex::schedule(ctx2.get_scheduler());
  auto sch3 = ex::schedule(ctx3.get_scheduler());

  int result = 41;

  ex::sender auto snd = exec::when_any_value(
    sch1 | ex::let_value([] { return exec::when_any_value(completes_if{false}); }),
    sch2 | ex::let_value([] { return completes_if{false}; }),
    sch3 | ex::then([&result] { result += 1; }),
    completes_if{false});
  static_assert(ex::sender<decltype(snd)>);
  // ex::__types<ex::completion_signatures_of_t<decltype(snd)>> t;
  ex::sync_wait(std::move(snd));
  REQUIRE(result == 42);
}
