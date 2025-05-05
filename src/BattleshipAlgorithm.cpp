#include "../include/BattleshipAlgorithm.h"
#include "../include/GameBoard.h"
#include <limits>
#include <cmath>
#include <random>
#include <algorithm>
#include <queue>
#include <set>

BattleshipAlgorithm::BattleshipAlgorithm(std::shared_ptr<GameBoard> board, int maxLives)
    : board_(board), maxLives_(maxLives), currentLives_(maxLives), hasLastHit(false) {
    initializeProbabilities();
}

void BattleshipAlgorithm::initializeProbabilities() {
    int size = board_->getSize();
    int totalShips = static_cast<int>(std::floor(0.2 * size * size));
    int totalMines = static_cast<int>(std::floor(0.03 * size * size));
    
    double initialShipProb = static_cast<double>(totalShips) / (size * size);
    double initialMineProb = static_cast<double>(totalMines) / (size * size);
    
    board_->setInitialShipProbability(initialShipProb);
    board_->setInitialMineProbability(initialMineProb);
}

double BattleshipAlgorithm::calculateRiskCoefficient() const {
    const double lambdaMax = 2.0;
    double lifeRatio = static_cast<double>(currentLives_) / maxLives_;
    
    // Экспоненциальная функция для более агрессивного изменения lambda
    return lambdaMax * std::exp(-3.0 * lifeRatio);
}

double BattleshipAlgorithm::calculateUtility(int x, int y) const {
    const auto& shipProbs = board_->getShipProbabilities();
    const auto& mineProbs = board_->getMineProbabilities();
    
    double shipProb = shipProbs[y][x];
    double mineProb = mineProbs[y][x];
    double lambda = calculateRiskCoefficient();
    
    // Добавляем штраф за клетки рядом с уже проверенными
    double neighborPenalty = 0.0;
    for (int dx = -1; dx <= 1; ++dx) {
        for (int dy = -1; dy <= 1; ++dy) {
            int nx = x + dx;
            int ny = y + dy;
            if (nx >= 0 && nx < board_->getSize() && ny >= 0 && ny < board_->getSize()) {
                if (board_->getBoard()[ny][nx] != 0) {
                    neighborPenalty += 0.1;
                }
            }
        }
    }
    
    return shipProb - lambda * mineProb - neighborPenalty;
}

std::pair<int, int> BattleshipAlgorithm::findKillMove() {
    double bestUtility = -std::numeric_limits<double>::infinity();
    std::pair<int, int> bestMove = {-1, -1};
    int size = board_->getSize();
    if (woundedCells_.size() > 1) {
        // Определяем ориентацию корабля
        bool isVertical = true, isHorizontal = true;
        int x0 = woundedCells_[0].first, y0 = woundedCells_[0].second;
        for (const auto& cell : woundedCells_) {
            if (cell.first != x0) isVertical = false;
            if (cell.second != y0) isHorizontal = false;
        }
        if (isVertical) {
            // Добиваем только вверх и вниз
            std::vector<int> ys;
            for (const auto& cell : woundedCells_) ys.push_back(cell.second);
            int minY = *std::min_element(ys.begin(), ys.end());
            int maxY = *std::max_element(ys.begin(), ys.end());
            int x = woundedCells_[0].first;
            if (minY - 1 >= 0 && board_->getShotsBoard()[minY - 1][x] == 0) {
                double utility = calculateUtility(x, minY - 1);
                if (utility > bestUtility) {
                    bestUtility = utility;
                    bestMove = {x, minY - 1};
                }
            }
            if (maxY + 1 < size && board_->getShotsBoard()[maxY + 1][x] == 0) {
                double utility = calculateUtility(x, maxY + 1);
                if (utility > bestUtility) {
                    bestUtility = utility;
                    bestMove = {x, maxY + 1};
                }
            }
        } else if (isHorizontal) {
            // Добиваем только влево и вправо
            std::vector<int> xs;
            for (const auto& cell : woundedCells_) xs.push_back(cell.first);
            int minX = *std::min_element(xs.begin(), xs.end());
            int maxX = *std::max_element(xs.begin(), xs.end());
            int y = woundedCells_[0].second;
            if (minX - 1 >= 0 && board_->getShotsBoard()[y][minX - 1] == 0) {
                double utility = calculateUtility(minX - 1, y);
                if (utility > bestUtility) {
                    bestUtility = utility;
                    bestMove = {minX - 1, y};
                }
            }
            if (maxX + 1 < size && board_->getShotsBoard()[y][maxX + 1] == 0) {
                double utility = calculateUtility(maxX + 1, y);
                if (utility > bestUtility) {
                    bestUtility = utility;
                    bestMove = {maxX + 1, y};
                }
            }
        } else {
            // Если не определена ориентация, fallback на старое поведение
            for (const auto& cell : woundedCells_) {
                int x = cell.first;
                int y = cell.second;
                std::vector<std::pair<int, int>> directions = {
                    {-1, 0}, {1, 0}, {0, -1}, {0, 1}
                };
                for (const auto& dir : directions) {
                    int nx = x + dir.first;
                    int ny = y + dir.second;
                    if (nx >= 0 && nx < size && ny >= 0 && ny < size) {
                        if (board_->getShotsBoard()[ny][nx] == 0) {
                            double utility = calculateUtility(nx, ny);
                            if (utility > bestUtility) {
                                bestUtility = utility;
                                bestMove = {nx, ny};
                            }
                        }
                    }
                }
            }
        }
    } else {
        // Одна раненая клетка — старое поведение
        for (const auto& cell : woundedCells_) {
            int x = cell.first;
            int y = cell.second;
            std::vector<std::pair<int, int>> directions = {
                {-1, 0}, {1, 0}, {0, -1}, {0, 1}
            };
            for (const auto& dir : directions) {
                int nx = x + dir.first;
                int ny = y + dir.second;
                if (nx >= 0 && nx < size && ny >= 0 && ny < size) {
                    if (board_->getShotsBoard()[ny][nx] == 0) {
                        double utility = calculateUtility(nx, ny);
                        if (utility > bestUtility) {
                            bestUtility = utility;
                            bestMove = {nx, ny};
                        }
                    }
                }
            }
        }
    }
    return bestMove;
}

std::vector<std::pair<int, int>> getSquarePattern(int n, int x0, int y0, int size) {
    std::vector<std::pair<int, int>> pattern;
    if (n == 4) {
        // Диагональ и побочные диагонали (пример шаблона)
        for (int i = 0; i < n; ++i) {
            if (x0 + i < size && y0 + i < size)
                pattern.emplace_back(x0 + i, y0 + i);
            if (x0 + n - 1 - i < size && y0 + i < size)
                pattern.emplace_back(x0 + n - 1 - i, y0 + i);
        }
    } else if (n == 3) {
        // Главная диагональ
        for (int i = 0; i < n; ++i) {
            if (x0 + i < size && y0 + i < size)
                pattern.emplace_back(x0 + i, y0 + i);
        }
    }
    return pattern;
}

int getMaxAliveShipLength(const GameBoard& board) {
    int maxLen = 0;
    for (const auto& ship : board.getShips()) {
        if (!ship.isSunk(board.getBoardInternal())) {
            int len = (int)ship.cells.size();
            if (len > maxLen) maxLen = len;
        }
    }
    return maxLen;
}

// Возвращает список центров всех возможных позиций для самого длинного корабля
std::vector<std::pair<int, int>> findMaxShipCandidates(const GameBoard& board, int maxShipLen) {
    std::vector<std::pair<int, int>> candidates;
    int size = board.getSize();
    const auto& shots = board.getShotsBoard();
    // По горизонтали
    for (int y = 0; y < size; ++y) {
        int streak = 0;
        for (int x = 0; x < size; ++x) {
            if (shots[y][x] == 0) streak++;
            else streak = 0;
            if (streak >= maxShipLen) {
                int center = x - maxShipLen / 2;
                if (center >= x - streak + 1 && center <= x) // центр в пределах streak
                    candidates.emplace_back(center, y);
            }
        }
    }
    // По вертикали
    for (int x = 0; x < size; ++x) {
        int streak = 0;
        for (int y = 0; y < size; ++y) {
            if (shots[y][x] == 0) streak++;
            else streak = 0;
            if (streak >= maxShipLen) {
                int center = y - maxShipLen / 2;
                if (center >= y - streak + 1 && center <= y)
                    candidates.emplace_back(x, center);
            }
        }
    }
    return candidates;
}

std::pair<int, int> BattleshipAlgorithm::findBestMove() {
    double bestUtility = -std::numeric_limits<double>::infinity();
    std::pair<int, int> bestMove = {-1, -1};
    // Если есть раненые клетки, используем режим добивания
    if (!woundedCells_.empty()) {
        auto move = findKillMove();
        if (move.first != -1) return move;
    }
    int size = board_->getSize();
    int n = getMaxAliveShipLength(*board_);
    const auto& mineProbs = board_->getMineProbabilities();
    // Если было попадание, сначала проверяем соседние клетки
    if (hasLastHit) {
        std::vector<std::pair<int, int>> directions = {
            {-1, 0}, {1, 0}, {0, -1}, {0, 1}
        };
        for (const auto& dir : directions) {
            int nx = lastHitX + dir.first;
            int ny = lastHitY + dir.second;
            if (nx >= 0 && nx < board_->getSize() && ny >= 0 && ny < board_->getSize()) {
                if (board_->getShotsBoard()[ny][nx] == 0) {
                    double utility = calculateUtility(nx, ny);
                    if (utility > bestUtility) {
                        bestUtility = utility;
                        bestMove = {nx, ny};
                    }
                }
            }
        }
    }
    // Новый шаг: ищем все возможные позиции для самого длинного корабля, приоритет — безопасность
    if (bestMove.first == -1) {
        auto candidates = findMaxShipCandidates(*board_, n);
        double minMineSum = 1e9;
        std::pair<int, int> safestCell = {-1, -1};
        for (auto [x, y] : candidates) {
            double mineSumH = 0.0, mineSumV = 0.0;
            bool validH = true, validV = true;
            // горизонталь
            for (int d = 0; d < n; ++d) {
                if (x + d >= size || board_->getShotsBoard()[y][x + d] != 0) { validH = false; break; }
                mineSumH += mineProbs[y][x + d];
            }
            // вертикаль
            for (int d = 0; d < n; ++d) {
                if (y + d >= size || board_->getShotsBoard()[y + d][x] != 0) { validV = false; break; }
                mineSumV += mineProbs[y + d][x];
            }
            if (validH && mineSumH < minMineSum) {
                minMineSum = mineSumH;
                safestCell = {x + n/2, y};
            }
            if (validV && mineSumV < minMineSum) {
                minMineSum = mineSumV;
                safestCell = {x, y + n/2};
            }
        }
        // Если мало жизней, избегаем дыр с высокой вероятностью мин
        double mineThreshold = 0.5 * n; // можно скорректировать
        if (currentLives_ <= 2 && minMineSum > mineThreshold) {
            // fallback — ищем менее опасные клетки по шаблону
        } else if (safestCell.first != -1) {
            bestMove = safestCell;
        }
    }
    // Если не нашли — шаблон квадратов
    if (bestMove.first == -1) {
        std::set<std::pair<int, int>> patternCells;
        for (int y0 = 0; y0 < size; y0 += n) {
            for (int x0 = 0; x0 < size; x0 += n) {
                auto pattern = getSquarePattern(n, x0, y0, size);
                for (auto cell : pattern) patternCells.insert(cell);
            }
        }
        for (auto [x, y] : patternCells) {
            if (board_->getShotsBoard()[y][x] == 0) {
                double utility = calculateUtility(x, y);
                if (utility > bestUtility) {
                    bestUtility = utility;
                    bestMove = {x, y};
                }
            }
        }
    }
    // Если все клетки паттерна уже прострелены, fallback — по всему полю
    if (bestMove.first == -1) {
        for (int i = 0; i < board_->getSize(); ++i) {
            for (int j = 0; j < board_->getSize(); ++j) {
                if (board_->getShotsBoard()[i][j] == 0) {
                    double utility = calculateUtility(j, i);
                    if (utility > bestUtility) {
                        bestUtility = utility;
                        bestMove = {j, i};
                    }
                }
            }
        }
    }
    return bestMove;
}

bool BattleshipAlgorithm::makeMove() {
    if (currentLives_ <= 0) return false;
    
    auto [x, y] = findBestMove();
    if (x == -1 || y == -1) return false;  // Нет доступных ходов
    
    bool hit = board_->makeShot(x, y);
    
    if (hit) {
        hasLastHit = true;
        lastHitX = x;
        lastHitY = y;
        // Добавляем в список раненых, если ещё не потоплен
        bool alreadyWounded = false;
        for (const auto& cell : woundedCells_) {
            if (cell.first == x && cell.second == y) {
                alreadyWounded = true;
                break;
            }
        }
        if (!alreadyWounded) {
            woundedCells_.push_back({x, y});
        }
        updateProbabilities(x, y, true, false);
        // Проверяем, не потоплен ли корабль (все клетки вокруг раненых)
        for (const auto& ship : board_->getShips()) {
            bool allHit = true;
            for (const auto& cell : ship.cells) {
                if (board_->getBoardInternal()[cell.second][cell.first] != 3) {
                    allHit = false;
                    break;
                }
            }
            if (allHit) {
                // Удаляем все клетки этого корабля из woundedCells_
                for (const auto& cell : ship.cells) {
                    woundedCells_.erase(std::remove(woundedCells_.begin(), woundedCells_.end(), cell), woundedCells_.end());
                }
            }
        }
    } else if (board_->getBoard()[y][x] == 4) {  // Попали в мину
        currentLives_--;
        updateProbabilities(x, y, false, true);
    } else {
        updateProbabilities(x, y, false, false);
    }
    
    return hit;
}

void BattleshipAlgorithm::updateProbabilities(int x, int y, bool hitShip, bool hitMine) {
    int size = board_->getSize();
    
    // Обновляем вероятности для текущей клетки
    board_->updateProbabilities(x, y, hitShip, hitMine);
    
    // Обновляем вероятности для соседних клеток
    for (int dx = -2; dx <= 2; ++dx) {
        for (int dy = -2; dy <= 2; ++dy) {
            int nx = x + dx;
            int ny = y + dy;
            if (nx >= 0 && nx < size && ny >= 0 && ny < size) {
                double distance = std::sqrt(dx*dx + dy*dy);
                if (distance <= 2.0) {  // Влияние на клетки в радиусе 2
                    double factor = std::exp(-distance);
                    if (hitShip) {
                        // Увеличиваем вероятность корабля по горизонтали/вертикали вокруг попадания
                        if ((dx == 0 && dy != 0) || (dx != 0 && dy == 0)) {
                            board_->updateProbabilities(nx, ny, false, false, factor * 0.7);
                        }
                    }
                    if (hitMine) {
                        // Увеличиваем вероятность мины вокруг попадания
                        board_->updateProbabilities(nx, ny, false, false, 0.0, factor * 0.3);
                    }
                }
            }
        }
    }
}

int BattleshipAlgorithm::getCurrentLives() const {
    return currentLives_;
} 