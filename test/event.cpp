#include "pch.h"
#include <future>
using namespace std;
using namespace Microsoft::VisualStudio::CppUnitTestFramework;
using namespace chesham;

namespace test
{
    TEST_CLASS(event_tests)
    {
    public:

        struct my_event_args : public cppext::event_args
        {
            bool isCancelled{ false };
        };

        struct subject : public cppext::subject<cppext::event_args>
        {
            typedef cppext::subject<cppext::event_args> base;

            void notify()
            {
                base::notify(event_args_type());
            }

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

        TEST_METHOD(event_invoked)
        {
            subject sub;
            auto isEventInvoked = false;
            auto e = make_shared<decltype(sub)::event_type>([&](...) { isEventInvoked = true; });
            sub += e;
            sub.notify();
            Assert::IsTrue(isEventInvoked);
        }

        TEST_METHOD(event_auto_unsub)
        {
            subject sub;
            auto isEventInvoked = false;
            auto e = make_shared<decltype(sub)::event_type>([&](...) { isEventInvoked = true; });
            sub += e;
            e.reset();
            sub.notify();
            Assert::IsFalse(isEventInvoked);
        }

        TEST_METHOD(managed_subscriber)
        {
            subject sub;
            auto isEventInvoked = false;
            auto e = sub += [&](...) { isEventInvoked = true; };
            sub.notify();
            Assert::IsTrue(isEventInvoked);
        }

        TEST_METHOD(managed_subscriber_auto_unsub)
        {
            subject sub;
            auto isEventInvoked = false;
            auto e = sub += [&](...) { isEventInvoked = true; };
            e.reset();
            sub.notify();
            Assert::IsFalse(isEventInvoked);
        }

        TEST_METHOD(managed_subscriber_can_exit_safely)
        {
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
        }

        TEST_METHOD(deal_with_custom_event_args)
        {
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
        }
    };
}
