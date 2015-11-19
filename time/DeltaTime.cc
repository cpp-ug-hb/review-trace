#include "DeltaTime.h"

#include <boost/static_assert.hpp>
#include <boost/math/special_functions/pow.hpp>

using namespace svt;

BOOST_STATIC_ASSERT(sizeof(Time) == 8);
BOOST_STATIC_ASSERT(DELTACYCLE_BITWIDTH + SIMCYCLE_BITWIDTH == 64);

const Time DeltaTime::MaxDeltaTime = 255ull;
const Time DeltaTime::MaxSimTime = 72057594037927935ull;
const DeltaTime DeltaTime::initTime = DeltaTime(MaxSimTime, MaxDeltaTime);

DeltaTime DeltaTime::operator+(Time simcycle) const {
  assert(_data._simcycle < MaxSimTime);
  return endOfCycle(_data._simcycle + simcycle);
}

DeltaTime &DeltaTime::operator++() {
  assert(_data._simcycle < MaxSimTime);
  ++_data._simcycle; // may be overlapped on 2^54 bits
  return *this;
}

DeltaTime DeltaTime::operator-(Time simcycle) const {
  assert(_data._simcycle > 0);
  return endOfCycle(_data._simcycle - simcycle);
}

DeltaTime DeltaTime::rebase(DeltaTime const &oldBase,
                            DeltaTime const &newBase) const {
  if (oldBase == newBase) {
    return *this;
  }

  Time positiveSide = _data._simcycle + newBase._data._simcycle;

  if (positiveSide < oldBase._data._simcycle) {
    // underflow
    return DeltaTime(0, 0);
  }

  Time positiveDelta = _data._deltacycle + newBase._data._deltacycle;

  if (positiveSide == oldBase._data._simcycle &&
      positiveDelta < oldBase._data._deltacycle) {
    // underflow
    return DeltaTime(0, 0);
  }

  Time cycle = positiveSide - oldBase._data._simcycle;
  Time delta = positiveDelta - oldBase._data._deltacycle;

  if (isEndOfCycle()) {
    if (oldBase._data._deltacycle < newBase._data._deltacycle) {
      // delta overflow
      if (cycle == MaxSimTime) {
        cycle = 0;
      } else {
        cycle += 1;
      }
    }
  }

  return DeltaTime(cycle, delta);
}

namespace std {
std::ostream &operator<<(std::ostream &os, const DeltaTime &a) {
  os << a.simcycle();
  if (a.isEndOfCycle()) {
    os << '$';
  } else {
    os << "+" << a.deltacycle();
  }
  return os;
}

std::istream &operator>>(std::istream &is, DeltaTime &a) {
  Time simcycle;
  char plus;
  is >> simcycle;
  if (!is) {
    return is;
  }
  if (is.eof()) {
    a = endOfCycle(simcycle);
    return is;
  }
  is >> plus;
  if (!is) {
    return is;
  }
  if (plus == '$') {
    a = endOfCycle(simcycle);
  } else if (plus == '+') {
    Time delta;
    is >> delta;
    if (!is) {
      return is;
    }
    a = DeltaTime(simcycle, delta);
  }
  return is;
}
}
