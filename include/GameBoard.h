#pragma once

#include <vector>
#include <random>
#include <memory>

class GameBoard {
public:
    struct Ship {
        std::vector<std::pair<int, int>> cells;
        bool isSunk(const std::vector<std::vector<int>>& board) const {
            for (auto [x, y] : cells) {
                if (board[y][x] != 3) // 3 — поражённая клетка корабля
                    return false;
            }
            return true;
        }
    };
    // Конструктор
    explicit GameBoard(int size);
    
    // Основные методы
    bool placeShip(int x, int y, int length, bool horizontal);
    bool placeMine(int x, int y);
    bool makeShot(int x, int y);
    void markSurroundingCells(const Ship& ship);
    
    // Проверки
    bool isValidPosition(int x, int y) const;
    bool canPlaceShip(int x, int y, int length, bool horizontal) const;
    bool canPlaceMine(int x, int y) const;
    bool isGameOver() const;
    bool isVictory() const;
    
    // Геттеры
    int getSize() const;
    int getRemainingShips() const;
    int getRemainingMines() const;
    const std::vector<std::vector<int>>& getBoard() const;
    const std::vector<std::vector<double>>& getShipProbabilities() const;
    const std::vector<std::vector<double>>& getMineProbabilities() const;
    const std::vector<std::vector<int>>& getShotsBoard() const;
    const std::vector<Ship>& getShips() const;
    const std::vector<std::vector<int>>& getBoardInternal() const;
    
    // Управление вероятностями
    void setInitialShipProbability(double prob);
    void setInitialMineProbability(double prob);
    void updateProbabilities(int x, int y, bool hitShip, bool hitMine, 
                           double shipFactor = 0.0, double mineFactor = 0.0);
    
private:
    // Размеры и состояние
    int size_;
    int remainingShips_;
    int remainingMines_;
    
    // Игровое поле и вероятности
    std::vector<std::vector<int>> board_;
    std::vector<std::vector<double>> shipProbabilities_;
    std::vector<std::vector<double>> mineProbabilities_;
    
    // Начальные вероятности
    double initialShipProb_ = 0.0;
    double initialMineProb_ = 0.0;

    std::vector<Ship> ships_;

    std::vector<std::vector<int>> shotsBoard_;

    friend class BattleshipAlgorithm;
}; 