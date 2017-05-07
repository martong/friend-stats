//============================================================================
//befriending class: boost::wave::cpplexer::re2clex::uchar_wrapper
//friendly function: boost::wave::cpplexer::re2clex::operator-
//friendDeclLoc: libs/wave/src/cpplexer/re2clex/cpp_re.cpp:397:5
//defLoc: libs/wave/src/cpplexer/re2clex/cpp_re.cpp:397:5
//diagName: boost::wave::cpplexer::re2clex::operator-
//usedPrivateVarsCount: 0
//parentPrivateVarsCount: 0
//usedPrivateMethodsCount: 0
//parentPrivateMethodsCount: 0
//types.usedPrivateCount: 0
//types.parentPrivateCount: 0
//============================================================================

// Befreinding class definition
struct uchar_wrapper
{
    //... // there is no any access specifier


    // Friend function definition
    friend std::ptrdiff_t
    operator- (uchar_wrapper const& lhs, uchar_wrapper const& rhs)
    {
        return lhs.base_cursor - rhs.base_cursor;
    }

    //...
};

