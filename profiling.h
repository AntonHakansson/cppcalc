#ifndef PROFILING_HEADER_INCLUDED_H
#define PROFILING_HEADER_INCLUDED_H

#include <intrin.h> // Or #include <ia32intrin.h> etc.

typedef long long TimeUnit;

#ifdef ENABLE_PROFILING

#define START_TIMEDBLOCK(name) startTimeStamp(name)
#define GET_TIMEDBLOCK(name)   getTimeStamp(name)
#define DEBUG_TIMEDBLOCK(name) printf("TIMEDBLOCK "name ": %lld clock cycles\n", getTimeStamp(name));

static const size_t globalTimeStampsCount = 256;
static TimeUnit globalTimeStamps[globalTimeStampsCount] = {};

static TimeUnit readTimeStampCounter() { // Returns time stamp counter
  int dummy[4];                          // For unused returns
  volatile int DontSkip;                 // Volatile to prevent optimizing
  TimeUnit clock;                        // Time
  __cpuid(dummy, 0);                     // Serialize
  DontSkip = dummy[0];                   // Prevent optimizing away cpuid
  clock = __rdtsc();                     // Read time
  return clock;
}

const size_t hash(char *str) {
  size_t hash = 5381;
  int c;

  while (*str) {
    c = *str++;
    hash = ((hash << 5) + hash) + c; /* hash * 33 + c */
  }

  return hash;
}

static void startTimeStamp(const char *name) {
  const size_t index = hash(const_cast<char*>(name)) % globalTimeStampsCount;
  globalTimeStamps[index] = readTimeStampCounter();
}

static TimeUnit getTimeStamp(const char *name) {
  const size_t index = hash(const_cast<char*>(name)) % globalTimeStampsCount;
  return readTimeStampCounter() - globalTimeStamps[index];
}

#else

#define START_TIMEDBLOCK(name)
#define GET_TIMEDBLOCK(name) 0
#define DEBUG_TIMEDBLOCK(name)

#endif

#endif // PROFILING_HEADER_INCLUDED_H
