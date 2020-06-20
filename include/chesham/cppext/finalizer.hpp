#pragma once
#include <functional>

namespace chesham
{
    namespace cppext
    {
        template<typename ...Args>
        class finalizer final
        {
            using delegate_type = std::function<void(Args...)>;
            std::function<void()> f;
        public:
            finalizer() = default;
            finalizer(const finalizer&) = delete;
            finalizer(finalizer&&) = delete;
            finalizer& operator=(const finalizer&) = delete;
            finalizer& operator=(finalizer&&) = delete;
            finalizer(delegate_type&& f) :
                f(std::move(std::forward<decltype(f)>(f)))
            {
            }
            void operator()(delegate_type&& f, Args ...args)
            {
                if (!f && this->f)
                {
                    this->f.swap(decltype(this->f){});
                    return;
                }
                this->f = std::bind(f, args...);
            }
            ~finalizer()
            {
                try
                {
                    if (!f)
                        return;
                    f();
                }
                catch (const std::exception&)
                {
                }
            }
        };
    }
}