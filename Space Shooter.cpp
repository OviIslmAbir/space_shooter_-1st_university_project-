#include <iostream>
#include <vector>
#include <conio.h>
#include <windows.h>
#include <cstdlib>
#include <ctime>
#include <string>
#include <algorithm>

using namespace std;

const int WIDTH  = 50;
const int HEIGHT = 25;

// Centering offsets
int offsetX = 0;
int offsetY = 0;
int difficulty = 1; // 1 = Easy, 2 = Medium, 3 = Hard

HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);

void gotoxy(int x, int y) {
    // Dynamically applies offset to center the game screen
    COORD coord = { (SHORT)(x + offsetX), (SHORT)(y + offsetY) };
    SetConsoleCursorPosition(hConsole, coord);
}

void hideCursor() {
    CONSOLE_CURSOR_INFO ci = { 1, FALSE };
    SetConsoleCursorInfo(hConsole, &ci);
}

void setColor(int color) {
    SetConsoleTextAttribute(hConsole, color);
}

void clearScreen() {
    system("cls");
}

struct Bullet {
    int x, y;
    bool active;
};

struct Enemy {
    int x, y;
    bool active;
    int type;
    int hp;
};

struct Explosion {
    int x, y;
    int timer;
};

int playerX, playerY;
int playerHP = 3;
int score    = 0;
int level    = 1;
int frameCount = 0;

vector<Bullet>    bullets;
vector<Bullet>    enemyBullets;
vector<Enemy>     enemies;
vector<Explosion> explosions;

bool gameOver = false;
bool paused   = false;

void calculateOffsets() {
    CONSOLE_SCREEN_BUFFER_INFO csbi;
    if (GetConsoleScreenBufferInfo(hConsole, &csbi)) {
        int consoleWidth = csbi.srWindow.Right - csbi.srWindow.Left + 1;
        int consoleHeight = csbi.srWindow.Bottom - csbi.srWindow.Top + 1;

        offsetX = max(0, (consoleWidth - (WIDTH + 2)) / 2);
        offsetY = max(0, (consoleHeight - (HEIGHT + 5)) / 2);
    } else {
        offsetX = 15; // Fallback values
        offsetY = 2;
    }
}

void drawBorder() {
    setColor(8);
    for (int y = 0; y <= HEIGHT; y++) {
        gotoxy(0, y);       cout << '|';
        gotoxy(WIDTH+1, y); cout << '|';
    }
    for (int x = 0; x <= WIDTH+1; x++) {
        gotoxy(x, 0);        cout << '-';
        gotoxy(x, HEIGHT+1); cout << '-';
    }
}

void drawHUD() {
    setColor(14);
    gotoxy(1, HEIGHT + 2);
    cout << " Score: " << score
         << "   Level: " << level
         << "   HP: ";
    setColor(12);
    for (int i = 0; i < playerHP; i++) cout << "<3 ";
    for (int i = playerHP; i < 3; i++) cout << "-- ";
    setColor(7);
    gotoxy(1, HEIGHT + 3);
    cout << " [\x1b\x1a] Move   [SPACE] Shoot   [P] Pause   [Q] Quit";
}

void spawnEnemy() {
    Enemy e;
    e.x      = 1 + rand() % (WIDTH - 1);
    e.y      = 1;
    e.active = true;
    e.type   = rand() % (level < 3 ? 2 : 3);
    e.hp     = (e.type == 2) ? 3 : 1;
    enemies.push_back(e);
}

void draw() {
    setColor(11);
    gotoxy(playerX - 1, playerY);     cout << " ^ ";
    gotoxy(playerX - 1, playerY + 1); cout << "/_\\";

    setColor(14);
    for (int i = 0; i < (int)bullets.size(); i++)
        if (bullets[i].active) { gotoxy(bullets[i].x, bullets[i].y); cout << '|'; }

    setColor(12);
    for (int i = 0; i < (int)enemyBullets.size(); i++)
        if (enemyBullets[i].active) { gotoxy(enemyBullets[i].x, enemyBullets[i].y); cout << '*'; }

    for (int i = 0; i < (int)enemies.size(); i++) {
        if (!enemies[i].active) continue;
        if (enemies[i].type == 0) { setColor(10); gotoxy(enemies[i].x-1, enemies[i].y); cout << "\\V/"; }
        if (enemies[i].type == 1) { setColor(13); gotoxy(enemies[i].x-1, enemies[i].y); cout << ">X<"; }
        if (enemies[i].type == 2) { setColor(12); gotoxy(enemies[i].x-1, enemies[i].y); cout << "[W]"; }
    }

    for (int i = 0; i < (int)explosions.size(); i++) {
        if (explosions[i].timer > 3)      { setColor(14); gotoxy(explosions[i].x-1, explosions[i].y); cout << "***"; }
        else if (explosions[i].timer > 0) { setColor(12); gotoxy(explosions[i].x-1, explosions[i].y); cout << " * "; }
    }
}

void erase() {
    setColor(0);
    gotoxy(playerX - 1, playerY);     cout << "   ";
    gotoxy(playerX - 1, playerY + 1); cout << "   ";

    for (int i = 0; i < (int)bullets.size(); i++)
        if (bullets[i].active) { gotoxy(bullets[i].x, bullets[i].y); cout << ' '; }

    for (int i = 0; i < (int)enemyBullets.size(); i++)
        if (enemyBullets[i].active) { gotoxy(enemyBullets[i].x, enemyBullets[i].y); cout << ' '; }

    for (int i = 0; i < (int)enemies.size(); i++)
        if (enemies[i].active) { gotoxy(enemies[i].x-1, enemies[i].y); cout << "   "; }

    for (int i = 0; i < (int)explosions.size(); i++)
        if (explosions[i].timer > 0) { gotoxy(explosions[i].x-1, explosions[i].y); cout << "   "; }
}

void handleInput() {
    if (!_kbhit()) return;
    int ch = _getch();

    // Check for Arrow keys (Extended keys trigger 0 or 224 first)
    if (ch == 0 || ch == 224) {
        ch = _getch();
        switch (ch) {
            case 75: // Left Arrow Key
                if (playerX > 2) playerX--;
                break;
            case 77: // Right Arrow Key
                if (playerX < WIDTH - 1) playerX++;
                break;
        }
    } else {
        switch (ch) {
            case ' ':
                bullets.push_back({ playerX, playerY - 1, true });
                break;
            case 'p': case 'P':
                paused = !paused;
                break;
            case 'q': case 'Q':
                gameOver = true;
                break;
        }
    }
}

void update() {
    frameCount++;
    level = 1 + score / 500;

    for (int i = 0; i < (int)bullets.size(); i++) {
        if (bullets[i].active) {
            bullets[i].y--;
            if (bullets[i].y < 1) bullets[i].active = false;
        }
    }

    for (int i = 0; i < (int)enemyBullets.size(); i++) {
        if (enemyBullets[i].active) {
            enemyBullets[i].y++;
            if (enemyBullets[i].y > HEIGHT) enemyBullets[i].active = false;
        }
    }

    // Dynamic Move Rate based on selected difficulty
    int baseMove = (difficulty == 1) ? 10 : (difficulty == 2) ? 8 : 5;
    int minMove  = (difficulty == 1) ? 4  : (difficulty == 2) ? 2 : 1;
    int moveRate = max(baseMove - level, minMove);

    if (frameCount % moveRate == 0) {
        for (int i = 0; i < (int)enemies.size(); i++) {
            if (!enemies[i].active) continue;
            enemies[i].y++;
            if (rand() % 15 == 0)
                enemyBullets.push_back({ enemies[i].x, enemies[i].y + 1, true });
            if (enemies[i].y >= HEIGHT) {
                enemies[i].active = false;
                playerHP--;
                explosions.push_back({ enemies[i].x, enemies[i].y, 6 });
                if (playerHP <= 0) gameOver = true;
            }
        }
    }

    // Dynamic Spawn Rate based on selected difficulty
    int baseSpawn = (difficulty == 1) ? 45 : (difficulty == 2) ? 30 : 18;
    int minSpawn  = (difficulty == 1) ? 15 : (difficulty == 2) ? 8  : 4;
    int spawnRate = max(baseSpawn - level * 3, minSpawn);

    if (frameCount % spawnRate == 0) spawnEnemy();

    for (int i = 0; i < (int)bullets.size(); i++) {
        if (!bullets[i].active) continue;
        for (int j = 0; j < (int)enemies.size(); j++) {
            if (!enemies[j].active) continue;
            if (abs(bullets[i].x - enemies[j].x) <= 1 && bullets[i].y == enemies[j].y) {
                bullets[i].active = false;
                enemies[j].hp--;
                if (enemies[j].hp <= 0) {
                    enemies[j].active = false;
                    score += (enemies[j].type == 2) ? 150 : (enemies[j].type == 1) ? 100 : 50;
                    explosions.push_back({ enemies[j].x, enemies[j].y, 6 });
                }
            }
        }
    }

    for (int i = 0; i < (int)enemyBullets.size(); i++) {
        if (!enemyBullets[i].active) continue;
        if (abs(enemyBullets[i].x - playerX) <= 1 &&
            (enemyBullets[i].y == playerY || enemyBullets[i].y == playerY + 1)) {
            enemyBullets[i].active = false;
            playerHP--;
            explosions.push_back({ playerX, playerY, 6 });
            if (playerHP <= 0) gameOver = true;
        }
    }

    for (int i = 0; i < (int)explosions.size(); i++)
        if (explosions[i].timer > 0) explosions[i].timer--;

    // Cleanup inactive elements
    vector<Bullet> newBullets, newEnemyBullets;
    vector<Enemy> newEnemies;
    vector<Explosion> newExplosions;

    for (int i = 0; i < (int)bullets.size(); i++)
        if (bullets[i].active) newBullets.push_back(bullets[i]);
    bullets = newBullets;

    for (int i = 0; i < (int)enemyBullets.size(); i++)
        if (enemyBullets[i].active) newEnemyBullets.push_back(enemyBullets[i]);
    enemyBullets = newEnemyBullets;

    for (int i = 0; i < (int)enemies.size(); i++)
        if (enemies[i].active) newEnemies.push_back(enemies[i]);
    enemies = newEnemies;

    for (int i = 0; i < (int)explosions.size(); i++)
        if (explosions[i].timer > 0) newExplosions.push_back(explosions[i]);
    explosions = newExplosions;
}

void titleScreen() {
    clearScreen();
    setColor(14);
    gotoxy(10, 4);  cout << "  ___  ____  ____  ___  ____  ";
    gotoxy(10, 5);  cout << " / __)(  _ \\(  __)/ __)(  __) ";
    gotoxy(10, 6);  cout << " \\__ \\ ) __/ ) _)( (__  ) _)  ";
    gotoxy(10, 7);  cout << " (___/(__)  (____)\\___)(____) ";
    setColor(11);
    gotoxy(12, 9);  cout << " ___  _  _  __   __  ____  ____  ";
    gotoxy(12, 10); cout << "/ __)( )( )/  \\ /  \\(_  _)(  __)";
    gotoxy(12, 11); cout << "\\__ \\ )__(  O )  O ) )(   ) _)  ";
    gotoxy(12, 12); cout << "(___/(____)\\__/ \\__/ (__) (____)";
    setColor(7);
    gotoxy(14, 15); cout << "Enemy Types:";
    setColor(10);  gotoxy(14, 16); cout << "\\V/ Basic (+50)";
    setColor(13);  gotoxy(14, 17); cout << ">X< Fast  (+100)";
    setColor(12);  gotoxy(14, 18); cout << "[W] Tank  (+150, 3 HP)";
    setColor(14);
    gotoxy(15, 21); cout << "Press ENTER to Continue...";
    setColor(7);
    while (_getch() != 13);
}

void selectDifficulty() {
    clearScreen();
    setColor(14);
    gotoxy(12, 6);  cout << "=============================";
    gotoxy(12, 7);  cout << "     SELECT DIFFICULTY       ";
    gotoxy(12, 8);  cout << "=============================";

    setColor(10); gotoxy(14, 11); cout << "[1] EASY   (Relaxed Speed)";
    setColor(13); gotoxy(14, 13); cout << "[2] MEDIUM (Standard Game)";
    setColor(12); gotoxy(14, 15); cout << "[3] HARD   (Fast & Furious)";

    setColor(14); gotoxy(12, 19); cout << "Select Option (1-3): ";

    while (true) {
        char choice = _getch();
        if (choice == '1') { difficulty = 1; break; }
        if (choice == '2') { difficulty = 2; break; }
        if (choice == '3') { difficulty = 3; break; }
    }
    clearScreen();
}

void gameOverScreen() {
    clearScreen();

    setColor(12);

    gotoxy(10, 8);  cout << "  _____    _    __  __ _____ ";
    gotoxy(10, 9);  cout << " / ____|  / \\  |  \\/  | ____|";
    gotoxy(10,10);  cout << "| |  _   / _ \\ | |\\/| |  _|  ";
    gotoxy(10,11);  cout << "| |_| | / ___ \\| |  | | |___ ";
    gotoxy(10,12);  cout << " \\____|/_/   \\_\\_|  |_|_____|";

    gotoxy(10,14);  cout << "  _____  _   _ _____ ____  ";
    gotoxy(10,15);  cout << " / _ \\ \\/ | | ____|  _ \\ ";
    gotoxy(10,16);  cout << "| | | |  | |  _| | |_) |";
    gotoxy(10,17);  cout << "| |_| |  | | |___|  _ < ";
    gotoxy(10,18);  cout << " \\___/   |_|_____|_| \\_\\";

    setColor(14);
    gotoxy(17, 20); cout << "Final Score  : " << score;
    gotoxy(17, 21); cout << "Level Reached: " << level;
    setColor(7);
    gotoxy(14, 23); cout << "Press ENTER to Exit...";
    while (_getch() != 13);
}

int main() {
    srand((unsigned)time(0));
    hideCursor();

    // 1. Show intro screens (without offset centering)
    titleScreen();
    selectDifficulty();

    // 2. Calculate offset to center the main gameplay window
    calculateOffsets();

    playerX = WIDTH / 2;
    playerY = HEIGHT - 2;

    drawBorder();
    drawHUD();

    while (!gameOver) {
        handleInput();

        if (!paused) {
            erase();
            update();
            draw();
            drawHUD();
        } else {
            setColor(15);
            gotoxy(WIDTH/2 - 4, HEIGHT/2);
            cout << "[ PAUSED ]";
            handleInput();
        }

        Sleep(40);
    }

    // 3. Reset offsets back to 0 for a centered full screen Game Over layout
    offsetX = 0; offsetY = 0;
    gameOverScreen();
    setColor(7);
    return 0;
}


