# Pong Game 🎮

A modern take on the classic Pong game with AI opponents, built in C++ using raylib.

## Features
- 🤖 **3 AI Difficulties**: Easy (reactive), Medium (predictive), Hard (perfect prediction)
- ✨ **Particle Effects**: Visual feedback on collisions
- 🎨 **Cyberpunk Aesthetic**: Glowing paddles and balls
- 🎵 **Sound Effects**: Paddle hits, wall bounces, scoring
- 📊 **Dynamic Physics**: Ball speed increases with each rally
- 🖥️ **Fullscreen Support**: Press F11

## Controls
- **↑/↓ Arrow Keys**: Move paddle
- **F11**: Toggle fullscreen
- **ESC**: Return to menu

## How to Build

### Prerequisites
- CMake
- C++ Compiler (GCC/MSVC/Clang)
- raylib

### Build Instructions
```bash
mkdir build
cd build
cmake ..
cmake --build .
```

## Gameplay
Try to score 10 points before the AI does! The ball gets faster with each hit, and where you hit the paddle affects the angle.

## Technologies Used
- **Language**: C++
- **Graphics**: raylib
- **Audio**: raylib audio system

## What I Learned
This was my first game development project. I learned:
- Game loop architecture
- Collision detection
- AI pathfinding and prediction
- Physics simulation
- State management

## License
MIT License 


*Day 1-2 of my game development journey!*
