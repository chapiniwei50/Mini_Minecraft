#pragma once

#include "smartpointerhelp.h"
#include "glm_includes.h"
#include "drawable.h"
#include <array>
#include <unordered_map>
#include <cstddef>
#include <stdio.h>
#include <stdexcept>
#include <set>


// Lets us use any enum class as the key of a
// std::unordered_map
struct EnumHash {
    template <typename T>
    size_t operator()(T t) const {
        return static_cast<size_t>(t);
    }
};


enum BlockType : unsigned char
{
    EMPTY, GRASS, DIRT, STONE, WATER, LAVA
};

// The six cardinal directions in 3D space
enum Direction : unsigned char
{
    XPOS, XNEG, YPOS, YNEG, ZPOS, ZNEG
};

enum class BiomeType : unsigned char{
    NULLBIOME,
    DESSERT,
    PLAIN,
    MOUNTAIN,
    RIVER,
    LAVA
};





#define GRID 0.0625

const static std::unordered_map<BlockType, std::unordered_map<Direction, glm::vec2, EnumHash>, EnumHash> blockFaceUVs
{
    {GRASS, std::unordered_map<Direction, glm::vec2, EnumHash>{
           {XPOS, glm::vec2(3 * GRID, 15 * GRID)},
           {XNEG, glm::vec2(3 * GRID, 15 * GRID)},
           {YPOS, glm::vec2(8 * GRID, 13 * GRID)},
           {YNEG, glm::vec2(2 * GRID, 15 * GRID)},
           {ZPOS, glm::vec2(3 * GRID, 15 * GRID)},
           {ZNEG, glm::vec2(3 * GRID, 15 * GRID)}}},
    {DIRT, std::unordered_map<Direction, glm::vec2, EnumHash>{
           {XPOS, glm::vec2(2 * GRID, 15 * GRID)},
           {XNEG, glm::vec2(2 * GRID, 15 * GRID)},
           {YPOS, glm::vec2(2 * GRID, 15 * GRID)},
           {YNEG, glm::vec2(2 * GRID, 15 * GRID)},
           {ZPOS, glm::vec2(2 * GRID, 15 * GRID)},
           {ZNEG, glm::vec2(2 * GRID, 15 * GRID)}}},
    {STONE, std::unordered_map<Direction, glm::vec2, EnumHash>{
           {XPOS, glm::vec2(1 * GRID, 15 * GRID)},
           {XNEG, glm::vec2(1 * GRID, 15 * GRID)},
           {YPOS, glm::vec2(1 * GRID, 15 * GRID)},
           {YNEG, glm::vec2(1 * GRID, 15 * GRID)},
           {ZPOS, glm::vec2(1 * GRID, 15 * GRID)},
           {ZNEG, glm::vec2(1 * GRID, 15 * GRID)}}},
    {WATER, std::unordered_map<Direction, glm::vec2, EnumHash>{
           {XPOS, glm::vec2(14 * GRID, 2 * GRID)},
           {XNEG, glm::vec2(14 * GRID, 2 * GRID)},
           {YPOS, glm::vec2(14 * GRID, 2 * GRID)},
           {YNEG, glm::vec2(14 * GRID, 2 * GRID)},
           {ZPOS, glm::vec2(14 * GRID, 2 * GRID)},
           {ZNEG, glm::vec2(14 * GRID, 2 * GRID)}}},
    {LAVA, std::unordered_map<Direction, glm::vec2, EnumHash>{
           {XPOS, glm::vec2(14 * GRID, 0 * GRID)},
           {XNEG, glm::vec2(14 * GRID, 0 * GRID)},
           {YPOS, glm::vec2(14 * GRID, 0 * GRID)},
           {YNEG, glm::vec2(14 * GRID, 0 * GRID)},
           {ZPOS, glm::vec2(14 * GRID, 0 * GRID)},
           {ZNEG, glm::vec2(14 * GRID, 0 * GRID)}}}
};
