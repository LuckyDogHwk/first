#include "Game.h"
#include "Config.h"
#include <SFML/Window/Keyboard.hpp>
#include <SFML/Window/Mouse.hpp>
#include <algorithm>
#include <cmath>
#include <ctime>
#include <sstream>

bool Game::Init()
{
    window_.create(
        sf::VideoMode(Config::WindowWidth, Config::WindowHeight),
        ToSfString(Config::WindowTitle));
    window_.setFramerateLimit(60);

    worldView_.setSize(sf::Vector2f(static_cast<float>(Config::WindowWidth), static_cast<float>(Config::WindowHeight)));
    uiView_.setSize(sf::Vector2f(static_cast<float>(Config::WindowWidth), static_cast<float>(Config::WindowHeight)));
    uiView_.setCenter(sf::Vector2f(Config::WindowWidth * 0.5f, Config::WindowHeight * 0.5f));

    if (!LoadFont())
        return false;

    LoadBGM();  // BGM 加载失败不阻塞
    LoadSkins();

    return true;
}

bool Game::LoadFont()
{
    if (font_.loadFromFile("C:/Windows/Fonts/msyh.ttc"))
        return true;
    if (font_.loadFromFile("C:/Windows/Fonts/simhei.ttf"))
        return true;
    if (font_.loadFromFile("C:/Windows/Fonts/arial.ttf"))
        return true;
    return false;
}

bool Game::LoadBGM()
{
    if (!bgm_.openFromFile("bgm.mp3"))
        return false;
    bgm_.setLoop(true);
    bgm_.setVolume(50.f);
    bgm_.play();
    return true;
}

bool Game::LoadSkins()
{
    skinFiles_ = { "skins/skin0.jpg", "skins/skin1.jpg", "skins/skin2.jpg", "skins/skin3.jpg" };
    for (const auto& file : skinFiles_)
    {
        sf::Texture tex;
        if (tex.loadFromFile(file))
            skinTextures_.push_back(std::move(tex));
    }
    return !skinTextures_.empty();
}

void Game::StartGame()
{
    const sf::Texture* skinTex = (selectedSkin_ >= 0 && selectedSkin_ < static_cast<int>(skinTextures_.size()))
        ? &skinTextures_[selectedSkin_] : nullptr;
    world_.SetPlayerInfo(playerName_, skinTex);
    world_.Reset(static_cast<unsigned int>(std::time(nullptr)));
    state_ = GameState::Playing;
}

sf::Vector2f Game::ScreenToWorld(const sf::Vector2f& screenPos) const
{
    return window_.mapPixelToCoords(
        sf::Vector2i(static_cast<int>(screenPos.x), static_cast<int>(screenPos.y)),
        worldView_);
}

// ==================== 主菜单 ====================

void Game::HandleMenuEvents(const sf::Event& event)
{
    if (event.type == sf::Event::KeyPressed)
    {
        if (nameEditing_)
        {
            // 昵称输入
            if (event.key.code == sf::Keyboard::Enter)
            {
                nameEditing_ = false;
                if (!nameInputBuffer_.isEmpty())
                    playerName_ = ToUtf8String(nameInputBuffer_);
            }
            else if (event.key.code == sf::Keyboard::Escape)
            {
                nameEditing_ = false;
                nameInputBuffer_ = ToSfString(playerName_);
            }
            else if (event.key.code == sf::Keyboard::BackSpace)
            {
                if (!nameInputBuffer_.isEmpty())
                {
                    // 删除最后一个字符（支持中文）
                    sf::String s = nameInputBuffer_;
                    s.erase(s.getSize() - 1);
                    nameInputBuffer_ = s;
                }
            }
        }
        else
        {
            if (event.key.code == sf::Keyboard::Enter)
            {
                StartGame();
            }
        }
    }

    // 文字输入（支持中文）
    if (event.type == sf::Event::TextEntered && nameEditing_)
    {
        sf::Uint32 c = event.text.unicode;
        // 可打印字符（排除控制字符），最多 10 个字符
        if (c >= 32 && nameInputBuffer_.getSize() < 10)
        {
            nameInputBuffer_ += c;
        }
    }

    if (event.type == sf::Event::MouseButtonPressed && event.mouseButton.button == sf::Mouse::Left)
    {
        // 直接使用屏幕坐标（菜单使用 uiView_，默认 1:1 映射）
        float mx = static_cast<float>(event.mouseButton.x);
        float my = static_cast<float>(event.mouseButton.y);

        // 计算中心点（基于窗口实际大小）
        float cx = static_cast<float>(window_.getSize().x) * 0.5f;
        float cy = static_cast<float>(window_.getSize().y) * 0.5f;

        // 开始游戏按钮
        float btnX = cx - 140.f, btnY = cy + 220.f, btnW = 280.f, btnH = 60.f;
        if (mx >= btnX && mx <= btnX + btnW && my >= btnY && my <= btnY + btnH)
        {
            StartGame();
            return;
        }

        // 昵称输入框
        float nameBoxX = cx - 180.f, nameBoxY = cy - 20.f, nameBoxW = 360.f, nameBoxH = 50.f;
        if (mx >= nameBoxX && mx <= nameBoxX + nameBoxW && my >= nameBoxY && my <= nameBoxY + nameBoxH)
        {
            nameEditing_ = true;
            nameInputBuffer_ = ToSfString(playerName_);
        }
        else
        {
            if (nameEditing_)
            {
                nameEditing_ = false;
                if (!nameInputBuffer_.isEmpty())
                    playerName_ = ToUtf8String(nameInputBuffer_);
            }
        }

        // 皮肤选择
        float skinStartX = cx - static_cast<float>(skinTextures_.size()) * 75.f * 0.5f;
        float skinY = cy + 110.f;
        for (int i = 0; i < static_cast<int>(skinTextures_.size()); ++i)
        {
            float sx = skinStartX + i * 75.f;
            if (mx >= sx && mx <= sx + 70.f && my >= skinY && my <= skinY + 70.f)
            {
                selectedSkin_ = (selectedSkin_ == i) ? -1 : i;
            }
        }
    }
}

void Game::UpdateMenu()
{
}

void Game::DrawMenu()
{
    // 白色背景
    window_.clear(sf::Color(255, 255, 255));
    window_.setView(uiView_);

    // 使用窗口实际大小，确保全屏后居中
    float viewW = static_cast<float>(window_.getSize().x);
    float viewH = static_cast<float>(window_.getSize().y);
    float cx = viewW * 0.5f;
    float cy = viewH * 0.5f;

    // 可爱装饰：彩色小圆点
    sf::CircleShape dot(8.f);
    dot.setFillColor(sf::Color(255, 182, 193, 180));  // 粉色
    for (int i = 0; i < 12; ++i)
    {
        float angle = i * 30.f * 3.14159f / 180.f;
        float r = 180.f + (i % 3) * 40.f;
        dot.setPosition(cx + std::cos(angle) * r - 8.f, cy - 200.f + std::sin(angle) * 60.f - 8.f);
        if (i % 3 == 1) dot.setFillColor(sf::Color(173, 216, 230, 180));  // 蓝色
        else if (i % 3 == 2) dot.setFillColor(sf::Color(144, 238, 144, 180));  // 绿色
        else dot.setFillColor(sf::Color(255, 182, 193, 180));  // 粉色
        window_.draw(dot);
    }

    // 装饰星星
    sf::CircleShape star(5.f, 4);  // 四角星
    star.setFillColor(sf::Color(255, 215, 0, 200));
    star.setPosition(cx - 280.f, cy - 180.f);
    window_.draw(star);
    star.setPosition(cx + 260.f, cy - 150.f);
    window_.draw(star);
    star.setPosition(cx - 250.f, cy + 200.f);
    window_.draw(star);
    star.setPosition(cx + 270.f, cy + 180.f);
    window_.draw(star);

    // 标题（放大）
    sf::Text title;
    title.setFont(font_);
    title.setString(ToSfString(u8"球球大作战"));
    title.setCharacterSize(72);
    title.setFillColor(sf::Color(100, 180, 255));
    title.setOutlineColor(sf::Color(60, 120, 200));
    title.setOutlineThickness(4.f);
    sf::FloatRect tb = title.getLocalBounds();
    title.setOrigin(tb.left + tb.width * 0.5f, tb.top + tb.height * 0.5f);
    title.setPosition(cx, cy - 180.f);
    window_.draw(title);

    // 昵称标签（放大）
    sf::Text nameLabel;
    nameLabel.setFont(font_);
    nameLabel.setString(ToSfString(u8"昵称:"));
    nameLabel.setCharacterSize(32);
    nameLabel.setFillColor(sf::Color(80, 80, 80));
    nameLabel.setPosition(cx - 180.f, cy - 60.f);
    window_.draw(nameLabel);

    // 昵称输入框（放大）
    float nameBoxX = cx - 180.f, nameBoxY = cy - 20.f, nameBoxW = 360.f, nameBoxH = 50.f;
    sf::RectangleShape nameBox(sf::Vector2f(nameBoxW, nameBoxH));
    nameBox.setPosition(nameBoxX, nameBoxY);
    nameBox.setFillColor(sf::Color(245, 245, 245));
    nameBox.setOutlineThickness(nameEditing_ ? 3.f : 2.f);
    nameBox.setOutlineColor(nameEditing_ ? sf::Color(100, 180, 255) : sf::Color(180, 180, 180));
    window_.draw(nameBox);

    sf::Text nameText;
    nameText.setFont(font_);
    sf::String displayName = nameEditing_ ? nameInputBuffer_ : ToSfString(playerName_);
    if (displayName.isEmpty()) displayName = ToSfString(u8"点击输入昵称");
    nameText.setString(displayName);
    nameText.setCharacterSize(28);
    nameText.setFillColor(nameEditing_ ? sf::Color(50, 50, 50) : sf::Color(100, 100, 100));
    nameText.setPosition(nameBoxX + 15.f, nameBoxY + 10.f);
    window_.draw(nameText);

    // 皮肤选择标签（放大）
    sf::Text skinLabel;
    skinLabel.setFont(font_);
    skinLabel.setString(ToSfString(u8"选择皮肤:"));
    skinLabel.setCharacterSize(28);
    skinLabel.setFillColor(sf::Color(80, 80, 80));
    skinLabel.setPosition(cx - 180.f, cy + 70.f);
    window_.draw(skinLabel);

    // 皮肤缩略图（放大）
    float skinStartX = cx - static_cast<float>(skinTextures_.size()) * 75.f * 0.5f;
    float skinY = cy + 110.f;
    for (int i = 0; i < static_cast<int>(skinTextures_.size()); ++i)
    {
        float sx = skinStartX + i * 75.f;
        sf::RectangleShape skinBg(sf::Vector2f(70.f, 70.f));
        skinBg.setPosition(sx, skinY);
        skinBg.setFillColor(sf::Color(245, 245, 245));
        skinBg.setOutlineThickness(selectedSkin_ == i ? 4.f : 2.f);
        skinBg.setOutlineColor(selectedSkin_ == i ? sf::Color(100, 200, 100) : sf::Color(180, 180, 180));
        window_.draw(skinBg);

        sf::Sprite skinSprite(skinTextures_[i]);
        skinSprite.setPosition(sx + 3.f, skinY + 3.f);
        skinSprite.setScale(64.f / static_cast<float>(skinTextures_[i].getSize().x),
                            64.f / static_cast<float>(skinTextures_[i].getSize().y));
        window_.draw(skinSprite);
    }

    // 开始游戏按钮（放大）
    float btnX = cx - 140.f, btnY = cy + 220.f, btnW = 280.f, btnH = 60.f;
    sf::RectangleShape btn(sf::Vector2f(btnW, btnH));
    btn.setPosition(btnX, btnY);
    btn.setFillColor(sf::Color(80, 180, 120));
    btn.setOutlineThickness(3.f);
    btn.setOutlineColor(sf::Color(60, 150, 90));
    window_.draw(btn);

    sf::Text btnText;
    btnText.setFont(font_);
    btnText.setString(ToSfString(u8"开始游戏"));
    btnText.setCharacterSize(36);
    btnText.setFillColor(sf::Color::White);
    sf::FloatRect btnBounds = btnText.getLocalBounds();
    btnText.setOrigin(btnBounds.left + btnBounds.width * 0.5f, btnBounds.top + btnBounds.height * 0.5f);
    btnText.setPosition(cx, btnY + btnH * 0.5f);
    window_.draw(btnText);

    // 提示
    sf::Text hint;
    hint.setFont(font_);
    hint.setString(ToSfString(u8"点击昵称框输入，按 Enter 确认，ESC 返回主菜单"));
    hint.setCharacterSize(20);
    hint.setFillColor(sf::Color(140, 140, 140));
    sf::FloatRect hintBounds = hint.getLocalBounds();
    hint.setOrigin(hintBounds.left + hintBounds.width * 0.5f, 0.f);
    hint.setPosition(cx, viewH - 60.f);
    window_.draw(hint);

    window_.display();
}

// ==================== 游戏内 ====================

void Game::HandleEvents()
{
    sf::Event event{};
    while (window_.pollEvent(event))
    {
        switch (event.type)
        {
        case sf::Event::Closed:
            window_.close();
            break;
        case sf::Event::Resized:
            uiView_.setSize(sf::Vector2f(static_cast<float>(event.size.width), static_cast<float>(event.size.height)));
            uiView_.setCenter(sf::Vector2f(event.size.width * 0.5f, event.size.height * 0.5f));
            worldView_.setSize(sf::Vector2f(static_cast<float>(event.size.width), static_cast<float>(event.size.height)));
            break;
        case sf::Event::MouseMoved:
            mouseScreenPos_ = sf::Vector2f(static_cast<float>(event.mouseMove.x), static_cast<float>(event.mouseMove.y));
            break;
        case sf::Event::KeyPressed:
            if (event.key.code == sf::Keyboard::Space)
            {
                sf::Vector2f splitDir = moveDir_;
                if (splitDir.x == 0.f && splitDir.y == 0.f)
                    splitDir = { 1.f, 0.f };
                world_.TrySplitPlayer(splitDir);
            }
            if (event.key.code == sf::Keyboard::R && !world_.IsPlayerAlive())
                StartGame();
            if (event.key.code == sf::Keyboard::Escape)
                state_ = GameState::Menu;
            break;
        default:
            break;
        }
    }
}

void Game::UpdateGame(float dt)
{
    // WASD 控制移动方向
    moveDir_ = { 0.f, 0.f };
    if (sf::Keyboard::isKeyPressed(sf::Keyboard::W))
        moveDir_.y -= 1.f;
    if (sf::Keyboard::isKeyPressed(sf::Keyboard::S))
        moveDir_.y += 1.f;
    if (sf::Keyboard::isKeyPressed(sf::Keyboard::A))
        moveDir_.x -= 1.f;
    if (sf::Keyboard::isKeyPressed(sf::Keyboard::D))
        moveDir_.x += 1.f;
    moveDir_ = Normalize(moveDir_);

    const sf::Vector2f playerCenter = world_.GetPlayerCenter();
    const sf::Vector2f targetPos = playerCenter + moveDir_ * 500.f;

    world_.Update(dt, targetPos);

    // M 键吐球
    const bool wantsSpit = window_.hasFocus() &&
        sf::Keyboard::isKeyPressed(sf::Keyboard::M);

    if (wantsSpit && world_.IsPlayerAlive())
    {
        sf::Vector2f spitDir = moveDir_;
        if (spitDir.x == 0.f && spitDir.y == 0.f)
            spitDir = { 1.f, 0.f };
        world_.TrySpitPlayer(spitDir);
    }

    worldView_.setCenter(playerCenter);
}

void Game::Update(float dt)
{
    if (state_ == GameState::Menu)
        UpdateMenu();
    else
        UpdateGame(dt);
}

void Game::DrawHud()
{
    std::ostringstream oss;
    oss << u8"质量: " << static_cast<int>(world_.GetPlayerTotalMass());

    sf::Text massText;
    massText.setFont(font_);
    massText.setString(ToSfString(oss.str()));
    massText.setCharacterSize(20);
    massText.setFillColor(sf::Color::White);
    massText.setPosition(16.f, 12.f);
    window_.draw(massText);
}

void Game::DrawLeaderboard()
{
    auto entries = world_.GetLeaderboard();
    if (entries.empty())
        return;

    const float viewWidth = uiView_.getSize().x;

    const float boardWidth = 180.f;
    const float boardHeight = 30.f + static_cast<float>(std::min(entries.size(), size_t(10))) * 24.f + 10.f;
    const float boardX = viewWidth - boardWidth - 16.f;
    const float boardY = 16.f;

    sf::RectangleShape bg(sf::Vector2f(boardWidth, boardHeight));
    bg.setPosition(boardX, boardY);
    bg.setFillColor(sf::Color(0, 0, 0, 150));
    bg.setOutlineThickness(1.f);
    bg.setOutlineColor(sf::Color(100, 100, 100));
    window_.draw(bg);

    sf::Text title;
    title.setFont(font_);
    title.setString(ToSfString(u8"排行榜"));
    title.setCharacterSize(18);
    title.setFillColor(sf::Color(255, 215, 0));
    title.setPosition(boardX + 10.f, boardY + 6.f);
    window_.draw(title);

    float y = boardY + 34.f;
    int rank = 1;
    for (size_t i = 0; i < entries.size() && rank <= 10; ++i, ++rank)
    {
        const auto& entry = entries[i];

        std::ostringstream oss;
        oss << rank << ". " << entry.name << ": " << static_cast<int>(entry.totalMass);

        sf::Text text;
        text.setFont(font_);
        text.setString(ToSfString(oss.str()));
        text.setCharacterSize(16);

        if (entry.isPlayer)
            text.setFillColor(sf::Color(255, 220, 100));
        else
            text.setFillColor(sf::Color(200, 200, 200));

        text.setPosition(boardX + 10.f, y);
        window_.draw(text);
        y += 24.f;
    }
}

void Game::DrawHelp()
{
    const char* lines[] = {
        u8"WASD: 移动方向",
        u8"空格: 分身(最多16个)",
        u8"M: 吐球",
        u8"R: 死亡后重开",
        u8"ESC: 返回主菜单"
    };

    float y = static_cast<float>(Config::WindowHeight) - 132.f;
    for (const char* line : lines)
    {
        sf::Text text;
        text.setFont(font_);
        text.setString(ToSfString(line));
        text.setCharacterSize(16);
        text.setFillColor(sf::Color(220, 220, 220));
        text.setPosition(16.f, y);
        window_.draw(text);
        y += 22.f;
    }
}

void Game::DrawGameOver()
{
    if (world_.IsPlayerAlive())
        return;

    sf::Text text;
    text.setFont(font_);
    text.setString(ToSfString(u8"你被吃掉了! 按 R 重新开始，ESC 返回主菜单"));
    text.setCharacterSize(28);
    text.setFillColor(sf::Color(255, 90, 90));
    const sf::FloatRect bounds = text.getLocalBounds();
    text.setOrigin(bounds.left + bounds.width * 0.5f, bounds.top + bounds.height * 0.5f);
    text.setPosition(Config::WindowWidth * 0.5f, Config::WindowHeight * 0.5f);
    window_.draw(text);
}

void Game::RenderGame()
{
    window_.clear(sf::Color(18, 20, 24));

    window_.setView(worldView_);
    world_.Draw(window_, &font_);

    window_.setView(uiView_);
    DrawHud();
    DrawLeaderboard();
    DrawHelp();
    DrawGameOver();

    window_.display();
}

void Game::Render()
{
    if (state_ == GameState::Menu)
        DrawMenu();
    else
        RenderGame();
}

void Game::Run()
{
    sf::Clock clock;
    while (window_.isOpen())
    {
        const float dt = std::min(clock.restart().asSeconds(), 0.05f);

        if (state_ == GameState::Menu)
        {
            sf::Event event{};
            while (window_.pollEvent(event))
            {
                if (event.type == sf::Event::Closed)
                    window_.close();
                if (event.type == sf::Event::Resized)
                {
                    uiView_.setSize(sf::Vector2f(static_cast<float>(event.size.width), static_cast<float>(event.size.height)));
                    uiView_.setCenter(sf::Vector2f(event.size.width * 0.5f, event.size.height * 0.5f));
                }
                HandleMenuEvents(event);
            }
        }
        else
        {
            HandleEvents();
        }

        Update(dt);
        Render();
    }
}
