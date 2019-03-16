#pragma once

#include <cmath>

#include <exception>
#include <random>

// Implementation of the Zipf's law.
// The code is adapted from https://stackoverflow.com/a/44154095,
// which seems to be itself a C++ translation of
// https://github.com/apache/commons-math/blob/138f84bfa5d36c8f6e2825640af1ed82daa9dc1d/src/main/java/org/apache/commons/math4/distribution/ZipfDistribution.java

namespace sse {

/** Clamp x to [min, max]. */
template<typename T>
static constexpr T clamp(const T x, const T min, const T max)
{
    return std::max(min, std::min(max, x));
}


template<class IntType = int, class RealType = double>
class ZipfianDistribution
{
public:
    using result_type    = IntType;
    using parameter_type = RealType;

    ZipfianDistribution(RealType alpha, IntType a, IntType b)
        : m_alpha(alpha), m_a(a), m_b(b), m_H_x1(H(1.5) - 1.0),
          m_H_n(H(m_b - m_a + 1.5)), m_unif_dist(m_H_x1, m_H_n)
    {
        if (a >= b) {
            throw std::invalid_argument("ZipfianDistribution constructor: a "
                                        "must be strictly smaller than b");
        }
        if (alpha <= 0) {
            throw std::invalid_argument("ZipfianDistribution constructor: "
                                        "alpha must be positive (strictly).");
        }
    }

    parameter_type alpha() const noexcept
    {
        return m_alpha;
    }

    result_type min() const noexcept
    {
        return m_a;
    }

    result_type max() const noexcept
    {
        return m_b;
    }

    result_type elements_count() const noexcept
    {
        return max() - min() + 1;
    }

    template<class Generator>
    result_type operator()(Generator& g) noexcept
    {
        const IntType n = elements_count();

        while (true) {
            const RealType u = m_unif_dist(g);
            const RealType x = H_inv(u);
            const IntType  k = clamp<IntType>(std::round(x), 1, n);
            if ((k - x <= m_alpha) || (u >= H(k + 0.5) - h(k))) {
                return k + min() - 1;
            }
        }
    }


    /** exp(x) - 1 / x */
    static constexpr RealType expxm1bx(const RealType x) noexcept
    {
        return (std::abs(x) > epsilon)
                   ? std::expm1(x) / x
                   : (1.0 + x / 2.0 * (1.0 + x / 3.0 * (1.0 + x / 4.0)));
    }

    /** log(1 + x) / x */
    static constexpr RealType log1pxbx(const RealType x) noexcept
    {
        return (std::abs(x) > epsilon)
                   ? std::log1p(x) / x
                   : 1.0 - x * ((1 / 2.0) - x * ((1 / 3.0) - x * (1 / 4.0)));
    }

    /** H(x) = log(x) if q == 1, (x^(1-q) - 1)/(1 - q) otherwise.
     * H(x) is an integral of h(x).
     *
     * Note the numerator is one less than in the paper order to work with all
     * positive q.
     */
    RealType H(const RealType x) const noexcept
    {
        const RealType log_x = std::log(x);
        return expxm1bx((1.0 - m_alpha) * log_x) * log_x;
    }

    /** The inverse function of H(x) */
    RealType H_inv(const RealType x) const noexcept
    {
        const RealType t = std::max(-1.0, x * (1.0 - m_alpha));
        return std::exp(log1pxbx(t) * x);
    }

    /** That hat function h(x) = 1 / (x ^ q) */
    RealType h(const RealType x) const noexcept
    {
        return std::exp(-m_alpha * std::log(x));
    }

private:
    // the parameters of the distribution
    RealType m_alpha;
    IntType  m_a;
    IntType  m_b;

    // useful pre-computed constants
    static constexpr RealType epsilon = 1e-8;

    RealType                                 m_H_x1;
    RealType                                 m_H_n;
    std::uniform_real_distribution<RealType> m_unif_dist;
};
} // namespace sse
