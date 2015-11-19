#include "Trace.h"

#include <trace/TraceFrameImpl.h>

#include <boost/optional.hpp>
#include <boost/lambda/construct.hpp>
#include <boost/foreach.hpp>

namespace svt {

////////////////////////////////////////////////////////////


std::ostream & operator<< ( std::ostream & out, TraceFrameCurser const & curser) {
  out << '[' << curser.frame << ' ' << curser.pos << ']';
  return out;
}

typedef std::vector<TraceFrame*> FrameSeq;

/**
 * move the Curser to the next entry
 **/
void move_forward(TraceFrameCurser & curser, FrameSeq const& frames) {
  if (curser.pos < frames[curser.frame]->num_used()-1) {
    ++curser.pos;
  } else {
    curser.pos = 0;
    ++curser.frame;
  }
}

/**
 * move the Curser to the previous entry
 **/
void move_backward(TraceFrameCurser & curser, FrameSeq const& frames) {
  if (curser.pos > 0) {
    --curser.pos;
  } else {
    if (curser.frame > 0)  {
      curser.pos = frames[curser.frame-1]->num_used()-1;
    } else {
      curser.pos = TraceFrameSize;
    }
    --curser.frame;
  }
}

bool is_end_of_frame(TraceFrameCurser const& curser, FrameSeq const& frames) {
    return curser.frame < frames.size()
        && curser.pos == frames[curser.frame]->num_used();
}

/**
 * checks if the curser points to a valid position in the frames.
 *
 * returns false either if curser.frame is outside the frames or
 * if thr frame is valid but the position is out of the frame
 **/
bool curser_valid(TraceFrameCurser const& curser, FrameSeq const& frames) {
    return curser.frame < frames.size()
        && curser.pos < frames[curser.frame]->num_used();
}

DeltaTimeFW & access_time( TraceFrameCurser const & curser, FrameSeq & frames) {
  return frames[curser.frame]->time_at(curser.pos);
}

const DeltaTimeFW & access_time( TraceFrameCurser const & curser, FrameSeq const & frames) {
  return frames[curser.frame]->time_at(curser.pos);
}

Bit & access_value( TraceFrameCurser const & curser, FrameSeq & frames) {
  return frames[curser.frame]->bit_at(curser.pos);
}

const Bit & access_value( TraceFrameCurser const & curser, FrameSeq const & frames) {
  return frames[curser.frame]->bit_at(curser.pos);
}

/**
 * set the position ot the curser to the position of the timepoint, if it exists,
 * or to the point where it has to be inserted.
 **/
void search_time(TraceFrameCurser & curser, FrameSeq const& frames, DeltaTimeFW const & time) {
  if ( !frames.empty() ) {
    TraceFrame* back = frames.back();
    if ( back != NULL) {
      unsigned last = back->num_used();
      if (last > 0) {
        --last;
      }
      assert( last < TraceFrameSize );
      if ( back->time_at(last) < time) {
        curser.frame = frames.size()-1;
        curser.pos = back->num_used();
        return;
      }
    }
  }

  FrameSeq::const_iterator it;
  it = lower_bound(frames.begin(), frames.end(), time, CompareTraceFrames());

  curser.frame = std::distance(frames.begin(), it);

  if (curser.frame > 0) {
    --curser.frame;
  }
  TraceFrame & frame = *frames[curser.frame];

  DeltaTimeFW const* end = frame.end();
  DeltaTimeFW const* lb = std::lower_bound(frame.begin(), end, time);
  curser.pos = std::distance(frame.begin(), lb);

  if ( lb == end && frame.full() ) {
      curser.pos = 0;
      ++curser.frame;
  }
}

/**
 * insert a new entry at the position before the curser. If it points to the beginning
 * of a Frame, it will be checked if the element can be inserted at the previous frame.
 * or to the point where it has to be inserted.
 *
 * insert will not check for duplicate value insertion
 **/
void insert(
    TraceFrameCurser & curser
  , FrameSeq & frames
  , DeltaTimeFW const & time
  , Bit const & value
) {
  const unsigned pos = curser.pos;
  const unsigned frame = curser.frame;

  if (frame < frames.size()) {
    if ( !frames[frame]->full() ) {
      frames[frame]->insert(pos, time, value);
    } else if (pos == 0) {
      TraceFrame *new_frame = new TraceFrame(time,value);
      assert(new_frame != NULL);
      frames.insert( frames.begin()+frame, new_frame);
    } else {
      TraceFrame* new_frame = frames[frame]->split(time);
      if ( new_frame == NULL )  {
        // cannot split because time is before or after the frame
        TraceFrame* current = frames[frame];
        new_frame = new TraceFrame(time, value);
        if ( time <  current->leader() ) {
          frames.insert( frames.begin()+frame, new_frame);
        } else {
          assert( time > current->closer() );
          frames.insert( frames.begin()+frame+1, new_frame);
        }
      } else {
        assert(new_frame != NULL);
        frames[frame]->set(time, value);
        frames.insert(frames.begin()+frame+1, new_frame);
      }
    }
  } else {
    frames.push_back(new TraceFrame(time,value));
  }
}

/**
 * Remove the entry at the curser position. It must be a valid position
 **/
void erase(TraceFrameCurser & curser, FrameSeq & frames) {
  assert( curser_valid(curser, frames) );

  const unsigned pos = curser.pos;
  const unsigned frame = curser.frame;

  TraceFrame* tf = frames[frame];

  if ( tf->num_used() == 1 ) {
    std::copy(
      frames.begin()+frame+1,
      frames.end(),
      frames.begin()+frame
    );
    frames.pop_back();
    delete tf;
  } else {
    tf->erase(pos);
  }
}

void truncate_frames( TraceFrameCurser const& curser, FrameSeq & frames ) {
  assert( curser_valid(curser, frames) );

  const unsigned pos = curser.pos;
  const unsigned frame = curser.frame;


  // remove frames after curser;
  std::for_each(frames.begin()+frame+1, frames.end(), boost::lambda::delete_ptr() );
  frames.erase( frames.begin()+frame+1, frames.end() );

  // remove the rest of the current frame
  TraceFrame* lastFrame = frames.at(frame);

  lastFrame->truncate( pos );

  if ( lastFrame->empty() ) {
    if ( frames.size() != 1 ) {
      delete lastFrame;
      frames.pop_back();
    } else {
      lastFrame->reset();
    }
  }

}

////////////////////////////////////////////////////////////


Trace::Trace (const Bit& initvalue)
{
  _numberOfReferences = 0;
  _frames.push_back(new TraceFrame());
  _initvalue = initvalue;
}

Trace::~Trace ()
{
    for (unsigned i = 0; i < _frames.size(); ++i) {
       delete _frames[i];
    }
}

Trace::const_iterator Trace::begin() const
{
    const_iterator ret(_frames);
    ret._curser.frame = 0;
    ret._curser.pos = 0;
    return ret;
}

Trace::const_iterator Trace::end() const
{
    const_iterator ret(_frames);
    ret._curser.frame = (unsigned) - 1;
    ret._curser.pos = (unsigned) - 1;
    return ret;
}

void Trace::add_ref() {
  ++_numberOfReferences;
}

bool Trace::release() {
  if (_numberOfReferences > 0) {
    --_numberOfReferences;
  }
  return _numberOfReferences==0;
}

unsigned Trace::numberOfReferences() {
  return _numberOfReferences;

}
namespace { // local helper functions
  void merge_earlier( FrameSeq & frames, TraceFrameCurser curser ) {
    TraceFrameCurser prev(curser);
    move_backward(prev,frames);

    if ( curser_valid(prev, frames) && access_value(prev, frames) == access_value(curser, frames)) {
      //PRINT_INFO("remove current value");
      erase(curser, frames);
    }
  }

  void merge_later( FrameSeq & frames, TraceFrameCurser curser ) {
    TraceFrameCurser next(curser);
    move_forward(next, frames);

    if ( curser_valid(next, frames) ) {
      if ( access_value(next, frames) ==  access_value(curser, frames) )
      {
        //PRINT_INFO("remove next value");
        erase(next, frames);
      }
    }
  }

  void clear_future( FrameSeq & frames, TraceFrameCurser const& curser ) {
    // delete all later frames
    for (unsigned i = curser.frame+1; i < frames.size(); ++i) {
      delete frames.back();
      frames.pop_back();
    }

    TraceFrame* frame = frames[curser.frame];
    // delete all later frames in the current frame
    for (unsigned i = frame->num_used()-1; i > curser.pos; --i) {
      frame->erase(i);
    }
  }

  void append_val(FrameSeq & frames, Bit const & assign, DeltaTimeFW const & atime) {
    if ( frames.empty() || frames.back()->full() ) {
      frames.push_back( new TraceFrame(atime, assign) );
    } else {
      frames.back()->set(atime, assign);
    }
  }

} // end local helper functions

void Trace::_handle_changes(TraceFrameCurser const& curser
  , TraceChangeMode const changeMode
  , DeltaTimeFW const& atime
  , Bit curVal
) {

  if (changeMode & TRACE_KEEP_FUTURE_CYCLE) {
    set( curVal, DeltaTimeFW(atime.get()+1), TRACE_MERGE_BOTH);
  }

  if ( !curser_valid( curser, _frames) ) {
    return;
  }

  // the order later then earlier is important to keep curser valid.
  if ( changeMode & TRACE_MERGE_LATER ) {
    merge_later(_frames, curser);
  }
  if ( changeMode & TRACE_MERGE_EARLIER ) {
    merge_earlier(_frames, curser);
  }
  if ( changeMode & TRACE_CLEAR_FUTURE ) {
    clear_future(_frames, curser);
  }
}

void Trace::set(const Bit & assign, const DeltaTimeFW& atime, TraceChangeMode const changeMode) {
  assert(!( (changeMode & TRACE_KEEP_FUTURE_CYCLE) && (changeMode & TRACE_CLEAR_FUTURE)));

  TraceFrameCurser curser;
  search_time(curser, _frames, atime);


  if (curser_valid(curser, _frames) ) {
    DeltaTime curTime = access_time(curser, _frames);
    Bit curVal = access_value(curser, _frames);

    if ( curTime == atime )  {

      if ( curVal != assign ) {
        access_value(curser, _frames) = assign;
      }
      _handle_changes(curser, changeMode, atime, curVal);
      return;

    } else if ( curTime > atime ) {
      curVal = _initvalue;
      TraceFrameCurser prev = curser;
      move_backward(prev, _frames);
      if ( curser_valid(prev, _frames) ) {
        curVal = access_value(prev, _frames);
      } else if (changeMode & TRACE_MERGE_EARLIER) {
        if ( assign == _initvalue) {
          return;
        }
      }

      insert(curser, _frames, atime, assign );
      _handle_changes(curser, changeMode, atime, curVal);
      return;

    } else {
      assert( false && "expected search to find the exact or later position");
    }
  } else {

    if ( is_end_of_frame( curser, _frames), "the curser should point to the end of a frame here" ) {
      Bit curVal = _initvalue;
      TraceFrameCurser prev = curser;
      move_backward(prev, _frames);
      if ( curser_valid(prev, _frames) ) {
        curVal = access_value(prev, _frames);
      }
      if ( (changeMode&TRACE_MERGE_EARLIER) && curVal == assign ) {
        return;
      }
      insert(curser, _frames, atime, assign );
      _handle_changes(curser, changeMode, atime, curVal);
      return;
    } else {

      if (_frames.empty() || (_frames.size() == 1  && _frames.front()->num_used() == 0) ) {
        if ( (changeMode & TRACE_MERGE_EARLIER) && assign == _initvalue) {
          return;
        }
      }

      append_val(_frames, assign, atime);
      Bit curVal = _initvalue;
      move_backward(curser, _frames);
      if ( curser_valid(curser, _frames) ) {
        curVal = access_value(curser, _frames);
      }

      _handle_changes(curser, changeMode, atime, curVal);
      return;
    }
  }

  assert( false && "invalid state. All cases should be handled here");
}


void Trace::setRange(Bit const newValue, DeltaTimeFW const& beginTime, DeltaTimeFW const& endTime) {
  assert( beginTime != endTime );
  TraceFrameCurser curser;
  search_time(curser, _frames, beginTime);

  Bit lastValue = _initvalue;
  Bit currentValue = _initvalue;

  {
    TraceFrameCurser prev = curser;
    move_backward(prev, _frames);
    if ( curser_valid(prev, _frames) ) {
      lastValue = access_value(prev, _frames);
    }
  }

  if ( curser_valid(curser, _frames) && access_time( curser, _frames) == beginTime ) {
    currentValue = access_value( curser, _frames);
  } else {
    currentValue = lastValue;
  }

  bool doSetBegin = (lastValue != newValue);

  boost::optional<TraceFrameCurser> beginCurser, endCurser;

  while (curser_valid(curser, _frames) && access_time(curser, _frames) <= endTime ) {
    currentValue = access_value(curser, _frames);
    if ( ! endCurser ) {
      if ( !beginCurser ) {
        beginCurser = curser;
      } else {
        endCurser = curser;
      }
      move_forward(curser,_frames);
    } else {
      erase(curser, _frames);
    }
  }

  bool doSetEnd = (currentValue != newValue);

  if (endCurser) {

    // erase identical successors
    if (curser_valid(curser, _frames) && access_value(curser, _frames) == currentValue) {
      erase(curser, _frames);
    }

    if (doSetEnd) {
      access_time(*endCurser, _frames) = endTime;
      access_value(*endCurser, _frames) = currentValue;
    } else {
      erase(*endCurser, _frames);
    }
  } else if (doSetEnd) {
    insert(curser, _frames, endTime, currentValue );
  }

  if (beginCurser) {
    if ( doSetBegin) {
      access_time(*beginCurser, _frames) = beginTime;
      access_value(*beginCurser, _frames) = newValue;
    } else {
      erase(*beginCurser, _frames);
    }
  } else if (doSetBegin) {
    insert(curser, _frames, beginTime, newValue );
  }
}

Bit Trace::get(const DeltaTimeFW& t) const
{
  TraceFrameCurser curser;
  search_time(curser, _frames, t);
  Bit value = _initvalue;
  if ( curser_valid(curser, _frames) ) {
    if ( t == access_time( curser, _frames)) {
      return access_value(curser, _frames);
    }
  }
  move_backward(curser, _frames);
  if (curser_valid(curser, _frames)) {
    value = access_value(curser, _frames);
  }

  return value;
}

void Trace::set(const Bit& assign, const DeltaTimeFW& time)
{
  set(assign, time, TRACE_MERGE_BOTH);
}

DeltaTime Trace::checkpoint(const DeltaTime& atimeT) const {
  TraceFrameCurser curser;
  DeltaTimeFW atime(atimeT);
  search_time(curser, _frames,  atime);


  if ( curser_valid(curser, _frames)
    && (atime == access_time(curser, _frames))
  ) {
    return atime;
  }

  move_backward( curser, _frames );
  if (curser_valid( curser, _frames)) {
    return access_time(curser, _frames);
  } else {
    return DeltaTime(0,0);
  }
}

std::vector<DeltaTimeFW> Trace::computeCheckpoints() const {
  std::vector<DeltaTimeFW> ret;

  BOOST_FOREACH( const TraceFrame* tf, _frames ) {
    BOOST_FOREACH( const DeltaTimeFW& t, *tf) {
        ret.push_back(t);
    }
  }

  return ret;
}

DeltaTimeFW Trace::firstCheckpoint() const {
  Trace::const_iterator it = begin();
  if (it == end()) {
    DeltaTimeFW result ( DeltaTime(0,0));
    return result;
  } else {
    return it.time();
  }
}

DeltaTimeFW Trace::lastCheckpoint() const {
  BOOST_REVERSE_FOREACH( TraceFrame* frame, _frames)
  {
    if (frame && ! frame->num_used() == 0 )
    {
      return frame->time_at( frame->num_used() -1);
    }
  }

  return DeltaTimeFW( DeltaTime (0,0) );
}

bool Trace::hasCheckpoints() const {
  return !_frames.empty() && _frames.front()->num_used() != 0;
}

std::size_t Trace::numberOfCheckpoints() const {
  size_t result = 0;

  BOOST_FOREACH( const TraceFrame* tf, _frames ) {
    result += tf->num_used();
  }

  return result;
}

std::size_t Trace::capacity() const {
  return _frames.size() * TraceFrameSize;
}

boost::optional<DeltaTimeFW> Trace::prevCheckpoint(const DeltaTimeFW& baseTime ) const {
  if ( !hasCheckpoints() ) {
    return boost::none;
  }

  TraceFrameCurser c;
  search_time( c, _frames, baseTime );

  if ( !curser_valid(c, _frames ) ) {
    move_backward(c, _frames);
  }

  while ( curser_valid(c, _frames)
    && access_time(c, _frames) >= baseTime
  ) {
    move_backward( c, _frames);
  }

  if ( curser_valid( c, _frames) ) {
    return access_time(c, _frames);
  } else {
    return boost::none;
  }
}

boost::optional<DeltaTimeFW>
Trace::nextCheckpoint( DeltaTimeFW const& baseTime ) const {
  TraceFrameCurser c;
  search_time( c, _frames, baseTime );

  while ( curser_valid(c, _frames)
    && access_time(c, _frames) <= baseTime
  ) {
    move_forward( c, _frames);
  }

  if ( curser_valid( c, _frames) ) {
    return access_time(c, _frames);
  } else {
    return boost::none;
  }
}

bool Trace::changed (const DeltaTime& timeT) const {
  DeltaTimeFW time(timeT);
  TraceFrameCurser curser;

  Bit currentVal = _initvalue;

  search_time(curser, _frames, time);
  if ( curser_valid(curser, _frames)
    && ( time == access_time( curser, _frames) )
  ) {

      currentVal =  access_value(curser, _frames);
  } else {
    move_backward(curser, _frames);
    if (curser_valid(curser, _frames)) {
      currentVal = access_value(curser, _frames);
    }
  }

  while (
        curser_valid(curser, _frames)
        && access_time(curser, _frames).get().simcycle() == timeT.simcycle()
  ) {
    move_backward(curser, _frames);
  }

  Bit prevVal;
  if (curser_valid(curser, _frames) ) {
    prevVal = access_value(curser, _frames);
  } else {
    prevVal = _initvalue;
  }

  return prevVal != currentVal;
}

namespace {

  class DifferenceFound : std::exception {};

  void throwOnDifference( DeltaTime, Bit, Bit ) {
    throw DifferenceFound();
  }

  class DoCompareTraces {
    public:
      DoCompareTraces(
        Trace const& a
      , Trace const& b
      , boost::function<void (DeltaTime, Bit, Bit)> const& log
      )
      : _result(true)
      , _currentTime( DeltaTime(0, 0) )
      , _itA( a.begin() )
      , _endA( a.end() )
      , _currentA( a.getInitvalue() )
      , _itB( b.begin() )
      , _endB( b.end() )
      , _currentB( b.getInitvalue() )
      , _log(log)
      {
      }

      bool operator() () {
        doSearchDiffernces();
        return _result;
      }

    private:

      void doSearchDiffernces() {
        while ( !bothAreAtEnd() ) {

          if ( aIsAtEnd() ) {
            advanceB();
            checkForDifference();

          } else if ( bIsAtEnd() ) {
            advanceA();
            checkForDifference();

          } else {
            advanceBasedOnTime();
            checkForDifference();
          }
        }
      }

      bool bothAreAtEnd() const {
        return aIsAtEnd() && bIsAtEnd();
      }

      bool aIsAtEnd() const {
        return _itA == _endA;
      }

      bool bIsAtEnd() const {
        return _itB == _endB;
      }

      void advanceA() {
        _currentA = _itA.value();
        _currentTime = _itA.time();
        ++_itA;
      }

      void advanceB() {
        _currentB = _itB.value();
        _currentTime = _itB.time();
        ++_itB;
      }

      void advanceBasedOnTime() {
        if ( _itA.time() < _itB.time() ) {
          advanceA();
        } else if ( _itA.time() > _itB.time() ) {
          advanceB();
        } else {
          advanceBoth();
        }
      }

      void advanceBoth() {
        _currentA = _itA.value();
        _currentB = _itB.value();
        _currentTime = _itA.time();
        ++_itA;
        ++_itB;
      }

      void checkForDifference() {
          if ( _currentA != _currentB ) {
            handleDifference();
          }
      }

      void handleDifference() {
        _result = false;
        _log(_currentTime, _currentA, _currentB);
      }


    private:
      bool _result;
      DeltaTimeFW _currentTime;
      Trace::const_iterator _itA;
      Trace::const_iterator const _endA;
      Bit _currentA;
      Trace::const_iterator _itB;
      Trace::const_iterator const _endB;
      Bit _currentB;
      boost::function<void (DeltaTime, Bit, Bit)> const& _log;
  };

}

bool compare_traces(
    Trace const & a
  , Trace const & b
  , boost::function<void (DeltaTime, Bit, Bit)> log
) {
    DoCompareTraces comparator(a, b, log);
    return comparator();
}

bool operator== ( Trace const & a, Trace const & b )
{
  try {
    return compare_traces(a,b, &throwOnDifference);
  } catch ( DifferenceFound const& ) {
    return false;
  }
}

void Trace::clear() {
  _frames[0]->reset( DeltaTimeFW(DeltaTime(0,0)) );
  for (unsigned i = 1; i < _frames.size(); ++i) {
       delete _frames[i];
  }
  _frames.resize(1);
}


namespace {

  void write_eoc( TraceFrameCurser & target, FrameSeq & frames, Time cycle, Bit value)
  {
    DeltaTimeFW eoc ( endOfCycle(cycle) );

    if (curser_valid(target, frames))
    {
      access_time(target, frames) = eoc;
      access_value( target, frames ) = value;
    } else {
      insert(target, frames, eoc, value );
    }

    move_forward( target, frames );
  }

}


void Trace::removeDeltaCycles()
{
  TraceFrameCurser changePosition = {0, 0};
  TraceFrameCurser currentPosition = {0, 0};

  Time currentCycle = 0;
  Bit currentValue = _initvalue;
  Bit previousValue = _initvalue;

  while ( curser_valid(currentPosition, _frames) ) {
    Time cycle = access_time(currentPosition, _frames).get().simcycle();
    if ( currentCycle != cycle )
    {
      if ( currentValue != previousValue) {
        write_eoc(changePosition, _frames, currentCycle, currentValue );
        previousValue = currentValue;
      }
    }

    currentCycle = cycle;
    currentValue = access_value(currentPosition, _frames);

    move_forward(currentPosition, _frames);
  }

  if ( currentValue != previousValue) {
    write_eoc( changePosition, _frames, currentCycle, currentValue );
  }

  if ( curser_valid(changePosition, _frames) ) {
    truncate_frames(changePosition, _frames);
  }

}

void Trace::setInitvalue( Bit const& initvalue) {
  _initvalue = initvalue;
}


boost::intrusive_ptr<Trace> Trace::clone() const {
  TracePtr theClone (new Trace(_initvalue));
  TraceFrameCurser a = {0,0};

  while ( curser_valid(a, _frames) ) {
    append_val(theClone->_frames
      , access_value(a, _frames)
      , access_time(a, _frames)
    );
    move_forward(a, _frames);
  }

  return theClone;
}

boost::intrusive_ptr<Trace> Trace::clone(DeltaTime const& upper_bound) const {
  TracePtr theClone (new Trace(_initvalue));
  TraceFrameCurser a = {0,0};

  while ( curser_valid(a, _frames) && access_time(a, _frames) <= upper_bound ) {
    append_val(theClone->_frames
      , access_value(a, _frames)
      , access_time(a, _frames)
    );

    move_forward(a, _frames);
  }
  return theClone;
}

Trace::const_iterator &Trace::const_iterator::operator ++()
{
    move_forward(_curser, _frames);
    return *this;
}

bool Trace::const_iterator::operator==(Trace::const_iterator const & other) const
{
  bool sameFrame = (_curser.frame == other._curser.frame);
  bool samePos = (_curser.pos == other._curser.pos);

  if (sameFrame && samePos ) {
    return true;
  }

  bool selfValid = curser_valid(_curser, _frames);
  bool otherValid = curser_valid(other._curser, _frames);

  return !selfValid && !otherValid;
}

bool Trace::const_iterator::operator !=(const Trace::const_iterator & other) const
{
    return ! (*this == other);
}

Trace::const_iterator::value_type Trace::const_iterator::operator *() const
{
    return value_type( access_time(_curser, _frames), access_value(_curser, _frames) );
}

const DeltaTimeFW& Trace::const_iterator::time() const
{
    return access_time(_curser, _frames);
}

const Bit &Trace::const_iterator::value() const
{
    return access_value(_curser, _frames);
}

Trace::const_iterator::const_iterator(std::vector<TraceFrame*> const & frames)
: _frames(frames)
{
}

} // namespace svt
