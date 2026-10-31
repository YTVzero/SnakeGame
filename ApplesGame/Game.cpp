#include "Game.h"
#include <algorithm>
#include <random>
#include <sstream>
#include <cstdio>

static std::mt19937& rng() {
    static std::mt19937 gen(std::random_device{}());
    return gen;
}

Game::Game()
    : window(sf::VideoMode(WINDOW_W, WINDOW_H), "Snake (SFML 2.5.1)")
{
    window.setFramerateLimit(60);

    if (!font.loadFromFile("Resources/font.ttf")) {
        std::fprintf(stderr, "Cannot load Resources/font.ttf\n");
    }
    text.setFont(font);

    initAssets();

    if (!DeserializeGame(records)) {
        records = {
            { "John",       100 },
            { "Jane",        50 },
            { "Alice",       33 },
            { "Bob",         25 },
            { "Clementine",  20 },
        };
    }
}

void Game::initAssets() {
    // --- звуки ---
    if (bufButton.loadFromFile("Resources/button.wav")) sndButton.setBuffer(bufButton);
    if (bufApple.loadFromFile("Resources/apple.wav"))  sndApple.setBuffer(bufApple);
    if (bufDeath.loadFromFile("Resources/death.wav"))  sndDeath.setBuffer(bufDeath);

    // --- музыка ---
    if (music.openFromFile("Resources/music.ogg")) {
        music.setLoop(true);
        music.setVolume(40.f);
    }

    // --- текстуры ---
    auto load = [](sf::Texture& t, const char* path) -> bool {
        if (!t.loadFromFile(path)) {
            std::fprintf(stderr, "Cannot load %s\n", path);
            return false;
        }
        t.setSmooth(true);
        return true;
    };

    bool ok = true;
    ok &= load(texBg,          "Resources/background.jpg");
    ok &= load(texApple,       "Resources/apple.png");
    ok &= load(texHeadUp,      "Resources/head_up.png");
    ok &= load(texHeadDown,    "Resources/head_down.png");
    ok &= load(texHeadLeft,    "Resources/head_left.png");
    ok &= load(texHeadRight,   "Resources/head_right.png");
    ok &= load(texTailUp,      "Resources/tail_down.png");
    ok &= load(texTailDown,    "Resources/tail_up.png");
    ok &= load(texTailLeft,    "Resources/tail_right.png");
    ok &= load(texTailRight,   "Resources/tail_left.png");
    ok &= load(texBodyH,       "Resources/body_horizontal.png");
    ok &= load(texBodyV,       "Resources/body_vertical.png");
    ok &= load(texBodyTL,      "Resources/body_topleft.png");
    ok &= load(texBodyTR,      "Resources/body_topright.png");
    ok &= load(texBodyBL,      "Resources/body_bottomleft.png");
    ok &= load(texBodyBR,      "Resources/body_bottomright.png");

    texturesLoaded = ok;

    if (texturesLoaded) {
        sprBg.setTexture(texBg);
        float scaleX = (float)WINDOW_W / texBg.getSize().x;
        float scaleY = (float)(WINDOW_H - TOP_BAR) / texBg.getSize().y;
        sprBg.setScale(scaleX, scaleY);
        sprBg.setPosition(0.f, (float)TOP_BAR);

        sprApple.setTexture(texApple);
        float appleScale = (float)CELL_SIZE / texApple.getSize().x;
        sprApple.setScale(appleScale, appleScale);
    }
}

void Game::updateMusic() {
    if (musicOn && (state == GameState::Session || state == GameState::GameOver ||
                    state == GameState::EnterNameYesNo || state == GameState::EnterNameInput)) {
        if (music.getStatus() != sf::Music::Playing)
            music.play();
    } else {
        if (music.getStatus() == sf::Music::Playing)
            music.stop();
    }
}

void Game::initSnake() {
    snake.clear();
    int cx = GRID_COLS / 2;
    int cy = GRID_ROWS / 2;
    snake.push_back({ cx,     cy });
    snake.push_back({ cx - 1, cy });
    snake.push_back({ cx - 2, cy });

    dir = Dir::Right;
    nextDir = Dir::Right;
    score = 0;
    paused = false;
    moveAccum = 0.f;
    startDelay = START_DELAY_SECONDS;

    const auto& d = DIFFICULTIES[difficultyIndex];
    currentStep = 1.f / d.speedCellsPerSec;

    spawnApple();
}

void Game::startSession() {
    initSnake();
    recordJustSet = false;
    pendingName = DEFAULT_PLAYER_NAME;
    state = GameState::Session;
    updateMusic();
}

void Game::spawnApple() {
    std::uniform_int_distribution<int> dx(0, GRID_COLS - 1);
    std::uniform_int_distribution<int> dy(0, GRID_ROWS - 1);
    for (int tries = 0; tries < 1000; ++tries) {
        Cell c{ dx(rng()), dy(rng()) };
        if (isWall(c))  continue;
        if (isOnSnake(c)) continue;
        apple = c;
        return;
    }
}

bool Game::isOnSnake(const Cell& c) const {
    for (const auto& s : snake) if (s == c) return true;
    return false;
}

bool Game::isWall(const Cell& c) const {
    return c.x == 0 || c.y == 0 ||
        c.x == GRID_COLS - 1 || c.y == GRID_ROWS - 1;
}

bool Game::isOpposite(Dir a, Dir b) const {
    return (a == Dir::Up && b == Dir::Down) ||
        (a == Dir::Down && b == Dir::Up) ||
        (a == Dir::Left && b == Dir::Right) ||
        (a == Dir::Right && b == Dir::Left);
}

Dir Game::directionBetween(const Cell& from, const Cell& to) const {
    if (to.x > from.x) return Dir::Right;
    if (to.x < from.x) return Dir::Left;
    if (to.y > from.y) return Dir::Down;
    return Dir::Up;
}

void Game::run() {
    sf::Clock clock;
    while (window.isOpen()) {
        sf::Event e;
        while (window.pollEvent(e)) handleEvent(e);
        update(clock.restart().asSeconds());
        draw();
    }
}

void Game::handleEvent(const sf::Event& e) {
    if (e.type == sf::Event::Closed) { window.close(); return; }

    if (e.type != sf::Event::KeyPressed) {
        if (state == GameState::EnterNameInput && e.type == sf::Event::TextEntered) {
            auto u = e.text.unicode;
            if (u >= 32 && u < 127 && pendingName.size() < 12)
                pendingName.push_back(static_cast<char>(u));
        }
        return;
    }

    auto key = e.key.code;

    switch (state) {
    case GameState::MainMenu: {
        const int N = 5;
        if (key == sf::Keyboard::W) selectedIndex = (selectedIndex + N - 1) % N;
        if (key == sf::Keyboard::S) selectedIndex = (selectedIndex + 1) % N;
        if (key == sf::Keyboard::Enter) {
            if (soundOn) sndButton.play();
            switch (selectedIndex) {
            case 0: startSession(); break;
            case 1: state = GameState::DifficultyMenu; selectedIndex = 0; break;
            case 2: state = GameState::RecordsTab; break;
            case 3: state = GameState::Settings; selectedIndex = 0; break;
            case 4: window.close(); break;
            }
        }
        break;
    }

    case GameState::DifficultyMenu:
        if (key == sf::Keyboard::W) selectedIndex = (selectedIndex + 4) % 5;
        if (key == sf::Keyboard::S) selectedIndex = (selectedIndex + 1) % 5;
        if (key == sf::Keyboard::Enter) {
            difficultyIndex = selectedIndex;
            if (soundOn) sndButton.play();
            state = GameState::MainMenu;
            selectedIndex = 0;
        }
        if (key == sf::Keyboard::B) {
            state = GameState::MainMenu;
            selectedIndex = 0;
        }
        break;

    case GameState::Settings:
        if (key == sf::Keyboard::W || key == sf::Keyboard::S)
            selectedIndex = (selectedIndex + 1) % 2;
        if (key == sf::Keyboard::Enter) {
            if (soundOn) sndButton.play();
            if (selectedIndex == 0) soundOn = !soundOn;
            else {
                musicOn = !musicOn;
                updateMusic();
            }
        }
        if (key == sf::Keyboard::B) { state = GameState::MainMenu; selectedIndex = 0; }
        break;

    case GameState::RecordsTab:
        if (key == sf::Keyboard::B) { state = GameState::MainMenu; selectedIndex = 0; }
        break;

    case GameState::Session:
        if (key == sf::Keyboard::P) { paused = !paused; break; }
        if (paused && key == sf::Keyboard::B) {
            state = GameState::MainMenu;
            selectedIndex = 0;
            updateMusic();
            break;
        }
        if (!paused) {
            if (key == sf::Keyboard::W && !isOpposite(dir, Dir::Up))    nextDir = Dir::Up;
            if (key == sf::Keyboard::S && !isOpposite(dir, Dir::Down))  nextDir = Dir::Down;
            if (key == sf::Keyboard::A && !isOpposite(dir, Dir::Left))  nextDir = Dir::Left;
            if (key == sf::Keyboard::D && !isOpposite(dir, Dir::Right)) nextDir = Dir::Right;
        }
        break;

    case GameState::GameOver:
        if (key == sf::Keyboard::W || key == sf::Keyboard::S)
            selectedIndex = (selectedIndex + 1) % 2;
        if (key == sf::Keyboard::Enter) {
            if (soundOn) sndButton.play();
            if (selectedIndex == 0) {
                if (recordJustSet) {
                    startSession();
                } else {
                    state = GameState::EnterNameYesNo;
                    selectedIndex = 0;
                }
            } else {
                state = GameState::MainMenu;
                selectedIndex = 0;
                updateMusic();
            }
        }
        break;

    case GameState::EnterNameYesNo:
        if (key == sf::Keyboard::W || key == sf::Keyboard::S)
            selectedIndex = (selectedIndex + 1) % 2;
        if (key == sf::Keyboard::Enter) {
            if (soundOn) sndButton.play();
            if (selectedIndex == 1) {
                pendingName.clear();
                state = GameState::EnterNameInput;
            } else {
                records[DEFAULT_PLAYER_NAME] =
                    std::max(records[DEFAULT_PLAYER_NAME], score);
                SerializeGame(records);
                recordJustSet = true;
                state = GameState::GameOver;
                selectedIndex = 0;
            }
        }
        break;

    case GameState::EnterNameInput:
        if (key == sf::Keyboard::Enter) {
            if (pendingName.empty()) pendingName = DEFAULT_PLAYER_NAME;
            records[pendingName] = std::max(records[pendingName], score);
            SerializeGame(records);
            recordJustSet = true;
            state = GameState::GameOver;
            selectedIndex = 0;
        }
        else if (key == sf::Keyboard::BackSpace) {
            if (!pendingName.empty()) pendingName.pop_back();
        }
        break;
    }
}

void Game::update(float dt) {
    if (state != GameState::Session || paused) return;

    if (startDelay > 0.f) { startDelay -= dt; return; }

    moveAccum += dt;
    while (moveAccum >= currentStep) {
        moveAccum -= currentStep;
        stepSnake();
    }
}

void Game::stepSnake() {
    dir = nextDir;

    Cell head = snake.front();
    switch (dir) {
    case Dir::Up:    head.y -= 1; break;
    case Dir::Down:  head.y += 1; break;
    case Dir::Left:  head.x -= 1; break;
    case Dir::Right: head.x += 1; break;
    }

    if (isWall(head)) { finishSession(); return; }

    bool hitBody = false;
    for (size_t i = 0; i + 1 < snake.size(); ++i)
        if (snake[i] == head) { hitBody = true; break; }
    if (hitBody) { finishSession(); return; }

    snake.push_front(head);

    if (head == apple) {
        score += DIFFICULTIES[difficultyIndex].pointsPerApple;
        if (soundOn) sndApple.play();
        for (int i = 0; i < LENGTH_GAIN_PER_APPLE - 1; ++i)
            snake.push_back(snake.back());
        spawnApple();
    } else {
        snake.pop_back();
    }
}

void Game::finishSession() {
    if (soundOn) sndDeath.play();
    state = GameState::GameOver;
    selectedIndex = 0;
}

void Game::draw() {
    window.clear(sf::Color::Black);
    switch (state) {
    case GameState::MainMenu:        drawMainMenu();        break;
    case GameState::DifficultyMenu:  drawDifficultyMenu();  break;
    case GameState::Settings:        drawSettings();        break;
    case GameState::RecordsTab:      drawRecordsTab();      break;
    case GameState::Session:         drawSession();         break;
    case GameState::GameOver:        drawSession(); drawGameOver(); break;
    case GameState::EnterNameYesNo:  drawSession(); drawEnterNameYesNo(); break;
    case GameState::EnterNameInput:  drawSession(); drawEnterNameInput(); break;
    }
    window.display();
}

void Game::drawCenteredText(const std::string& s, float y,
    unsigned size, sf::Color color)
{
    text.setString(sf::String::fromUtf8(s.begin(), s.end()));
    text.setCharacterSize(size);
    text.setFillColor(color);
    sf::FloatRect b = text.getLocalBounds();
    text.setPosition((WINDOW_W - b.width) / 2.f - b.left, y);
    window.draw(text);
}

void Game::drawMenuItems(const std::vector<std::string>& items, float y0) {
    float y = y0;
    for (size_t i = 0; i < items.size(); ++i) {
        sf::Color c = (int)i == selectedIndex ? COL_SELECTED : COL_TEXT;
        drawCenteredText(items[i], y, 26, c);
        y += 44.f;
    }
}

void Game::drawMainMenu() {
    drawCenteredText("Snake", 60, 40, COL_TEXT);
    drawMenuItems({
        "Start play",
        "Difficulty level",
        "High Score table",
        "Settings",
        "Exit"
        }, 160.f);
    drawCenteredText("XYZ School", WINDOW_H - 30.f, 16, sf::Color(140, 140, 140));
}

void Game::drawDifficultyMenu() {
    drawCenteredText("Difficulty level", 40, 32);
    float y = 110.f;
    for (int i = 0; i < 5; ++i) {
        sf::Color c = (i == selectedIndex) ? COL_SELECTED : COL_TEXT;
        drawCenteredText(DIFFICULTIES[i].name, y, 26, c);
        y += 42.f;
    }
    drawCenteredText("B — back", WINDOW_H - 30.f, 16, sf::Color(140, 140, 140));
}

void Game::drawSettings() {
    drawCenteredText("Settings", 40, 32);
    drawCenteredText("Song", 160, 26, selectedIndex == 0 ? COL_SELECTED : COL_TEXT);
    drawCenteredText("Music", 210, 26, selectedIndex == 1 ? COL_SELECTED : COL_TEXT);

    auto drawBox = [&](float y, bool on) {
        sf::RectangleShape r({ 26, 26 });
        r.setPosition(WINDOW_W - 90.f, y);
        r.setFillColor(sf::Color::Transparent);
        r.setOutlineThickness(2);
        r.setOutlineColor(COL_TEXT);
        window.draw(r);
        if (on) {
            sf::RectangleShape fill({ 18, 18 });
            fill.setPosition(WINDOW_W - 86.f, y + 4.f);
            fill.setFillColor(COL_SELECTED);
            window.draw(fill);
        }
    };
    drawBox(160, soundOn);
    drawBox(210, musicOn);

    drawCenteredText("B — back", WINDOW_H - 30.f, 16, sf::Color(140, 140, 140));
}

std::vector<std::pair<std::string, int>> Game::sortedRecords() const {
    std::vector<std::pair<std::string, int>> v(records.begin(), records.end());
    std::sort(v.begin(), v.end(),
        [](const auto& a, const auto& b) { return a.second > b.second; });
    return v;
}

void Game::drawRecordsTab() {
    drawCenteredText("Table of records", 30, 30);
    auto list = sortedRecords();
    float y = 100.f;
    for (size_t i = 0; i < list.size() && i < (size_t)RECORDS_ON_TAB; ++i) {
        std::ostringstream oss;
        oss << (i + 1) << ". " << list[i].first << "   " << list[i].second;
        drawCenteredText(oss.str(), y, 22);
        y += 32.f;
    }
    drawCenteredText("B — back", WINDOW_H - 30.f, 16, sf::Color(140, 140, 140));
}

void Game::drawTopBar() {
    sf::RectangleShape bar({ (float)WINDOW_W, (float)TOP_BAR });
    bar.setFillColor(sf::Color(40, 40, 40));
    window.draw(bar);

    text.setString("Score: " + std::to_string(score));
    text.setCharacterSize(20);
    text.setFillColor(COL_TEXT);
    text.setPosition(15.f, 12.f);
    window.draw(text);

    text.setString("P — pause");
    sf::FloatRect b = text.getLocalBounds();
    text.setPosition(WINDOW_W - b.width - 15.f, 12.f);
    window.draw(text);
}

void Game::drawSnake() {
    if (!texturesLoaded || snake.empty()) {
        sf::RectangleShape body({ (float)CELL_SIZE - 2, (float)CELL_SIZE - 2 });
        for (size_t i = 0; i < snake.size(); ++i) {
            body.setFillColor(i == 0 ? COL_HEAD : COL_BODY);
            body.setPosition(snake[i].x * CELL_SIZE + 1,
                TOP_BAR + snake[i].y * CELL_SIZE + 1);
            window.draw(body);
        }
        return;
    }

    const float scale = (float)CELL_SIZE / 40.f;

    auto drawSeg = [&](const sf::Texture& tex, int x, int y) {
        sf::Sprite s(tex);
        s.setScale(scale, scale);
        s.setPosition(x * CELL_SIZE, TOP_BAR + y * CELL_SIZE);
        window.draw(s);
    };

    // голова
    const Cell& head = snake.front();
    switch (dir) {
    case Dir::Up:    drawSeg(texHeadUp,    head.x, head.y); break;
    case Dir::Down:  drawSeg(texHeadDown,  head.x, head.y); break;
    case Dir::Left:  drawSeg(texHeadLeft,  head.x, head.y); break;
    case Dir::Right: drawSeg(texHeadRight, head.x, head.y); break;
    }

    // тело и хвост
    for (size_t i = 1; i < snake.size(); ++i) {
        const Cell& cur  = snake[i];
        const Cell& prev = snake[i - 1];
        Dir fromPrev = directionBetween(cur, prev);

        if (i + 1 == snake.size()) {
            switch (fromPrev) {
            case Dir::Up:    drawSeg(texTailUp,    cur.x, cur.y); break;
            case Dir::Down:  drawSeg(texTailDown,  cur.x, cur.y); break;
            case Dir::Left:  drawSeg(texTailLeft,  cur.x, cur.y); break;
            case Dir::Right: drawSeg(texTailRight, cur.x, cur.y); break;
            }
        } else {
            const Cell& next = snake[i + 1];
            Dir toNext = directionBetween(cur, next);

            if ((fromPrev == Dir::Left && toNext == Dir::Right) ||
                (fromPrev == Dir::Right && toNext == Dir::Left)) {
                drawSeg(texBodyH, cur.x, cur.y);
            }
            else if ((fromPrev == Dir::Up && toNext == Dir::Down) ||
                     (fromPrev == Dir::Down && toNext == Dir::Up)) {
                drawSeg(texBodyV, cur.x, cur.y);
            }
            else {
                if ((fromPrev == Dir::Up && toNext == Dir::Left) ||
                    (fromPrev == Dir::Left && toNext == Dir::Up))
                    drawSeg(texBodyTL, cur.x, cur.y);
                else if ((fromPrev == Dir::Up && toNext == Dir::Right) ||
                         (fromPrev == Dir::Right && toNext == Dir::Up))
                    drawSeg(texBodyTR, cur.x, cur.y);
                else if ((fromPrev == Dir::Down && toNext == Dir::Left) ||
                         (fromPrev == Dir::Left && toNext == Dir::Down))
                    drawSeg(texBodyBL, cur.x, cur.y);
                else if ((fromPrev == Dir::Down && toNext == Dir::Right) ||
                         (fromPrev == Dir::Right && toNext == Dir::Down))
                    drawSeg(texBodyBR, cur.x, cur.y);
                else
                    drawSeg(texBodyH, cur.x, cur.y);
            }
        }
    }
}

void Game::drawSession() {
    drawTopBar();

    if (texturesLoaded) {
        window.draw(sprBg);
    } else {
        sf::RectangleShape field({ (float)WINDOW_W, (float)(WINDOW_H - TOP_BAR) });
        field.setPosition(0, TOP_BAR);
        field.setFillColor(COL_BG);
        window.draw(field);
    }

    // лёгкая сетка
    sf::VertexArray grid(sf::Lines);
    sf::Color gridCol(0, 0, 0, 40);
    for (int x = 0; x <= GRID_COLS; ++x) {
        float px = x * CELL_SIZE;
        grid.append(sf::Vertex({ px, (float)TOP_BAR }, gridCol));
        grid.append(sf::Vertex({ px, (float)WINDOW_H }, gridCol));
    }
    for (int y = 0; y <= GRID_ROWS; ++y) {
        float py = TOP_BAR + y * CELL_SIZE;
        grid.append(sf::Vertex({ 0.f, py }, gridCol));
        grid.append(sf::Vertex({ (float)WINDOW_W, py }, gridCol));
    }
    window.draw(grid);

    // стены
    sf::RectangleShape wall({ (float)CELL_SIZE, (float)CELL_SIZE });
    wall.setFillColor(sf::Color(80, 50, 20, 180));
    for (int y = 0; y < GRID_ROWS; ++y)
        for (int x = 0; x < GRID_COLS; ++x) {
            if (!isWall({x, y})) continue;
            wall.setPosition(x * CELL_SIZE, TOP_BAR + y * CELL_SIZE);
            window.draw(wall);
        }

    // яблоко
    if (texturesLoaded) {
        sprApple.setPosition(apple.x * CELL_SIZE, TOP_BAR + apple.y * CELL_SIZE);
        window.draw(sprApple);
    } else {
        sf::CircleShape appleShape(CELL_SIZE * 0.35f);
        appleShape.setFillColor(COL_APPLE);
        appleShape.setPosition(apple.x * CELL_SIZE + CELL_SIZE * 0.15f,
            TOP_BAR + apple.y * CELL_SIZE + CELL_SIZE * 0.15f);
        window.draw(appleShape);
    }

    drawSnake();

    if (startDelay > 0.f) {
        drawCenteredText("GET READY...", WINDOW_H / 2.f, 30, COL_SELECTED);
    }

    if (paused) {
        sf::RectangleShape ov({ (float)WINDOW_W, (float)WINDOW_H });
        ov.setFillColor(sf::Color(0, 0, 0, 180));
        window.draw(ov);
        drawCenteredText("PAUSA", WINDOW_H / 2.f - 40, 40);
        drawCenteredText("P — continue", WINDOW_H / 2.f + 20, 20);
        drawCenteredText("B — exit the menu", WINDOW_H / 2.f + 50, 20);
    }
}

void Game::drawGameOver() {
    sf::RectangleShape ov({ (float)WINDOW_W, (float)WINDOW_H });
    ov.setFillColor(sf::Color(0, 0, 0, 200));
    window.draw(ov);

    drawCenteredText("Number of points", 100, 30);
    drawCenteredText(std::to_string(score), 145, 34, COL_SELECTED);
    drawCenteredText("Records", 200, 22);

    auto list = sortedRecords();
    float y = 240.f;
    for (size_t i = 0; i < list.size() && i < (size_t)RECORDS_IN_POPUP; ++i) {
        std::ostringstream oss;
        oss << (i + 1) << ". " << list[i].first << "   " << list[i].second;
        drawCenteredText(oss.str(), y, 20);
        y += 28.f;
    }

    drawMenuItems({ "Start the game", "In the main menu" }, 400.f);
}

void Game::drawEnterNameYesNo() {
    sf::RectangleShape ov({ (float)WINDOW_W, (float)WINDOW_H });
    ov.setFillColor(sf::Color(0, 0, 0, 220));
    window.draw(ov);

    drawCenteredText("Enter a name?", 180, 32);
    drawCenteredText("No", 280, 30, selectedIndex == 0 ? COL_SELECTED : COL_TEXT);
    drawCenteredText("Yes", 340, 30, selectedIndex == 1 ? COL_SELECTED : COL_TEXT);
}

void Game::drawEnterNameInput() {
    sf::RectangleShape ov({ (float)WINDOW_W, (float)WINDOW_H });
    ov.setFillColor(sf::Color(0, 0, 0, 220));
    window.draw(ov);

    drawCenteredText("Enter the name (Enter — save)", 160, 24);
    drawCenteredText(pendingName + "_", 240, 34, COL_SELECTED);
}
