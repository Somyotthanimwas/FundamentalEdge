#ifndef CONFIG_H
#define CONFIG_H

// ============================================================
// FundamentalUpdater_rev12
// Swing Scanner configuration
//
// แก้ค่าการ Scan ทั้งหมดที่ไฟล์นี้ไฟล์เดียว
// ============================================================

namespace SwingConfig
{
    // จำนวนแท่งขั้นต่ำ
    constexpr int MIN_BARS = 55;

    // Trend
    constexpr int SMA_FAST_PERIOD = 20;
    constexpr int SMA_SLOW_PERIOD = 50;

    // Momentum
    constexpr int ROC_PERIOD = 10;
    constexpr double MIN_ROC_PERCENT = 4.0;

    // Breakout
    constexpr int BREAKOUT_LOOKBACK = 20;
    constexpr double BREAKOUT_THRESHOLD = 0.99;

    // Volume
    constexpr int VOLUME_AVG_PERIOD = 20;
    constexpr double MIN_VOLUME_RATIO = 1.20;

    // RSI
    constexpr int RSI_PERIOD = 14;
    constexpr double RSI_MIN = 50.0;
    constexpr double RSI_MAX = 70.0;

    // ATR
    constexpr int ATR_PERIOD = 14;
    constexpr double MAX_ATR_PERCENT = 8.0;

    // Signal requirements
    constexpr int MIN_CONFIRMATIONS = 3;
    constexpr int MIN_SCORE = 75;

    // Result
    constexpr int TOP_N = 5;

    // Target
    constexpr double TARGET_1_PERCENT = 5.0;
    constexpr double TARGET_2_PERCENT = 10.0;

    // Stop loss
    constexpr double MIN_STOP_PERCENT = 4.0;
    constexpr double ATR_STOP_MULTIPLIER = 2.0;
    constexpr double MAX_STOP_PERCENT = 10.0;
}

#endif






