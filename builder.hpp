#ifndef _H_TMP_BUILDER_PATTERN__
#define _H_TMP_BUILDER_PATTERN__

#include <type_traits>
#include <experimental/type_traits>
#include <utility>

namespace tmp_builder {

namespace detail {
    template<auto p>
    struct ReadMemPtr {
        static_assert(p!=p,"not a member pointer");
    };

    template<typename Class, typename Result, Result Class::* ptr>
    struct ReadMemPtr<ptr> {
        static constexpr auto value = ptr;
        using type = Result;
        using containing_type = Class;
    };

    template<auto p>
    using memPtrType = typename ReadMemPtr<p>::type;
    
    template<auto,auto>
    struct isEqualMemPtr {
        static constexpr auto value = false;
    };

    template<typename Class, typename Result, Result Class::* ptr, Result Class::* ptr2>
    struct isEqualMemPtr<ptr,ptr2> {
        static constexpr auto value = (ptr == ptr2);
    };

    template<auto ptr, auto ptr2>
    constexpr bool isEqualMemPtr_v = isEqualMemPtr<ptr,ptr2>::value;

    template<auto ptr>
    struct canApplyMemPtr {
        template<typename T>
        using check = decltype(std::declval<T>().*ptr);
    };

}

template<typename T,auto...>
class Builder;

template<typename T>
class Builder<T> {
    T value;
    template<typename,auto...>
    friend class Builder;
    template<auto... newptrs>
    using addInFront = Builder<T,newptrs...>;
    constexpr T& getValue() {return value;}
    public:
        constexpr Builder() = default;
        constexpr Builder(const Builder&) = default;
        constexpr Builder(Builder&&) = default;
        constexpr explicit Builder(const T& t) : value(t) {};
        constexpr explicit Builder(T&& t) : value(std::move(t)) {};
        constexpr T& get() & {
            return value;
        }
        constexpr T&& get() && {
            return std::move(value);
        }
        template<auto p>
        constexpr Builder<T>& set(const detail::memPtrType<p>& v) & {
            value.*p = v;
            return *this;
        }
        template<auto p>
        constexpr Builder<T>& set(detail::memPtrType<p>&& v) & {
            value.*p = std::move(v);
            return *this;
        }
        template<auto p>
        constexpr Builder<T>&& set(const detail::memPtrType<p>& v) && {
            value.*p = v;
            return std::move(*this);
        }
        template<auto p>
        constexpr Builder<T>&& set(detail::memPtrType<p>&& v) && {
            value.*p = std::move(v);
            return std::move(*this);
        }
};

template<typename T, auto ptr, auto... ptrs>
class Builder<T,ptr,ptrs...> : private Builder<T,ptrs...> {
    using parent = Builder<T,ptrs...>;
    using info = detail::ReadMemPtr<ptr>;
    using infoT = std::remove_reference_t<typename info::type>;
    static_assert(std::experimental::is_detected_v<detail::canApplyMemPtr<ptr>::template check,T>, "mismatching member pointer types");
    
    template<typename,auto...>
    friend class Builder;
    
    template<auto... newptrs>
    using addInFront = Builder<T,newptrs...,ptr,ptrs...>;

    constexpr Builder(const parent& t) : parent(t) {};
    constexpr Builder(parent&& t) : parent(std::move(t)) {};

    template<auto p, typename Val>
    [[nodiscard]] constexpr auto set_nested(Val&& v) & {
        using returnType = std::remove_reference_t<decltype(std::declval<parent>().template set<p>(std::forward<Val>(v)))>;
        return typename returnType::template addInFront<ptr>(static_cast<parent>(*this).template set<p>(std::forward<Val>(v)));
    }
    template<auto p, typename Val>
    [[nodiscard]] constexpr auto set_nested(Val&& v) && {
        using returnType = std::remove_reference_t<decltype(std::declval<parent&&>().template set<p>(std::forward<Val>(v)))>;
        return typename returnType::template addInFront<ptr>{static_cast<parent&&>(std::move(*this)).template set<p>(std::forward<Val>(v))};
    }

    public:
        constexpr Builder() = default;
        constexpr Builder(const Builder&) = default;
        constexpr Builder(Builder&&) = default;
        constexpr explicit Builder(const T& t) : parent(t) {};
        constexpr explicit Builder(T&& t) : parent(std::move(t)) {};

        template<int check = 1>
        constexpr T& get() {
            static_assert(check != 1, "Can't get from incompleted build");
        }
        
        //Simple case: the member set is the same as this Builder's, return parent with changed value.
        template<auto p, typename = std::enable_if_t<detail::isEqualMemPtr_v<p,ptr>>>
        [[nodiscard]] constexpr parent& set(const infoT& val) & {
            this->getValue().*ptr = val;
            return *this;
        }
        template<auto p, typename = std::enable_if_t<detail::isEqualMemPtr_v<p,ptr>>>
        [[nodiscard]] constexpr parent& set(infoT&& val) & {
            this->getValue().*ptr = std::move(val);
            return *this;
        }
        template<auto p, typename = std::enable_if_t<detail::isEqualMemPtr_v<p,ptr>>>
        [[nodiscard]] constexpr parent&& set(const infoT& val) && {
            this->getValue().*ptr = val;
            return std::move(*this);
        }
        template<auto p, typename = std::enable_if_t<detail::isEqualMemPtr_v<p,ptr>>>
        [[nodiscard]] constexpr parent&& set(infoT&& val) && {
            this->getValue().*ptr = std::move(val);
            return std::move(*this);
        }

        //More difficult case: the member set is not ours, push set through in case it is a required parameter
        template<auto p, typename = std::enable_if_t<!detail::isEqualMemPtr_v<p,ptr>>>
        [[nodiscard]] constexpr auto set(const detail::memPtrType<p>& v) & {
            return set_nested<p>(v);
        }
        template<auto p, typename = std::enable_if_t<!detail::isEqualMemPtr_v<p,ptr>>>
        [[nodiscard]] constexpr auto set(detail::memPtrType<p>&& v) & {
            return set_nested<p>(std::move(v));
        }
        template<auto p, typename = std::enable_if_t<!detail::isEqualMemPtr_v<p,ptr>>>
        [[nodiscard]] constexpr auto set(const detail::memPtrType<p>& v) && {
            return std::move(*this).template set_nested<p>(v);
        }
        template<auto p, typename = std::enable_if_t<!detail::isEqualMemPtr_v<p,ptr>>>
        [[nodiscard]] constexpr auto set(detail::memPtrType<p>&& v) && {
            return std::move(*this).template set_nested<p>(std::move(v));
        }
};
}

#endif
