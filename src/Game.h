#pragma once

#include "World.h"
#include <SFML/Graphics.hpp>
#include <SFML/Audio.hpp>
#include <optional>
#include <string>
#include <vector>

class Game
{
public:
    bool Init();
    void Run();

private:
    enum class GameState { Menu, Playing };

    sf::RenderWindow window_;
    sf::View worldView_;
    sf::View uiView_;
    sf::Font font_;
    World world_;

    GameState state_ = GameState::Menu;
    sf::Vector2f mouseScreenPos_{};
    sf::Vector2f moveDir_{};  // WASD 移动方向
    sf::Music bgm_;           // 背景音乐

    // 主菜单
    std::string playerName_ = u8"你";
    int selectedSkin_ = -1;   // -1 表示无皮肤
    std::vector<sf::Texture> skinTextures_;
    std::vector<std::string> skinFiles_;
    bool nameEditing_ = false;
    sf::String nameInputBuffer_;  // 使用 sf::String 支持中文

    bool LoadFont();
    bool LoadBGM();
    bool LoadSkins();
    void StartGame();

    sf::Vector2f ScreenToWorld(const sf::Vector2f& screenPos) const;
    void HandleEvents();
    void Update(float dt);
    void Render();

    // 主菜单
    void UpdateMenu();
    void DrawMenu();
    void HandleMenuEvents(const sf::Event& event);

    // 游戏内
    void UpdateGame(float dt);
    void RenderGame();
    void DrawHud();
    void DrawLeaderboard();
    void DrawHelp();
    void DrawGameOver();
};
