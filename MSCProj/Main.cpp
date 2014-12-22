#include <cstdio>
#include <vector>
#include <algorithm> // for vector processing
#include <ctime> // for timing the computation
#include <xmmintrin.h> // use SSE intrinsics to make this code more awesome

// include & define dependent libraries and types
#include <map>
#include <utility>
typedef struct { float left, top, right, bottom; } rect_t;
typedef struct { float v[4]; } com_t;
// include the mamdani matrix generated with gen_cleaned.pl
#include <mamdani.ixx>

// computes the squared distance between two points
static inline void pDistanceX2(
	std::pair<float, float> a1,
	std::pair<float, float> b1,
	std::pair<float, float> a2,
	std::pair<float, float> b2,
	float* output)
{
	// we need aligned memory to load the xmmN registers
	__declspec(align(16)) float va[4] = {
		a2.second,
		a2.first,
		a1.second,
		a1.first,
	};
	__declspec(align(16)) float vb[4] = {
		b2.second,
		b2.first,
		b1.second,
		b1.first,
	};
	// load the values in two SSE registers
	__m128 xmm0 = _mm_load_ps(va);
	__m128 xmm1 = _mm_load_ps(vb);
	// subtract them
	xmm0 = _mm_sub_ps(xmm0, xmm1);
	// raise the results ^2
	xmm1 = xmm0;
	xmm0 = _mm_mul_ps(xmm0, xmm1);
	// store them back in main memory
	_mm_store_ps(va, xmm0);

	// return the result
	output[0] = va[2] + va[3];
	output[1] = va[0] + va[1];
}

int main(int argc, char* argv[])
{
	// time i/o separately since it takes a while
	volatile clock_t cstartio = clock();
#pragma warning(disable:4996)
	FILE* f = fopen("date.txt", "r");
	FILE* g = fopen("output.txt", "w");
	// store the input in a vector for faster processing
	std::vector<float> inputs;
	inputs.reserve(10002);

	while (!feof(f)) {
		float d;
		fscanf(f, "%f", &d);
		inputs.push_back(d);
	}

	// declare & initialize our outputs vector now that
	// we know how many results we'll have
	std::vector<float> outputs(inputs.size());

	// stop timing I/O
	volatile clock_t cstopio = clock();
	volatile double spentio = (double)(cstopio - cstartio) / CLOCKS_PER_SEC;

	printf("Took %lfs to _read_ %ld values\n", spentio, inputs.size());

	// time the computation proper
	volatile clock_t cstart = clock();

	// state variables
	float lastErr = 0.0; // previous error; initially 0
	auto func = [&](float err) -> float {
		// compute the derivate of the error using the previous value
		float derr = err - lastErr;
		// update state
		lastErr = err;

		// locate the partition the current point falls in
		auto found = std::find_if(g_mamdani.begin(), g_mamdani.end(), [&derr, &err](decltype(g_mamdani.front())& o) -> bool {
			return o.first.left <= err
				&& o.first.right > err
				&& o.first.top <= derr
				&& o.first.bottom > derr;
		});

		if (found != g_mamdani.end()) {
			// NOTE: bilinear interpolation didn't work; see revision history for code if you're interested

			// inverse distance weighing interpolation
			//     v00 * w00 + v10 * w10 + v01 * w01 + v11 * w11
			// o = ---------------------------------------------
			//                 w00 + w10 + w01 + w11
			//
			//          1
			// w = -----------
			//        /   1  \
			//        |------|
			//      d^\ 1.857/
			//
			// 1.857 was computed experimentally to reduce the
			// error between this and gnu octave / matlab
			//
			// we can use SSE for a lot of the math since
			// we're working with 4 sets of values that can
			// be done in parallel for the most part
			//
			// for that, loading and storing memory to/from
			// the xmmN registers needs to be done with aligned
			// memory

			// prepare the 4 computed results in some aligned memory;
			// we'll load it in an SSE register later
			__declspec(align(16)) float va[4];
			memcpy(va, (float*)&(found->second), 4 * sizeof(float));
			// compute the coordinates of the points in normalized space
			std::pair<float, float> p(
				(err - found->first.left)/g_extents[0],
				(derr - found->first.top)/g_extents[1]);
			// these are constants
			static std::pair<float, float> const
				c1(0.f, 0.f),
				c2(1.f, 0.f),
				c3(0.f, 1.f),
				c4(1.f, 1.f);
			// compute the distances and store results in aligned memory
			__declspec(align(16)) float vb[4] = {};
			pDistanceX2(c1, p, c2, p, vb);
			pDistanceX2(c3, p, c4, p, vb + 2);
			// if a distance is 0, the result is that point's
			for (size_t i = 0; i < 4; ++i) {
				if (vb[i] < 1.0e-7f) {
					return va[i];
				} else {
					// 1.857 was computed experimentally to reduce the
					// error between this and gnu octave / matlab
					vb[i] = powf(vb[i], 1.f / 1.857f);
				}
			}

			// our xmmN registers
			__m128 xmm0, xmm1;
			// compute weights
			//
			// stream the 4 divisions
			//      lane1 lane2 lane3 lane4
			// xmm1     1     1     1     1   /
			// xmm0   d00   d10   d01   d11
			//        w00   w10   w01   w11
			// -- load distances
			xmm0 = _mm_load_ps(vb);
			// -- invert (1/x) them to compute the weights
			static __declspec(align(16)) float ones[4] = { 1.f, 1.f, 1.f, 1.f };
			xmm1 = _mm_load_ps(ones);
			xmm1 = _mm_div_ps(xmm1, xmm0);
			// multiply the values with the weights
			//
			// stream the four multiplications
			//      lane1 lane2 lane3 lane4
			// xmm0   v00   v10   v01   v11   *
			// xmm1   w00   w10   w01   w11
			//        m00   m10   m01   m11
			xmm0 = _mm_load_ps(va);
			xmm0 = _mm_mul_ps(xmm0, xmm1);
			// retrieve the values
			_mm_store_ps(va, xmm0);
			_mm_store_ps(vb, xmm1);
			// compute final value
			//
			// use SSE again because we have 6 sums to do
			//  (m00 + m10) + (m01 + m11)
			//  -------------------------
			//  (w00 + w10) + (w01 + w11)
			//
			// so stream 4 of them
			//      lane1 lane2 lane3 lane4
			// xmm0   m00   m10   w00   w01   +
			// xmm1   m01   m11   w10   w11
			//      m0001 m1011 w0001 m1011
			//
			// but first we need to shuffle our registers a bit
			__m128 xmm2, xmm3;
			xmm2 = _mm_movehl_ps(xmm1, xmm0);
			xmm3 = _mm_movelh_ps(xmm0, xmm1);
			xmm2 = _mm_add_ps(xmm2, xmm3);
			// save results
			_mm_store_ps(va, xmm2);

			// finish the final two sums and divide them
			//     (m0001 + m1011)/(w0001 + w1011)
			// and that's our interpolated value
			return (va[0] + va[1]) / (va[2] + va[3]);
		} else {
			return 0.f;
		}
	};

	// process all inputs with our stateful function
	std::transform(inputs.begin(), inputs.end(), outputs.begin(), func);

	volatile clock_t cend = clock();

	// report spent time
	volatile double spent = (double)(cend - cstart) / CLOCKS_PER_SEC;

	printf("Took %lfs to process %ld values\n", spent, inputs.size());

	// time the output separately
	cstartio = clock();
	lastErr = 0;
	for (size_t i = 0; i < inputs.size(); ++i) {
		fprintf(g, "%10.6f, %10.6f => %6.2f\n", inputs[i], inputs[i] - lastErr, outputs[i]);
		lastErr = inputs[i];
	}
	cstopio = clock();
	spentio = (double)(cstopio - cstartio) / CLOCKS_PER_SEC;
	printf("Took %lfs to write %ld values to disk\n", spentio, outputs.size());

	return 0;
}