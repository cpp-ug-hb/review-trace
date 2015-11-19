#pragma once

#include "Bit.h"

#include <boost/array.hpp>

namespace svt
{

const unsigned TraceFrameSize = 32;
struct TraceFrameCurser;

class TraceFrame
{
public:
    TraceFrame();
    TraceFrame(const DeltaTimeFW& leader);
    TraceFrame(const DeltaTimeFW& time, Bit const & value);
    ~TraceFrame();

    DeltaTimeFW leader() const;
    DeltaTimeFW closer() const;

    bool full() const;

    bool set( const DeltaTimeFW& t, const Bit& value);

    TraceFrame* split(const DeltaTimeFW& t);

    // Access funtions for cursers
    DeltaTimeFW&        time_at ( size_t pos);
    DeltaTimeFW const&  time_at ( size_t pos) const;
    Bit&              bit_at  ( size_t pos);
    Bit const&        bit_at  ( size_t pos) const;

    unsigned num_used() const;
    bool empty() const;


    // iterators
    typedef const DeltaTimeFW* const_iterator;

    const DeltaTimeFW* begin() const;
    const DeltaTimeFW* end() const;

    void reset(const DeltaTimeFW& leader = DeltaTimeFW(0,0));
    void truncate(unsigned maxLength);
    void erase(size_t pos);
    void insert(size_t pos, DeltaTimeFW const& t, Bit const& value);

    // output
    friend std::ostream & operator<< (std::ostream & o, TraceFrame const & );

private:
    DeltaTimeFW _leader;
    unsigned _used;
    boost::array<DeltaTimeFW, TraceFrameSize> _times;
    boost::array<Bit, TraceFrameSize>  _values;

};

} //namespace svt
