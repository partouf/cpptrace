#ifndef RESULT_HPP
#define RESULT_HPP

#include <new>
#include <type_traits>
#include <utility>

#include "utils/common.hpp"
#include "utils/error.hpp"
#include "utils/optional.hpp"

namespace cpptrace {
namespace detail {
    template<typename T, typename E, typename std::enable_if<!std::is_same<T, E>::value, int>::type = 0>
    class Result {
        union {
            T value_;
            E error_;
        };
        enum class member { value, error };
        member active;
    public:
        Result(T&& value) : value_(std::move(value)), active(member::value) {}
        Result(E&& error) : error_(std::move(error)), active(member::error) {
            if(!absorb_trace_exceptions.load()) {
                std::fprintf(stderr, "%s\n", unwrap_error().what());
            }
        }
        Result(T& value) : value_(T(value)), active(member::value) {}
        Result(E& error) : error_(E(error)), active(member::error) {
            if(!absorb_trace_exceptions.load()) {
                std::fprintf(stderr, "%s\n", unwrap_error().what());
            }
        }
        Result(Result&& other) : active(other.active) {
            if(other.active == member::value) {
                new (&value_) T(std::move(other.value_));
            } else {
                new (&error_) E(std::move(other.error_));
            }
        }
        ~Result() {
            if(active == member::value) {
                value_.~T();
            } else {
                error_.~E();
            }
        }

        bool has_value() const {
            return active == member::value;
        }

        bool is_error() const {
            return active == member::error;
        }

        explicit operator bool() const {
            return has_value();
        }

        NODISCARD optional<T> value() const & {
            return has_value() ? value_ : nullopt;
        }

        NODISCARD optional<E> error() const & {
            return is_error() ? error_ : nullopt;
        }

        NODISCARD optional<T> value() && {
            return has_value() ? std::move(value_) : nullopt;
        }

        NODISCARD optional<E> error() && {
            return is_error() ? std::move(error_) : nullopt;
        }

        NODISCARD T& unwrap_value() & {
            ASSERT(has_value(), "Result does not contain a value");
            return value_;
        }

        NODISCARD const T& unwrap_value() const & {
            ASSERT(has_value(), "Result does not contain a value");
            return value_;
        }

        NODISCARD T unwrap_value() && {
            ASSERT(has_value(), "Result does not contain a value");
            return std::move(value_);
        }

        NODISCARD E& unwrap_error() & {
            ASSERT(is_error(), "Result does not contain an error");
            return error_;
        }

        NODISCARD const E& unwrap_error() const & {
            ASSERT(is_error(), "Result does not contain an error");
            return error_;
        }

        NODISCARD E unwrap_error() && {
            ASSERT(is_error(), "Result does not contain an error");
            return std::move(error_);
        }

        template<typename U>
        NODISCARD T value_or(U&& default_value) const & {
            return has_value() ? value_ : static_cast<T>(std::forward<U>(default_value));
        }

        template<typename U>
        NODISCARD T value_or(U&& default_value) && {
            return has_value() ? std::move(value_) : static_cast<T>(std::forward<U>(default_value));
        }

        void drop_error() const {
            if(is_error()) {
                std::fprintf(stderr, "%s\n", unwrap_error().what());
            }
        }
    };

    struct monostate {};
}
}

#endif
