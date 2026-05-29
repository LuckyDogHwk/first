#include "World.h"
#include "Config.h"
#include <algorithm>
#include <cmath>
#include <map>

void World::Reset(unsigned int seed)
{
    rng_.seed(seed);
    pellets_.clear();
    thornBalls_.clear();
    balls_.clear();
    nextBotId_ = 0;

    SpawnPellets();
    SpawnThornBalls();
    SpawnPlayer();
    SpawnBots();
}

void World::SetPlayerInfo(const std::string& playerName, const sf::Texture* skinTex)
{
    playerName_ = playerName;
    playerSkinTex_ = skinTex;
}

void World::SpawnPellets()
{
    std::uniform_real_distribution<float> distX(20.f, Config::WorldWidth - 20.f);
    std::uniform_real_distribution<float> distY(20.f, Config::WorldHeight - 20.f);

    pellets_.reserve(Config::PelletCount);
    for (int i = 0; i < Config::PelletCount; ++i)
    {
        Pellet p;
        p.position = { distX(rng_), distY(rng_) };
        p.color = RandomBrightColor(static_cast<unsigned int>(rng_()));
        pellets_.push_back(p);
    }
}

void World::SpawnThornBalls()
{
    std::uniform_real_distribution<float> distX(100.f, Config::WorldWidth - 100.f);
    std::uniform_real_distribution<float> distY(100.f, Config::WorldHeight - 100.f);

    thornBalls_.reserve(Config::ThornBallCount);
    for (int i = 0; i < Config::ThornBallCount; ++i)
    {
        ThornBall t;
        t.position = { distX(rng_), distY(rng_) };
        // 前几个是大刺球，后面是小刺球
        if (i < Config::ThornBallCount - Config::ThornBallSmallCount)
        {
            t.mass = Config::ThornBallMassLarge;  // 大刺球
            t.color = sf::Color(100, 220, 80);    // 深绿色
        }
        else
        {
            t.mass = Config::ThornBallMassSmall;  // 小刺球
            t.color = sf::Color(150, 255, 100);   // 亮绿色
        }
        thornBalls_.push_back(t);
    }
}

void World::SpawnPlayer()
{
    Ball player;
    player.position = { Config::WorldWidth * 0.5f, Config::WorldHeight * 0.5f };
    player.mass = Config::PlayerStartMass;
    player.color = sf::Color(70, 160, 255);
    player.name = playerName_;
    player.owner = BallOwner::Player;
    player.ownerId = 0;
    player.isPlayerPart = true;
    player.skinTexture = playerSkinTex_;
    balls_.push_back(player);
}

void World::SpawnBots()
{
    std::uniform_real_distribution<float> distX(100.f, Config::WorldWidth - 100.f);
    std::uniform_real_distribution<float> distY(100.f, Config::WorldHeight - 100.f);
    std::uniform_real_distribution<float> distMass(Config::BotStartMassMin, Config::BotStartMassMax);

    static const char* botNames[] = {
        u8"小红", u8"大蓝", u8"绿巨人", u8"闪电", u8"影子",
        u8"糖果", u8"风暴", u8"火箭", u8"幽灵", u8"王者",
        u8"新手", u8"猎手"
    };

    for (int i = 0; i < Config::BotCount; ++i)
    {
        Ball bot;
        bot.position = { distX(rng_), distY(rng_) };
        bot.mass = distMass(rng_);
        bot.color = RandomBrightColor(static_cast<unsigned int>(rng_() + i * 17));
        bot.name = botNames[i % 12];
        bot.owner = BallOwner::Bot;
        bot.ownerId = nextBotId_++;
        balls_.push_back(bot);
    }
}

std::vector<size_t> World::GetPlayerBallIndices() const
{
    std::vector<size_t> indices;
    for (size_t i = 0; i < balls_.size(); ++i)
    {
        if (balls_[i].isPlayerPart)
            indices.push_back(i);
    }
    return indices;
}

sf::Vector2f World::GetPlayerCenter() const
{
    auto indices = GetPlayerBallIndices();
    if (indices.empty())
        return { Config::WorldWidth * 0.5f, Config::WorldHeight * 0.5f };

    sf::Vector2f sum{};
    for (size_t idx : indices)
        sum += balls_[idx].position;
    return sum / static_cast<float>(indices.size());
}

float World::GetPlayerTotalMass() const
{
    float total = 0.f;
    for (const auto& ball : balls_)
    {
        if (ball.isPlayerPart)
            total += ball.mass;
    }
    return total;
}

bool World::IsPlayerAlive() const
{
    for (const auto& ball : balls_)
    {
        if (ball.isPlayerPart)
            return true;
    }
    return false;
}

void World::Update(float dt, const sf::Vector2f& mouseWorldPos)
{
    for (size_t i = 0; i < balls_.size(); ++i)
    {
        Ball& ball = balls_[i];
        const bool controlled = ball.isPlayerPart;
        const sf::Vector2f target = controlled ? mouseWorldPos : ball.position;
        ball.Update(dt, target, controlled);
    }

    // 处理同玩家球之间的相互作用（吸引 + 接触保持）
    ApplyPlayerBallInteractions(dt);

    UpdateBots(dt);
    ResolveCollisions();
    ResolveMerges();  // 处理分身合并（小分身优先与最近分身合并）
    RemoveDeadBalls();
    MaintainPelletCount();
    MaintainThornBallCount();
}

void World::UpdateBots(float dt)
{
    std::uniform_real_distribution<float> jitter(-1.f, 1.f);

    // 统计每个 Bot 的分身数量和质心
    std::map<int, int> botBallCount;
    std::map<int, sf::Vector2f> botCenter;
    for (const Ball& ball : balls_)
    {
        if (ball.owner == BallOwner::Bot && ball.mass > 0.f)
        {
            botBallCount[ball.ownerId]++;
            botCenter[ball.ownerId] += ball.position;
        }
    }
    for (auto& pair : botCenter)
    {
        if (botBallCount[pair.first] > 0)
            pair.second /= static_cast<float>(botBallCount[pair.first]);
    }

    for (Ball& bot : balls_)
    {
        if (bot.owner != BallOwner::Bot)
            continue;

        sf::Vector2f target = bot.position;

        // 如果有多个分身，向质心聚拢（类似玩家控制）
        int count = botBallCount[bot.ownerId];
        if (count > 1)
        {
            target = botCenter[bot.ownerId];
            target += sf::Vector2f(jitter(rng_), jitter(rng_)) * 20.f;
            bot.Update(dt, target, true);
            continue;
        }

        // 单个分身时正常 AI 寻路
        float bestDist = 1e9f;

        for (const Pellet& pellet : pellets_)
        {
            const float d = Length(pellet.position - bot.position);
            if (d < bestDist)
            {
                bestDist = d;
                target = pellet.position;
            }
        }

        // Bot 也追刺球
        for (const ThornBall& thorn : thornBalls_)
        {
            if (thorn.mass <= 0.f)
                continue;
            if (CanEat(bot, thorn))
            {
                const float d = Length(thorn.position - bot.position);
                if (d < 800.f && d < bestDist)
                {
                    bestDist = d;
                    target = thorn.position;
                }
            }
        }

        for (const Ball& other : balls_)
        {
            if (&other == &bot)
                continue;
            if (CanEat(bot, other))
            {
                const float d = Length(other.position - bot.position);
                if (d < 600.f && d < bestDist)
                {
                    bestDist = d;
                    target = other.position;
                }
            }
            else if (CanEat(other, bot))
            {
                const float d = Length(other.position - bot.position);
                if (d < bot.Radius() * 3.f)
                {
                    target = bot.position - Normalize(other.position - bot.position) * 200.f;
                    break;
                }
            }
        }

        target += sf::Vector2f(jitter(rng_), jitter(rng_)) * 40.f;
        bot.Update(dt, target, true);
    }

    (void)dt;
}

void World::TrySplitPlayer(const sf::Vector2f& direction)
{
    // 检查当前玩家球数量是否已达上限
    int playerBallCount = 0;
    for (const Ball& ball : balls_)
    {
        if (ball.isPlayerPart)
            ++playerBallCount;
    }
    if (playerBallCount >= Config::MaxPlayerBalls)
        return;

    sf::Vector2f dir = Normalize(direction);
    if (dir.x == 0.f && dir.y == 0.f)
        dir = { 1.f, 0.f };

    std::vector<Ball> newBalls;
    for (Ball& ball : balls_)
    {
        if (!ball.isPlayerPart || ball.mass < Config::MinSplitMass || ball.splitTimer > 0.f)
            continue;

        const float halfMass = ball.mass * 0.5f;
        if (halfMass < Config::MinBallMass)
            continue;

        // 先记录分裂前的半径（用于计算分身距离）
        const float oldRadius = ball.Radius();
        
        ball.mass = halfMass;
        ball.splitTimer = Config::SplitCooldown;
        // 合并延迟 = 基础延迟 + 原半径 × 增长系数，体积越大越久
        ball.mergeTimer = Config::MergeDelay + oldRadius * Config::MergeDelayRadiusFactor;
        // 原球稍微后退一点
        ball.position -= dir * oldRadius * 0.5f;

        Ball clone = ball;
        clone.mass = halfMass;
        // 克隆球向前弹出，距离为原半径的3倍（确保不立即重叠）
        sf::Vector2f startPos = clone.position;
        sf::Vector2f endPos = startPos + dir * oldRadius * 3.0f;
        clone.position = endPos;
        clone.velocity = dir * Config::SplitSpeed;
        clone.splitTimer = Config::SplitCooldown;
        clone.mergeTimer = Config::MergeDelay + oldRadius * Config::MergeDelayRadiusFactor;

        // 分身扫过区域吃彩豆和孢子
        float splitRadius = clone.Radius();
        // 吃彩豆
        for (Pellet& pellet : pellets_)
        {
            if (pellet.mass <= 0.f)
                continue;
            // 检查彩豆是否在分身路径上（点到线段的距离）
            float dist = PointToSegmentDistance(pellet.position, startPos, endPos);
            if (dist < splitRadius)
            {
                clone.mass += pellet.mass;
                pellet.mass = 0.f;
            }
        }
        // 吃孢子
        for (Ball& spore : balls_)
        {
            if (spore.owner != BallOwner::Spore || spore.mass <= 0.f)
                continue;
            float dist = PointToSegmentDistance(spore.position, startPos, endPos);
            if (dist < splitRadius)
            {
                clone.mass += spore.mass;
                spore.mass = 0.f;
            }
        }
        // 吃其他玩家的球（分身比对方大时）
        for (Ball& other : balls_)
        {
            if (other.owner == BallOwner::Spore || other.mass <= 0.f)
                continue;
            if (other.ownerId == clone.ownerId)
                continue;  // 不吃自己的球
            if (!CanEat(clone, other))
                continue;  // 必须比对方大
            float dist = PointToSegmentDistance(other.position, startPos, endPos);
            if (dist < splitRadius)
            {
                clone.mass += other.mass;
                other.mass = 0.f;
            }
        }

        newBalls.push_back(clone);

        // 每分裂一个球就增加一个，检查是否超过上限
        if (playerBallCount + static_cast<int>(newBalls.size()) >= Config::MaxPlayerBalls)
            break;
    }

    for (Ball& b : newBalls)
        balls_.push_back(b);
}

void World::TrySpitPlayer(const sf::Vector2f& direction)
{
    sf::Vector2f dir = Normalize(direction);
    if (dir.x == 0.f && dir.y == 0.f)
        dir = { 1.f, 0.f };

    for (Ball& ball : balls_)
    {
        if (!ball.isPlayerPart || ball.spitTimer > 0.f)
            continue;
        
        // 必须够吐出孢子，且吐后不低于最小质量
        if (ball.mass < Config::MinBallMass + Config::SporeMass)
            continue;

        // 先用扣减前的半径计算生成位置，避免孢子闪现在球体内
        const float ballRadius = ball.Radius();
        ball.mass -= Config::SporeMass;
        ball.spitTimer = Config::SpitCooldown;

        // 固定方向吐出，沿运动方向排成直线
        Ball spore;
        spore.position = ball.position + dir * (ballRadius + Config::SporeRadius);
        spore.velocity = dir * Config::SpitSpeed;
        spore.mass = Config::SporeMass;  // 孢子固定质量
        spore.color = ball.color;
        spore.owner = BallOwner::Spore;
        spore.ownerId = ball.ownerId;
        spore.name = "";
        balls_.push_back(spore);
    }
}

void World::TrySplitBot(int ownerId, const sf::Vector2f& direction)
{
    sf::Vector2f dir = Normalize(direction);
    if (dir.x == 0.f && dir.y == 0.f)
        dir = { 1.f, 0.f };

    // 检查该 Bot 当前球数量
    int botBallCount = 0;
    for (const Ball& ball : balls_)
    {
        if (ball.owner == BallOwner::Bot && ball.ownerId == ownerId)
            ++botBallCount;
    }
    if (botBallCount >= Config::MaxPlayerBalls)
        return;

    std::vector<Ball> newBalls;
    for (Ball& ball : balls_)
    {
        if (ball.owner != BallOwner::Bot || ball.ownerId != ownerId)
            continue;
        if (ball.mass < Config::MinSplitMass || ball.splitTimer > 0.f)
            continue;

        const float halfMass = ball.mass * 0.5f;
        if (halfMass < Config::MinBallMass)
            continue;

        const float oldRadius = ball.Radius();
        ball.mass = halfMass;
        ball.splitTimer = Config::SplitCooldown;
        ball.mergeTimer = Config::MergeDelay + oldRadius * Config::MergeDelayRadiusFactor;
        ball.position -= dir * oldRadius * 0.5f;

        Ball clone = ball;
        clone.mass = halfMass;
        sf::Vector2f startPos = clone.position;
        sf::Vector2f endPos = startPos + dir * oldRadius * 3.0f;
        clone.position = endPos;
        clone.velocity = dir * Config::SplitSpeed;
        clone.splitTimer = Config::SplitCooldown;
        clone.mergeTimer = Config::MergeDelay + oldRadius * Config::MergeDelayRadiusFactor;

        // 分身扫过区域吃彩豆和孢子
        float splitRadius = clone.Radius();
        for (Pellet& pellet : pellets_)
        {
            if (pellet.mass <= 0.f) continue;
            float dist = PointToSegmentDistance(pellet.position, startPos, endPos);
            if (dist < splitRadius) { clone.mass += pellet.mass; pellet.mass = 0.f; }
        }
        for (Ball& spore : balls_)
        {
            if (spore.owner != BallOwner::Spore || spore.mass <= 0.f) continue;
            float dist = PointToSegmentDistance(spore.position, startPos, endPos);
            if (dist < splitRadius) { clone.mass += spore.mass; spore.mass = 0.f; }
        }
        // 吃其他玩家的球（分身比对方大时）
        for (Ball& other : balls_)
        {
            if (other.owner == BallOwner::Spore || other.mass <= 0.f) continue;
            if (other.ownerId == clone.ownerId) continue;
            if (!CanEat(clone, other)) continue;
            float dist = PointToSegmentDistance(other.position, startPos, endPos);
            if (dist < splitRadius) { clone.mass += other.mass; other.mass = 0.f; }
        }

        newBalls.push_back(clone);
        if (botBallCount + static_cast<int>(newBalls.size()) >= Config::MaxPlayerBalls)
            break;
    }

    for (Ball& b : newBalls)
        balls_.push_back(b);
}

void World::TrySpitBot(int ownerId, const sf::Vector2f& direction)
{
    sf::Vector2f dir = Normalize(direction);
    if (dir.x == 0.f && dir.y == 0.f)
        dir = { 1.f, 0.f };

    for (Ball& ball : balls_)
    {
        if (ball.owner != BallOwner::Bot || ball.ownerId != ownerId)
            continue;
        if (ball.spitTimer > 0.f)
            continue;
        if (ball.mass < Config::MinBallMass + Config::SporeMass)
            continue;

        // 先用扣减前的半径计算生成位置
        const float ballRadius = ball.Radius();
        ball.mass -= Config::SporeMass;
        ball.spitTimer = Config::SpitCooldown;

        Ball spore;
        spore.position = ball.position + dir * (ballRadius + Config::SporeRadius);
        spore.velocity = dir * Config::SpitSpeed;
        spore.mass = Config::SporeMass;
        spore.color = ball.color;
        spore.owner = BallOwner::Spore;
        spore.ownerId = ball.ownerId;
        spore.name = "";
        balls_.push_back(spore);
    }
}

bool World::CanEat(const Ball& eater, const Ball& prey) const
{
    if (&eater == &prey)
        return false;
    // 同一 owner 的球不能互相吞噬（合并由 ResolveMerges 处理）
    if (eater.ownerId == prey.ownerId)
        return false;
    // 孢子不能吃其他球（但可以吃彩豆）
    if (eater.owner == BallOwner::Spore && prey.owner != BallOwner::Spore)
        return false;
    return eater.mass > prey.mass * Config::EatRatio;
}

bool World::CanEat(const Ball& eater, const Pellet& pellet) const
{
    (void)pellet;
    return eater.mass > Config::PelletMass;
}

void World::Absorb(Ball& eater, Ball& prey)
{
    eater.mass += prey.mass;
    prey.mass = 0.f;
}

void World::Absorb(Ball& eater, Pellet& pellet)
{
    eater.mass += pellet.mass;
    pellet.mass = 0.f;
}

void World::ApplyPlayerBallInteractions(float dt)
{
    // 同一owner的球之间的相互作用：吸引靠拢 + 接触保持（玩家和Bot都适用）
    for (size_t i = 0; i < balls_.size(); ++i)
    {
        for (size_t j = i + 1; j < balls_.size(); ++j)
        {
            Ball& a = balls_[i];
            Ball& b = balls_[j];
            // 同一ownerId的球之间有引力（玩家或同一个Bot）
            if (a.ownerId != b.ownerId)
                continue;
            // 孢子不参与引力
            if (a.owner == BallOwner::Spore || b.owner == BallOwner::Spore)
                continue;
            if (a.mass <= 0.f || b.mass <= 0.f)
                continue;

            const float dist = Length(a.position - b.position);
            const float rSum = a.Radius() + b.Radius();
            const float minDist = rSum;  // 刚好接触的距离

            // 如果已经接触或重叠
            if (dist < minDist + 10.f)
            {
                // 冷却期内：保持接触但不合并，速度同步实现"相对静止"
                if (a.mergeTimer > 0.f || b.mergeTimer > 0.f)
                {
                    // 轻微推开到刚好接触的位置
                    if (dist > 0.001f && dist < minDist)
                    {
                        sf::Vector2f pushDir = (b.position - a.position) / dist;
                        const float overlap = minDist - dist;
                        const float pushAmount = overlap * 0.5f;
                        a.position -= pushDir * pushAmount;
                        b.position += pushDir * pushAmount;
                    }
                    
                    // 速度同步：让两个球趋向于相同速度（相对静止效果）
                    sf::Vector2f avgVel = (a.velocity + b.velocity) * 0.5f;
                    a.velocity = avgVel;
                    b.velocity = avgVel;
                }
            }
            else
            {
                // 未接触：相互吸引靠拢
                const float attractRange = 200.f;  // 吸引范围
                if (dist < attractRange && dist > 0.001f)
                {
                    sf::Vector2f attractDir = (b.position - a.position) / dist;
                    const float attractStrength = 20.f;  // 吸引强度（降低，缓慢靠拢）
                    const float t = std::min(1.f, dt * 3.f);
                    a.velocity += attractDir * attractStrength * t;
                    b.velocity -= attractDir * attractStrength * t;
                }
            }
        }
    }
}

void World::ResolveCollisions()
{
    // 球与彩豆碰撞
    for (Ball& ball : balls_)
    {
        // 分身冷却期内不能吃彩豆
        if (ball.splitTimer > 0.f)
            continue;

        for (Pellet& pellet : pellets_)
        {
            if (pellet.mass <= 0.f)
                continue;
            const float dist = Length(ball.position - pellet.position);
            if (dist < ball.Radius() && CanEat(ball, pellet))
                Absorb(ball, pellet);
        }
    }

    // 球与刺球碰撞
    for (Ball& ball : balls_)
    {
        for (ThornBall& thorn : thornBalls_)
        {
            if (thorn.mass <= 0.f)
                continue;
            const float dist = Length(ball.position - thorn.position);
            // 球比刺球大才能吃，且需要覆盖70%
            if (dist < ball.Radius() + thorn.Radius() * 0.3f && CanEat(ball, thorn))
            {
                AbsorbThornBall(ball, thorn);
                break;  // 一次只能吃一个刺球
            }
        }
    }

    // 球与球碰撞（只处理不同玩家之间的吞噬）
    for (size_t i = 0; i < balls_.size(); ++i)
    {
        for (size_t j = i + 1; j < balls_.size(); ++j)
        {
            Ball& a = balls_[i];
            Ball& b = balls_[j];
            if (a.mass <= 0.f || b.mass <= 0.f)
                continue;

            // 同一 owner 的球：玩家吃自己的孢子直接处理，其他的在 ResolveMerges 中处理
            if (a.ownerId == b.ownerId)
            {
                // 玩家吃自己的孢子（一方是孢子，一方不是）
                bool aIsSpore = (a.owner == BallOwner::Spore);
                bool bIsSpore = (b.owner == BallOwner::Spore);
                if (aIsSpore != bIsSpore)  // 一个是孢子，一个不是
                {
                    const float dist = Length(a.position - b.position);
                    const float ballR = aIsSpore ? b.Radius() : a.Radius();
                    const float eatDist = ballR + Config::SporeRadius * 0.30f;
                    if (dist < eatDist)
                    {
                        if (aIsSpore)
                            Absorb(b, a);
                        else
                            Absorb(a, b);
                    }
                }
                continue;
            }

            // 孢子之间互不吞噬，相互独立
            if (a.owner == BallOwner::Spore && b.owner == BallOwner::Spore)
            {
                continue;  // 两个孢子之间无交互
            }

            // 处理孢子被球吃：本体覆盖孢子70%即可吃到
            if (a.owner == BallOwner::Spore || b.owner == BallOwner::Spore)
            {
                const float dist = Length(a.position - b.position);
                
                // 球覆盖孢子70%：球心到孢子心距离 < 球半径 + 孢子半径×30%
                const float ballR = (a.owner == BallOwner::Spore) ? b.Radius() : a.Radius();
                const float eatDist = ballR + Config::SporeRadius * 0.30f;
                
                if (dist < eatDist)
                {
                    // 孢子被球吃
                    if (a.owner == BallOwner::Spore)
                        Absorb(b, a);
                    else
                        Absorb(a, b);
                }
                continue;
            }

            // 不同玩家的球：70% 重叠 + 质量比判定才能吞噬
            const float dist = Length(a.position - b.position);
            const float rSum = a.Radius() + b.Radius();
            const float overlapDist = rSum * (1.f - Config::EatOverlapRatio);
            
            if (dist < overlapDist)
            {
                if (CanEat(a, b))
                    Absorb(a, b);
                else if (CanEat(b, a))
                    Absorb(b, a);
            }
        }
    }

    pellets_.erase(
        std::remove_if(pellets_.begin(), pellets_.end(), [](const Pellet& p) { return p.mass <= 0.f; }),
        pellets_.end());
}

void World::ResolveMerges()
{
    // 收集所有可以合并的球（冷却结束），按 ownerId 分组
    // 先收集所有球
    std::vector<size_t> mergeableIndices;
    for (size_t i = 0; i < balls_.size(); ++i)
    {
        if (balls_[i].mergeTimer <= 0.f && balls_[i].mass > 0.f)
            mergeableIndices.push_back(i);
    }

    if (mergeableIndices.size() < 2)
        return;

    // 按质量从小到大排序（小分身优先合并）
    std::sort(mergeableIndices.begin(), mergeableIndices.end(),
        [this](size_t a, size_t b) { return balls_[a].mass < balls_[b].mass; });

    std::vector<bool> merged(balls_.size(), false);

    for (size_t idx : mergeableIndices)
    {
        if (merged[idx])
            continue;

        Ball& ball = balls_[idx];
        float closestDist = std::numeric_limits<float>::max();
        size_t closestIdx = -1;

        // 找最近的同 owner 可合并分身
        for (size_t otherIdx : mergeableIndices)
        {
            if (otherIdx == idx || merged[otherIdx])
                continue;

            // 必须是同一个 owner（玩家或同一个Bot）
            if (ball.ownerId != balls_[otherIdx].ownerId)
                continue;

            // 孢子之间不能合并
            if (ball.owner == BallOwner::Spore && balls_[otherIdx].owner == BallOwner::Spore)
                continue;

            const float dist = Length(ball.position - balls_[otherIdx].position);
            const float rSum = ball.Radius() + balls_[otherIdx].Radius();
            const float overlapDist = rSum * (1.f - Config::EatOverlapRatio);

            // 必须重叠达到70%
            if (dist < overlapDist && dist < closestDist)
            {
                closestDist = dist;
                closestIdx = otherIdx;
            }
        }

        // 与最近的分身合并
        if (closestIdx != -1)
        {
            Ball& other = balls_[closestIdx];
            if (ball.mass >= other.mass)
                Absorb(ball, other);
            else
                Absorb(other, ball);
            merged[idx] = true;
            merged[closestIdx] = true;
        }
    }
}

void World::RemoveDeadBalls()
{
    balls_.erase(
        std::remove_if(balls_.begin(), balls_.end(), [](const Ball& b) {
            if (b.mass <= 0.f)
                return true;
            if (b.owner == BallOwner::Spore)
                return false;
            return b.mass < Config::MinBallMass;
        }),
        balls_.end());

    // 清理已被吃的刺球
    thornBalls_.erase(
        std::remove_if(thornBalls_.begin(), thornBalls_.end(), [](const ThornBall& t) {
            return t.mass <= 0.f;
        }),
        thornBalls_.end());
}

void World::MaintainPelletCount()
{
    std::uniform_real_distribution<float> distX(20.f, Config::WorldWidth - 20.f);
    std::uniform_real_distribution<float> distY(20.f, Config::WorldHeight - 20.f);

    // 彩豆被吃后立即在随机位置生成新的，维持固定数量
    int missingPellets = Config::PelletCount - static_cast<int>(pellets_.size());
    for (int i = 0; i < missingPellets; ++i)
    {
        Pellet p;
        p.position = { distX(rng_), distY(rng_) };
        p.color = RandomBrightColor(static_cast<unsigned int>(rng_()));
        pellets_.push_back(p);
    }
}

void World::MaintainThornBallCount()
{
    std::uniform_real_distribution<float> distX(100.f, Config::WorldWidth - 100.f);
    std::uniform_real_distribution<float> distY(100.f, Config::WorldHeight - 100.f);

    // 刺球被吃后在随机位置生成新的，维持固定数量
    int missingThorns = Config::ThornBallCount - static_cast<int>(thornBalls_.size());
    for (int i = 0; i < missingThorns; ++i)
    {
        ThornBall t;
        t.position = { distX(rng_), distY(rng_) };
        // 随机生成大或小刺球
        if (rng_() % 100 < 25)  // 25% 概率生成大刺球
        {
            t.mass = Config::ThornBallMassLarge;
            t.color = sf::Color(100, 220, 80);
        }
        else
        {
            t.mass = Config::ThornBallMassSmall;
            t.color = sf::Color(150, 255, 100);
        }
        thornBalls_.push_back(t);
    }
}

bool World::CanEat(const Ball& eater, const ThornBall& thorn) const
{
    // 必须比刺球大才能吃
    return eater.mass > thorn.mass * Config::EatRatio;
}

void World::AbsorbThornBall(Ball& eater, ThornBall& thorn)
{
    // 增加质量
    float totalMass = eater.mass + thorn.mass;
    thorn.mass = 0.f;  // 标记刺球为已吃

    const int splitCount = Config::ThornBallSplitCount;  // 周围8个分身

    // 保存原球信息
    sf::Vector2f center = eater.position;
    sf::Color color = eater.color;
    BallOwner owner = eater.owner;
    int ownerId = eater.ownerId;
    std::string name = eater.name;
    bool isPlayerPart = eater.isPlayerPart;
    const sf::Texture* skinTex = eater.skinTexture;

    // 移除原球
    eater.mass = 0.f;

    // 计算中间球占比：小质量时占1/9，大质量时占比更高
    // 总份数 = 1(中间) + 8(周围) = 9份
    // 中间占比从 1/9 逐渐增加到最多 90%
    float minCenterRatio = 1.0f / 9.0f;  // 最低占比约11%
    float maxCenterRatio = 0.9f;          // 最高占比90%
    
    // 根据总质量计算中间占比（质量越大，中间占比越高）
    float massFactor = std::min(1.0f, (totalMass - 10000.f) / 50000.f);  // 10000质量开始增加，60000质量达到最大
    float centerRatio = minCenterRatio + (maxCenterRatio - minCenterRatio) * massFactor;
    
    float centerMass = totalMass * centerRatio;
    float smallTotalMass = totalMass - centerMass;
    float smallMass = smallTotalMass / splitCount;

    // 确保最小质量
    if (centerMass < Config::MinBallMass)
        centerMass = Config::MinBallMass;
    if (smallMass < Config::MinBallMass)
        smallMass = Config::MinBallMass;

    // 重新调整比例确保总质量守恒
    float actualTotal = centerMass + smallMass * splitCount;
    float scale = totalMass / actualTotal;
    centerMass *= scale;
    smallMass *= scale;

    // 创建中间大球
    Ball centerBall;
    centerBall.mass = centerMass;
    centerBall.position = center;
    centerBall.color = color;
    centerBall.owner = owner;
    centerBall.ownerId = ownerId;
    centerBall.name = name;
    centerBall.isPlayerPart = isPlayerPart;
    centerBall.skinTexture = skinTex;
    centerBall.splitTimer = Config::SplitCooldown;
    centerBall.mergeTimer = Config::MergeDelay + centerBall.Radius() * Config::MergeDelayRadiusFactor;
    balls_.push_back(centerBall);

    // 创建周围8个相同大小的小球
    for (int i = 0; i < splitCount; ++i)
    {
        Ball splitBall;
        splitBall.mass = smallMass;
        // 在中心球边缘生成
        float angle = i * 2.f * 3.14159f / splitCount;
        sf::Vector2f dir(std::cos(angle), std::sin(angle));
        splitBall.position = center + dir * (centerBall.Radius() * 2.f + MassToRadius(smallMass) + 20.f);
        splitBall.color = color;
        splitBall.owner = owner;
        splitBall.ownerId = ownerId;
        splitBall.name = name;
        splitBall.isPlayerPart = isPlayerPart;
        splitBall.skinTexture = skinTex;
        splitBall.velocity = dir * Config::SplitSpeed * 2.5f;  // 炸得更远
        splitBall.splitTimer = Config::SplitCooldown;
        splitBall.mergeTimer = Config::MergeDelay + splitBall.Radius() * Config::MergeDelayRadiusFactor;
        balls_.push_back(splitBall);
    }
}

void World::Draw(sf::RenderTarget& target, const sf::Font* font) const
{
    sf::RectangleShape bg;
    bg.setSize(sf::Vector2f(Config::WorldWidth, Config::WorldHeight));
    bg.setFillColor(sf::Color(28, 32, 38));
    target.draw(bg);

    // 彩豆
    for (const Pellet& pellet : pellets_)
        pellet.Draw(target);

    // 计算刺球平均半径，用于判断渲染层级
    float avgThornRadius = 0.f;
    for (const ThornBall& thorn : thornBalls_)
        avgThornRadius += thorn.Radius();
    if (!thornBalls_.empty())
        avgThornRadius /= static_cast<float>(thornBalls_.size());

    // 分离小球和大球
    std::vector<const Ball*> smallBalls;  // 比刺球小的球
    std::vector<const Ball*> largeBalls;  // 比刺球大的球
    
    for (const Ball& ball : balls_)
    {
        if (ball.mass <= 0.f) continue;
        if (ball.Radius() < avgThornRadius)
            smallBalls.push_back(&ball);
        else
            largeBalls.push_back(&ball);
    }

    // 按半径排序（小的先画）
    auto sortByRadius = [](const Ball* a, const Ball* b) {
        return a->Radius() < b->Radius();
    };
    std::sort(smallBalls.begin(), smallBalls.end(), sortByRadius);
    std::sort(largeBalls.begin(), largeBalls.end(), sortByRadius);

    // 绘制顺序：小球 -> 刺球 -> 大球
    for (const Ball* ball : smallBalls)
        ball->Draw(target, font);

    for (const ThornBall& thorn : thornBalls_)
        thorn.Draw(target);

    for (const Ball* ball : largeBalls)
        ball->Draw(target, font);
}

std::vector<LeaderboardEntry> World::GetLeaderboard() const
{
    std::vector<LeaderboardEntry> entries;

    // 计算玩家总质量
    float playerTotalMass = 0.f;
    for (const Ball& ball : balls_)
    {
        if (ball.isPlayerPart)
            playerTotalMass += ball.mass;
    }
    if (playerTotalMass > 0.f)
    {
        entries.push_back({ playerName_, playerTotalMass, true });
    }

    // 计算每个机器人的总质量
    std::map<int, float> botMasses;
    std::map<int, std::string> botNames;
    for (const Ball& ball : balls_)
    {
        if (ball.owner == BallOwner::Bot && ball.mass > 0.f)
        {
            botMasses[ball.ownerId] += ball.mass;
            if (botNames.find(ball.ownerId) == botNames.end())
                botNames[ball.ownerId] = ball.name;
        }
    }

    for (const auto& pair : botMasses)
    {
        entries.push_back({ botNames[pair.first], pair.second, false });
    }

    // 按质量降序排序
    std::sort(entries.begin(), entries.end(), [](const LeaderboardEntry& a, const LeaderboardEntry& b) {
        return a.totalMass > b.totalMass;
    });

    return entries;
}
