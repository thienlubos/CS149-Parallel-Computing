#include <stdio.h>
#include <algorithm>
#include <pthread.h>
#include <math.h>
#include <immintrin.h>

#include "utils.h"
#include "CycleTimer.h"
#include "sqrt_ispc.h"



using namespace ispc;

extern void sqrtSerial(int N, float startGuess, float* values, float* output);

static void verifyResult(int N, float* result, float* gold) {
    for (int i=0; i<N; i++) {
        if (fabs(result[i] - gold[i]) > 1e-4) {
            printf("Error: [%d] Got %f expected %f\n", i, result[i], gold[i]);
        }
    }
}

void sqrt_avx2(int N, float *values, float *output) {
    for (int i = 0; i < N; i += 8) {
        __m256 x = _mm256_loadu_ps(values + i);
        __m256 y = _mm256_rsqrt_ps(x);
        __m256 result = _mm256_mul_ps(x, y);
        _mm256_storeu_ps(output + i, result);
    }
}

int main() {

    const unsigned int N = 20 * 1000 * 1000;
    const float initialGuess = 1.0f;

    float* values = new float[N];
    float* output = new float[N];
    float* gold = new float[N];

    const float minValue = 0.001f;
    const float maxValue = 2.998f;
    const float steps = 30;
    const float inc = (maxValue - minValue) / steps;

    for (int step=0; step<=steps; step++) {
        float assignedValue = minValue + inc * step;
        for (unsigned int i=0; i<N; i++)
        {
            // TODO: CS149 students.  Attempt to change the values in the
            // array here to meet the instructions in the handout: we want
            // to you generate best and worse-case speedups
            
            // starter code populates array with random input values
            // values[i] = .001f + 2.998f * static_cast<float>(rand()) / RAND_MAX;
            values[i] =  assignedValue;        //@@ Thien Lu add
        }

        // generate a gold version to check results
        for (unsigned int i=0; i<N; i++)
            gold[i] = sqrt(values[i]);

        //
        // And run the serial implementation 3 times, again reporting the
        // minimum time.
        //
        double minSerial = 1e30;
        for (int i = 0; i < 3; ++i) {
            double startTime = CycleTimer::currentSeconds();
            sqrtSerial(N, initialGuess, values, output);
            double endTime = CycleTimer::currentSeconds();
            minSerial = std::min(minSerial, endTime - startTime);
        }

        printf("[sqrt serial]:\t\t[%.3f] ms\n", minSerial * 1000);

        verifyResult(N, output, gold);

        //
        // Compute the image using the ispc implementation; report the minimum
        // time of three runs.
        //
        double minISPC = 1e30;
        for (int i = 0; i < 3; ++i) {
            double startTime = CycleTimer::currentSeconds();
            sqrt_ispc(N, initialGuess, values, output);
            double endTime = CycleTimer::currentSeconds();
            minISPC = std::min(minISPC, endTime - startTime);
        }

        printf("[sqrt ispc]:\t\t[%.3f] ms\n", minISPC * 1000);

        verifyResult(N, output, gold);

        // Clear out the buffer
        for (unsigned int i = 0; i < N; ++i)
            output[i] = 0;

        //
        // Tasking version of the ISPC code
        //
        double minTaskISPC = 1e30;
        for (int i = 0; i < 3; ++i) {
            double startTime = CycleTimer::currentSeconds();
            sqrt_ispc_withtasks(N, initialGuess, values, output);
            double endTime = CycleTimer::currentSeconds();
            minTaskISPC = std::min(minTaskISPC, endTime - startTime);
        }

        printf("[sqrt task ispc]:\t[%.3f] ms\n", minTaskISPC * 1000);

        verifyResult(N, output, gold);

        // Clear out the buffer
        for (unsigned int i = 0; i < N; ++i)
            output[i] = 0;

        //
        // Compute the image using the AVX2 implementation
        //
        double minAVX2 = 1e30;
        for (int i = 0; i < 3; ++i) {
            double startTime = CycleTimer::currentSeconds();
            sqrt_avx2(N, values, output);
            double endTime = CycleTimer::currentSeconds();
            minAVX2 = std::min(minAVX2, endTime - startTime);
        }

        printf("[sqrt AVX2]:\t\t[%.3f] ms\n", minAVX2 * 1000);

        printf("\t\t\t\t(%.2fx speedup from ISPC)\n", minSerial/minISPC);
        printf("\t\t\t\t(%.2fx speedup from task ISPC)\n", minSerial/minTaskISPC);
        printf("\t\t\t\t(%.2fx speedup from AVX2)\n", minSerial/minAVX2);

        //@@ Thien Lu add
        std::string outdir = "results";
        float taskSpeedup = minSerial/minTaskISPC;
        float speedup = minSerial/minISPC;
        float AVX2Speedup = minSerial/minAVX2;

        std::string speedout = outdir + "/speed_results_ispc_nohup.txt";
        writeSpeedResults(speedout, assignedValue, speedup);

        std::string speedoutTask = outdir + "/speed_results_task_ispc_nohup.txt";
        writeSpeedResults(speedoutTask, assignedValue, taskSpeedup);

        std::string speedoutAVX2 = outdir + "/speed_results_AVX2_nohup.txt";
        writeSpeedResults(speedoutAVX2, assignedValue, AVX2Speedup);
        //@@
    }

    delete [] values;
    delete [] output;
    delete [] gold;

    return 0;
}
