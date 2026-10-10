#pragma once
// Lock-free audio analysis for the visualizer.
//   Audio thread : push()    - copies samples into a ring buffer (no locks, no allocation)
//   GL thread    : update()  - FFT, log-frequency spectrum, bands, beat detection, waveform
// No JUCE dependency on purpose, so it can be unit-tested anywhere.
#include <array>
#include <atomic>
#include <algorithm>
#include <cmath>
#include <complex>
#include <cstdint>
#include <cstring>
#include <vector>

class AudioAnalyzer
{
public:
    static constexpr int kN    = 2048;      // FFT size
    static constexpr int kBins = 512;       // display bins (log frequency)
    static constexpr int kWave = 512;       // waveform points
    static constexpr int kRing = 1 << 14;   // ring buffer length per channel

    AudioAnalyzer()
    {
        for (auto& r : ring) r.fill(0.f);
        spec.fill(0.f); waveL.fill(0.f); waveR.fill(0.f);
        ages.fill(99.f); seeds.fill(0.f);
        tex.fill(0); histRow.fill(0);
        for (int i = 0; i < kN; ++i)
            window[(size_t) i] = 0.5f - 0.5f * std::cos(6.28318530718f * (float) i / (float) (kN - 1));
        // twiddles + bit reversal
        for (int i = 0; i < kN / 2; ++i)
            twiddle[(size_t) i] = std::polar(1.0f, -6.28318530718f * (float) i / (float) kN);
        int bits = 0; while ((1 << bits) < kN) ++bits;
        for (int i = 0; i < kN; ++i)
        {
            int r = 0; for (int b = 0; b < bits; ++b) if (i & (1 << b)) r |= 1 << (bits - 1 - b);
            bitrev[(size_t) i] = r;
        }
        rebuildMap();
    }

    // ---- audio thread -------------------------------------------------------
    void setSampleRate(double sr) noexcept
    {
        if (sr > 1000.0 && sr != sampleRate.load()) { sampleRate.store(sr); mapDirty.store(true); }
    }

    void push(const float* l, const float* r, int n) noexcept
    {
        if (l == nullptr) return;
        if (r == nullptr) r = l;
        const uint64_t w = writePos.load(std::memory_order_relaxed);
        for (int i = 0; i < n; ++i)
        {
            const size_t idx = (size_t) ((w + (uint64_t) i) & (uint64_t) (kRing - 1));
            ring[0][idx] = l[i];
            ring[1][idx] = r[i];
        }
        writePos.store(w + (uint64_t) n, std::memory_order_release);
    }

    // ---- GL thread ----------------------------------------------------------
    void update(float dt, float sens, float smoothing)
    {
        dt = std::clamp(dt, 0.001f, 0.1f);
        if (mapDirty.exchange(false)) rebuildMap();

        const uint64_t w = writePos.load(std::memory_order_acquire);
        if (w != lastW) idle = 0.f; else idle += dt;
        lastW = w;
        const bool silent = idle > 0.25f || w < (uint64_t) kN;
        const float gainDb = 20.f * std::log10(std::max(0.01f, sens));

        if (!silent)
            for (int i = 0; i < kN; ++i)
            {
                const size_t idx = (size_t) ((w - (uint64_t) kN + (uint64_t) i) & (uint64_t) (kRing - 1));
                L[(size_t) i] = ring[0][idx];
                R[(size_t) i] = ring[1][idx];
            }
        else { L.fill(0.f); R.fill(0.f); }

        // mono + RMS
        float e2 = 0.f;
        for (int i = 0; i < kN; ++i)
        {
            const float m = 0.5f * (L[(size_t) i] + R[(size_t) i]);
            mono[(size_t) i] = m; e2 += m * m;
        }
        const float rms = std::sqrt(e2 / (float) kN);

        // FFT
        for (int i = 0; i < kN; ++i)
            fftBuf[(size_t) bitrev[(size_t) i]] = std::complex<float>(mono[(size_t) i] * window[(size_t) i], 0.f);
        for (int len = 2; len <= kN; len <<= 1)
        {
            const int half = len >> 1, step = kN / len;
            for (int i = 0; i < kN; i += len)
                for (int j = 0; j < half; ++j)
                {
                    const auto t = fftBuf[(size_t) (i + j + half)] * twiddle[(size_t) (j * step)];
                    const auto u = fftBuf[(size_t) (i + j)];
                    fftBuf[(size_t) (i + j)] = u + t;
                    fftBuf[(size_t) (i + j + half)] = u - t;
                }
        }
        for (int i = 0; i <= kN / 2; ++i) mag[(size_t) i] = std::abs(fftBuf[(size_t) i]);

        // log-frequency display bins, in dB
        const float norm = 4.f / (float) kN;   // 0 dBFS sine ~ 1.0 (Hann coherent gain 0.5)
        for (int j = 0; j < kBins; ++j)
        {
            float m = 0.f;
            if (!useRange[(size_t) j])
            {
                const float p = binPos[(size_t) j];
                const int i0 = std::min((int) p, kN / 2 - 1);
                const float fr = p - (float) i0;
                m = mag[(size_t) i0] * (1.f - fr) + mag[(size_t) (i0 + 1)] * fr;
            }
            else
                for (int b = binLo[(size_t) j]; b <= binHi[(size_t) j]; ++b) m = std::max(m, mag[(size_t) b]);
            const float db = 20.f * std::log10(m * norm + 1e-7f);
            raw[(size_t) j] = std::clamp((db + gainDb + tilt[(size_t) j] + 78.f) / 66.f, 0.f, 1.f);
        }

        // smoothing (fast attack, adjustable release)
        const float atk = 1.f - std::exp(-dt * 80.f);
        const float relRate = 40.f + (2.5f - 40.f) * std::clamp(smoothing, 0.f, 1.f);
        const float rel = 1.f - std::exp(-dt * relRate);
        for (int j = 0; j < kBins; ++j)
        {
            float& s = spec[(size_t) j];
            s += (raw[(size_t) j] - s) * (raw[(size_t) j] > s ? atk : rel);
        }

        auto mean = [&](int a, int b) { float s = 0.f; for (int j = a; j < b; ++j) s += raw[(size_t) j]; return s / (float) std::max(1, b - a); };
        const float bR = mean(0, jBass), mR = mean(jBass, jMid), tR = mean(jMid, kBins);
        auto follow = [&](float& v, float target) { v += (target - v) * (target > v ? atk : rel); };
        follow(bass, bR); follow(mid, mR * 1.4f); follow(treble, tR * 1.8f);
        const float lvl = std::clamp((20.f * std::log10(rms + 1e-6f) + gainDb + 54.f) / 48.f, 0.f, 1.f);
        follow(level, lvl);

        // beat detection: positive flux of bass energy vs running average
        const float flux = std::max(0.f, bR - prevBass); prevBass = bR;
        fluxAvg += (flux - fluxAvg) * (1.f - std::exp(-dt * 2.f));
        sinceBeat += dt;
        if (flux > fluxAvg * 2.2f + 0.03f && bR > 0.15f && sinceBeat > 0.2f)
        {
            sinceBeat = 0.f; beat = 1.f;
            for (int i = 7; i > 0; --i) { ages[(size_t) i] = ages[(size_t) i - 1]; seeds[(size_t) i] = seeds[(size_t) i - 1]; }
            ages[0] = 0.f; seeds[0] = (float) (beatCount % 1000);
            ++beatCount;
        }
        beat *= std::exp(-dt * 5.5f);
        for (auto& a : ages) a = std::min(99.f, a + dt);

        // waveform with rising zero-crossing trigger
        int start = 512;
        for (int i = 200; i < 900; ++i) if (mono[(size_t) i - 1] < 0.f && mono[(size_t) i] >= 0.f) { start = i; break; }
        const float wg = std::max(0.1f, sens) * 1.4f;
        for (int k = 0; k < kWave; ++k)
        {
            const size_t idx = (size_t) (start + k * 2);
            waveL[(size_t) k] = std::tanh(L[idx] * wg);
            waveR[(size_t) k] = std::tanh(R[idx] * wg);
        }

        // pack GPU texture: row0 spectrum, row1 waveL, row2 waveR  (16-bit in R,G)
        for (int i = 0; i < kBins; ++i)
        {
            pack(0, i, spec[(size_t) i]);
            pack(1, i, waveL[(size_t) i] * 0.5f + 0.5f);
            pack(2, i, waveR[(size_t) i] * 0.5f + 0.5f);
            pack(3, i, 0.f);
            histRow[(size_t) i * 4 + 0] = (uint8_t) (std::clamp(spec[(size_t) i], 0.f, 1.f) * 255.f + 0.5f);
            histRow[(size_t) i * 4 + 3] = 255;
        }
    }

    // ---- results (read on the GL thread) -------------------------------------
    const uint8_t* texData() const noexcept { return tex.data(); }        // kBins x 4 RGBA8
    const uint8_t* histRowData() const noexcept { return histRow.data(); } // kBins RGBA8
    float getBass() const noexcept { return bass; }
    float getMid() const noexcept { return mid; }
    float getTreble() const noexcept { return treble; }
    float getLevel() const noexcept { return level; }
    float getBeat() const noexcept { return beat; }
    int   getBeatCount() const noexcept { return beatCount; }
    const std::array<float, 8>& getAges() const noexcept { return ages; }
    const std::array<float, 8>& getSeeds() const noexcept { return seeds; }
    const std::array<float, kWave>& getWaveL() const noexcept { return waveL; }
    const std::array<float, kWave>& getWaveR() const noexcept { return waveR; }
    const std::array<float, kBins>& getSpectrum() const noexcept { return spec; }

private:
    void pack(int row, int i, float v01) noexcept
    {
        const uint32_t q = (uint32_t) (std::clamp(v01, 0.f, 1.f) * 65535.f + 0.5f);
        uint8_t* p = &tex[((size_t) row * kBins + (size_t) i) * 4];
        p[0] = (uint8_t) (q >> 8); p[1] = (uint8_t) (q & 255u); p[2] = 0; p[3] = 255;
    }

    void rebuildMap()
    {
        const double sr = sampleRate.load();
        const double binHz = sr / kN;
        const double fMin = 30.0, fMax = std::min(18000.0, sr * 0.45);
        const double ratio = fMax / fMin;
        const double halfStep = std::pow(ratio, 0.5 / (kBins - 1));
        jBass = jMid = 1;
        for (int j = 0; j < kBins; ++j)
        {
            const double f = fMin * std::pow(ratio, (double) j / (kBins - 1));
            binPos[(size_t) j] = (float) (f / binHz);
            const int lo = (int) std::ceil(f / halfStep / binHz), hi = (int) std::floor(f * halfStep / binHz);
            binLo[(size_t) j] = std::max(1, lo);
            binHi[(size_t) j] = std::min(kN / 2 - 1, hi);
            useRange[(size_t) j] = binHi[(size_t) j] > binLo[(size_t) j];
            tilt[(size_t) j] = (float) std::clamp(3.0 * std::log2(f / 700.0), -9.0, 12.0);
            if (f <= 150.0) jBass = j + 1;
            if (f <= 2500.0) jMid = j + 1;
        }
    }

    // ring buffer
    std::array<std::array<float, kRing>, 2> ring;
    std::atomic<uint64_t> writePos { 0 };
    std::atomic<double> sampleRate { 44100.0 };
    std::atomic<bool> mapDirty { false };

    // GL-thread state
    uint64_t lastW = 0; float idle = 0.f;
    std::array<float, kN> L {}, R {}, mono {}, window {};
    std::array<std::complex<float>, kN> fftBuf {};
    std::array<std::complex<float>, kN / 2> twiddle {};
    std::array<int, kN> bitrev {};
    std::array<float, kN / 2 + 1> mag {};
    std::array<float, kBins> raw {}, spec {}, binPos {}, tilt {};
    std::array<int, kBins> binLo {}, binHi {};
    std::array<bool, kBins> useRange {};
    int jBass = 1, jMid = 1;
    std::array<float, kWave> waveL {}, waveR {};
    std::array<uint8_t, (size_t) kBins * 4 * 4> tex {};
    std::array<uint8_t, (size_t) kBins * 4> histRow {};
    float bass = 0, mid = 0, treble = 0, level = 0, beat = 0;
    float prevBass = 0, fluxAvg = 0, sinceBeat = 1.f;
    int beatCount = 0;
    std::array<float, 8> ages {}, seeds {};
};
