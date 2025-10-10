#pragma once

#include <iostream>
#include <set>
#include <functional>
#include "MinigridGrammar.h"
#include "PrismPrinter.h"
#include "ConfigYaml.h"


std::string northUpdate(const AgentName &a);
std::string southUpdate(const AgentName &a);
std::string eastUpdate(const AgentName &a);
std::string westUpdate(const AgentName &a);

namespace prism {
  class PrismModulesPrinter {
    public:
      PrismModulesPrinter(std::ostream& os, const ModelType &modelType, const coordinates &maxBoundaries, const cells &lockedDoors, const cells &unlockedDoors, const cells &keys, const std::map<std::string, cells> &slipperyTiles, const AgentNameAndPositionMap &agentNameAndPositionMap, std::vector<Configuration> config, const float probIntended, const float faultyProbability, const bool anyLava, const bool anyGoals);

      std::ostream& print();

      void printModelType(const ModelType &modelType);


      bool isGame() const;
    private:
      void printPortableObjectModule(const cell &object);
      void printPortableObjectActions(const std::string &agentName, const std::string &identifier, const bool canBeDroped = false);

      void printDoorModule(const cell &object, const bool &opened);
      void printLockedDoorActions(const std::string &agentName, const std::string &identifier);
      void printUnlockedDoorActions(const std::string &agentName, const std::string &identifier);

      void printRobotModule(const AgentName &agentName, const coordinates &initialPosition);
      void printPortableObjectActionsForRobot(const std::string &agentName, const std::string &identifier, const bool canBeDroped = false);
      void printUnlockedDoorActionsForRobot(const std::string &agentName, const std::string &identifier);
      void printLockedDoorActionsForRobot(const std::string &agentName, const std::string &identifier, const std::string &key);
      void printMovementActionsForRobot(const std::string &a);
      void printTurnActionsForRobot(const std::string &a);
      void printNonMovementActionsForRobot(const AgentName &agentName);
      void printSlipperyMovementActionsForRobot(const AgentName &a);
      void printSlipperyMovementActionsForNorth(const AgentName &a);
      void printSlipperyMovementActionsForEast(const AgentName &a);
      void printSlipperyMovementActionsForSouth(const AgentName &a);
      void printSlipperyMovementActionsForWest(const AgentName &a);
      void printSlipperyTurnActionsForNorth(const AgentName &a);
      void printSlipperyTurnActionsForEast(const AgentName &a);
      void printSlipperyTurnActionsForSouth(const AgentName &a);
      void printSlipperyTurnActionsForWest(const AgentName &a);
      void printSlipperyMovementActionsForNorthWest(const AgentName &a);
      void printSlipperyTurnActionsForNorthWest(const AgentName &a);
      void printSlipperyMovementActionsForNorthEast(const AgentName &a);
      void printSlipperyTurnActionsForNorthEast(const AgentName &a);
      void printSlipperyMovementActionsForSouthWest(const AgentName &a);
      void printSlipperyTurnActionsForSouthWest(const AgentName &a);
      void printSlipperyMovementActionsForSouthEast(const AgentName &a);
      void printSlipperyTurnActionsForSouthEast(const AgentName &a);

      std::string printMovementGuard(const AgentName &a, const std::string &direction, const size_t &viewDirection);
      std::string printMovementUpdate(const AgentName &a, const update &update) const;
      std::string printTurnGuard(const AgentName &a, const std::string &direction, const ActionId &actionId, const std::string &cond = "");
      std::string printTurnUpdate(const AgentName &a, const update &u, const ActionId &actionId) const;
      std::string printSlipperyMovementGuard(const AgentName &a, const std::string &direction, const ViewDirection &viewDirection, const std::vector<std::string> &guards);
      std::string printSlipperyMovementUpdate(const AgentName &a, const std::string &direction, const updates &u) const;
      std::string printSlipperyTurnGuard(const AgentName &a, const std::string &direction, const std::string &tiltDirection, const ActionId &actionId, const std::vector<std::string> &guards, const std::string &cond);
      std::string printSlipperyTurnUpdate(const AgentName &a, const updates &u);

      void printFaultyMovementModule(const AgentName &a);
      void printMoveModule();

      void printConstants(const std::vector<std::string> &constants);

      void printDoneActions(const AgentName &agentName);
      void printPlayerStruct(const AgentName &agentName);
      void printRewards(const AgentName &agentName, const std::map<coordinates, float> &stateRewards, const cells &lava, const cells &goals, const std::map<Color, cells> &backgroundTiles);

      void printConfiguration(const std::vector<Configuration>& configurations);
      void printConfiguredActions(const AgentName &agentName);

      bool anyPortableObject() const;
      bool faultyBehaviour() const;
      bool slipperyBehaviour() const;
      std::string moveGuard(const AgentName &agentName) const;
      std::string faultyBehaviourGuard(const AgentName &agentName, const ActionId &actionId) const;
      std::string faultyBehaviourUpdate(const AgentName &agentName, const ActionId &actionId) const;
      std::string moveUpdate(const AgentName &agentName) const;
      std::string updatesToString(const updates &updates) const;
      std::string updateToString(const update &u) const;

      std::string viewVariable(const AgentName &agentName, const size_t &agentDirection) const;


      std::string buildConjunction(const AgentName &a, std::vector<std::string> formulae) const;


      std::ostream &os;
      std::stringstream actionStream;

      ModelType const &modelType;
      coordinates const &maxBoundaries;
      AgentName agentName;
      cells lockedDoors;
      cells unlockedDoors;
      cells keys;
      std::map<std::string, cells> slipperyTiles;

      const bool anyLava;
      const bool anyGoals;

      AgentNameAndPositionMap agentNameAndPositionMap;
      std::map<AgentName, size_t> agentIndexMap;
      size_t numberOfPlayer;
      float const faultyProbability;
      float const probIntended;
      std::vector<Configuration> configuration;
      std::vector<ViewDirection> viewDirections = {0, 1, 2, 3};
      std::map<ViewDirection, std::string> viewDirectionToString = {{0, "East"}, {1, "South"}, {2, "West"}, {3, "North"}};
      std::vector<std::pair<size_t, std::string>> nonMovementActions = { {PICKUP, "pickup"}, {DROP, "drop"}, {TOGGLE, "toggle"}, {DONE, "done"} };

      std::map<AgentName, std::set<std::pair<ActionId, std::string>>> agentNameActionMap;
  };
}
