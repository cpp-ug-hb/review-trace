#include <trace/Trace.h>

#include <benchmark/benchmark.h>

using svt::Trace;
using svt::DeltaTime;
using svt::DeltaTimeFW;

static void BM_append(benchmark::State &state) {
  std::size_t processed;
  Trace trace(0);
  while (state.KeepRunning()) {
    trace.clear();
    DeltaTime time(0, 0);
    uint8_t value = 1;
    for (size_t i = 0; i < state.range_x(); ++i) {
      ++time;
      value += 1;
      trace.set(value, DeltaTimeFW(time));
      processed += 1;
    }
    benchmark::DoNotOptimize(trace);
  }
  state.SetItemsProcessed(processed);
}

static void BM_construct_trace(benchmark::State &state) {
  while (state.KeepRunning()) {
    Trace trace(0);
    benchmark::DoNotOptimize(trace);
  }
}

static void BM_increment(benchmark::State &state) {
  DeltaTime time(0, 0);
  while (state.KeepRunning()) {
    ++time;
  }
  benchmark::DoNotOptimize(time);
}

BENCHMARK(BM_construct_trace);
BENCHMARK(BM_increment);
BENCHMARK(BM_append)
    ->Arg(8)
    ->Arg(64)
    ->Arg(512)
    ->Arg(1 << 10)
    ->Arg(1 << 15)
    ->Arg(1 << 20)
    ->Arg(1 << 21)
    ->Arg(1 << 22);

BENCHMARK_MAIN();
