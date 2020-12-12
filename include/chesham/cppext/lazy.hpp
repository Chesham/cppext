#pragma once
#include <chrono>
#include <future>
#include <functional>

namespace chesham
{
    namespace cppext
    {
        template<class T>
        class lazy final
        {
        public:
            using element_type = T;
            using value_function = std::function<void(std::promise<T>&)>;
        private:
            mutable std::promise<T> prom;
            std::shared_future<T> fut{ prom.get_future() };
            value_function fn;
        public:
            lazy() = default;
            lazy(const value_function& fn) :
                fn(fn)
            {
            }
            lazy(const lazy&) = delete;
            lazy& operator=(const lazy&) = delete;
            lazy(lazy&&) = default;
            lazy& operator=(lazy&&) = default;
            lazy& set_value_function(const value_function& fn)
            {
                this->fn = fn;
                return *this;
            }
            operator const T& () const
            {
                if (fut.wait_for(std::chrono::milliseconds(0)) != std::future_status::ready)
                {
                    if (fn)
                        fn(prom);
                    else
                        prom.set_exception(std::make_exception_ptr(std::runtime_error("invalid value function")));
                }
                return fut.get();
            }
            const T& get() const
            {
                return *this;
            }
        };
    }
}