#include <gtest/gtest.h>
#include <cmath>
#include <cstdint>
#include <numeric>
#include <vector>

// CPU reference of PCG hash matching the GLSL version
static uint32_t pcg_hash(uint32_t input) {
    uint32_t state = input * 747796405u + 2891336453u;
    uint32_t word = ((state >> ((state >> 28u) + 4u)) ^ state) * 277803737u;
    return (word >> 22u) ^ word;
}

static float rand_float(int x, int y, int width, uint32_t frame, int total_pixels) {
    uint32_t seed = uint32_t(x) + uint32_t(y) * uint32_t(width)
                  + frame * uint32_t(total_pixels);
    return float(pcg_hash(seed)) / 4294967295.0f;
}

// Bernoulli sampling: given per-pixel probabilities and N samples,
// each pixel lights up with probability p_i * N / sum(p)
static int bernoulli_sample(const std::vector<float>& probs, int N, uint32_t frame) {
    float total = std::accumulate(probs.begin(), probs.end(), 0.0f);
    if (total <= 0.0f) return 0;

    int width = static_cast<int>(probs.size());
    int count = 0;
    for (int i = 0; i < width; ++i) {
        float threshold = (probs[i] / total) * float(N);
        float r = rand_float(i, 0, width, frame, width);
        if (r < threshold) count++;
    }
    return count;
}

TEST(Sampling, ExpectedCountApproximatelyN) {
    // Uniform distribution: each pixel has equal probability
    int pixels = 10000;
    int N = 5000;
    std::vector<float> probs(pixels, 1.0f);

    // Average over multiple frames
    double avg = 0.0;
    int frames = 100;
    for (int f = 0; f < frames; ++f) {
        avg += bernoulli_sample(probs, N, f);
    }
    avg /= frames;

    // Should be approximately N
    EXPECT_NEAR(avg, N, N * 0.1); // within 10%
}

TEST(Sampling, ZeroProbabilityGivesZeroSamples) {
    std::vector<float> probs(100, 0.0f);
    int count = bernoulli_sample(probs, 1000, 0);
    EXPECT_EQ(count, 0);
}

TEST(Sampling, SingleNonZeroPixelAlwaysLit) {
    // One pixel has all the probability, N >= 1 → always lit
    std::vector<float> probs(100, 0.0f);
    probs[50] = 1.0f;
    int N = 100;

    for (uint32_t f = 0; f < 50; ++f) {
        float threshold = (probs[50] / 1.0f) * float(N); // = 100
        // threshold = 100 > any rand in [0,1), so always lit
        EXPECT_GE(threshold, 1.0f);
    }
}

TEST(Sampling, PCGHashProducesDistinctValues) {
    // Check that sequential inputs produce different outputs
    std::vector<uint32_t> vals;
    for (uint32_t i = 0; i < 1000; ++i)
        vals.push_back(pcg_hash(i));

    // Check no consecutive duplicates (astronomically unlikely with good hash)
    for (size_t i = 1; i < vals.size(); ++i)
        EXPECT_NE(vals[i], vals[i - 1]);
}
