#pragma once

namespace CameraRefillReward
{
    void Initialize();
    void RefillFilm(int amount);
    void SetFilmCount(int count);
    int GetCurrentFilmCount();
    void ScanPlayerForCamera(); // Debug function
}