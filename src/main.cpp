
#include <array>
#include <vector>
#include <memory>
#include <functional>
#include <iostream>
#include <source_location>
#include <type_traits>
#include <concepts>
#include <hng/nullsafety/nullsafety.h>

namespace hng {
    namespace nullsafety_tests {

        static_assert(std::same_as<decltype(hng::nullsafety::notnull(std::declval<int* const&>())),hng::nullsafety::notnull<int*>>);

        static_assert(sizeof(hng::nullsafety::notnull<int*>) == sizeof(int*));
        static_assert(sizeof(hng::nullsafety::notnull<std::shared_ptr<long>>) == sizeof(std::shared_ptr<long>));
        static_assert(sizeof(hng::nullsafety::derefchecked<int*>) == sizeof(int*));
        struct SampleDtor {};
        static_assert(sizeof(hng::nullsafety::derefchecked<std::unique_ptr<long, SampleDtor>>) == sizeof(std::unique_ptr<long, SampleDtor>));
        static_assert(alignof(hng::nullsafety::derefchecked<std::unique_ptr<long, SampleDtor>>) == alignof(std::unique_ptr<long, SampleDtor>));

        static constexpr int const static_assertion_variable_x = 5;
        static_assert([]() constexpr {
            int const* y = &static_assertion_variable_x;
            auto z = hng::nullsafety::notnull<int const*>(y);
            int const* const a = y;
            auto w = hng::nullsafety::notnull(a);
            static_assert(std::is_same_v<decltype(w), hng::nullsafety::notnull<int const*>>);
            return true;
            }());

        static_assert([]() constexpr {
            auto x = std::vector<int>();
            x.push_back(5);
            auto p = &x.data()[0];
            hng::nullsafety::notnull q(p);
            int a = *q;
            return a == 5;
            }());

        static_assert([]() constexpr {
            struct S {
                constexpr void f(int& a) {
                    a = 2;
                }
            };
            S s{};
            auto p = hng::nullsafety::notnull(&s);
            hng::nullsafety::notnull q = p;
            static_assert(std::same_as<decltype(p), decltype(q)>);
            int a = 0;
            q->f(a);
            return a == 2;
            }());

        static_assert([]() constexpr {
            std::array a{ 10, 20 };
            auto p = hng::nullsafety::notnull(&a[0]);
            auto q = hng::nullsafety::notnull(&a[1]);
            return (p <=> q) != 0
                && (p <=> &a[0]) == 0
                && p < q
                && q >= p
                && q == &a[1]
                && q != nullptr
                ;
            }());

        static_assert([]() constexpr {
            std::array a{ 10, 20 };
            hng::nullsafety::derefchecked p = &a[0];
            hng::nullsafety::derefchecked q = &a[1];
            hng::nullsafety::derefchecked<int*> r = nullptr;
            return (p <=> q) != 0
                && (p <=> &a[0]) == 0
                && p < q
                && q >= p
                && q == &a[1]
                && q != nullptr
                && r == nullptr
                ;
            }());

        static_assert([]() constexpr {
            std::array a{ 10, 20 };
            hng::nullsafety::derefchecked p0 = &a[0];
            hng::nullsafety::derefchecked p1 = &a[1];
            auto q0 = hng::nullsafety::notnull(&a[0]);
            auto q1 = hng::nullsafety::notnull(&a[1]);
            hng::nullsafety::derefchecked<int*> r = nullptr;
            return p0 == q0 && p1 == q1 && p0 != q1 && p1 != q0 && p0 != p1 && q0 != q1
                && p0 < q1 && p1 > q0 && p1 >= q0
                && (q1 <=> q0) > 0
                ;
            }());

        static_assert([]() constexpr {
            struct S {
                static constexpr void f(int* p) {
                    *p = 2;
                }
            };
            int a = 0;
            auto p = hng::nullsafety::notnull(&a);
            S::f(p);
            return a == 2;
            }(), "implicit convert from notnull<T*> to T*");

        struct NotNullFunctionParameterDetail {
            inline static hng::nullsafety::notnull<int*> next(hng::nullsafety::notnull<int*> p) {
                *p += 1;
                return p + 1;
            }
            template<class T>
            inline static auto next2(T&& value) {
                auto p = next(std::forward<T>(value));
                p = next(p);
                return p;
            }
        };



        template<class F>
        bool test(std::string_view name, F&& f, std::source_location location = std::source_location::current()) {
            bool success = false;
            try {
                if constexpr (std::convertible_to<std::invoke_result_t<F&&, std::string_view const&>, bool>) {
                    success = std::invoke(std::forward<F>(f), std::as_const(name));
                }
                else {
                    std::invoke(std::forward<F>(f), std::as_const(name));
                    success = true;
                }
            }
            catch (...) {
                std::cout << "Test failed: Test named \"" << name << "\" failed: in " << location.file_name() << ":line " << location.line() << std::endl;
                return false;
            }
            if (!success) {
                std::cout << "Test failed: Test named \"" << name << "\" failed: in " << location.file_name() << ":line " << location.line() << std::endl;
            }
            return success;
        }

        void run_tests() {
            std::vector<std::function<bool()>> tests;

            tests.emplace_back([] { return test("derefchecked should throw if null is dereferenced at runtime", [](auto const& test_name) {
                {
                    int a = 5;
                    auto p = hng::nullsafety::derefchecked(&a);
                    *p += 1;
                    if (a != 6)
                        return false;

                    bool threw = false;
                    try {
                        p = nullptr;
                        int b = *p;
                    }
                    catch (hng::nullsafety::nullptr_error const&) {
                        threw = true;
                    }
                    return threw;
                }
                }); });
            tests.emplace_back([] { return test("notnull smart pointer should throw if constructed from null at runtime", [](auto const& test_name) {
                {
                    int a = 5;
                    hng::nullsafety::notnull p = &a;
                    *p += 1;
                    if (a != 6)
                        return false;

                    hng::nullsafety::notnull u = std::make_shared<int>(2);
                    auto v = std::move(u);
                    int b = *v;
                    if (b != 2)
                        return false;
                    *v += 1;
                    int c = *u;
                    if (c != 3)
                        return false;

                    bool threw = false;
                    try {
                        v = static_cast<std::shared_ptr<int>>(nullptr);
                    }
                    catch (hng::nullsafety::nullptr_error const&) {
                        threw = true;
                    }
                    return threw;
                }
                }); });
            tests.emplace_back([] { return test("notnull unique_ptr", [](auto const& test_name) {
                {
                    hng::nullsafety::notnull u = std::make_unique<int>(6);
                    //auto v = std::move(u); // does not compile
                    auto& v = u;
                    return *v == 6;
                }
                }); });
            tests.emplace_back([] { return test("notnull unique_ptr exchange", [](auto const& test_name) {
                {
                    hng::nullsafety::notnull u = std::make_unique<int>(4);

                    // option 1: hng::nullsafety::exchange(notnull, newVal) returns hng::nullsafety::derefchecked<P>
                    using std::exchange;
                    hng::nullsafety::notnull v = exchange(u, std::make_unique<int>(5));

                    static_assert(std::is_same_v<decltype(v), hng::nullsafety::notnull<std::unique_ptr<int>>>);

                    // option 2: notnull.exchange_inner_ptr()
                    hng::nullsafety::notnull w = v.exchange_inner_ptr(std::make_unique<int>(6));

                    // option 3: hardcoded boilerplate
                    hng::nullsafety::notnull x(w.unsafe_release());
                    w = std::make_unique<int>(7);

                    return *u == 5 && *v == 6 && *w == 7 && *x == 4;
                }
                }); });
            tests.emplace_back([] { return test("as_span_of_notnull", [](auto const& test_name) {
                {
                    std::array a{ 0, 1, 2, 3, 4 };
                    std::array const v{ &a[0], &a[2], &a[1], &a[3], &a[4] };
                    std::span<int const*const> const s = v;
                    auto const nns = hng::nullsafety::as_span_of_notnull(s);
                    static_assert(std::is_same_v<decltype((nns[0])), hng::nullsafety::notnull<int const*> const&>);
                    return *nns[0] == 0 && *nns[1] == 2 && *nns[2] == 1 && *nns[3] == 3 && *nns[4] == 4;
                }
                }); });
            tests.emplace_back([] { return test("as_span_of_notnull mutable", [](auto const& test_name) {
                {
                    std::array a{ 0, 1, 2, 3, 4 };
                    std::array v{ &a[0], &a[2], &a[1], &a[3], &a[4] };
                    std::span s = v;
                    auto nns = hng::nullsafety::as_span_of_notnull(s);
                    static_assert(std::is_same_v<decltype((nns[0])), hng::nullsafety::notnull<int*>&>);
                    *nns[1] = 20;
                    nns[2] = &a[4];
                    return *nns[0] == 0 && *nns[1] == 20 && *nns[2] == 4 && *nns[3] == 3 && *nns[4] == 4
                        && a[2] == 20 && nns[2] == nns[4] && nns[2] == &a[4]
                        ;
                }
                }); });
            tests.emplace_back([] { return test("calling as_span_of_notnull with a span that contains null elements should throw", [](auto const& test_name) {
                {
                    std::array a{ 0, 1, 2, 3, 4 };
                    std::array v{ &a[0], &a[2], static_cast<int*>(nullptr), &a[1], &a[3], &a[4] };
                    std::span s = v;
                    bool threw = false;
                    try {
                        auto nns = hng::nullsafety::as_span_of_notnull(s);
                    }
                    catch (hng::nullsafety::nullptr_error const&) {
                        threw = true;
                    }
                    return threw;
                }
                }); });
            tests.emplace_back([] { return test("as_span_of_derefchecked", [](auto const& test_name) {
                {
                    std::array a{ 0, 1, 2, 3, 4 };
                    std::array v{ &a[0], &a[2], &a[1], static_cast<int*>(nullptr), &a[3], &a[4] };
                    std::span s = v;
                    auto dcs = hng::nullsafety::as_span_of_derefchecked(s);
                    if (!(
                        *dcs[0] == 0 && *dcs[1] == 2 && *dcs[2] == 1 && *dcs[4] == 3 && *dcs[5] == 4
                        )) {
                        return false;
                    }
                    bool threw = false;
                    auto p = dcs[3];
                    try {
                        int x = *p;
                    }
                    catch (hng::nullsafety::nullptr_error const&) {
                        threw = true;
                    }
                    return threw;
                }
                }); });
            tests.emplace_back([] { return test("convert reference_wrapper to notnull", [](auto const& test_name) {
                {
                    struct S {
                        static constexpr void f(int* p) {
                            *p = 3;
                        }
                    };
                    int a = 0;
                    std::reference_wrapper r(a);
                    auto p = hng::nullsafety::notnull(r);
                    S::f(p);
                    return a == 3;
                }
                }); });
            tests.emplace_back([] { return test("notnull int", [](auto const& test_name) {
                {
                    hng::nullsafety::notnull<int> x(3);
                    hng::nullsafety::notnull<int> z(-4);
                    try {
                        hng::nullsafety::notnull<int> y(0);
                        return false;
                    }
                    catch (hng::nullsafety::nullptr_error const&) {
                    }
                    try {
                        z = 0;
                        return false;
                    }
                    catch (hng::nullsafety::nullptr_error const&) {
                    }
                    return x == 3 && z == -4;
                }
                }); });
            tests.emplace_back([] { return test("notnull function parameter", [](auto const& test_name) {
                {
                    int a[]{ 10, 20, 30 };
                    hng::nullsafety::notnull<int*> x(&a[0]);
                    x = NotNullFunctionParameterDetail::next2(x);
                    return x == &a[2] && a[0] == 11 && a[1] == 21 && a[2] == 30;
                }
                }); });
            tests.emplace_back([] { return test("readme example", [](auto const& test_name) {
                {
                    int x = 2;
                    int* y = &x;
                    hng::nullsafety::notnull<int*> p = y;
                    //std::cout << *p << std::endl;
                    // ^ prints "2".
                    if (*p != 2)
                        return false;

                    //p = nullptr;
                    // ^ will not compile.

                    try {
                        p = static_cast<int*>(nullptr);
                        // ^ throws a hng::nullsafety::nullptr_error at runtime, p is unchanged.
                        return false;
                    }
                    catch (hng::nullsafety::nullptr_error const&) {
                    }

                    try {
                        hng::nullsafety::notnull p2 = static_cast<int*>(nullptr);
                        // ^ throws a hng::nullsafety::nullptr_error at runtime.
                        return false;
                    }
                    catch (hng::nullsafety::nullptr_error const&) {
                    }

                    hng::nullsafety::derefchecked<int*> q = nullptr;
                    // ^ ok

                    try {
                        *q = 5;
                        // ^ throws a hng::nullsafety::nullptr_error at runtime.
                        return false;
                    }
                    catch (hng::nullsafety::nullptr_error const&) {
                    }

                    //std::cout << *p << std::endl;
                    
                    return true;
                }
                }); });



            bool all = true;
            for (auto& test : tests) {
                if (!std::invoke(std::move(test))) {
                    all = false;
                }
            }
            if (!all) {
                throw std::runtime_error("Some tests failed");
            }
        }
    }
}

int main() {
    try {
        hng::nullsafety_tests::run_tests();
        std::cout << "All tests passed." << std::endl;
    }
    catch (std::exception const& ex) {
        std::cerr << "Testing failed: " << ex.what() << std::endl;
    }
    catch (...) {
        std::cerr << "Testing failed" << std::endl;
    }
}
