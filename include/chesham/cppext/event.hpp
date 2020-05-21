#pragma once
#include <functional>
#include <memory>
#include <mutex>
#include <vector>
#include <thread>

namespace chesham
{
    namespace cppext
    {
        struct event_args
        {
            virtual ~event_args() { }
        };

        template<typename T>
        class subject;

        template<typename T>
        class observer final
        {
            typedef subject<T> handler_type;

            typedef typename handler_type::event_type event_type;

            template<typename T>
            friend class subject;

            std::shared_ptr<event_type> event;

            observer(std::shared_ptr<event_type>&& event) :
                event(std::move(event))
            {
            }

        public:

            observer() { }

            void subscribe(handler_type& h, const event_type& e)
            {
                event = std::make_shared<event_type>(e);
                h += event;
            }

            ~observer()
            {
                std::weak_ptr<event_type> w(event);
                event.reset();
                while (!w.expired())
                    this_thread::sleep_for(10ms);
            }
        };

        template<typename T>
        class subject
        {
        public:

            typedef T event_args_type;

            typedef std::function<void(subject*, event_args_type&)> event_type;

        protected:

            std::mutex mtx;

            std::vector<std::weak_ptr<event_type>> subs;

            virtual void notify(event_args_type& args, bool stopWhileError = false)
            {
                decltype(subs) subs;
                {
                    std::lock_guard<std::mutex> l(mtx);
                    subs = this->subs;
                }
                for (auto& i : subs)
                {
                    auto e = i.lock();
                    if (!e)
                        continue;
                    try
                    {
                        (*e)(this, args);
                    }
                    catch (const std::exception&)
                    {
                        if (stopWhileError)
                            throw;
                    }
                }
            }

        public:

            virtual ~subject() { }

            virtual void operator +=(const std::shared_ptr<event_type>& e)
            {
                std::lock_guard<std::mutex> l(mtx);
                subs.emplace_back(e);
            }

            virtual std::shared_ptr<observer<T>> operator +=(const event_type& e)
            {
                std::lock_guard<std::mutex> l(mtx);
                auto managed = std::make_shared<event_type>(e);
                subs.emplace_back(managed);
                return std::shared_ptr<observer<T>>(new observer<T>(std::move(managed)));
            }

            virtual void operator -=(const std::shared_ptr<event_type>& e)
            {
                std::lock_guard<std::mutex> l(mtx);
                for (auto it = subs.begin(); it != subs.end(); )
                {
                    auto t = it->lock();
                    if (t)
                    {
                        if (t == e)
                        {
                            subs.erase(it);
                            return;
                        }
                        else
                            ++it;
                    }
                    else
                        it = subs.erase(it);
                }
            }
        };
    }
}
