#include "../include/GameBoard.h"
#include <stdexcept>
#include <algorithm>
#include <cmath>

GameBoard::GameBoard(int size)
    : size_(size)
    , remainingShips_(0)               
    , remainingMines_(0)
    , board_(size, std::vector<int>(size, 0))
    , shipProbabilities_(size, std::vector<double>(size, 0.0))
    , mineProbabilities_(size, std::vector<double>(size, 0.0))
    , shotsBoard_(size, std::vector<int>(size, 0))
{
}

bool GameBoard::isValidPosition(int x, int y) const {
    return x >= 0 && x < size_ && y >= 0 && y < size_;
}

bool GameBoard::canPlaceShip(int x, int y, int length, bool horizontal) const {
    if (!isValidPosition(x, y)) return false;
    
    // Проверяем, что корабль помещается на поле
    if (horizontal && x + length > size_) return false;
    if (!horizontal && y + length > size_) return false;
    
    // Проверяем, что вокруг корабля нет других кораблей
    for (int i = -1; i <= length; ++i) {
        for (int j = -1; j <= 1; ++j) {
            int checkX = horizontal ? x + i : x + j;
            int checkY = horizontal ? y + j : y + i;
            
            if (isValidPosition(checkX, checkY) && board_[checkY][checkX] != 0) {
                return false;
            }
        }
    }
    
    return true;
}

bool GameBoard::placeShip(int x, int y, int length, bool horizontal) {
    if (!canPlaceShip(x, y, length, horizontal)) {
        return false;
    }
    Ship newShip;
    for (int i = 0; i < length; ++i) {
        int shipX = horizontal ? x + i : x;
        int shipY = horizontal ? y : y + i;
        board_[shipY][shipX] = 1;
        newShip.cells.push_back({shipX, shipY});
    }
    ships_.push_back(newShip);
    remainingShips_ += length;
    return true;
}

bool GameBoard::canPlaceMine(int x, int y) const {
    if (!isValidPosition(x, y) || board_[y][x] != 0) return false;
    // Проверяем все соседние клетки (включая диагональ)
    for (int dx = -1; dx <= 1; ++dx) {
        for (int dy = -1; dy <= 1; ++dy) {
            int nx = x + dx;
            int ny = y + dy;
            if (isValidPosition(nx, ny) && board_[ny][nx] == 1) {
                return false;
            }
        }
    }
    return true;
}

bool GameBoard::placeMine(int x, int y) {
    if (!canPlaceMine(x, y)) {
        return false;
    }
    
    board_[y][x] = 2;
    remainingMines_++;
    return true;
}

bool GameBoard::makeShot(int x, int y) {
    if (!isValidPosition(x, y)) {
        return false;
    }
    shotsBoard_[y][x] = 1;
    if (board_[y][x] == 1) {
        board_[y][x] = 3;  // Пораженный корабль
        remainingShips_--;
        // Проверяем, потоплен ли корабль
        for (auto& ship : ships_) {
            for (auto [sx, sy] : ship.cells) {
                if (sx == x && sy == y) {
                    if (ship.isSunk(board_)) {
                        markSurroundingCells(ship);
                        // Можно добавить вывод "Потоплен!"
                    } else {
                        // Можно добавить вывод "Ранен!"
                    }
                    break;
                }
            }
        }
        return true;
    } else if (board_[y][x] == 2) {
        board_[y][x] = 4;  // Сработавшая мина
    } else if (board_[y][x] == 0) {
        board_[y][x] = 5;  // Промах
    }
    return false;
}

bool GameBoard::isGameOver() const {
    return remainingShips_ == 0;
}

bool GameBoard::isVictory() const {
    return remainingShips_ == 0;
}

int GameBoard::getRemainingShips() const {
    return remainingShips_;
}

int GameBoard::getSize() const {
    return size_;
}

int GameBoard::getRemainingMines() const {
    return remainingMines_;
}

const std::vector<std::vector<int>>& GameBoard::getBoard() const {
    return board_;
}

const std::vector<std::vector<double>>& GameBoard::getShipProbabilities() const {
    return shipProbabilities_;
}

const std::vector<std::vector<double>>& GameBoard::getMineProbabilities() const {
    return mineProbabilities_;
}

const std::vector<std::vector<int>>& GameBoard::getShotsBoard() const {
    return shotsBoard_;
}

void GameBoard::setInitialShipProbability(double prob) {
    initialShipProb_ = prob;
    for (auto& row : shipProbabilities_) {
        std::fill(row.begin(), row.end(), prob);
    }
}

void GameBoard::setInitialMineProbability(double prob) {
    initialMineProb_ = prob;
    for (auto& row : mineProbabilities_) {
        std::fill(row.begin(), row.end(), prob);
    }
}

void GameBoard::updateProbabilities(int x, int y, bool hitShip, bool hitMine, 
                                  double shipFactor, double mineFactor) {
    // Обновляем вероятности для текущей клетки
    if (hitShip) {
        shipProbabilities_[y][x] = 1.0;
        mineProbabilities_[y][x] = 0.0;
    } else if (hitMine) {
        shipProbabilities_[y][x] = 0.0;
        mineProbabilities_[y][x] = 1.0;
    } else {
        shipProbabilities_[y][x] = 0.0;
        mineProbabilities_[y][x] = 0.0;
    }
    
    // Применяем факторы изменения вероятностей
    if (shipFactor != 0.0 || mineFactor != 0.0) {
        for (int dx = -2; dx <= 2; ++dx) {
            for (int dy = -2; dy <= 2; ++dy) {
                int nx = x + dx;
                int ny = y + dy;
                
                if (isValidPosition(nx, ny)) {
                    double distance = std::sqrt(dx*dx + dy*dy);
                    if (distance <= 2.0) {
                        double factor = std::exp(-distance);
                        
                        if (shipFactor != 0.0) {
                            shipProbabilities_[ny][nx] = std::max(0.0, 
                                std::min(1.0, shipProbabilities_[ny][nx] + shipFactor * factor));
                        }
                        
                        if (mineFactor != 0.0) {
                            mineProbabilities_[ny][nx] = std::max(0.0, 
                                std::min(1.0, mineProbabilities_[ny][nx] + mineFactor * factor));
                        }
                    }
                }
            }
        }
    }
}

void GameBoard::markSurroundingCells(const Ship& ship) {
    for (auto [x, y] : ship.cells) {
        for (int dx = -1; dx <= 1; ++dx) {
            for (int dy = -1; dy <= 1; ++dy) {
                int nx = x + dx;
                int ny = y + dy;
                if (isValidPosition(nx, ny) && board_[ny][nx] == 0) {
                    board_[ny][nx] = 5; // 5 — промах/пустая клетка
                }
            }
        }
    }
}

const std::vector<GameBoard::Ship>& GameBoard::getShips() const {
    return ships_;
}

const std::vector<std::vector<int>>& GameBoard::getBoardInternal() const {
    return board_;
} 