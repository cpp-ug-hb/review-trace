#pragma once

#include <time/DeltaTime.h>

#include <boost/flyweight.hpp>
#include <boost/flyweight/hashed_factory.hpp>

namespace svt {
  typedef boost::flyweight<
      DeltaTime
    , boost::flyweights::hashed_factory<>
  > DeltaTimeFW;
} /* namespace svt */
