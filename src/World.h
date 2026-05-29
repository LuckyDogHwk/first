#pragma once

#include "Ball.h"
#include "Pellet.h"
#include "ThornBall.h"
#include <SFML/Graphics.hpp>
#include <random>
#include <vector>
#include <string>

// 排行榜条目
struct LeaderboardEntry
{
    std::string name;
    float totalMass;
    bool isPlayer;
};

class World
{
public:
    void Reset(unsigned int seed);
    void SetPlayerInfo(const std::string& playerName, const sf::Texture* skinTex);
    void Update(float dt, const sf::Vector2f& mouseWorldPos);

    void TrySplitPlayer(const sf::Vector2f& direction);
    void TrySpitPlayer(const sf::Vector2f& direction);
    void TrySplitBot(int ownerId, const sf::Vector2f& direction);
    void TrySpitBot(int ownerId, const sf::Vector2f& direction);

    void Draw(sf::RenderTarget& target, const sf::Font* font) const;

    sf::Vector2f GetPlayerCenter() const;
    float GetPlayerTotalMass() const;
    bool IsPlayerAlive() const;
    std::vector<LeaderboardEntry> GetLeaderboard() const;  // 获取排行榜数据

private:
    std::mt19937 rng_{ 42u };
    std::vector<Pellet> pellets_;
    std::vector<ThornBall> thornBalls_;
    std::vector<Ball> balls_;

    int nextBotId_ = 0;
    std::string playerName_ = u8"你";
    const sf::Texture* playerSkinTex_ = nullptr;

    void SpawnPellets();
    void SpawnThornBalls();
    void SpawnBots();
    void SpawnPlayer();
    void ResolveCollisions();
    void ResolveMerges();
    void ApplyPlayerBallInteractions(float dt);
    void UpdateBots(float dt);
    void RemoveDeadBalls();
    void MaintainPelletCount();
    void MaintainThornBallCount();

    bool CanEat(const Ball& eater, const Ball& prey) const;
    bool CanEat(const Ball& eater, const Pellet& pellet) const;
    bool CanEat(const Ball& eater, const ThornBall& thorn) const;
    void Absorb(Ball& eater, Ball& prey);
    void Absorb(Ball& eater, Pellet& pellet);
    void AbsorbThornBall(Ball& eater, ThornBall& thorn);  // 吞噬刺球并烟花分裂

    std::vector<size_t> GetPlayerBallIndices() const;
};
