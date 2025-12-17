#include <SFML/Graphics.hpp>
#include <iostream>
#include <vector>
#include <queue>
#include <map>
#include <cmath>
#include <string>

using namespace std;

const int TILE = 25; 
const int MAP_WIDTH = 21;
const int MAP_HEIGHT = 21; 
const int WINDOW_OFFSET_X = 50;
const int WINDOW_OFFSET_Y = 50;

struct Entity {
    int x, y;            
    float px, py;        
    sf::Color originalColor; 
    int startX, startY;  
    bool isDead;         
    int dirX, dirY;      
};


class PacmanGame {
private:
    sf::RenderWindow window;
    vector<string> mapData;
    vector<string> originalMap; 
    Entity player;
    vector<Entity> ghosts;
    
    bool gameStarted;
    int score;
    int frameCount;
    int frightenedTimer;
    
    int reqDirX, reqDirY; 
    float pacRotation;

    const vector<string> INITIAL_MAP = {
        "111111111111111111111",
        "120000000100000000021",
        "101111110101111111101",
        "101111110101111111101",
        "100000000000000000001",
        "101110111111111011101",
        "101000000100000000101",
        "101011110101111110101",
        "000010000G00000100000",
        "111110111G11110111111",
        "1000001GGGGG100000001",
        "111110111111110111111",
        "000000000000000000000",
        "111110111111111011111",
        "100000000P00000000001",
        "101110111010111011101",
        "100010000010000010001",
        "111010111111111010111",
        "120000000000000000021",
        "111111111111111111111",
        "111111111111111111111"
    };

public:
    PacmanGame() : window(sf::VideoMode(MAP_WIDTH * TILE + 2 * WINDOW_OFFSET_X, MAP_HEIGHT * TILE + 2 * WINDOW_OFFSET_Y), "Pac-Man Final") {
        window.setFramerateLimit(60); 
        initGame();
    }

    void run() {
        while (window.isOpen()) {
            processEvents();
            if (gameStarted) update();
            render();
        }
    }

private:
    void initGame() {
        mapData = INITIAL_MAP;
        originalMap = INITIAL_MAP;
        score = 0;
        frameCount = 0;
        frightenedTimer = 0;
        gameStarted = false;
        reqDirX = 0; reqDirY = 0;
        pacRotation = 0.f;

        // Initialize Player
        player.dirX = 1; player.dirY = 0;
        for (int y = 0; y < MAP_HEIGHT; y++) {
            for (int x = 0; x < MAP_WIDTH; x++) {
                if (mapData[y][x] == 'P') {
                    player.x = x; player.y = y;
                    player.startX = x; player.startY = y;
                    mapData[y][x] = ' '; 
                }
            }
        }
        updatePixelPos(player);

        // Initialize Ghosts
        ghosts.clear();
        vector<sf::Color> colors = { sf::Color::Red, sf::Color(255, 184, 255), sf::Color(0, 255, 255), sf::Color(255, 184, 82) };
        int cx = 10, cy = 10;
        
        ghosts.push_back({ cx, cy - 2, 0, 0, colors[0], cx, cy - 2, false, 0, 0 });
        ghosts.push_back({ cx - 1, cy, 0, 0, colors[1], cx - 1, cy, false, 0, 0 });
        ghosts.push_back({ cx + 1, cy, 0, 0, colors[2], cx + 1, cy, false, 0, 0 });
        ghosts.push_back({ cx, cy + 1, 0, 0, colors[3], cx, cy + 1, false, 0, 0 });

        for (auto& g : ghosts) updatePixelPos(g);
    }

    // --- Helper Functions ---
    void updatePixelPos(Entity& e) {
        e.px = e.x * TILE + WINDOW_OFFSET_X;
        e.py = e.y * TILE + WINDOW_OFFSET_Y;
    }

    bool isWall(int x, int y, bool isGhost) {
        if (y < 0 || y >= MAP_HEIGHT || x < 0 || x >= MAP_WIDTH) return true;
        char tile = mapData[y][x];
        if (tile == '1') return true; 
        if (tile == 'G' && !isGhost) return true; // Player blocked by Ghost House
        return false; 
    }

    // --- Pathfinding Logic (BFS) ---
    pair<int, int> findNextMove(const Entity& ghost, int targetX, int targetY) {
        using Pos = pair<int, int>;
        if (ghost.x == targetX && ghost.y == targetY) return {0, 0};

        queue<Pos> q;
        q.push({ghost.x, ghost.y});
        map<Pos, Pos> predecessor;
        predecessor[{ghost.x, ghost.y}] = {-1, -1}; 

        const Pos directions[] = {{0, 1}, {0, -1}, {1, 0}, {-1, 0}};
        Pos target_pos = {-1, -1};

        while (!q.empty()) {
            Pos current = q.front();
            q.pop();

            for (const auto& dir : directions) {
                int nx = current.first + dir.first;
                int ny = current.second + dir.second;
                
                if (nx < 0) nx = MAP_WIDTH - 1; else if (nx >= MAP_WIDTH) nx = 0;
                Pos next_pos = {nx, ny};

                if (nx == targetX && ny == targetY) {
                    predecessor[next_pos] = current;
                    target_pos = next_pos;
                    std::queue<Pos>().swap(q); 
                    break; 
                }

                if (!isWall(nx, ny, true) && predecessor.find(next_pos) == predecessor.end()) {
                    predecessor[next_pos] = current;
                    q.push(next_pos);
                }
            }
            if (target_pos.first != -1) break;
        }

        if (target_pos.first == -1) return {0, 0};
        
        Pos step = target_pos;
        while (predecessor[step].first != ghost.x || predecessor[step].second != ghost.y) {
            if (predecessor[step].first == -1) break; 
            step = predecessor[step];
        }
        
        int dx = step.first - ghost.x;
        int dy = step.second - ghost.y;
        if (abs(dx) > 1) dx = (dx < 0) ? 1 : -1; 
        return {dx, dy};
    }

    // --- Core Loops ---

    void processEvents() {
        sf::Event e;
        while (window.pollEvent(e)) {
            if (e.type == sf::Event::Closed) window.close();
            if (e.type == sf::Event::KeyPressed) {
                if (!gameStarted) {
                    if (e.key.code == sf::Keyboard::Space) {
                        // Reset map on restart
                         if (score == 0) initGame(); 
                         gameStarted = true;
                    }
                } else {
                    if (e.key.code == sf::Keyboard::W || e.key.code == sf::Keyboard::Up)    { reqDirX = 0; reqDirY = -1; }
                    if (e.key.code == sf::Keyboard::S || e.key.code == sf::Keyboard::Down)  { reqDirX = 0; reqDirY = 1; }
                    if (e.key.code == sf::Keyboard::A || e.key.code == sf::Keyboard::Left)  { reqDirX = -1; reqDirY = 0; }
                    if (e.key.code == sf::Keyboard::D || e.key.code == sf::Keyboard::Right) { reqDirX = 1; reqDirY = 0; }
                }
            }
        }
    }

    void update() {
        frameCount++;
        if (frightenedTimer > 0 && frameCount % 10 == 0) frightenedTimer--;

        // 1. Player Update (Faster: Every 10 frames)
        if (frameCount % 10 == 0) {
            if (reqDirX != 0 || reqDirY != 0) {
                player.dirX = reqDirX;
                player.dirY = reqDirY;
                if (player.dirX == 1) pacRotation = 0.f;
                if (player.dirX == -1) pacRotation = 180.f;
                if (player.dirY == 1) pacRotation = 90.f;
                if (player.dirY == -1) pacRotation = 270.f;
            }

            int nx = player.x + player.dirX;
            int ny = player.y + player.dirY;
            if (nx < 0) nx = MAP_WIDTH - 1; else if (nx >= MAP_WIDTH) nx = 0;

            if (ny >= 0 && ny < MAP_HEIGHT && !isWall(nx, ny, false)) {
                player.x = nx; player.y = ny;
                updatePixelPos(player);
                
                // Eat Items
                char item = mapData[ny][nx];
                if (item == '0') { score += 10; mapData[ny][nx] = ' '; }
                else if (item == '2') { 
                    score += 50; mapData[ny][nx] = ' '; 
                    frightenedTimer = 60; // Scatter mode duration
                    for (auto& g : ghosts) g.isDead = false;
                }
                window.setTitle("Pac-Man | Score: " + to_string(score));
            }
        }

        // 2. Ghost Update (Much Slower: Every 25 frames)
        if (frameCount % 25 == 0) {
            // Scatter targets for corners
            vector<pair<int, int>> corners = {{1,1}, {MAP_WIDTH-2,1}, {1,MAP_HEIGHT-2}, {MAP_WIDTH-2,MAP_HEIGHT-2}};
            
            for (size_t i = 0; i < ghosts.size(); ++i) {
                Entity& g = ghosts[i];
                int tx, ty;

                if (g.isDead) { tx = g.startX; ty = g.startY; }
                else if (frightenedTimer > 0) { tx = corners[i].first; ty = corners[i].second; }
                else { tx = player.x; ty = player.y; }

                pair<int, int> move = findNextMove(g, tx, ty);
                if (move.first != 0 || move.second != 0) {
                    g.dirX = move.first; g.dirY = move.second;
                    g.x += g.dirX; g.y += g.dirY;
                }
                
                // Wrap around
                if (g.x < 0) g.x = MAP_WIDTH - 1;
                if (g.x >= MAP_WIDTH) g.x = 0;
                
                updatePixelPos(g);
                
                if (g.isDead && g.x == g.startX && g.y == g.startY) g.isDead = false;
            }
        }

        // 3. Collision Check
        for (auto& g : ghosts) {
            if (g.x == player.x && g.y == player.y) {
                if (g.isDead) continue;
                if (frightenedTimer > 0) {
                    score += 200; g.isDead = true;
                } else {
                    gameStarted = false;
                    score = 0; // Reset score display but keep internal state for full reset on space
                    window.setTitle("Game Over! Press Space");
                }
            }
        }
    }

    void render() {
        window.clear(sf::Color::Black);
        
        sf::RectangleShape tile(sf::Vector2f(TILE, TILE));
        sf::CircleShape dot, bigDot;
        
        // Draw Map
        for (int y = 0; y < MAP_HEIGHT; y++) {
            for (int x = 0; x < MAP_WIDTH; x++) {
                char t = mapData[y][x];
                if (t == '1') tile.setFillColor(sf::Color(20, 20, 180));
                else if (t == 'G') tile.setFillColor(sf::Color::Black);
                else tile.setFillColor(sf::Color(10, 10, 10));
                
                tile.setPosition(x * TILE + WINDOW_OFFSET_X, y * TILE + WINDOW_OFFSET_Y);
                window.draw(tile);

                if (gameStarted) {
                    if (t == '0') {
                        dot.setRadius(TILE/8); dot.setFillColor(sf::Color(255, 220, 180));
                        dot.setPosition(x*TILE + WINDOW_OFFSET_X + TILE/2 - TILE/8, y*TILE + WINDOW_OFFSET_Y + TILE/2 - TILE/8);
                        window.draw(dot);
                    } else if (t == '2') {
                        bigDot.setRadius(TILE/3); bigDot.setFillColor(sf::Color(255, 220, 180));
                        if (frameCount % 30 < 15) bigDot.setFillColor(sf::Color::White);
                        bigDot.setPosition(x*TILE + WINDOW_OFFSET_X + TILE/2 - TILE/3, y*TILE + WINDOW_OFFSET_Y + TILE/2 - TILE/3);
                        window.draw(bigDot);
                    }
                }
            }
        }

        // Draw Player
        sf::CircleShape pac(TILE / 2);
        pac.setFillColor(sf::Color::Yellow);
        pac.setOrigin(TILE / 2, TILE / 2);
        pac.setPosition(player.px + TILE / 2, player.py + TILE / 2);
        pac.setRotation(pacRotation);
        window.draw(pac);
        // 

        // Mouth Animation
        if (frameCount % 20 < 10) {
            sf::ConvexShape mouth;
            mouth.setPointCount(3);
            mouth.setPoint(0, sf::Vector2f(TILE/2, TILE/2));
            mouth.setPoint(1, sf::Vector2f(TILE, TILE/4));
            mouth.setPoint(2, sf::Vector2f(TILE, TILE*0.75));
            mouth.setFillColor(sf::Color::Black);
            mouth.setOrigin(TILE/2, TILE/2);
            mouth.setPosition(player.px + TILE/2, player.py + TILE/2);
            mouth.setRotation(pacRotation);
            window.draw(mouth);
        }

        // Draw Ghosts
        for (auto& g : ghosts) {
            sf::CircleShape body(TILE/2);
            if (g.isDead) body.setFillColor(sf::Color::Transparent);
            else if (frightenedTimer > 0) {
                body.setFillColor(sf::Color::Blue);
                if (frightenedTimer < 20 && frameCount % 10 < 5) body.setFillColor(sf::Color::White);
            } else body.setFillColor(g.originalColor);
            
            body.setPosition(g.px, g.py);
            window.draw(body);

            // Eyes & Pupils
            sf::CircleShape eye(TILE/6); eye.setFillColor(sf::Color::White);
            sf::CircleShape pupil(TILE/12); pupil.setFillColor(sf::Color::Blue);
            
            float pX = g.dirX * 3, pY = g.dirY * 3;
            
            eye.setPosition(g.px + TILE/6, g.py + TILE/4); window.draw(eye);
            eye.setPosition(g.px + TILE/2 + 2, g.py + TILE/4); window.draw(eye);
            
            pupil.setPosition(g.px + TILE/6 + 2 + pX, g.py + TILE/4 + 2 + pY); window.draw(pupil);
            pupil.setPosition(g.px + TILE/2 + 4 + pX, g.py + TILE/4 + 2 + pY); window.draw(pupil);
        }

        window.display();
    }
};

int main() {
    PacmanGame game;
    game.run();
    return 0;

}
