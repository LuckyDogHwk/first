#pragma once

#include "Config.h"
#include "MathUtils.h"
#include <SFML/Graphics.hpp>
#include <cmath>
#include <string>

enum class BallOwner
{
    Player,
    Bot,
    Spore
};

struct Ball
{
    sf::Vector2f position{};
    sf::Vector2f velocity{};
    float mass = 0.f;
    sf::Color color = sf::Color::White;
    std::string name;
    BallOwner owner = BallOwner::Bot;
    int ownerId = -1;
    float splitTimer = 0.f;
    float spitTimer = 0.f;
    float mergeTimer = 0.f;  // 合并倒计时，分身后需要等待才能合并
    bool isPlayerPart = false;
    float sporeTravelDist = 0.f;  // 孢子已滑行距离
    const sf::Texture* skinTexture = nullptr;  // 皮肤纹理（可选）

    float Radius() const 
    { 
        // 孢子使用固定半径
        if (owner == BallOwner::Spore)
            return Config::SporeRadius;
        return MassToRadius(mass); 
    }

    float SpeedForMass() const
    {
        const float r = std::max(Radius(), 12.f);
        // 指数提高，重量越大速度降低更快
        const float speed = Config::BaseSpeed *
            std::pow(Config::SpeedReferenceRadius / r, 0.65f);
        return std::min(speed, Config::MaxBallSpeed);
    }

    void Draw(sf::RenderTarget& target, const sf::Font* font) const;
    void Update(float dt, const sf::Vector2f& targetPos, bool isControlled);
};
