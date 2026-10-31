#pragma once
#include <SFML/Graphics.hpp>

// ====== Игровое поле ======
constexpr int CELL_SIZE = 32;
constexpr int GRID_COLS = 25;
constexpr int GRID_ROWS = 19;
constexpr int TOP_BAR = 48;
constexpr int WINDOW_W = GRID_COLS * CELL_SIZE;
constexpr int WINDOW_H = GRID_ROWS * CELL_SIZE + TOP_BAR;

// ====== Параметры из ТЗ, п.2.5 ======
struct DifficultyPreset {
    const char* name;
    float       speedCellsPerSec;   // V
    int         pointsPerApple;     // P
};

// Скорости в клетках/сек (адаптированы из относительных 1..5 ТЗ для комфортной игры)
// Очки — по таблице 2.4.2 ТЗ
static const DifficultyPreset DIFFICULTIES[5] = {
    { "Simple",             4.0f,  2 },
    { "Harder than easy",    6.0f,  4 },
    { "Medium",             8.0f,  6 },
    { "Lighter than heavy",     11.0f,  8 },
    { "Heavy",            15.0f, 10 },
};

constexpr int   LENGTH_GAIN_PER_APPLE = 1;    // L
constexpr float START_DELAY_SECONDS = 1.5f; // T
constexpr int   RECORDS_IN_POPUP = 5;    // X
constexpr int   RECORDS_ON_TAB = 10;   // Y

constexpr const char* DEFAULT_PLAYER_NAME = "XYZ";

// ====== Цвета ======
static const sf::Color COL_BG       = sf::Color(60, 120, 60);
static const sf::Color COL_GRID     = sf::Color(80, 140, 80);
static const sf::Color COL_HEAD     = sf::Color(40, 200, 60);
static const sf::Color COL_BODY     = sf::Color(60, 170, 70);
static const sf::Color COL_WALL     = sf::Color(150, 100, 40);
static const sf::Color COL_APPLE    = sf::Color(220, 40, 40);
static const sf::Color COL_TEXT     = sf::Color(230, 230, 230);
static const sf::Color COL_SELECTED = sf::Color(60, 220, 60);
