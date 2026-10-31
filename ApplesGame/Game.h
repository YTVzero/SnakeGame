#pragma once

#include <SFML/Graphics.hpp>
#include <SFML/Audio.hpp>
#include <deque>
#include <string>
#include <vector>
#include <utility>
#include "Records.h"
#include "Constants.h"

enum class GameState {
    MainMenu,
    DifficultyMenu,
    Settings,
    RecordsTab,
    Session,
    GameOver,
    EnterNameYesNo,
    EnterNameInput
};

enum class Dir { Up, Down, Left, Right };

struct Cell {
    int x, y;
    bool operator==(const Cell& o) const { return x == o.x && y == o.y; }
};

class Game {
public:
    Game();
    void run();

private:
    sf::RenderWindow window;
    sf::Font         font;
    sf::Text         text;

    GameState state = GameState::MainMenu;
    int  selectedIndex = 0;
    int  difficultyIndex = 0;

    bool soundOn = true;
    bool musicOn = true;

    std::deque<Cell> snake;
    Cell  apple;
    Dir   dir = Dir::Right;
    Dir   nextDir = Dir::Right;
    int   score = 0;
    bool  paused = false;
    float startDelay = 0.f;
    float moveAccum = 0.f;
    float currentStep = 0.1f;

    RecordsTable records;
    std::string  pendingName = DEFAULT_PLAYER_NAME;
    bool         recordJustSet = false;

    // sounds
    sf::SoundBuffer bufButton, bufApple, bufDeath;
    sf::Sound       sndButton, sndApple, sndDeath;
    sf::Music       music;

    // textures
    sf::Texture texBg;
    sf::Texture texApple;
    sf::Texture texHeadUp, texHeadDown, texHeadLeft, texHeadRight;
    sf::Texture texTailUp, texTailDown, texTailLeft, texTailRight;
    sf::Texture texBodyH, texBodyV;
    sf::Texture texBodyTL, texBodyTR, texBodyBL, texBodyBR;
    sf::Sprite  sprBg;
    sf::Sprite  sprApple;
    bool        texturesLoaded = false;

    void initAssets();
    void initSnake();
    void spawnApple();
    void updateMusic();

    void handleEvent(const sf::Event& e);
    void update(float dt);
    void draw();

    void drawMainMenu();
    void drawDifficultyMenu();
    void drawSettings();
    void drawRecordsTab();
    void drawSession();
    void drawGameOver();
    void drawEnterNameYesNo();
    void drawEnterNameInput();
    void drawTopBar();
    void drawSnake();

    void startSession();
    void stepSnake();
    bool isOnSnake(const Cell& c) const;
    bool isWall(const Cell& c) const;
    bool isOpposite(Dir a, Dir b) const;
    Dir  directionBetween(const Cell& from, const Cell& to) const;

    void finishSession();
    std::vector<std::pair<std::string, int>> sortedRecords() const;

    void drawCenteredText(const std::string& s, float y,
        unsigned size = 24,
        sf::Color color = COL_TEXT);
    void drawMenuItems(const std::vector<std::string>& items, float y0);
};
