#include <iostream>
#include <iomanip>
#include <vector>
#include <algorithm>

#include <tbb/tbb.h>
#include "oneapi/tbb/blocked_range.h"
#include "oneapi/tbb/parallel_for.h"

using namespace std;
using namespace oneapi;

/**
 * Perform a parallel map on the elements of an input array and store
 * them in a caller-provided output array. Should work with any other
 * "array-like" data type that supports the subscript operator `[]`.
 *
 * If the template parameters seem confusing, imagine that:
 *    - \c OutArrT is \c OutT[]
 *    - \c InArrT is \c InT[]
 *    - \c FuncT is \c OutT(InT)
 *
 * NOTE: Thanks to TAD (Template Argument Deduction), we do not need to
 * specify any template parameters for this function template.
 *
 * @param out   Output array
 * @param in    Input array
 * @param func  Map function
 * @param n     Length of the arrays
 */
template<typename OutArrT, typename InArrT, typename FuncT>
void doMap(OutArrT& out, const InArrT& in, FuncT&& func, size_t n)
{
	tbb::parallel_for(
		tbb::blocked_range<size_t>(0, n),

		// lambda function
		[&](tbb::blocked_range<size_t> r) {
			for (size_t i = r.begin(); i != r.end(); i++) {
				out[i] = invoke(func, in[i]);
			}
		}
	);
}

/**
 * Calculate a parallel prefix/scan of an input array and store the
 * individual results in an output array. Should work with any other
 * "array-like" data type that supports the subscript operator `[]`.
 *
 * The TBB function for a parallel scan takes two functions: one to
 * perform a sequential scan over a range, and another to combine two
 * summaries. This abstraction defines the former function from the
 * latter, so the caller only needs to provide the combiner function.
 *
 * If the template parameters seem confusing, imagine that:
 *    - \c ArrT is \c T[]
 *    - \c InArrT is \c InT[]
 *    - \c FuncT is \c T(T, T)
 *
 * The only reason why \c InArrT and \c ArrT are different types is to
 * enable implicit casting of input values to the output type without
 * having to do a separate map step. For example, implicitly casting
 * booleans in an array to \c size_t when computing the prefix sum.
 *
 * NOTE: Thanks to TAD (Template Argument Deduction), we do not need to
 * specify any template parameters for this function template.
 *
 * @param out   Output array
 * @param in    Input array
 * @param ident The identity element for the function
 * @param func  Combiner function
 * @param n     Length of the arrays
 *
 * @return The summary computed over the whole range
 */
template<typename T, typename ArrT, typename InArrT, typename FuncT>
T doScan(ArrT &out, const InArrT& in, const T& ident, FuncT&& func, size_t n)
{
	return tbb::parallel_scan(
		tbb::blocked_range<size_t>(0, n), // range
		ident,
		[&](tbb::blocked_range<size_t> r, T sum, bool is_final_scan) {
			T tmp = sum;
			for (size_t i = r.begin(); i != r.end(); i++) {
				tmp = invoke(func, tmp, in[i]);
				if (is_final_scan) {
					out[i] = tmp;
				}
			}
			return tmp;
		},
		//[&](T left, T right) {
		//	return invoke(func, left, right);
		//}
		func /* No lambda needed, just use the provided function directly */
	);
}

/**
 * Filter the elements of an input array according to a boolean match
 * array and store them in an output array at the position specified
 * by the value in an index match array. Should work with any other
 * "array-like" data type that supports the subscript operator `[]`.
 *
 * Note that a proper filter function would only take the input array
 * and a \c bool(T) predicate function. However, this program is meant
 * to show how a parallel filter function can be implemented, so the
 * signature of this function takes the intermediate results that were
 * previously computed.
 *
 * NOTE: Thanks to TAD (Template Argument Deduction), we do not need to
 * specify any template parameters for this function template.
 *
 * @param out       Output array
 * @param in        Input array
 * @param bolMatch  Array of bool-like (result of mapping a predicate on \p in array)
 * @param ixMatch   Array of indices plus 1 (result of prefix sum over \p bolMatch array)
 * @param n         Length of the input arrays (output array can be shorter)
 */
template<typename ArrT, typename BoolArrT, typename IdxArrT>
void doFilter(ArrT& out, const ArrT& in, const BoolArrT& bolMatch, const IdxArrT& ixMatch, size_t n)
{
	tbb::parallel_for(
		tbb::blocked_range<size_t>(0, n),
		[&](tbb::blocked_range<size_t> r) {
			for (size_t i = r.begin(); i < r.end(); i++) {
				if (bolMatch[i]) {
					out[ixMatch[i] - 1] = in[i];
				}
			}
		}
	);
}

/**
 * Print the length and contents of a vector. Assumes elements can be
 * printed using the \c << operator of \c cout directly. Attempts to
 * pad values using \c setw() so that they remain aligned.
 *
 * @param vec   The vector to print the info of
 * @param name  The name to show for this vector
 */
template<typename T>
void printVec(const vector<T>& vec, const string& name)
{
	const string prefix = name + " [" + to_string(vec.size()) + "]:";
	cout << setw(16) << prefix;
	for (const T& e : vec) {
		cout << setw(4) << e << ",";
	}
	cout << endl;
}

/**
 * Perform a parallel filter operation on an input vector. The output
 * vector only contains the elements of the input vector for which the
 * predicate returned true, preserving the order of the input elements.
 *
 * We use vectors here for convenience, since we need to allocate both
 * intermediate and final results.
 *
 * If the template parameters seem confusing, imagine that:
 *    - \c PredT is \c bool(T)
 *
 * @param inputVec  Input vector
 * @param predicate Function to test each element with
 *
 * @return A vector with the filtered elements from the input
 */
template<typename T, typename PredT>
vector<T> vecFilter(const vector<T>& inputVec, PredT&& predicate)
{
	const size_t n = inputVec.size();
	printVec(inputVec, "inputVec");

	/**
	 * MAP: Apply the predicate to each element of the input vector.
	 * We obtain a \c bolMatch vector where each boolean value is
	 * the value returned by the predicate for a given input value.
	 */
	vector<bool> bolMatch(n);
	doMap(bolMatch, inputVec, predicate, n);
	printVec(bolMatch, "bolMatch");

	/**
	 * SCAN: Compute the prefix sum of the \c bolMatch vector. The
	 * resulting \c ixMatch vector tells us the index in the output
	 * vector where a given input value needs to be stored (plus 1).
	 */
	const auto scanFunc = [](size_t a, size_t b) { return a + b; };
	const size_t identity = 0;
	vector<size_t> ixMatch(n);
	/**
	 * We can calculate the output length (i.e. the number of values
	 * that passed the predicate) using a REDUCE operation. However,
	 * what \c doScan returns (the summary computed over the entire
	 * range) is exactly the same a REDUCE operation would return.
	 */
	size_t outSize = doScan(ixMatch, bolMatch, identity, scanFunc, n);
	printVec(ixMatch, "ixMatch");

	/**
	 * JOIN: Using \c bolMatch and \c ixMatch from previous steps,
	 * copy the input elements that passed the predicate (i.e. for
	 * which \c bolMatch is true) to the output vector, making use
	 * of \c ixMatch to know in which position the elements should
	 * be inserted.
	 */
	vector<T> filteredVec(outSize);
	doFilter(filteredVec, inputVec, bolMatch, ixMatch, n);
	printVec(filteredVec, "filteredVec");

	return filteredVec;
}

int main()
{
	const vector<int> input{7, 1, 0, 13, 0, 15, 20, -1};
	const auto predicate = [](int x) { return x > 10; };

	tbb::tick_count t0 = tbb::tick_count::now();

	const vector<int> output = vecFilter(input, predicate);

	/* NOTE: this includes the time spent printing the contents of vectors */
	cout << "\nTime: " << (tbb::tick_count::now() - t0).seconds() << " seconds" << endl;
	return 0;
}
