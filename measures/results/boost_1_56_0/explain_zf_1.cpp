//============================================================================
//befriending class: boost::thread::id
//friendly function: boost::this_thread::get_id
//friendDeclLoc: ./boost/thread/detail/thread.hpp:652:50
//defLoc: ./boost/thread/detail/thread.hpp:747:27
//diagName: boost::this_thread::get_id
//usedPrivateVarsCount: 0
//parentPrivateVarsCount: 1
//usedPrivateMethodsCount: 0
//parentPrivateMethodsCount: 1
//types.usedPrivateCount: 0
//types.parentPrivateCount: 1
//============================================================================

// Befreinding class defintion:

    class BOOST_SYMBOL_VISIBLE thread::id
    {
        //...
    private:
        id(data thread_data_):
            thread_data(thread_data_)
        {}
        friend id BOOST_THREAD_DECL this_thread::get_id() BOOST_NOEXCEPT;


// Friend function definition:

    namespace this_thread
    {
        inline thread::id get_id() BOOST_NOEXCEPT
        {
        #if defined BOOST_THREAD_PROVIDES_BASIC_THREAD_ID
             return pthread_self();
        #else
            boost::detail::thread_data_base* const thread_info=get_or_make_current_thread_data();
            return (thread_info?thread::id(thread_info->shared_from_this()):thread::id());
        #endif
        }
    }

// Once the BOOST_THREAD_PROVIDES_BASIC_THREAD_ID flag is defined, than the function
// does not use any private entity!!!

