#pragma once

#include <memory>
#include <vector>
#include <utility>

class GameBoard;

class BattleshipAlgorithm {
public:
    // Конструктор
    BattleshipAlgorithm(std::shared_ptr<GameBoard> board, int maxLives);
    
    // Основные методы
    bool makeMove();
    int getCurrentLives() const;
    
    // Геттеры
    double getCurrentLambda() const;

private:
    // Вспомогательные методы
    void initializeProbabilities();
    double calculateRiskCoefficient() const;
    double calculateUtility(int x, int y) const;
    std::pair<int, int> findBestMove();
    std::pair<int, int> findKillMove();
    void updateProbabilities(int x, int y, bool hitShip, bool hitMine);
    
    // Состояние алгоритма
    std::shared_ptr<GameBoard> board_;
    int maxLives_;
    int currentLives_;
    bool hasLastHit;
    int lastHitX;
    int lastHitY;
    std::vector<std::pair<int, int>> woundedCells_;
}; 