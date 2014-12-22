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
static inline float pDistance(
	std::pair<float, float> a,
	std::pair<float, float> b)
{
	// we need aligned memory to load the xmmN registers
	__declspec(align(16)) float va[4] = {
		0.f,
		0.f,
		a.second,
		a.first,
	};
	__declspec(align(16)) float vb[4] = {
		0.f,
		0.f,
		b.second,
		b.first,
	};
	// load the values in two SSE registers
	__m128 reg1 = _mm_load_ps(va);
	__m128 reg2 = _mm_load_ps(vb);
	// subtract them
	reg1 = _mm_sub_ps(reg1, reg2);
	// raise the results ^2
	reg2 = reg1;
	reg1 = _mm_mul_ps(reg1, reg2);
	// store them back in main memory
	_mm_store_ps(va, reg1);

	// return the result
	return va[2] + va[3];
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

	while (!feof(f))
	{
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
			// bilinear interpolation didn't work; see revision history for code if you're interested

			// inverse distance weighing interpolation
			// (v[i] * (1-w[i]) / (1-w[i])
			//          1
			// w = -----------
			//           1
			//        ------
			//      d^ 1.857

			// prepare the 4 computed results in some aligned memory;
			// we'll load it in an SSE register later
			__declspec(align(16)) float va[4];
			memcpy(va, (float*)&(found->second), 4 * sizeof(float));
			// compute the coordinates of the points in normalized space
			std::pair<float, float> p((err - found->first.left)/g_extents[0], (derr - found->first.top)/g_extents[1]);
			// these are constants
			static std::pair<float, float> const
				c1(0.f, 0.f),
				c2(1.f, 0.f),
				c3(0.f, 1.f),
				c4(1.f, 1.f);
			// compute the distances and store results in aligned memory
			__declspec(align(16)) float vb[4] = {
				pDistance(c1, p),
				pDistance(c2, p),
				pDistance(c3, p),
				pDistance(c4, p),
			};
			// if a distance is 0, the result is that point's
			for (size_t i = 0; i < 4; ++i) {
				if (vb[i] < 1.0e-7f) {
					return va[i];
					
				}
				else {
					// 1.857 was computed experimentally to reduce the
					// error between this and gnu octave / matlab
					vb[i] = powf(vb[i], 1.f / 1.857f);
				}
			}
			// our xmmN registers
			__m128 reg1, reg2;
			// compute weights
			// -- load distances
			reg1 = _mm_load_ps(vb);
			// -- invert (1/x) them to compute the weights
			static __declspec(align(16)) float ones[4] = { 1.f, 1.f, 1.f, 1.f };
			reg2 = _mm_load_ps(ones);
			reg2 = _mm_div_ps(reg2, reg1);
			// multiply the values with the weights
			reg1 = _mm_load_ps(va);
			reg1 = _mm_mul_ps(reg1, reg2);
			// retrieve the values
			_mm_store_ps(va, reg1);
			_mm_store_ps(vb, reg2);
			// compute final value
			// use SSE again because we have 6 sums to do
			// so stream 4 of them
			//          lane1 lane2 lane3 lane4
			//    xmm0   top1  top2  bot1  bot2
			//    xmm1   top3  top4  bot3  bot4
			__declspec(align(16)) float vc[8] = {
				va[0], va[1],
				vb[0], vb[1],
				va[2], va[3],
				vb[2], vb[3],
			};
			reg1 = _mm_load_ps(vc);
			reg2 = _mm_load_ps(vc + 4);
			reg1 = _mm_add_ps(reg1, reg2);
			// save results
			_mm_store_ps(va, reg1);

			// finish the final two sums
			// and divide them
			// and that's our interpolated value
			return (va[0] + va[1]) / (va[2] + va[3]);
		}
		else {
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