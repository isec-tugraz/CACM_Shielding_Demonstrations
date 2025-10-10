#pragma once

#include <iostream>
#include <functional>
#include "MinigridGrammar.h"
#include "PrismPrinter.h"
#include "ConfigYaml.h"


std::string oneOffToString(const int &offset);
std::string vectorToDisjunction(const std::vector<std::string> &formulae);
std::string cellToConjunction(const AgentName &agentName, const cell &c);
std::string cellToConjunctionWithOffset(const AgentName &agentName, const cell &c, const std::string &xOffset, const std::string &yOffset);
std::string coordinatesToConjunction(const AgentName &agentName, const coordinates &c, const ViewDirection viewDirection);
std::string objectPositionToConjunction(const AgentName &agentName, const std::string &identifier, const std::pair<int, int> &relativePosition);
std::string objectPositionToConjunction(const AgentName &agentName, const std::string &identifier, const std::pair<int, int> &relativePosition, const ViewDirection viewDirection);
std::map<ViewDirection, coordinates> getAdjacentCells(const cell &c);
std::map<ViewDirection, std::pair<int, int>> getRelativeAdjacentCells();
std::map<std::string, std::pair<int, int>> getRelativeSurroundingCells();

namespace prism {
  class PrismFormulaPrinter {
    public:
      PrismFormulaPrinter(std::ostream &os, const std::map<std::string, cells> &restrictions, const cells &walls, const cells &lockedDoors, const cells &unlockedDoors, const cells &keys, const std::map<std::string, cells> &slipperyTiles, const cells &lava, const cells &goals, const AgentNameAndPositionMap &agentNameAndPositionMap, const bool faulty);

      void print(const AgentName &agentName);

      void printRestrictionFormula(const AgentName &agentName, const std::string &direction, const cells &grid_cells);
      void printIsOnFormula(const AgentName &agentName, const std::string &type, const cells &grid_cells, const std::string &direction = "");
      void printIsNextToFormula(const AgentName &agentName, const std::string &type, const std::map<ViewDirection, coordinates> &coordinates);
      void printRestrictionFormulaWithCondition(const AgentName &agentName, const std::string &reason, const std::map<ViewDirection, coordinates> &coordinates, const std::string &condition);
      void printRelativeIsInFrontOfFormulaWithCondition(const AgentName &agentName, const std::string &reason, const std::string &condition);
      void printSlipRestrictionFormula(const AgentName &agentName, const std::string &direction);

      void printCollisionFormula(const std::string &agentName);
      void printCollisionLabel();

      void printInitStruct();
    private:
      std::string buildFormula(const std::string &formulaName, const std::string &formula, const bool semicolon = true);
      std::string buildLabel(const std::string &labelName, const std::string &label);
      std::string buildDisjunction(const AgentName &agentName, const std::map<ViewDirection, coordinates> &cells);
      std::string buildDisjunction(const AgentName &agentName, const cells &cells);
      std::string buildDisjunction(const AgentName &agentName, const std::string &reason);
      std::string buildDisjunction(const AgentName &agentName, const cells &cells, const std::pair<int, int> &offset);

      bool slipperyBehaviour() const;
      bool anyPortableObject() const;


      std::ostream &os;
      std::map<std::string, cells> restrictions;
      cells walls;
      cells lockedDoors;
      cells unlockedDoors;
      cells keys;
      std::map<std::string, cells> slipperyTiles;
      cells lava;
      cells goals;

      AgentNameAndPositionMap agentNameAndPositionMap;

      std::vector<std::string> conditionalMovementRestrictions;
      std::vector<std::string> portableObjects;

      bool faulty;
  };
}
