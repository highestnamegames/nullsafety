#ifndef HNG_NULLSAFETY_HEADERGUARD
#define HNG_NULLSAFETY_HEADERGUARD
//
//	Author:		Elijah Shadbolt
//	Date:		28 Dec 2023
//	Licence:	MIT
//	GitHub:		https://github.com/highestnamegames/nullsafety
//	Version:	v1.0.1
//
//	Summary:
//		C++ header only library for null safety utilities, including notnull and derefnullchecked.
//

#include <memory>
#include <stdexcept>
#include <type_traits>
#include <span>
#include <algorithm>

namespace hng {
    namespace nullsafety {
        class nullptr_error : public std::runtime_error
        {
        public:
            nullptr_error() : runtime_error("pointer is null") {}
        };

        namespace detail {
            struct private_unsafe_notnull_from_nullable_t {};
            inline constexpr private_unsafe_notnull_from_nullable_t const private_unsafe_notnull_from_nullable{};

            template<class Lambda, int = (Lambda{}(), 0) >
            inline constexpr bool is_constexpr(Lambda) { return true; }
            inline constexpr bool is_constexpr(...) { return false; }
        }

        template<class P> requires (!std::is_reference_v<P> && !std::is_volatile_v<P> && !std::is_const_v<P>)
            class alignas(P) derefnullchecked;

        // The notnull class invariant guarantees that the inner value is not falsy.
        // If P is a pointer-like type, then the notnull class invariant guarantees that the inner pointer is not null,
        // according to the assumption that if the pointer is null then the pointer converted to bool is false.
        // Scenarios involving thread safety issues or const_cast are out of scope for the guarantees this class provides.
        template<class P> requires (!std::is_reference_v<P> && !std::is_volatile_v<P> && !std::is_const_v<P>)
            class alignas(P) notnull {
            private:
                P m_ptr = P();
            public:
                inline constexpr ~notnull() noexcept(std::is_nothrow_destructible_v<P>) = default;
                inline constexpr explicit notnull(detail::private_unsafe_notnull_from_nullable_t, P&& ptr) noexcept(std::is_nothrow_move_constructible_v<P>) : m_ptr(std::move(ptr)) {}
                inline constexpr explicit notnull(detail::private_unsafe_notnull_from_nullable_t, P const& ptr) noexcept(std::is_nothrow_copy_constructible_v<P>) : m_ptr(ptr) {}
                template<class...CArgs>
                inline constexpr explicit notnull(detail::private_unsafe_notnull_from_nullable_t, std::in_place_t, CArgs&&...args) noexcept(std::is_nothrow_constructible_v<P, CArgs&&...>)
                    : m_ptr(std::forward<CArgs>(args)...)
                {
                }
                inline constexpr notnull() noexcept(std::is_nothrow_default_constructible_v<P>) {
                    if (!m_ptr) throw nullptr_error();
                }
                notnull(std::nullptr_t) = delete;
                notnull& operator=(std::nullptr_t) = delete;
                inline constexpr /*implicit*/ notnull(P&& ptr) : m_ptr([&ptr]() -> P&& {
                    if (!ptr) throw nullptr_error();
                    return std::move(ptr);
                    }())
                {
                }
                inline constexpr /*implicit*/ notnull(P const& ptr) : m_ptr([&ptr]() -> P const& {
                    if (!ptr) throw nullptr_error();
                    return ptr;
                    }())
                {
                }
                template<class T> inline constexpr explicit notnull(std::reference_wrapper<T> const& refw) requires std::constructible_from<P, decltype(&std::declval<std::reference_wrapper<T> const&>().get())>
                : notnull(detail::private_unsafe_notnull_from_nullable, std::in_place, &refw.get())
                {
                }
                inline constexpr notnull(notnull const&) noexcept(std::is_nothrow_copy_constructible_v<P>) = default;
                inline constexpr notnull(notnull&& other) noexcept(std::is_nothrow_default_constructible_v<P>&& std::is_nothrow_swappable_v<P>)
                    requires std::is_default_constructible_v<P> && (detail::is_constexpr([] { static_cast<bool>(P()); }) && static_cast<bool>(P()))
                : m_ptr()
                {
                    using std::swap;
                    swap(m_ptr, other.m_ptr);
                }
                inline constexpr notnull(notnull&& other) noexcept(std::is_nothrow_copy_constructible_v<P>)
                    requires !(std::is_default_constructible_v<P> && (detail::is_constexpr([] { static_cast<bool>(P()); }) && static_cast<bool>(P())))
                : notnull(std::as_const(other))
                {
                }
                template<class...CArgs>
                inline constexpr explicit notnull(std::in_place_t, CArgs&&...args)
                    : m_ptr(std::forward<CArgs>(args)...)
                {
                    if (!m_ptr) throw nullptr_error();
                }
                inline constexpr /*implicit*/ notnull(derefnullchecked<P>&& other)
                    : notnull(std::move(other.ptr()))
                {
                }
                inline constexpr /*implicit*/ notnull(derefnullchecked<P> const& other)
                    : notnull(other.ptr())
                {
                }
                inline constexpr notnull& operator=(notnull const&) noexcept(std::is_nothrow_copy_assignable_v<P>) = default;
                inline constexpr notnull& operator=(notnull&& other) noexcept(std::is_nothrow_swappable_v<P>) {
                    using std::swap;
                    swap(m_ptr, other.m_ptr);
                    return *this;
                }
                inline constexpr notnull& operator=(derefnullchecked<P> const& other) {
                    if (!other.ptr()) throw nullptr_error();
                    m_ptr = other.ptr();
                    return *this;
                }
                inline constexpr notnull& operator=(derefnullchecked<P>&& other) {
                    if (!other.ptr()) throw nullptr_error();
                    m_ptr = std::move(other.ptr());
                    return *this;
                }
                inline constexpr notnull& operator=(P ptr) {
                    using std::swap;
                    if (!ptr) throw nullptr_error();
                    swap(m_ptr, ptr);
                    return *this;
                }
                inline constexpr void swap(notnull& other) noexcept(std::is_nothrow_swappable_v<P>) {
                    using std::swap;
                    swap(m_ptr, other.m_ptr);
                }
                inline constexpr P const& as_nullable() const noexcept { return m_ptr; }
                inline constexpr P const& ptr() const noexcept { return m_ptr; }
                inline constexpr /*implicit*/ operator P const& () const noexcept { return m_ptr; }
                inline constexpr explicit operator bool() const noexcept { return true; }
                inline constexpr bool operator!() const noexcept { return false; }
                inline constexpr decltype(auto) operator*() const noexcept(noexcept(*m_ptr)) { return *m_ptr; }
                inline constexpr decltype(auto) operator*() noexcept(noexcept(*m_ptr)) { return *m_ptr; }
                inline constexpr auto const& operator->() const noexcept { return m_ptr; }
                inline constexpr auto const& operator->() noexcept { return m_ptr; }
                //inline constexpr auto operator<=>(notnull const&) const = default;
                //inline constexpr auto operator<=>(P const& ptr) const { return m_ptr <=> ptr; }
                //inline constexpr auto operator<=>(std::nullptr_t const& nullp) const { return m_ptr <=> nullp; }

                // After unsafe_release() has returned, it is the programmer's responsibility
                // to ensure the emptied pointer is assigned a new value (or end of scope is reached and the destructor is executed)
                // before any other methods are called (operator bool, operator!, dereference, etc).
                inline constexpr P unsafe_release() noexcept(std::is_nothrow_move_constructible_v<P>) { return std::move(m_ptr); }

                template<class U>
                inline constexpr P exchange_inner_ptr(U&& newVal) {
                    using std::exchange;
                    auto p = exchange(m_ptr, std::forward<U>(newVal));
                    if (!m_ptr) {
                        using std::swap;
                        swap(m_ptr, p);
                        throw nullptr_error();
                    }
                    return p;
                }
        };

        template<class T> notnull(std::reference_wrapper<T> const&) -> notnull<std::decay_t<decltype(&std::declval<std::reference_wrapper<T> const&>().get())>>;

        template<class P>
        inline constexpr void swap(notnull<P>& lhs, notnull<P>& rhs) noexcept(std::is_nothrow_swappable_v<P>) {
            lhs.swap(rhs);
        }

        template<class P, class U>
        inline constexpr derefnullchecked<P> exchange(notnull<P>& val, U&& newVal) {
            notnull<P> nnn(std::forward<U>(newVal));
            return val.exchange_inner_ptr(nnn.unsafe_release());
        }

        // The derefnullchecked class is nullable, but the pointer is checked for null when it is dereferenced using the * or -> operators
        // and may throw a nullptr_error exception (instead of causing undefined behaviour).
        // Unlike notnull<P>, derefnullchecked<P> is default constructible and move constructible, which means it can be returned from functions.
        template<class P> requires (!std::is_reference_v<P> && !std::is_volatile_v<P> && !std::is_const_v<P>)
            class alignas(P) derefnullchecked {
            private:
                P m_ptr = P();
            public:
                inline constexpr ~derefnullchecked() noexcept(std::is_nothrow_destructible_v<P>) = default;
                inline constexpr derefnullchecked() noexcept(std::is_nothrow_default_constructible_v<P>) = default;
                inline constexpr /*implicit*/ derefnullchecked(std::nullptr_t nullp) noexcept(std::is_nothrow_constructible_v<P, std::nullptr_t&&>) : m_ptr(std::move(nullp)) {}
                inline constexpr /*implicit*/ derefnullchecked(P&& ptr) noexcept(std::is_nothrow_move_constructible_v<P>) : m_ptr(std::move(ptr)) {}
                inline constexpr /*implicit*/ derefnullchecked(P const& ptr) noexcept(std::is_nothrow_copy_constructible_v<P>) : m_ptr(ptr) {}
                inline constexpr derefnullchecked(derefnullchecked&&) noexcept(std::is_nothrow_move_constructible_v<P>) = default;
                inline constexpr derefnullchecked(derefnullchecked const&) noexcept(std::is_nothrow_copy_constructible_v<P>) = default;
                template<class...CArgs>
                inline constexpr explicit derefnullchecked(std::in_place_t, CArgs&&...args) noexcept(std::is_nothrow_constructible_v<P, CArgs&&...>)
                    : m_ptr(std::forward<CArgs>(args)...)
                {
                }
                inline constexpr explicit derefnullchecked(notnull<P> const& other) noexcept(std::is_nothrow_copy_constructible_v<P>)
                    : m_ptr(other.as_nullable())
                {
                }
                inline constexpr derefnullchecked& operator=(std::nullptr_t nullp) noexcept(std::is_nothrow_assignable_v<P, std::nullptr_t&&>) { m_ptr = std::move(nullp); return *this; }
                inline constexpr derefnullchecked& operator=(derefnullchecked&&) noexcept(std::is_nothrow_move_assignable_v<P>) = default;
                inline constexpr derefnullchecked& operator=(derefnullchecked const&) noexcept(std::is_nothrow_copy_assignable_v<P>) = default;
                inline constexpr derefnullchecked& operator=(P ptr) noexcept(std::is_nothrow_swappable_v<P>) {
                    using std::swap;
                    swap(m_ptr, ptr);
                    return *this;
                }
                inline constexpr void swap(derefnullchecked& other) noexcept(std::is_nothrow_swappable_v<P>) {
                    using std::swap;
                    swap(m_ptr, other.m_ptr);
                }
                inline constexpr void swap(P& other) noexcept(std::is_nothrow_swappable_v<P>) {
                    using std::swap;
                    swap(m_ptr, other);
                }
                inline constexpr P const& ptr() const noexcept { return m_ptr; }
                inline constexpr P& ptr() noexcept { return m_ptr; }
                inline constexpr /*implicit*/ operator P const& () const noexcept { return m_ptr; }
                inline constexpr explicit operator bool() const noexcept(noexcept(static_cast<bool>(m_ptr))) {
                    return static_cast<bool>(m_ptr);
                }
                inline constexpr bool operator!() const noexcept(noexcept(!m_ptr)) {
                    return !m_ptr;
                }
                inline constexpr decltype(auto) operator*() const {
                    if (!operator bool()) throw nullptr_error();
                    return *m_ptr;
                }
                inline constexpr decltype(auto) operator*() {
                    if (!operator bool()) throw nullptr_error();
                    return *m_ptr;
                }
                inline constexpr auto const& operator->() const {
                    if (!operator bool()) throw nullptr_error();
                    return m_ptr;
                }
                inline constexpr auto& operator->() {
                    if (!operator bool()) throw nullptr_error();
                    return m_ptr;
                }
                //inline constexpr auto operator<=>(derefnullchecked const&) const = default;
                //inline constexpr auto operator<=>(P const& ptr) const { return m_ptr <=> ptr; }
                //inline constexpr auto operator<=>(notnull<P> const& ptr) const { return m_ptr <=> ptr; }
                //inline constexpr auto operator<=>(std::nullptr_t const& nullp) const { return m_ptr <=> nullp; }
        };

        template<class P>
        inline constexpr void swap(derefnullchecked<P>& lhs, derefnullchecked<P>& rhs) noexcept {
            using std::swap;
            swap(lhs.ptr(), rhs.ptr());
        }
        template<class P>
        inline constexpr void swap(derefnullchecked<P>& lhs, P& rhs) noexcept {
            using std::swap;
            swap(lhs.ptr(), rhs);
        }
        template<class P>
        inline constexpr void swap(P& lhs, derefnullchecked<P>& rhs) noexcept {
            using std::swap;
            swap(lhs, rhs.ptr());
        }

        // returns the pointer unchanged, or throws nullptr_error if the pointer is null (falsy).
        template<class P> inline constexpr decltype(auto) throw_if_null(P&& ptr) { if (!ptr) throw nullptr_error(); return std::forward<P>(ptr); }

        // returns the pointer unchanged, or throws nullptr_error if the pointer is null (falsy).
        template<class P> inline constexpr decltype(auto) throw_if_null(notnull<P> const& ptr) noexcept { return std::forward<notnull<P> const&>(ptr); }

        // returns the pointer unchanged, or throws nullptr_error if the pointer is null (falsy).
        template<class P> inline constexpr decltype(auto) throw_if_null(notnull<P>&& ptr) noexcept { return std::forward<notnull<P>&&>(ptr); }



        template<class P, size_t E>
        inline constexpr std::span<notnull<P>, E> as_span_of_notnull(std::span<P, E> const& span)
            requires (sizeof(P) == sizeof(notnull<P>)) && (alignof(P) == alignof(notnull<P>))
        && (!std::is_volatile_v<P>)
        {
            if (std::all_of(std::cbegin(span), std::cend(span), std::identity{})) {
                return std::span<notnull<P>, E>(reinterpret_cast<notnull<P>*>(span.data()), span.size());
            }
            throw nullptr_error();
        }
        template<class P, size_t E>
        inline constexpr std::span<notnull<P> const, E> as_span_of_notnull(std::span<P const, E> const& span)
            requires (sizeof(P const) == sizeof(notnull<P> const)) && (alignof(P const) == alignof(notnull<P> const))
        && (!std::is_volatile_v<P>)
        {
            if (std::all_of(std::cbegin(span), std::cend(span), std::identity{})) {
                return std::span<notnull<P> const, E>(reinterpret_cast<notnull<P> const*>(span.data()), span.size());
            }
            throw nullptr_error();
        }
        template<class P, size_t E>
        inline constexpr std::span<notnull<P>, E> as_span_of_notnull(std::span<notnull<P>, E> const& span) noexcept
        {
            return span;
        }

        template<class P, size_t E>
        inline constexpr std::span<derefnullchecked<P>, E> as_span_of_derefnullchecked(std::span<P, E> const& span)
            requires (sizeof(P) == sizeof(derefnullchecked<P>)) && (alignof(P) == alignof(derefnullchecked<P>))
        && (!std::is_volatile_v<P>)
        {
            return std::span<derefnullchecked<P>, E>(reinterpret_cast<derefnullchecked<P>*>(span.data()), span.size());
        }
        template<class P, size_t E>
        inline constexpr std::span<derefnullchecked<P> const, E> as_span_of_derefnullchecked(std::span<P const, E> const& span)
            requires (sizeof(P const) == sizeof(derefnullchecked<P> const)) && (alignof(P const) == alignof(derefnullchecked<P> const))
        && (!std::is_volatile_v<P>)
        {
            return std::span<derefnullchecked<P> const, E>(reinterpret_cast<derefnullchecked<P> const*>(span.data()), span.size());
        }
        template<class P, size_t E>
        inline constexpr std::span<derefnullchecked<P>, E> as_span_of_derefnullchecked(std::span<derefnullchecked<P>, E> const& span) noexcept
        {
            return span;
        }

    }
}

#endif //~ HNG_NULLSAFETY_HEADERGUARD