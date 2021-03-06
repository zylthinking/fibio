//
//  fiber.hpp
//  fibio
//
//  Created by Chen Xu on 14-3-1.
//  Copyright (c) 2014 0d0a.com. All rights reserved.
//

#ifndef __fibio__fiber__
#define __fibio__fiber__

#include <memory>
#include <functional>
#include <chrono>
#include <utility>
#include <type_traits>
#include <asio/io_service.hpp>
#include <fibio/fibers/detail/forward.hpp>
#include <fibio/fibers/detail/make_tuple_indices.hpp>

namespace fibio { namespace fibers {
    namespace detail {
        using fibio::detail::wrap;
    }
    struct scheduler {
        scheduler();
        scheduler(std::shared_ptr<detail::scheduler_object>);
        
        void start(size_t nthr=1);
        void join();
        
        // FIXME: It doesn't work correctly
        // void add_worker_thread(size_t nthr=1);
        
        static scheduler get_instance();
        static void reset_instance();
        
        void attach_std_stream();
        void restore_std_stream();
        
        std::shared_ptr<detail::scheduler_object> m_;
    };
    
    struct fiber {
        typedef uintptr_t id;
        
        explicit fiber(fiber &&other) noexcept;
        explicit fiber(scheduler &s, std::function<void()> &&f);

#ifdef NO_VARIADIC_TEMPLATE
        fiber(std::function<void()> &&f);
#else
        template<typename Fn, typename... Args>
        explicit fiber(Fn &&fn, Args&&... args) {
            start(detail::wrap(std::forward<Fn>(fn), std::forward<Args>(args)...));
        }
        void start(std::function<void()> &&f);
#endif
 
        fiber() = default;
        fiber(const fiber&) = delete;
        
        bool joinable() const;
        id get_id() const;
        void join();
        void detach();
        void swap(fiber &other) noexcept(true);
        
        static unsigned hardware_concurrency();

        void set_name(const std::string &s);
        std::string get_name();
      
        std::shared_ptr<detail::fiber_object> m_;
    };
    
    constexpr fiber::id not_a_fiber=0;
    
    namespace this_fiber {
        namespace detail {
            void sleep_usec(uint64_t usec);
            asio::io_service &get_io_service();
        }
        
        void yield();
        
        fiber::id get_id();
        
        bool is_a_fiber() noexcept(true);
        
        template< class Rep, class Period >
        void sleep_for( const std::chrono::duration<Rep,Period>& sleep_duration ) {
            detail::sleep_usec(std::chrono::duration_cast<std::chrono::microseconds>(sleep_duration).count());
        }
        
        template< class Clock, class Duration >
        void sleep_until( const std::chrono::time_point<Clock,Duration>& sleep_time ) {
            detail::sleep_usec(std::chrono::duration_cast<std::chrono::microseconds>(sleep_time - std::chrono::system_clock::now()).count());
        }
    }   // End of namespace this_fiber
}}   // End of namespace fibio::fibers

namespace std {
    inline void swap(fibio::fibers::fiber &lhs, fibio::fibers::fiber &rhs) noexcept(true) {
        lhs.swap(rhs);
    }
}   // End of namespace std

namespace fibio {
    using fibers::scheduler;
    using fibers::fiber;
    namespace this_fiber {
        using fibers::this_fiber::yield;
        using fibers::this_fiber::get_id;
        using fibers::this_fiber::is_a_fiber;
        using fibers::this_fiber::sleep_for;
        using fibers::this_fiber::sleep_until;
    }
}   // End of namespace fibio

#endif /* defined(__fibio__fiber__) */
