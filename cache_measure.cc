#include <time.h>
#include <unistd.h>
#include <cassert>
#include <iostream>

static __inline__ uint64_t rdtsc(void) {
  uint32_t hi, lo;
  __asm__ __volatile__("rdtsc" : "=a"(lo), "=d"(hi));
  return ((uint64_t)hi << 32) | lo;
}

const int NANO_SECONDS_IN_SEC = 1000'000'000;
/* returns a static buffer of struct timespec with the time difference of ts1
   and ts2 ts1 is assumed to be greater than ts2 */
struct timespec *TimeSpecDiff(struct timespec *ts1, struct timespec *ts2) {
  static struct timespec ts;
  ts.tv_sec = ts1->tv_sec - ts2->tv_sec;
  ts.tv_nsec = ts1->tv_nsec - ts2->tv_nsec;
  if (ts.tv_nsec < 0) {
    ts.tv_sec--;
    ts.tv_nsec += NANO_SECONDS_IN_SEC;
  }
  return &ts;
}

double MeasureTSCTicksPerNs() {
  struct timespec begints, endts;
  uint64_t begin = 0, end = 0;
  clock_gettime(CLOCK_MONOTONIC, &begints);
  begin = rdtsc();
  usleep(0.5 * 1000 * 1000);
  end = rdtsc();
  clock_gettime(CLOCK_MONOTONIC, &endts);
  struct timespec *tmpts = TimeSpecDiff(&endts, &begints);
  uint64_t nsecElapsed = tmpts->tv_sec * 1000'000'000LL + tmpts->tv_nsec;
  return (double)(end - begin) / (double)nsecElapsed;
}

using namespace std;

constexpr uint64_t kPageSize = 4 * 1024;
constexpr uint64_t kPageSizeExponent = 12;

void MeasureCacheReadLatency(double tick_per_ns) {
  constexpr uint64_t kRangeMin = 1ULL << 10;
  constexpr uint64_t kRangeMax = 1ULL << 24;
  constexpr uint64_t array_size_in_pages =
      (sizeof(int) * kRangeMax + kPageSize - 1) >> kPageSizeExponent;
  int *array =
      reinterpret_cast<int *>(malloc(array_size_in_pages << kPageSizeExponent));
  assert(array);
  printf("memory vaddr range: [0x%016lX - 0x%016lX)\n", (uint64_t)array,
         (uint64_t)array + (array_size_in_pages << kPageSizeExponent));
  printf("row=range[bytes], col=stride[bytes], val=latency[ns]\n");

  uint64_t nextstep, i, index;
  uint64_t csize, stride;
  uint64_t steps, tsteps;
  uint64_t t0, t1, tick_sum_overall, tick_sum_loop_only;
  uint64_t kDurationTick = (uint64_t)(0.1 * 1e9) * tick_per_ns;

  printf(", ");
  for (stride = 1; stride <= kRangeMax / 2; stride = stride * 2)
    printf("%ld, ", stride * sizeof(int));
  printf("\n");

  for (csize = kRangeMin; csize <= kRangeMax; csize = csize * 2) {
    printf("%ld, ", csize * sizeof(int));
    for (stride = 1; stride <= csize / 2; stride = stride * 2) {
      for (index = 0; index < csize; index = index + stride) {
        array[index] = (int)(index + stride);
      }
      array[index - stride] = 0;

      // measure time spent on (reading data from memory) + (loop, instrs,
      // etc...)
      steps = 0;
      nextstep = 0;
      t0 = rdtsc();
      do {
        for (i = stride; i != 0; i = i - 1) {
          nextstep = 0;
          do
            nextstep = array[nextstep];
          while (nextstep != 0);
        }
        steps = steps + 1;
        t1 = rdtsc();
      } while ((t1 - t0) < kDurationTick);  // originary 20.0
      tick_sum_overall = t1 - t0;

      // measure time spent on (loop, instrs, etc...) only
      tsteps = 0;
      t0 = rdtsc();
      do {
        for (i = stride; i != 0; i = i - 1) {
          index = 0;
          do
            index = index + stride;
          while (index < csize);
        }
        tsteps = tsteps + 1;
        t1 = rdtsc();
      } while (tsteps < steps);
      tick_sum_loop_only = t1 - t0;

      // avoid negative value
      if (tick_sum_loop_only >= tick_sum_overall) {
        tick_sum_loop_only = tick_sum_overall;
      }

      const uint64_t tick_sum_of_mem_read =
          tick_sum_overall - tick_sum_loop_only;
      const double latency_ns =
          (double)tick_sum_of_mem_read / (steps * csize) / tick_per_ns;
      if (latency_ns > 0) printf("%lf, ", latency_ns);
      else printf(", ");
    };
    printf("\n");
  };
}

int main(int argc, char *argv[]) {
  double tsc_tick_per_ns = MeasureTSCTicksPerNs();
  cout << "TSC tick / ns = " << tsc_tick_per_ns << endl;
  MeasureCacheReadLatency(tsc_tick_per_ns);
  return 0;
}
