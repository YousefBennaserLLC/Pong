#include <raylib.h>
#include <cmath>
#include <time.h>
#include <vector>

// Constants
const int SCREEN_WIDTH = 1600;
const int SCREEN_HEIGHT = 900;
const int BORDER_THICKNESS = 40; // Decorative border
const int PLAY_AREA_TOP = BORDER_THICKNESS;
const int PLAY_AREA_BOTTOM = SCREEN_HEIGHT - BORDER_THICKNESS;
const int PADDLE_WIDTH = 20;
const int PADDLE_HEIGHT = 200;
const int BALL_RADIUS = 20;
const float BALL_SPEED = 4.0f;
const int WIN_SCORE = 10;

enum Collisions {
    NO_COL,
    LOWER_BORDER,
    UPPER_BORDER,
    LEFT_BORDER,
    RIGHT_BORDER,
    PLAYER_COL,
    BOT_COL,
};

enum GameState {
    MENU,
    GAME,
    OVER,
};

enum Difficulty {
    EASY,
    MEDIUM,
    HARD,
};

struct Circle {
    Vector2 center;
    Collisions Collision;
};

struct Button {
    Rectangle rect;
    const char* text;
    Color normalColor;
    Color hoverColor;
    bool isHovered;
};

struct Particle {
    Vector2 position;
    Vector2 velocity;
    float lifetime;
    Color color;
};

// Color scheme - Cyberpunk vibes!
const Color BG_COLOR = Color{10, 10, 30, 255};
const Color PADDLE_COLOR = Color{0, 255, 200, 255};
const Color BALL_COLOR = Color{255, 50, 150, 255};
const Color UI_COLOR = Color{100, 200, 255, 150};
const Color ACCENT_COLOR = Color{255, 100, 255, 255};

// Particle system
std::vector<Particle> particles;

void SpawnParticles(Vector2 position, Color color, int count) {
    for (int i = 0; i < count; i++) {
        Particle p;
        p.position = position;
        float angle = GetRandomValue(0, 360) * DEG2RAD;
        float speed = GetRandomValue(50, 200);
        p.velocity = {(float)cos(angle) * speed,(float) sin(angle) * speed};
        p.lifetime = GetRandomValue(20, 60) / 60.0f;
        p.color = color;
        particles.push_back(p);
    }
}

void UpdateParticles(float deltaTime) {
    for (auto it = particles.begin(); it != particles.end();) {
        it->lifetime -= deltaTime;
        if (it->lifetime <= 0) {
            it = particles.erase(it);
        } else {
            it->position.x += it->velocity.x * deltaTime;
            it->position.y += it->velocity.y * deltaTime;
            it->velocity.x *= 0.95f;
            it->velocity.y *= 0.95f;
            ++it;
        }
    }
}

void DrawParticles() {
    for (const auto& p : particles) {
        float alpha = p.lifetime;
        DrawCircleV(p.position, 3, ColorAlpha(p.color, alpha));
    }
}

// Operator overloads
inline bool operator==(const Vector2& v1, const Vector2& v2) {
    return (v1.x == v2.x) && (v2.y == v2.y);
}
inline Vector2 operator+(const Vector2& v1, const Vector2& v2) {
    return {v1.x + v2.x, v1.y + v2.y};
}
inline Vector2 operator-(const Vector2& v1, const Vector2& v2) {
    return {v1.x - v2.x, v1.y - v2.y};
}
inline void operator*=(Vector2& v, float x) {
    v.x *= x;
    v.y *= x;
}

// Collision detection
void CircleCollideWith(Circle& circle, Rectangle player, Rectangle bot) {
    if (circle.center.x < BALL_RADIUS) { circle.Collision = LEFT_BORDER; return; }
    if (circle.center.x > SCREEN_WIDTH - BALL_RADIUS) { circle.Collision = RIGHT_BORDER; return; }
    if (circle.center.y < PLAY_AREA_TOP + BALL_RADIUS) { circle.Collision = UPPER_BORDER; return; }
    if (circle.center.y > PLAY_AREA_BOTTOM - BALL_RADIUS) { circle.Collision = LOWER_BORDER; return; }
    if (CheckCollisionCircleRec(circle.center, BALL_RADIUS, player)) { circle.Collision = PLAYER_COL; return; }
    if (CheckCollisionCircleRec(circle.center, BALL_RADIUS, bot)) { circle.Collision = BOT_COL; return; }
    circle.Collision = NO_COL;
}

void CircleCollideWith(Circle& circle) {
    if (circle.center.x < BALL_RADIUS) { circle.Collision = LEFT_BORDER; return; }
    if (circle.center.x > SCREEN_WIDTH - BALL_RADIUS) { circle.Collision = RIGHT_BORDER; return; }
    if (circle.center.y < PLAY_AREA_TOP + BALL_RADIUS) { circle.Collision = UPPER_BORDER; return; }
    if (circle.center.y > PLAY_AREA_BOTTOM - BALL_RADIUS) { circle.Collision = LOWER_BORDER; return; }
    circle.Collision = NO_COL;
}

Vector2 randomDirection(float dep) {
    int x = GetRandomValue(120, 240);
    double deg = (double)x * PI / 180.0;
    return {dep * (float)cos(deg), dep * (float)sin(deg)};
}

bool withinHigh(Rectangle rec) {
    return rec.y > PLAY_AREA_TOP;
}

bool withinLow(Rectangle rec) {
    return (rec.y + rec.height) < PLAY_AREA_BOTTOM;
}

Circle PointOfCollision(Circle circle, Vector2 oldPosition) {
    Vector2 Velocity = circle.center - oldPosition;
    CircleCollideWith(circle);
    while (circle.Collision == NO_COL) {
        circle.center = circle.center + Velocity;
        CircleCollideWith(circle);
    }
    return circle;
}

void IncreaseSpeed(Circle circle, Vector2& Velocity, float& currentSpeed) {
    if (circle.Collision == PLAYER_COL || circle.Collision == BOT_COL) {
        float speed = sqrt(pow(Velocity.x, 2) + pow(Velocity.y, 2));
        float increase = 2.0f / currentSpeed; // Logarithmic increase
        Velocity.x += (increase * Velocity.x) / speed;
        Velocity.y += (increase * Velocity.y) / speed;
        currentSpeed += increase;
    }
}

Circle RightBorderCollision(Circle circle, Vector2 oldPosition) {
    Circle futureCollision = PointOfCollision(circle, oldPosition);
    Vector2 Velocity = circle.center - oldPosition;
    int bounceCount = 0;
    while (futureCollision.Collision != RIGHT_BORDER && bounceCount < 10) {
        if (futureCollision.Collision == UPPER_BORDER || futureCollision.Collision == LOWER_BORDER) {
            Velocity.y = -Velocity.y;
            futureCollision = PointOfCollision({futureCollision.center + Velocity, NO_COL}, futureCollision.center);
            bounceCount++;
        } else {
            break;
        }
    }
    return futureCollision;
}

int move(Circle& circle, Vector2& OldPosition, float dep, Rectangle player, Rectangle bot, float& currentSpeed, Sound paddleHit, Sound wallHit) {
    int score = 0;
    Vector2 Velocity;

    if (OldPosition == circle.center) {
        Velocity = randomDirection(dep);
        OldPosition = circle.center;
        circle.center = circle.center + Velocity;
        return 0;
    }

    CircleCollideWith(circle, player, bot);

    switch (circle.Collision) {
        case NO_COL: {
            Velocity = circle.center - OldPosition;
            break;
        }
        case UPPER_BORDER:
        case LOWER_BORDER: {
            PlaySound(wallHit);
            SpawnParticles(circle.center, PADDLE_COLOR, 8);
            Velocity.x = circle.center.x - OldPosition.x;
            Velocity.y = OldPosition.y - circle.center.y;

            // Push ball away from border
            if (circle.Collision == UPPER_BORDER) circle.center.y = PLAY_AREA_TOP + BALL_RADIUS + 2;
            else circle.center.y = PLAY_AREA_BOTTOM - BALL_RADIUS - 2;
            break;
        }
        case BOT_COL: {
            PlaySound(paddleHit);
            SpawnParticles(circle.center, BALL_COLOR, 12);
            Velocity = circle.center - OldPosition;
            float speed = sqrt(pow(Velocity.x, 2) + pow(Velocity.y, 2));
            float dif = circle.center.y - bot.y;
            float hitPoint = dif / PADDLE_HEIGHT;
            if (hitPoint < 0) hitPoint = 0;
            if (hitPoint > 1) hitPoint = 1;
            // FIXED: Y grows DOWN, so angles need to be inverted
            // hitPoint 0 (top) should send ball UP (negative Y velocity)
            // hitPoint 1 (bottom) should send ball DOWN (positive Y velocity)
            double angle = (255 - (hitPoint * 150)) * (PI / 180.0); // Inverted from 255° to 105°
            Velocity.x = speed * (float)cos(angle);
            Velocity.y = speed * (float)sin(angle);

            // Push ball away from paddle to prevent multi-collision
            circle.center.x = bot.x - BALL_RADIUS - 2;

            score = 1;
            break;
        }
        case PLAYER_COL: {
            PlaySound(paddleHit);
            SpawnParticles(circle.center, BALL_COLOR, 12);
            Velocity = circle.center - OldPosition;
            float speed = sqrt(pow(Velocity.x, 2) + pow(Velocity.y, 2));
            float dif = circle.center.y - player.y;
            float hitPoint = dif / PADDLE_HEIGHT;
            if (hitPoint < 0) hitPoint = 0;
            if (hitPoint > 1) hitPoint = 1;
            // FIXED: Y grows DOWN, so angles need to be inverted
            // hitPoint 0 (top) should send ball UP (negative Y velocity)
            // hitPoint 1 (bottom) should send ball DOWN (positive Y velocity)
            double angle = (-75 + (hitPoint * 150)) * (PI / 180.0); // From -75° to 75°
            Velocity.x = speed * (float)cos(angle);
            Velocity.y = speed * (float)sin(angle);

            // Push ball away from paddle
            circle.center.x = player.x + PADDLE_WIDTH + BALL_RADIUS + 2;

            score = 1;
            break;
        }
    }

    IncreaseSpeed(circle, Velocity, currentSpeed);
    OldPosition = circle.center;
    circle.center = circle.center + Velocity;
    return score;
}

void moveBotEasy(Rectangle& bot, Vector2 circle) {
    float botCenterLine = bot.y + PADDLE_HEIGHT / 2;
    if ((circle.y > botCenterLine) && withinLow(bot)) {
        bot.y += 5;
    }
    if (circle.y < botCenterLine && withinHigh(bot)) {
        bot.y -= 5;
    }
}

void moveBotMedium(Rectangle& bot, Circle circle, Vector2 oldPosition, Circle& futureCollision) {
    if (circle.Collision == PLAYER_COL || circle.Collision == UPPER_BORDER || circle.Collision == LOWER_BORDER) {
        futureCollision = PointOfCollision(circle, oldPosition);
    }

    if (futureCollision.Collision != RIGHT_BORDER) {
        moveBotEasy(bot, circle.center);
    } else {
        float botCenterLine = bot.y + PADDLE_HEIGHT / 2;
        if ((futureCollision.center.y > botCenterLine) && withinLow(bot)) {
            bot.y += 5;
        }
        if (futureCollision.center.y < botCenterLine && withinHigh(bot)) {
            bot.y -= 5;
        }
    }
}

void moveBotHard(Rectangle& bot, Circle circle, Vector2 oldPosition, Circle& futureCollision) {
    if (circle.Collision == PLAYER_COL) {
        futureCollision = RightBorderCollision(circle, oldPosition);
    }
    float botCenterLine = bot.y + PADDLE_HEIGHT / 2;
    if ((futureCollision.center.y > botCenterLine) && withinLow(bot)) {
        bot.y += 7;
    }
    if (futureCollision.center.y < botCenterLine && withinHigh(bot)) {
        bot.y -= 7;
    }
}

// Draw rounded paddle with glow
void DrawRoundedPaddle(Rectangle rec, Color color) {
    // Glow effect
    DrawRectangleGradientEx(
        {rec.x - 8, rec.y - 8, rec.width + 16, rec.height + 16},
        ColorAlpha(color, 0), ColorAlpha(color, 0.1f),
        ColorAlpha(color, 0.1f), ColorAlpha(color, 0)
    );

    // Main paddle body with rounded corners
    DrawRectangleRounded(rec, 0.5f, 10, color);

    // Bright center line
    DrawRectangleRounded({rec.x + rec.width/2 - 2, rec.y + 5, 4, rec.height - 10},
                         0.5f, 5, ColorAlpha(WHITE, 0.5f));
}

// Draw ball with glow and trail
void DrawGlowBall(Vector2 pos, float radius, Color color) {
    // Outer glow layers
    for (int i = 3; i > 0; i--) {
        float glowRadius = radius + (i * 5);
        float alpha = 0.15f / i;
        DrawCircleV(pos, glowRadius, ColorAlpha(color, alpha));
    }

    // Main ball
    DrawCircleV(pos, radius, color);

    // Highlight
    DrawCircleV({pos.x - 5, pos.y - 5}, radius / 3, ColorAlpha(WHITE, 0.6f));
}

// UI Functions
bool IsButtonClicked(Button& button, Vector2 mousePos) {
    button.isHovered = CheckCollisionPointRec(mousePos, button.rect);
    return button.isHovered && IsMouseButtonPressed(MOUSE_LEFT_BUTTON);
}

void DrawButton(Button button) {
    Color color = button.isHovered ? button.hoverColor : button.normalColor;

    // Glow on hover
    if (button.isHovered) {
        DrawRectangleRounded({button.rect.x - 5, button.rect.y - 5, button.rect.width + 10, button.rect.height + 10},
                             0.3f, 10, ColorAlpha(button.hoverColor, 0.3f));
    }

    // Button
    DrawRectangleRounded(button.rect, 0.3f, 10, color);
    DrawRectangleRoundedLines(button.rect, 0.3f, 10, ColorAlpha(WHITE, 0.8f));

    int textWidth = MeasureText(button.text, 30);
    DrawText(button.text,
             button.rect.x + (button.rect.width - textWidth) / 2,
             button.rect.y + 18,
             30, WHITE);
}

void DrawMenu() {
    DrawText("P O N G", SCREEN_WIDTH / 2 - 200, 150, 100, ACCENT_COLOR);
    DrawText("Choose Your Difficulty", SCREEN_WIDTH / 2 - 200, 280, 30, UI_COLOR);
}

void DrawGameUI(int playerScore, int botScore) {
    // Draw decorative borders
    // Top border
    DrawRectangleGradientV(0, 0, SCREEN_WIDTH, BORDER_THICKNESS,
                           ColorAlpha(ACCENT_COLOR, 0.3f), ColorAlpha(ACCENT_COLOR, 0.1f));
    DrawRectangle(0, BORDER_THICKNESS - 3, SCREEN_WIDTH, 3, ACCENT_COLOR);

    // Bottom border
    DrawRectangleGradientV(0, PLAY_AREA_BOTTOM, SCREEN_WIDTH, BORDER_THICKNESS,
                           ColorAlpha(ACCENT_COLOR, 0.1f), ColorAlpha(ACCENT_COLOR, 0.3f));
    DrawRectangle(0, PLAY_AREA_BOTTOM, SCREEN_WIDTH, 3, ACCENT_COLOR);

    // Decorative corner accents
    DrawCircle(40, BORDER_THICKNESS / 2, 5, PADDLE_COLOR);
    DrawCircle(SCREEN_WIDTH - 40, BORDER_THICKNESS / 2, 5, PADDLE_COLOR);
    DrawCircle(40, PLAY_AREA_BOTTOM + BORDER_THICKNESS / 2, 5, PADDLE_COLOR);
    DrawCircle(SCREEN_WIDTH - 40, PLAY_AREA_BOTTOM + BORDER_THICKNESS / 2, 5, PADDLE_COLOR);

    // Dashed center line (only in play area)
    for (int i = PLAY_AREA_TOP; i < PLAY_AREA_BOTTOM; i += 30) {
        DrawRectangle(SCREEN_WIDTH / 2 - 3, i, 6, 20, ColorAlpha(UI_COLOR, 0.3f));
    }

    // Scores with glow
    DrawText(TextFormat("%d", playerScore), SCREEN_WIDTH / 2 - 100, BORDER_THICKNESS + 10, 60, ColorAlpha(PADDLE_COLOR, 0.8f));
    DrawText(TextFormat("%d", botScore), SCREEN_WIDTH / 2 + 60, BORDER_THICKNESS + 10, 60, ColorAlpha(PADDLE_COLOR, 0.8f));

    // FPS
    DrawText(TextFormat("FPS: %d", GetFPS()), 10, 10, 20, ColorAlpha(WHITE, 0.5f));

    // Instructions
    DrawText("F11 - Fullscreen | ESC - Menu", SCREEN_WIDTH - 350, 10, 20, ColorAlpha(WHITE, 0.5f));
}

void DrawGameOver(int playerScore, int botScore) {
    // Overlay
    DrawRectangle(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, ColorAlpha(BLACK, 0.7f));

    if (playerScore >= WIN_SCORE) {
        DrawText("YOU WIN!", SCREEN_WIDTH / 2 - 250, 250, 100, PADDLE_COLOR);
    } else {
        DrawText("YOU LOSE!", SCREEN_WIDTH / 2 - 280, 250, 100, BALL_COLOR);
    }

    DrawText(TextFormat("Final Score: %d - %d", playerScore, botScore),
             SCREEN_WIDTH / 2 - 220, 400, 40, WHITE);
    DrawText("Press ENTER to return to menu", SCREEN_WIDTH / 2 - 280, 550, 30, UI_COLOR);
    DrawText("Press ESC to quit", SCREEN_WIDTH / 2 - 150, 600, 25, ColorAlpha(WHITE, 0.7f));
}

int main() {
    InitWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "Pong");
    SetTargetFPS(120);
    SetRandomSeed(time(NULL));

    // Load icon
    Image icon = LoadImage("C:/Users/youss/Pictures/Pong2.png");
    SetWindowIcon(icon);
    UnloadImage(icon);

    // Initialize audio
    InitAudioDevice();
    Sound paddleHit = LoadSound("sounds/paddle_hit.wav");
    Sound wallHit = LoadSound("sounds/wall_hit.wav");
    Sound scoreSound = LoadSound("sounds/score.wav");

    GameState state = MENU;
    Difficulty difficulty = EASY;

    Circle circle = {{SCREEN_WIDTH / 2.0f, SCREEN_HEIGHT / 2.0f}, NO_COL};
    Rectangle player = {10, SCREEN_HEIGHT / 2.0f - PADDLE_HEIGHT / 2.0f, PADDLE_WIDTH, PADDLE_HEIGHT};
    Rectangle bot = {SCREEN_WIDTH - 30.0f, SCREEN_HEIGHT / 2.0f - PADDLE_HEIGHT / 2.0f, PADDLE_WIDTH, PADDLE_HEIGHT};
    Vector2 OldPosition = circle.center;
    Circle futureCollision = circle;

    int playerScore = 0;
    int botScore = 0;
    float currentSpeed = BALL_SPEED;

    while (!WindowShouldClose()) {
        // Fullscreen toggle
        if (IsKeyPressed(KEY_F11)) {
            ToggleFullscreen();
        }

        float deltaTime = GetFrameTime();
        UpdateParticles(deltaTime);

        switch (state) {
            case MENU: {
                Vector2 mousePos = GetMousePosition();

                Button easyBtn = {{550, 380, 500, 70}, "EASY - Reactive Bot", Color{0, 100, 0, 255}, PADDLE_COLOR, false};
                Button mediumBtn = {{550, 470, 500, 70}, "MEDIUM - Predictive Bot", Color{180, 100, 0, 255}, Color{255, 200, 0, 255}, false};
                Button hardBtn = {{550, 560, 500, 70}, "HARD - Perfect Prediction", Color{100, 0, 100, 255}, ACCENT_COLOR, false};

                if (IsButtonClicked(easyBtn, mousePos)) {
                    difficulty = EASY;
                    state = GAME;
                    playerScore = 0;
                    botScore = 0;
                    currentSpeed = BALL_SPEED;
                    circle = {{SCREEN_WIDTH / 2.0f, SCREEN_HEIGHT / 2.0f}, NO_COL};
                    OldPosition = circle.center;
                    player.y = SCREEN_HEIGHT / 2.0f - PADDLE_HEIGHT / 2.0f;
                    bot.y = SCREEN_HEIGHT / 2.0f - PADDLE_HEIGHT / 2.0f;
                    particles.clear();
                }
                if (IsButtonClicked(mediumBtn, mousePos)) {
                    difficulty = MEDIUM;
                    state = GAME;
                    playerScore = 0;
                    botScore = 0;
                    currentSpeed = BALL_SPEED;
                    circle = {{SCREEN_WIDTH / 2.0f, SCREEN_HEIGHT / 2.0f}, NO_COL};
                    OldPosition = circle.center;
                    player.y = SCREEN_HEIGHT / 2.0f - PADDLE_HEIGHT / 2.0f;
                    bot.y = SCREEN_HEIGHT / 2.0f - PADDLE_HEIGHT / 2.0f;
                    particles.clear();
                }
                if (IsButtonClicked(hardBtn, mousePos)) {
                    difficulty = HARD;
                    state = GAME;
                    playerScore = 0;
                    botScore = 0;
                    currentSpeed = BALL_SPEED;
                    circle = {{SCREEN_WIDTH / 2.0f, SCREEN_HEIGHT / 2.0f}, NO_COL};
                    OldPosition = circle.center;
                    player.y = SCREEN_HEIGHT / 2.0f - PADDLE_HEIGHT / 2.0f;
                    bot.y = SCREEN_HEIGHT / 2.0f - PADDLE_HEIGHT / 2.0f;
                    particles.clear();
                }

                BeginDrawing();
                ClearBackground(BG_COLOR);
                DrawMenu();
                DrawButton(easyBtn);
                DrawButton(mediumBtn);
                DrawButton(hardBtn);
                DrawParticles();
                EndDrawing();
                break;
            }

            case GAME: {
                // Player controls
                if (IsKeyDown(KEY_UP) && withinHigh(player)) {
                    player.y -= 7;
                }
                if (IsKeyDown(KEY_DOWN) && withinLow(player)) {
                    player.y += 7;
                }

                // ESC to menu
                if (IsKeyPressed(KEY_ESCAPE)) {
                    state = MENU;
                    particles.clear();
                }

                // Check for scoring BEFORE moving
                CircleCollideWith(circle, player, bot);
                if (circle.Collision == LEFT_BORDER) {
                    PlaySound(scoreSound);
                    botScore++;
                    SpawnParticles({50, SCREEN_HEIGHT / 2.0f}, BALL_COLOR, 30);
                    circle = {{SCREEN_WIDTH / 2.0f, SCREEN_HEIGHT / 2.0f}, NO_COL};
                    OldPosition = circle.center;
                    currentSpeed = BALL_SPEED;
                }
                if (circle.Collision == RIGHT_BORDER) {
                    PlaySound(scoreSound);
                    playerScore++;
                    SpawnParticles({SCREEN_WIDTH - 50, SCREEN_HEIGHT / 2.0f}, PADDLE_COLOR, 30);
                    circle = {{SCREEN_WIDTH / 2.0f, SCREEN_HEIGHT / 2.0f}, NO_COL};
                    OldPosition = circle.center;
                    currentSpeed = BALL_SPEED;
                }

                // Win condition
                if (playerScore >= WIN_SCORE || botScore >= WIN_SCORE) {
                    state = OVER;
                }

                // Game logic
                move(circle, OldPosition, BALL_SPEED, player, bot, currentSpeed, paddleHit, wallHit);

                if (difficulty == EASY) moveBotEasy(bot, circle.center);
                else if (difficulty == MEDIUM) moveBotMedium(bot, circle, OldPosition, futureCollision);
                else moveBotHard(bot, circle, OldPosition, futureCollision);

                BeginDrawing();
                ClearBackground(BG_COLOR);
                DrawGameUI(playerScore, botScore);
                DrawRoundedPaddle(player, PADDLE_COLOR);
                DrawRoundedPaddle(bot, PADDLE_COLOR);
                DrawGlowBall(circle.center, BALL_RADIUS, BALL_COLOR);
                DrawParticles();
                EndDrawing();
                break;
            }

            case OVER: {
                if (IsKeyPressed(KEY_ENTER)) {
                    state = MENU;
                    particles.clear();
                }

                BeginDrawing();
                ClearBackground(BG_COLOR);
                DrawGameOver(playerScore, botScore);
                DrawParticles();
                EndDrawing();
                break;
            }
        }
    }

    // Cleanup
    UnloadSound(paddleHit);
    UnloadSound(wallHit);
    UnloadSound(scoreSound);
    CloseAudioDevice();
    CloseWindow();
    return 0;
}