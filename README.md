# Snake Game in C (ncurses)

A classic terminal-based Snake game written in C using the ncurses library. Control your snake, eat food to grow, and try to reach level 12!

## Requirements

Install ncurses development library:
```bash
sudo apt-get install libncurses5-dev
```

## Build & Run

```bash
gcc -o snake snake.c -lncurses
./snake
```

## Controls

- **Arrow keys**: Move the snake
- **X**: Quit game

Eat food (@) to grow and level up. Avoid walls and your own body. Reach level 12 to win!