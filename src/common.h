#pragma once

#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>

typedef uint8_t uint8;
typedef uint16_t uint16;
typedef uint32_t uint32;
typedef uint64_t uint64;

typedef int8_t int8;
typedef int16_t int16;
typedef int32_t int32;
typedef int64_t int64;

typedef float float32;
typedef double float64;

typedef uintptr_t uintptr;
typedef size_t usize;
typedef ptrdiff_t size;

typedef uint32_t bool32;

#define true 1
#define false 0

#ifndef offsetof
	#define offsetof(type, member) (usize)(&(((type *)0)->member))
#endif

#if defined(__STDC_VERSION__) && __STDC_VERSION__ >= 201112L
	#define alignof(type) _Alignof(type)
#elif defined(__GNUC__) || defined(__clang__)
	#define alignof(type) __alignof__(type)
#elif defined(_MSC_VER)
	#define alignof(type) __alignof(type)
#else
	#define alignof(type) offsetof(struct { char c; type member; }, member)
#endif

#define sizeof_member(type, member) (sizeof(((type *)0)->member))

#define countof(array) (sizeof(array) / sizeof((array)[0]))

#define max(a, b) ((a) > (b) ? (a) : (b))
#define min(a, b) ((a) < (b) ? (a) : (b))

#define clamp(value, low, high) ((value) < (low) ? (low) : ((value) > (high) ? (high) : (value)))

#define INVALID_INDEX UINT32_MAX
#define INVALID_UUID UINT64_MAX

#define STATIC_ASSERT_(COND, LINE)

#define STATIC_ASSERT_PASTE_(a, b) a##b
#define STATIC_ASSERT_PASTE(a, b) STATIC_ASSERT_PASTE_(a, b)

#define STATIC_ASSERT(COND) typedef char STATIC_ASSERT_PASTE(static_assertion_failed_at_line_, __LINE__)[(COND) ? 1 : -1]

#define KB(bytes) ((uint64_t)(bytes) * 1000ULL)
#define MB(bytes) ((KB(bytes)) * 1000ULL)
#define GB(bytes) ((MB(bytes)) * 1000ULL)

#define KiB(bytes) ((uint64_t)(bytes) * 1024ULL)
#define MiB(bytes) ((KiB(bytes)) * 1024ULL)
#define GiB(bytes) ((MiB(bytes)) * 1024ULL)

static inline uint64_t aligned_address(uint64_t address, uint64_t alignment) {
	return ((address + (alignment - 1)) & ~(alignment - 1));
}

// TODO: Move this?
typedef struct {
	usize size;
	char *content;
} FileContent;
