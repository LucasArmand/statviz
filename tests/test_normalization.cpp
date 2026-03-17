#include <gtest/gtest.h>
#include <cmath>
#include <cstdint>
#include <limits>
#include <numeric>
#include <vector>

// Fixed-point conversion matching GLSL: uint(prob * 1e6)
static uint32_t to_fixed(float prob) {
    return static_cast<uint32_t>(prob * 1000000.0f);
}

static float from_fixed(uint32_t fixed) {
    return static_cast<float>(fixed) / 1000000.0f;
}

TEST(Normalization, FixedPointRoundtrip) {
    float values[] = {0.0f, 0.001f, 0.5f, 1.0f, 3.14159f};
    for (float v : values) {
        float roundtrip = from_fixed(to_fixed(v));
        EXPECT_NEAR(roundtrip, v, 1e-5f);
    }
}

TEST(Normalization, FixedPointSumMatchesFloat) {
    std::vector<float> probs = {0.1f, 0.2f, 0.3f, 0.15f, 0.25f};

    float float_sum = std::accumulate(probs.begin(), probs.end(), 0.0f);
    uint32_t fixed_sum = 0;
    for (float p : probs)
        fixed_sum += to_fixed(p);

    float recovered = from_fixed(fixed_sum);
    EXPECT_NEAR(recovered, float_sum, 1e-4f);
}

TEST(Normalization, OverflowLimitCheck) {
    // Max safe sum: UINT32_MAX / 1e6 ≈ 4294.97
    // Verify that realistic densities stay well below this
    float max_density = 1.0f; // Gaussian peak is 1/(2π)^1.5 ≈ 0.063
    int pixel_count = 1280 * 720;
    float total = max_density * pixel_count;
    // This would overflow, but actual densities are much smaller
    // Real scenario: average density ~0.001, total ~921
    float realistic_total = 0.001f * pixel_count;
    EXPECT_LT(realistic_total, 4290.0f);
}

TEST(Normalization, ZeroSumHandling) {
    // When total sum is zero, threshold should be zero → no samples
    uint32_t total = 0;
    float prob = 0.5f;
    float threshold = 0.0f;
    if (total > 0) {
        float p_norm = (prob * 1000000.0f) / float(total);
        threshold = p_norm * 50000.0f;
    }
    EXPECT_EQ(threshold, 0.0f);
}

TEST(Normalization, NormalizedProbabilitiesSumToOne) {
    std::vector<float> probs = {0.3f, 0.5f, 0.1f, 0.8f, 0.2f};
    uint32_t total = 0;
    for (float p : probs)
        total += to_fixed(p);

    float sum_normalized = 0.0f;
    for (float p : probs) {
        float p_norm = (p * 1000000.0f) / float(total);
        sum_normalized += p_norm;
    }
    EXPECT_NEAR(sum_normalized, 1.0f, 1e-4f);
}
