#pragma once
#include <string>

struct Player {
    std::string id;       // local = "local", remote = photon actor id
    std::string username;

    float x = 0.0f;      // world position XZ plane
    float z = 0.0f;
    float speed = 5.0f;  // units per second

    float colorR = 1.0f;
    float colorG = 0.8f;
    float colorB = 0.0f; // local player is yellow

    bool isLocal = false;
};
