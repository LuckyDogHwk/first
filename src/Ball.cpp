#include "Ball.h"
#include "Config.h"

void Ball::Draw(sf::RenderTarget& target, const sf::Font* font) const
{
    const float r = Radius();

    sf::CircleShape circle(r);
    circle.setOrigin(r, r);
    circle.setPosition(position);

    if (skinTexture && owner != BallOwner::Spore)
    {
        // 使用皮肤纹理
        circle.setTexture(skinTexture, true);
        circle.setFillColor(sf::Color::White);
    }
    else
    {
        circle.setFillColor(color);
    }

    circle.setOutlineThickness(std::max(1.f, r * 0.04f));
    circle.setOutlineColor(color + sf::Color(30, 30, 30));
    target.draw(circle);

    if (font && r > 18.f && !name.empty())
    {
        sf::Text text;
        text.setFont(*font);
        text.setString(ToSfString(name));
        text.setCharacterSize(static_cast<unsigned int>(std::clamp(r * 0.35f, 12.f, 28.f)));
        text.setFillColor(sf::Color::White);
        text.setOutlineColor(sf::Color(0, 0, 0, 180));
        text.setOutlineThickness(1.f);
        const sf::FloatRect bounds = text.getLocalBounds();
        text.setOrigin(bounds.left + bounds.width * 0.5f, bounds.top + bounds.height * 0.5f);
        text.setPosition(position);
        target.draw(text);
    }
}

void Ball::Update(float dt, const sf::Vector2f& targetPos, bool isControlled)
{
    splitTimer = std::max(0.f, splitTimer - dt);
    spitTimer = std::max(0.f, spitTimer - dt);
    mergeTimer = std::max(0.f, mergeTimer - dt);

    if (isControlled)
    {
        sf::Vector2f dir = Normalize(targetPos - position);
        const float speed = SpeedForMass();
        const sf::Vector2f targetVel = dir * speed;
        velocity += (targetVel - velocity) * std::min(1.f, dt * 5.f);
    }

    // 孢子滑行固定距离后停止
    if (owner == BallOwner::Spore)
    {
        float moveDist = Length(velocity) * dt;
        sporeTravelDist += moveDist;
        
        if (sporeTravelDist >= Config::SporeSlideDist)
        {
            velocity = { 0.f, 0.f };  // 达到固定距离，停止
        }
    }

    position += velocity * dt;
    position = ClampToWorld(position, Radius(), Config::WorldWidth, Config::WorldHeight);

    if (!isControlled && owner != BallOwner::Spore)
    {
        velocity *= std::pow(0.05f, dt);  // 非孢子：快速停止
    }
}
