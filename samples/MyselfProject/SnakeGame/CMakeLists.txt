cmake_minimum_required(VERSION 3.13.1)
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(SnakeGame)
target_sources(app PRIVATE src/main.c  src/Adafruit_GFX.c src/coll_ball.c)
# src/snake.c src/solve_snake.c