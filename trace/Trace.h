#pragma once

#include <time/DeltaTimeFW.h>
#include <trace/Bit.h>

#include <boost/smart_ptr.hpp>
#include <boost/function.hpp>

namespace svt {
  class TraceFrame;
  struct TraceFrameCurser;

/**
 * A type for specifying how Trace::set should behave.
 **/
enum TraceChangeMode {
  /**
   * Just set the current value, do not change anything else (no merging)
   **/
  TRACE_NO_CHANGE = 0,

  /**
   * If the write has the same value as the previous checkpoint, merge the
   * write with the previous checkpoint.
   * e.g. {1@1}, set(1,@2) -> {1@1}, i.e. no change
   **/
  TRACE_MERGE_EARLIER = 1,

  /**
   * If the write has the same value as the next checkpoint, merge with the
   * next checkpoint.
   * e.g. 1@3, set(1,@2) -> {1@2}
   **/
  TRACE_MERGE_LATER = 2,

  /**
   * The combination of TRACE_MERGE_EARLIER and TRACE_MERGE_LATER
   **/
  TRACE_MERGE_BOTH = 3,

  /**
   * Remove all checkpoints after the current write.
   *
   * This mode can not be used together with TRACE_KEEP_FUTURE_CYCLE.
   **/
  TRACE_CLEAR_FUTURE = 4,


  /**
   * Change the current time point, but keep the values in the next cycle.
   * given value@t == 1
   * set (value@t = 0+0) -> value@t + 1+0 == 1 (oldvalue) and value@t == 0
   *
   * This mode can not be used together with TRACE_CLEAR_FUTURE.
   */
  TRACE_KEEP_FUTURE_CYCLE = 8,
};

struct TraceFrameCurser {
  unsigned frame;
  unsigned pos;
};

/**
 * @brief represents trace data for a single signal over time
 *
 */
class Trace
{
  public: // typedef
    class const_iterator {
      public:
        const_iterator & operator++();
        bool operator==( const_iterator const& ) const;
        bool operator!=( const_iterator const& ) const;

        typedef std::pair< DeltaTimeFW const&, Bit const& > value_type;
        value_type operator*() const;

        DeltaTimeFW const& time() const;
        Bit const& value() const;

      private:
        const_iterator(std::vector<TraceFrame*> const &);


      private:
        TraceFrameCurser _curser;
        std::vector<TraceFrame*> const & _frames;

        friend class Trace;
    };

  public:
  Trace (const Bit& initvalue);
  ~Trace ();

  const_iterator begin() const;
  const_iterator end() const;

  bool changed (const DeltaTime& time) const;

  DeltaTime checkpoint(const DeltaTime& time) const;
  std::vector<DeltaTimeFW> computeCheckpoints() const;

  DeltaTimeFW firstCheckpoint() const;
  DeltaTimeFW lastCheckpoint() const;

  bool hasCheckpoints() const;
  std::size_t numberOfCheckpoints() const;
  std::size_t capacity() const;

  boost::optional<DeltaTimeFW> prevCheckpoint( DeltaTimeFW const& baseTime ) const;
  boost::optional<DeltaTimeFW> nextCheckpoint( DeltaTimeFW const& baseTime ) const;

  Bit get(const DeltaTimeFW& t) const;

  void set(const Bit & assign, const DeltaTimeFW& time);

  void set(const Bit & assign, const DeltaTimeFW& time, TraceChangeMode const changeMode);

  void setRange(Bit const value, DeltaTimeFW const& beginT, DeltaTimeFW const& endT);

  /**
     removes all values from trace
   */
  void clear();

  /**
    * remove all non-delta values and store only the endOfCycle
    **/
  void removeDeltaCycles();


  void add_ref();
  bool release();

  unsigned numberOfReferences();

  /**
   * will raise an assertion if the Trace contains inconsistent data.
   **/
  void check_consistency() const;

  Bit getInitvalue() const { return _initvalue; }
  void setInitvalue( Bit const& initvalue);

  boost::intrusive_ptr<Trace> clone() const;
  /**
    * copy trace while time <= upper_bound
    **/
  boost::intrusive_ptr<Trace> clone(DeltaTime const& upper_bound) const;

  private:
  //disabled
  Trace (const Trace& other);
  Trace& operator= (const Trace& other);

  void _handle_changes(TraceFrameCurser const& curser
    , TraceChangeMode const changeMode
    , DeltaTimeFW const& atime
    , Bit curVal
  );

  unsigned _numberOfReferences;
  Bit _initvalue;
 protected:

  std::vector<TraceFrame*> _frames;

  /**
   * compare_traces: similiar to operator==, but additionally logs all changes in the format
   *    log(time, aValue, bValue)
   **/
  friend bool compare_traces(
      Trace const & a
    , Trace const & b
    , boost::function<void (DeltaTime, Bit, Bit)> log
  );
  friend bool operator== ( Trace const & a, Trace const & b );
  friend bool operator!= ( Trace const & a, Trace const & b ) { return !(a==b); }

};

/**
 * A memory managed version of Trace.
 * Reference counting is automatically applied
 **/
typedef boost::intrusive_ptr<Trace> TracePtr;

inline void intrusive_ptr_add_ref (Trace* t) {
    t->add_ref();
}

inline void intrusive_ptr_release (Trace* t) {
  if (t->release()) {
    delete t;
  }
}


} // namespace svt
