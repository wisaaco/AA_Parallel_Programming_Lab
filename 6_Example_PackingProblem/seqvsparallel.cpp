//https://stackoverflow.com/questions/16557690/no-speedup-achieved-using-parallel-scan-component-of-intel-thread-building-block

#include <iostream>
#include <stdlib.h>
#include <time.h>

#include <tbb/tbb.h>
#include "oneapi/tbb/blocked_range.h"
#include "oneapi/tbb/parallel_for.h"

using namespace std;
using namespace oneapi;
using namespace tbb;

/**
 * This allows selecting between two different versions of the inner
 * loop function in the parallel scan implementation. This optimised
 * version is more verbose, but avoids evaluating the same condition
 * each and every loop iteration.
 *
 * When not using compiler optimisations, the unoptimised version is
 * much worse (about +33% exec time for ARRAY_SIZE = 100000000). But
 * with \c -O3 optimisations both approaches perform about the same,
 * i.e. the compiler is smart enough to optimise this if allowed to.
 *
 * This shows that manual optimisations (which often make the source
 * code harder to read and maintain) may be useless when things like
 * the compiler can automatically perform the same optimisations. So
 * consider profiling / benchmarking before wasting time optimising.
 */
#define USE_OPTIMISED_LOOP 0

template<typename T>
class Body
{
	T sum;
	T* const y;
	const T* const x;

public:
	Body(T y_[], const T x_[]) : sum(0), x(x_), y(y_) {}

	T get_sum() const { return sum; }

	template<typename Tag>
	void operator()(const blocked_range<size_t>& r, Tag)
	{
#if USE_OPTIMISED_LOOP
		/**
		 * Evaluate \c Tag::is_final_scan() once outside the
		 * loop, but we need to have two separate loops.
		 */
		if (Tag::is_final_scan()) {
			for (size_t i = r.begin(); i < r.end(); ++i) {
				sum = sum + x[i];
				y[i] = sum;
			}
		} else {
			for (size_t i = r.begin(); i < r.end(); ++i) {
				sum = sum + x[i];
			}
		}
#else
		/**
		 * Less verbose, but \c Tag::is_final_scan() is
		 * evaluated each and every iteration. Or is it?
		 */
		for (size_t i = r.begin(); i < r.end(); ++i) {
			sum = sum + x[i];
			if (Tag::is_final_scan()) {
				y[i] = sum;
			}
		}
#endif
	}

	Body(Body& b, split) : x(b.x), y(b.y), sum(0) {}

	void reverse_join(Body& a)
	{
		sum = a.sum + sum;
	}

	void assign(Body& b)
	{
		sum = b.sum;
	}
};

template<typename T>
T DoParallelScan(T y[], const T x[], size_t n)
{
	Body<T> body(y, x);
	const tick_count t0 = tick_count::now();
	parallel_scan(blocked_range<size_t>(0, n), body);
	const tick_count t1 = tick_count::now();
	cout << "Time Taken for parallel scan is: " << (t1 - t0).seconds() << endl;
	return body.get_sum();
}

template<typename T>
T DoSerialScan(T y[], const T x[], size_t n)
{
	const tick_count t0 = tick_count::now();
	T temp = 0;
	for (size_t i = 0; i < n; ++i) {
		temp = temp + x[i];
		y[i] = temp;
	}
	const tick_count t1 = tick_count::now();
	cout << "Time Taken for   serial scan is: " << (t1 - t0).seconds() << endl;
	return temp;
}

/**
 * The size of the input and output arrays used to perform the serial
 * and parallel scan operations. If the arrays do not fit in RAM then
 * performance will be awful and/or the program may get killed.
 */
static const size_t ARRAY_SIZE = 1000000;

int main()
{
	/* For some reason, using very large C-style arrays causes segfaults in the loop */
	std::vector<int> y1(ARRAY_SIZE), x1(ARRAY_SIZE);

	for (size_t i = 0; i < ARRAY_SIZE; i++) {
		x1[i] = static_cast<int>(i & 0x7fffffff);
	}

	const int outSerial = DoSerialScan(y1.data(), x1.data(), ARRAY_SIZE);
	const int outParallel = DoParallelScan(y1.data(), x1.data(), ARRAY_SIZE);

	cout << "  serial scan output is \t" << outSerial << endl;
	cout << "parallel scan output is \t" << outParallel << endl;

	return 0;
}
