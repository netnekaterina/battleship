#include "../include/GameBoard.h"
#include "../include/BattleshipAlgorithm.h"
#include <iostream>
#include <memory>
#include <cmath>
#include <windows.h>
#include <fcntl.h>
#include <io.h>
#include <clocale>
#include <limits>
#include <string>
#include <iomanip>
#include <map>
#include <vector>

void clearScreen() {
    system("cls");
}

void printBoard(const GameBoard& board, bool showShips = false) {
    const auto& gameBoard = board.getBoard();
    std::cout << "\n  ";
    for (int i = 0; i < board.getSize(); ++i) {
        std::cout << i << " ";
    }
    std::cout << "\n";
    
    for (int i = 0; i < board.getSize(); ++i) {
        std::cout << i << " ";
        for (int j = 0; j < board.getSize(); ++j) {
            if (!showShips && (gameBoard[i][j] == 1 || gameBoard[i][j] == 2)) {
                std::cout << "  "; // Скрываем корабли и мины
            } else if (gameBoard[i][j] == 0) {
                std::cout << "  "; // Пустая клетка всегда — пробел
            } else {
                switch (gameBoard[i][j]) {
                    case 1: std::cout << "X "; break; // Корабль
                    case 2: std::cout << "* "; break; // Мина
                    case 3: std::cout << "X "; break; // Пораженный корабль
                    case 4: std::cout << "! "; break; // Сработавшая мина
                    case 5: std::cout << "o "; break; // Промах
                    default: std::cout << "? "; break;
                }
            }
        }
        std::cout << "\n";
    }
    std::cout << "\n";
}

int calculateShipCount(int size) {
    // По формуле из ReadMe: K = ⌊(20/100) * N²⌋
    return static_cast<int>(std::floor(0.2 * size * size));
}

int calculateMineCount(int size) {
    // По формуле из ReadMe: M = ⌊0.03 * N²⌋
    return static_cast<int>(std::floor(0.03 * size * size));
}

bool getValidInput(int& value, const std::string& prompt, int min, int max) {
    while (true) {
        std::cout << prompt;
        if (std::cin >> value) {
            if (value >= min && value <= max) {
                return true;
            }
            std::cout << "Error: value must be between " << min << " and " << max << "\n";
        } else {
            std::cout << "Error: enter a valid number\n";
            std::cin.clear();
            std::cin.ignore(1000000, '\n');
        }
    }
}

struct ShipType {
    int length;
    int count;
};

std::vector<ShipType> calculateFleet(int size) {
    int area = size * size;
    int totalDecks = static_cast<int>(area * 0.2 + 0.5); // 20% of area, rounded
    std::vector<ShipType> baseFleet = {
        {4, 1}, {3, 2}, {2, 3}
    };
    int usedDecks = 0;
    std::vector<ShipType> fleet;
    int scale = size / 10;
    if (scale < 1) scale = 1;
    // Масштабируем флот
    for (auto ship : baseFleet) {
        int newCount = ship.count * scale;
        int newLength = ship.length * scale;
        if (newLength > size) newLength = size;
        fleet.push_back({newLength, newCount});
        usedDecks += newCount * newLength;
    }
    // Остаток — однопалубные
    int singleShips = totalDecks - usedDecks;
    if (singleShips > 0) {
        fleet.push_back({1, singleShips});
    }
    return fleet;
}

bool placeShips(GameBoard& board) {
    int size = board.getSize();
    auto fleet = calculateFleet(size);
    std::map<int, int> placed; // length -> count
    for (const auto& ship : fleet) placed[ship.length] = 0;
    
    std::cout << "\nPlacing ships (total area: " << static_cast<int>(size*size*0.2+0.5) << "):\n";
    for (const auto& ship : fleet) {
        std::cout << "Ships of length " << ship.length << ": " << ship.count << "\n";
    }
    
    for (const auto& ship : fleet) {
        for (int i = 0; i < ship.count; ++i) {
            clearScreen();
            std::cout << "\nCurrent field:\n";
            printBoard(board, true);
            for (const auto& s : fleet) {
                std::cout << "Ships of length " << s.length << ": " << placed[s.length] << " of " << s.count << " placed\n";
            }
            std::cout << "\nPlacing ship of length " << ship.length << " (" << (i+1) << " of " << ship.count << ")\n";
            int x, y;
            char direction = 'h';
            if (!getValidInput(x, "Enter X coordinate (0-" + std::to_string(size-1) + "): ", 0, size-1)) continue;
            if (!getValidInput(y, "Enter Y coordinate (0-" + std::to_string(size-1) + "): ", 0, size-1)) continue;
            bool placedOk = false;
            if (ship.length == 1) {
                placedOk = board.placeShip(x, y, 1, true);
            } else {
                std::cout << "Enter direction (h - horizontally, v - vertically): ";
                std::cin >> direction;
                direction = std::tolower(direction);
                if (direction != 'h' && direction != 'v') {
                    std::cout << "Error: direction must be 'h' or 'v'\n";
                    --i; // retry
                    std::cout << "Press Enter to continue...";
                    std::cin.ignore(1000000, '\n');
                    std::cin.get();
                    continue;
                }
                placedOk = board.placeShip(x, y, ship.length, direction == 'h');
            }
            if (placedOk) {
                placed[ship.length]++;
                std::cout << "Ship placed!\n";
            } else {
                std::cout << "Failed to place ship. Check coordinates and ensure ships do not touch.\n";
                --i; // retry
            }
            std::cout << "Press Enter to continue...";
            std::cin.ignore(1000000, '\n');
            std::cin.get();
        }
    }
    return true;
}

bool placeMines(GameBoard& board) {
    int size = board.getSize();
    int totalMines = calculateMineCount(size);
    int minesPlaced = 0;
    
    std::cout << "\nPlacing mines:\n";
    std::cout << "Total mines to place: " << totalMines << "\n";
    
    while (minesPlaced < totalMines) {
        clearScreen();
        std::cout << "\nCurrent field:\n";
        printBoard(board, true);
        std::cout << "\nMines placed: " << minesPlaced << " of " << totalMines << "\n";
        
        int x, y;
        if (!getValidInput(x, "Enter X coordinate (0-" + std::to_string(size-1) + "): ", 0, size-1)) continue;
        if (!getValidInput(y, "Enter Y coordinate (0-" + std::to_string(size-1) + "): ", 0, size-1)) continue;
        
        if (board.placeMine(x, y)) {
            minesPlaced++;
            std::cout << "Mine placed!\n";
        } else {
            std::cout << "Failed to place mine. Check coordinates and ensure the cell is free.\n";
        }
        
        std::cout << "Press Enter to continue...";
        std::cin.ignore(1000000, '\n');
        std::cin.get();
    }
    
    return true;
}

int main() {
    // Locale and encoding settings are not needed for English output
    
    try {
        int size;
        if (!getValidInput(size, "Enter board size (>=10): ", 10, 100)) {
            throw std::invalid_argument("Invalid board size");
        }
        
        // Инициализация игры
        auto board = std::make_shared<GameBoard>(size);
        
        // Размещение кораблей и мин
        if (!placeShips(*board)) {
            throw std::runtime_error("Error placing ships");
        }
        
        if (!placeMines(*board)) {
            throw std::runtime_error("Error placing mines");
        }
        
        // Инициализация алгоритма
        int maxLives = calculateMineCount(size);
        BattleshipAlgorithm algorithm(board, maxLives);
        
        // Игровой цикл
        int moves = 0;
        bool gameOver = false;
        
        clearScreen();
        std::cout << "\nStart game!\n";
        std::cout << "Board size: " << size << "x" << size << "\n";
        std::cout << "Lives: " << maxLives << "\n\n";
        
        while (!gameOver) {
            moves++;
            std::cout << "\nMove " << moves << ":\n";
            
            bool hit = algorithm.makeMove();
            if (hit) {
                std::cout << "Hit!\n";
            } else {
                std::cout << "Miss!\n";
            }
            
            std::cout << "Lives left: " << algorithm.getCurrentLives() << "\n";
            std::cout << "Remaining ships: " << board->getRemainingShips() << "\n";
            
            printBoard(*board);
            
            if (board->isVictory()) {
                std::cout << "\nVictory! All ships destroyed!\n";
                gameOver = true;
            } else if (algorithm.getCurrentLives() <= 0) {
                std::cout << "\nGame over! No more lives!\n";
                gameOver = true;
            }
            
            std::cout << "Press Enter for next move...";
            std::cin.ignore(1000000, '\n');
            std::cin.get();
            clearScreen();
        }
        
        std::cout << "\nGame results:\n";
        std::cout << "Total moves: " << moves << "\n";
        std::cout << "Lives left: " << algorithm.getCurrentLives() << "\n";
        std::cout << "Remaining ships: " << board->getRemainingShips() << "\n";
        
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
    
    std::cout << "\nPress Enter to exit...";
    std::cin.ignore(1000000, '\n');
    std::cin.get();
    return 0;
} 