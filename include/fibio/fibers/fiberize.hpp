//
//  fiberize.hpp
//  fibio
//
//  Created by Chen Xu on 14-3-8.
//  Copyright (c) 2014 0d0a.com. All rights reserved.
//

#ifndef fibio_fiberize_hpp
#define fibio_fiberize_hpp

#include <type_traits>
#include <fibio/fibers/fiber.hpp>
#include <fibio/stream/basic_streambuf.hpp>

namespace fibio { namespace fibers {
    namespace detail {
        struct fiberized_std_stream_guard {
            typedef stream::basic_fibio_streambuf<io::posix::stream_descriptor> sbuf_t;
            typedef sbuf_t *sbuf_ptr_t;
            
            fiberized_std_stream_guard(asio::io_service &iosvc);
            ~fiberized_std_stream_guard();
            
            void open();
            
            std::streambuf *old_cin_buf_;
            std::streambuf *old_cout_buf_;
            std::streambuf *old_cerr_buf_;
            sbuf_ptr_t cin_buf_;
            sbuf_ptr_t cout_buf_;
            sbuf_ptr_t cerr_buf_;
        };
        
        typedef std::shared_ptr<fiberized_std_stream_guard> fiberized_std_stream_guard_ptr_t;
    }   // End of namespace fibio::fibers::detail
    
#ifdef __clang__
    // Doesn't compile under GCC 4.8.1?
    namespace detail {
        template<typename T>
        struct to_int_if_void {
            typedef T type;
            inline static void assign(T &lhs, T&&rhs) { lhs=std::move(rhs); }
            inline static void assign(T &lhs, const T& rhs) { lhs=rhs; }
        };
        
        template<>
        struct to_int_if_void<void> {
            typedef int type;
            inline static void assign(type &lhs, ...) { lhs=0; }
        };
    }   // End of namespace fibio::fibers::detail
    
    template<typename Fn, typename ...Args,
        typename Ret=typename detail::to_int_if_void<typename std::result_of<Fn(Args...)>::type>::type
    >
    typename std::result_of<Fn(Args...)>::type fiberize(size_t nthr, Fn &&fn, Args&& ...args) {
        typedef typename std::result_of<Fn(Args...)>::type ActualRet;
        Ret ret;
        try {
            fibio::scheduler sched=fibio::scheduler::get_instance();
            sched.start(nthr);
            fibio::fiber f([&](){
                detail::fiberized_std_stream_guard guard(this_fiber::detail::get_io_service());
                detail::to_int_if_void<ActualRet>::assign(ret, fn(args...));
            });
            sched.join();
        } catch (std::exception& e) {
            std::cerr << "Exception: " << e.what() << "\n";
        }
        fibio::scheduler::reset_instance();
        return (ActualRet)(ret);
    }
#else
    inline int fiberize(size_t nthr, std::function<int(int, char *[])> &&fn, int argc, char *argv[]) {
        int ret;
        try {
            fibio::scheduler sched=fibio::scheduler::get_instance();
            sched.start(nthr);
            fibio::fiber f([&](){
                detail::fiberized_std_stream_guard guard(this_fiber::detail::get_io_service());
                ret=fn(argc, argv);
            });
            sched.join();
        } catch (std::exception& e) {
            std::cerr << "Exception: " << e.what() << "\n";
        }
        fibio::scheduler::reset_instance();
        return ret;
    }
#endif
}}  // End of namespace fibio::fibers

namespace fibio {
    using fibers::fiberize;
}

#endif
