#pragma once

#include <trace/TraceFrame.h>

namespace svt {

TraceFrame::TraceFrame() : _leader(DeltaTime(0, 0)), _used(0) {}

TraceFrame::TraceFrame(const DeltaTimeFW &leader) : _leader(leader), _used(0) {}

TraceFrame::TraceFrame(const DeltaTimeFW &leader, Bit const &value)
    : _leader(leader), _used(1) {
  _times[0] = leader;
  _values[0] = value;
}

TraceFrame::~TraceFrame() {}

void TraceFrame::reset(const DeltaTimeFW &leader) {
  _used = 0;
  _times[0] = leader;
}

void TraceFrame::erase(size_t pos) {
  assert(_used != 0);
  assert(pos < TraceFrameSize);

  std::copy(_times.begin() + pos + 1, _times.begin() + num_used(),
            _times.begin() + pos);
  std::copy(_values.begin() + pos + 1, _values.begin() + num_used(),
            _values.begin() + pos);
  --_used;
}

void TraceFrame::insert(size_t pos, DeltaTimeFW const &t, Bit const &value) {
  assert(!full());
  assert(pos < TraceFrameSize);

  std::copy_backward(_times.begin() + pos, _times.begin() + num_used(),
                     _times.begin() + num_used() + 1);
  std::copy_backward(_values.begin() + pos, _values.begin() + num_used(),
                     _values.begin() + num_used() + 1);

  _times[pos] = t;
  _values[pos] = value;

  ++_used;
}

void TraceFrame::truncate(unsigned maxLength) {
  if (maxLength < _used) {
    _used = maxLength;
  }
}

DeltaTimeFW TraceFrame::leader() const {
  if (_used == 0) {
    return _leader;
  } else {
    return _times[0];
  }
}

DeltaTimeFW TraceFrame::closer() const {
  if (_used == 0) {
    return _leader;
  } else {
    return _times[_used - 1];
  }
}

bool TraceFrame::full() const { return _used == TraceFrameSize; }

TraceFrame *TraceFrame::split(const DeltaTimeFW &t) {
  DeltaTimeFW *end = _times.begin() + _used;
  DeltaTimeFW *lb = std::lower_bound(_times.begin(), end, t);

  if (lb == end) {
    return NULL;
  }
  if (lb == _times.begin()) {
    return NULL;
  }

  size_t pos = lb - _times.begin();
  TraceFrame *new_frame = new TraceFrame(*lb);

  std::copy(_times.begin() + pos, _times.end(), new_frame->_times.begin());
  std::copy(_values.begin() + pos, _values.end(), new_frame->_values.begin());

  new_frame->_used = _used - pos;
  _used = pos;

  return new_frame;
}

bool TraceFrame::set(const DeltaTimeFW &t, const Bit &value) {

  DeltaTimeFW *end = _times.begin() + _used;
  DeltaTimeFW *lb = std::lower_bound(_times.begin(), end, t);
  size_t pos = lb - _times.begin();

  if (lb == end) {
    if (full()) {
      return false;
    }
    _times[_used] = t;
    _values[_used] = value;
    ++_used;
  } else if (*lb == t) {
    _values[pos] = value;
  } else {
    if (full()) {
      return false;
    }

    for (unsigned i = _used; i > pos; --i) {
      _times[i] = _times[i - 1];
      _values[i] = _values[i - 1];
    }
    _times[pos] = t;
    _values[pos] = value;
    ++_used;
  }

  return true;
}

struct CompareTraceFrames {
  bool operator()(TraceFrame const &a, TraceFrame const &b) const {
    return a.leader() < b.leader();
  }

  bool operator()(TraceFrame const *a, TraceFrame const *b) const {
    if (a == b) {
      return false;
    }
    if (a == NULL) {
      return false;
    }
    if (b == NULL) {
      return true;
    }
    return (*this)(*a, *b);
  }

  bool operator()(TraceFrame const &a, DeltaTime const &b) const {
    return a.leader() < b;
  }

  bool operator()(TraceFrame const *a, DeltaTime const &b) const {
    return a->leader() < b;
  }
};

const DeltaTimeFW *TraceFrame::begin() const { return _times.begin(); }

const DeltaTimeFW *TraceFrame::end() const { return _times.begin() + _used; }

std::ostream &operator<<(std::ostream &o, TraceFrame const &tf) {
  o << "[ ";
  for (unsigned i = 0; i < tf._used; ++i) {
    o << tf._values[i] << '@' << tf._times[i] << ' ';
  }
  o << ']';
  return o;
}

DeltaTimeFW &TraceFrame::time_at(size_t pos) { return _times[pos]; }

DeltaTimeFW const &TraceFrame::time_at(size_t pos) const { return _times[pos]; }

Bit &TraceFrame::bit_at(size_t pos) { return _values[pos]; }

Bit const &TraceFrame::bit_at(size_t pos) const { return _values[pos]; }

unsigned TraceFrame::num_used() const { return _used; }

bool TraceFrame::empty() const { return _used == 0; }

} // namespace svt
