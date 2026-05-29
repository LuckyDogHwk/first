#pragma once

#include "Config.h"
#include "MathUtils.h"
#include <SFML/Graphics.hpp>

// 刺球结构
struct ThornBall
{
    sf::Vector2f position{};
    sf::Color color = sf::Color(150, 255, 100);  // 亮绿色
    float mass = Config::ThornBallMassSmall;  // 默认小刺球

    float Radius() const { return MassToRadius(mass); }

    void Draw(sf::RenderTarget& target) const
    {
        float r = Radius();
        const int spikeCount = 16;           // 锯齿数量
        const float innerR = r * 0.85f;      // 内圈半径（锯齿底部，改小锯齿）
        const float outerR = r * 1.05f;      // 外圈半径（锯齿尖端，改小锯齿）

        sf::ConvexShape thornShape;
        thornShape.setPointCount(spikeCount * 2);
        thornShape.setFillColor(color);
        thornShape.setOutlineThickness(2.f);
        thornShape.setOutlineColor(sf::Color(80, 180, 40));

        for (int i = 0; i < spikeCount; ++i)
        {
            float angleBase = i * 2.f * 3.14159f / spikeCount;
            float angleNext = (i + 1) * 2.f * 3.14159f / spikeCount;
            float angleMid = (angleBase + angleNext) * 0.5f;

            // 内圈点（锯齿底部）
            sf::Vector2f innerPoint(
                position.x + std::cos(angleBase) * innerR,
                position.y + std::sin(angleBase) * innerR
            );
            // 外圈点（锯齿尖端）
            sf::Vector2f outerPoint(
                position.x + std::cos(angleMid) * outerR,
                position.y + std::sin(angleMid) * outerR
            );

            thornShape.setPoint(i * 2, innerPoint);
            thornShape.setPoint(i * 2 + 1, outerPoint);
        }

        target.draw(thornShape);
    }
};
