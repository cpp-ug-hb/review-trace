#pragma once
#include <time/Time.hpp>

#include <boost/functional/hash.hpp>

namespace svt {

// can be modified on different architectures if necesary.
// Required bit widths are automatically determined.
#define SIMCYCLE_BITWIDTH 56
#define DELTACYCLE_BITWIDTH 8

class DeltaTime {
public:
  static const Time MaxDeltaTime; // 2^DELTACYCLE_BITWIDTH
  static const Time MaxSimTime;   // 2^SIMCYCLE_BITWIDTH

public:
  DeltaTime() : _data(0, 0) {}

  DeltaTime(const Time &simCycle, const Time &deltaCycle)
      : _data(simCycle, deltaCycle) {}

  static const DeltaTime initTime;

  bool operator<(const DeltaTime &other) const {
    return (_data._simcycle < other._data._simcycle) ||
           (_data._simcycle == other._data._simcycle &&
            _data._deltacycle < other._data._deltacycle);
  }

  bool operator<=(const DeltaTime &other) const {
    return (_data._simcycle < other._data._simcycle) ||
           (_data._simcycle == other._data._simcycle &&
            _data._deltacycle <= other._data._deltacycle);
  }

  bool operator>(const DeltaTime &other) const {
    return (_data._simcycle > other._data._simcycle) ||
           (_data._simcycle == other._data._simcycle &&
            _data._deltacycle > other._data._deltacycle);
  }

  bool operator>=(const DeltaTime &other) const {
    return (_data._simcycle >= other._data._simcycle) ||
           (_data._simcycle == other._data._simcycle &&
            _data._deltacycle >= other._data._deltacycle);
  }

  bool operator==(const DeltaTime &other) const {
    return (_data._simcycle == other._data._simcycle) &&
           (_data._deltacycle == other._data._deltacycle);
  }

  bool operator!=(const DeltaTime &other) const { return !operator==(other); }

  // REVIEW: this operator confuses a lot, "arithmetic" operations should return
  // a const reference of the same object
  // this function creates a new object. Solution: create a function named
  // shifted
  DeltaTime operator+(Time simcycle) const;
  DeltaTime operator-(Time simcycle) const;

  DeltaTime previousDeltaTime(const Time &delay = 1) const {
    if (delay <= _data._deltacycle) {
      return DeltaTime(_data._simcycle, _data._deltacycle - delay);
    }

    if (_data._simcycle == 0) {
      return DeltaTime(_data._simcycle, 0);
    }

    return DeltaTime(_data._simcycle - 1, MaxDeltaTime);
  }

  DeltaTime nextDeltaTime(const Time &delay = 1) const {
    if (isEndOfCycle()) {
      return DeltaTime(_data._simcycle + 1, 0);
    }

    Time d = MaxDeltaTime - _data._deltacycle;
    if (delay > d) {
      return DeltaTime(_data._simcycle + 1, delay - d);
    }
    return DeltaTime(_data._simcycle, _data._deltacycle + delay);
  }

  DeltaTime &operator++();

  Time simcycle() const { return _data._simcycle; }
  Time deltacycle() const { return _data._deltacycle; }

  bool isEndOfCycle() const { return _data._deltacycle == MaxDeltaTime; }
  bool isBeginOfCycle() const { return _data._deltacycle == 0; }

  bool isBeginOrEndOfCycle() const {
    return isBeginOfCycle() || isEndOfCycle();
  }

  /**
  * change the basetime of a cycle.
  */
  DeltaTime rebase(DeltaTime const &oldBase, DeltaTime const &newBase) const;

private:
  struct TimeData {
    TimeData(Time simcycle, Time deltacycle)
        : _simcycle(simcycle), _deltacycle(deltacycle) {}

    Time _simcycle : SIMCYCLE_BITWIDTH;
    Time _deltacycle : DELTACYCLE_BITWIDTH;
  } _data;
};

inline std::size_t hash_value(svt::DeltaTime const &dt) {
  std::size_t ret = 0;
  boost::hash_combine(ret, dt.simcycle());
  boost::hash_combine(ret, dt.deltacycle());
  return ret;
}

inline DeltaTime endOfCycle(const Time t) {
  return DeltaTime(t, DeltaTime::MaxDeltaTime);
}

} // end namespace svt

namespace std {
std::ostream &operator<<(std::ostream &os, const svt::DeltaTime &a);
std::istream &operator>>(std::istream &is, svt::DeltaTime &a);
}
