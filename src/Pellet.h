#pragma once

#include "Config.h"
#include "MathUtils.h"
#include <SFML/Graphics.hpp>

struct Pellet
{
    sf::Vector2f position{};
    sf::Color color = sf::Color::White;
    float mass = Config::PelletMass;

    float Radius() const { return Config::PelletRadius; }

    void Draw(sf::RenderTarget& target) const
    {
        sf::CircleShape circle(Config::PelletRadius);
        circle.setOrigin(Config::PelletRadius, Config::PelletRadius);
        circle.setPosition(position);
        circle.setFillColor(color);
        target.draw(circle);
    }
};
