#pragma once

#include <iostream>
#include <fstream>
#include <map>
#include <utility>

#include "MinigridGrammar.h"
#include "PrismPrinter.h"
#include "PrismModulesPrinter.h"
#include "PrismFormulaPrinter.h"
#include "ConfigYaml.h"

class Grid {
  public:
    Grid(cells gridCells, cells background, const std::map<coordinates, float> &stateRewards = {}, const float probIntended = 1.0, const float faultyProbability = 0);

    cells getGridCells();

    bool isBlocked(coordinates p);
    bool isWall(coordinates p);
    void printToPrism(std::ostream &os, std::vector<Configuration>& configuration);
    void applyOverwrites(std::string& str, std::vector<Configuration>& configuration);

    void setModelType(prism::ModelType type);

    std::array<bool, 8> getWalkableDirOf8Neighborhood(cell c);

    friend std::ostream& operator<<(std::ostream& os, const Grid &grid);

  private:
    cells allGridCells;
    cells background;
    coordinates maxBoundaries;

    prism::ModelType modelType;

    cell agent;
    cells adversaries;
    AgentNameAndPositionMap agentNameAndPositionMap;
    KeyNameAndPositionMap keyNameAndPositionMap;

    cells walls;
    cells floor;
    cells slipperyNorth;
    cells slipperyEast;
    cells slipperySouth;
    cells slipperyWest;
    cells slipperyNorthWest;
    cells slipperyNorthEast;
    cells slipperySouthWest;
    cells slipperySouthEast;
    cells lockedDoors;
    cells unlockedDoors;
    cells boxes;
    cells balls;
    cells lava;

    cells goals;
    cells keys;

    std::map<Color, cells> backgroundTiles;

    std::map<coordinates, float> stateRewards;
    const float probIntended;
    const float faultyProbability;
};
