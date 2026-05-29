#pragma once

namespace Config
{
    // 窗口
    inline constexpr int WindowWidth = 1280;
    inline constexpr int WindowHeight = 720;
    inline constexpr const char* WindowTitle = u8"球球大作战";

    // 移动：BaseSpeed 为参考半径下的速度，越大球越慢
    inline constexpr float SpeedReferenceRadius = 35.f;
    inline constexpr float MaxBallSpeed = 180.f;

    // 世界地图（逻辑坐标，比窗口大）
    inline constexpr float WorldWidth = 5000.f;
    inline constexpr float WorldHeight = 5000.f;

    // 彩豆
    inline constexpr int PelletCount = 1500;  // 更多彩豆
    inline constexpr float PelletRadius = 5.f;
    inline constexpr float PelletMass = 50.f;  // 彩豆质量（再次提高）

    // 刺球（分大小两种）
    inline constexpr int ThornBallCount = 20;  // 刺球数量
    inline constexpr int ThornBallSmallCount = 15;  // 小刺球数量
    inline constexpr float ThornBallMassLarge = 8800.f;  // 大刺球：初始大小的4倍
    inline constexpr float ThornBallMassSmall = 6600.f;  // 小刺球：初始大小的3倍
    inline constexpr int ThornBallSplitCount = 8;  // 吞噬后分裂成8个分身

    // 孢子
    inline constexpr float SporeRadius = 8.f;      // 孢子固定半径
    inline constexpr float SporeMass = 300.f;      // 孢子质量 = 彩豆×10
    inline constexpr float SporeSlideDist = 80.f;  // 孢子固定滑行距离

    // 球体（最小半径约为彩豆 3.5 倍）
    inline constexpr float MinBallMass = 1000.f;
    inline constexpr float PlayerStartMass = 1800.f;  // 初始体积减小
    inline constexpr float MinSplitMass = 1600.f;  // 分身门槛降低
    inline constexpr float MinSpitMass = 500.f;  // 吐球最低质量（只要够吐出一颗就行）
    inline constexpr float SpitMassCost = 400.f;
    inline constexpr float BaseSpeed = 95.f;
    inline constexpr float SpitSpeed = 180.f;  // 孢子初始速度（降低，固定距离）
    inline constexpr float SplitSpeed = 220.f;
    inline constexpr float SplitCooldown = 0.5f;
    inline constexpr float SpitCooldown = 0.08f;
    inline constexpr float MergeDelay = 8.f; // 分身基础合并延迟（缩短）
    inline constexpr float MergeDelayRadiusFactor = 0.15f; // 合并延迟随半径增长系数（缩短）
    inline constexpr float EatOverlapRatio = 0.70f; // 吞噬/合并所需重叠比例（70%）
    inline constexpr int MaxPlayerBalls = 16;  // 最多分身4次 = 16个球

    // AI 机器人
    inline constexpr int BotCount = 12;
    inline constexpr float BotStartMassMin = 1200.f;
    inline constexpr float BotStartMassMax = 3500.f;

    // 吞噬判定：需比对方大一定比例
    inline constexpr float EatRatio = 1.15f;
}
