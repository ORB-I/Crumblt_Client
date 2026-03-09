// Part.h
#pragma once
#include <string>

struct Part {
    std::string id;

    // Position
    float posX = 0;
    float posY = 0;
    float posZ = 0;

    // Size
    float sizeX = 1;
    float sizeY = 1;
    float sizeZ = 1;

    // Color (0-1 range)
    float colorR = 1;
    float colorG = 1;
    float colorB = 1;

    bool dirty = false; // Needs render update
};
