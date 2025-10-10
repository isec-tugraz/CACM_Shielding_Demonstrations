#include "PrismModulesPrinter.h"

#include <map>
#include <string>
#include <stdexcept>


std::string northUpdate(const AgentName &a) { return "(row"+a+"'=row"+a+"-1)"; }
std::string southUpdate(const AgentName &a) { return "(row"+a+"'=row"+a+"+1)"; }
std::string eastUpdate(const AgentName &a)  { return "(col"+a+"'=col"+a+"+1)"; }
std::string westUpdate(const AgentName &a)  { return "(col"+a+"'=col"+a+"-1)"; }

namespace prism {

  PrismModulesPrinter::PrismModulesPrinter(std::ostream& os, const ModelType &modelType, const coordinates &maxBoundaries, const cells &lockedDoors, const cells &unlockedDoors, const cells &keys, const std::map<std::string, cells> &slipperyTiles, const AgentNameAndPositionMap &agentNameAndPositionMap, std::vector<Configuration> config, const float probIntended, const float faultyProbability, const bool anyLava, const bool anyGoals)
    : os(os), modelType(modelType), maxBoundaries(maxBoundaries), lockedDoors(lockedDoors), unlockedDoors(unlockedDoors), keys(keys), slipperyTiles(slipperyTiles), agentNameAndPositionMap(agentNameAndPositionMap), configuration(config), probIntended(probIntended), faultyProbability(faultyProbability), anyLava(anyLava), anyGoals(anyGoals) {
      numberOfPlayer = agentNameAndPositionMap.size();
      size_t index = 0;
      for(auto begin = agentNameAndPositionMap.begin(); begin != agentNameAndPositionMap.end(); begin++, index++) {
        agentIndexMap[begin->first] = index;
      }
  }

  void PrismModulesPrinter::printModelType(const ModelType &modelType) {
    switch(modelType) {
      case(ModelType::MDP):
        os << "mdp";
        break;
      case(ModelType::SMG):
        os << "smg";
        break;
    }
    os << "\n\n";
  }

  std::ostream& PrismModulesPrinter::print() {
    for(const auto [agentName, initialPosition] : agentNameAndPositionMap)  {
      agentNameActionMap[agentName] = {};
    }

    for(const auto &key : keys) {
      printPortableObjectModule(key);
    }
    for(const auto &door : unlockedDoors) {
      printDoorModule(door, true);
    }
    for(const auto &door : lockedDoors) {
      printDoorModule(door, false);
    }

    for(const auto [agentName, initialPosition] : agentNameAndPositionMap)  {
      printRobotModule(agentName, initialPosition);
    }

    if(agentNameAndPositionMap.size() > 1) {
      printMoveModule();
    }

    if(faultyBehaviour()) {
      for(const auto [agentName, initialPosition] : agentNameAndPositionMap)  {
        printFaultyMovementModule(agentName);
      }
    }

    if(agentNameAndPositionMap.size() > 1) {
      for(const auto [agentName, index] : agentIndexMap) {
        printPlayerStruct(agentName);
      }
    }

    if (!configuration.empty()) {
      printConfiguration(configuration);
    }

    return os;
  }


  void PrismModulesPrinter::printConfiguration(const std::vector<Configuration>& configurations) {
    for (auto& configuration : configurations) {
      if (configuration.overwrite_ || configuration.type_ == ConfigType::Module) {
        continue;
      }
      os << configuration.expression_ << std::endl;
    }
  }

  void PrismModulesPrinter::printConstants(const std::vector<std::string> &constants) {
    for (auto& constant : constants) {
      os << constant << std::endl;
    }
  }

  void PrismModulesPrinter::printPortableObjectModule(const cell &object) {
    std::string identifier = capitalize(object.getColor()) + object.getType();
    os << "\nmodule " << identifier << std::endl;
    os << "  col" << identifier << " : [-1.." << maxBoundaries.first  << "];\n";
    os << "  row" << identifier << " : [-1.." << maxBoundaries.second << "];\n";
    os << "  " << identifier << "PickedUp : bool;\n";
    os << "\n";

    for(const auto [name, position] : agentNameAndPositionMap) {
      printPortableObjectActions(name, identifier);
    }
    os << "endmodule\n\n";
  }

  void PrismModulesPrinter::printPortableObjectActions(const std::string &agentName, const std::string &identifier, const bool canBeDroped) {
    std::string actionName = "[" + agentName + "_pickup_" + identifier + "]";
    agentNameActionMap.at(agentName).insert({PICKUP, actionName});
    os << "  " << actionName << " true -> (col" << identifier << "'=-1) & (row" << identifier << "'=-1) & (" << identifier << "PickedUp'=true);\n";
    if(canBeDroped) {
      actionName = "[" + agentName + "_drop_" + identifier + "_north]";
      agentNameActionMap.at(agentName).insert({DROP, actionName});
      os << "  " << actionName << " " << " true -> (col" << identifier << "'=col" << agentName << ")   & (row" << identifier << "'=row" << agentName << "-1) & (" << identifier << "PickedUp'=false);\n";
      actionName = "[" + agentName + "_drop_" + identifier + "_west]";
      agentNameActionMap.at(agentName).insert({DROP, actionName});
      os << "  " << actionName << " " << " true -> (col" << identifier << "'=col" << agentName << "-1) & (row" << identifier << "'=row" << agentName << ") & (" << identifier << "PickedUp'=false);\n";
      actionName = "[" + agentName + "_drop_" + identifier + "_south]";
      agentNameActionMap.at(agentName).insert({DROP, actionName});
      os << "  " << actionName << " " << " true -> (col" << identifier << "'=col" << agentName << ")   & (row" << identifier << "'=row" << agentName << "+1) & (" << identifier << "PickedUp'=false);\n";
      actionName = "[" + agentName + "_drop_" + identifier + "_east]";
      agentNameActionMap.at(agentName).insert({DROP, actionName});
      os << "  " << actionName << " " << " true -> (col" << identifier << "'=col" << agentName << "+1) & (row" << identifier << "'=row" << agentName << ") & (" << identifier << "PickedUp'=false);\n";
    }
  }

  void PrismModulesPrinter::printDoorModule(const cell &door, const bool &opened) {
    std::string identifier = capitalize(door.getColor()) + door.getType();
    os << "\nmodule " << identifier << std::endl;
    os << "  " << identifier << "Open : bool;\n";
    os << "\n";

    if(opened) {
      for(const auto [name, position] : agentNameAndPositionMap) {
        printUnlockedDoorActions(name, identifier);
      }
    } else {
      for(const auto [name, position] : agentNameAndPositionMap) {
        printLockedDoorActions(name, identifier);
      }
    }
    os << "endmodule\n\n";
  }

  void PrismModulesPrinter::printLockedDoorActions(const std::string &agentName, const std::string &identifier) {
    std::string actionName = "[" + agentName + "_toggle_" + identifier + "]";
    agentNameActionMap.at(agentName).insert({NOFAULT, actionName});
    os << "  " << actionName << " !" << identifier << "Open -> (" << identifier << "Open'=true);\n";
    actionName = "[" + agentName + "_toggle_" + identifier + "]";
    agentNameActionMap.at(agentName).insert({NOFAULT, actionName});
    os << "  " << actionName << " " << identifier << "Open -> (" << identifier << "Open'=false);\n";
  }

  void PrismModulesPrinter::printUnlockedDoorActions(const std::string &agentName, const std::string &identifier) {
    std::string actionName = "[" + agentName + "_toggle_" + identifier + "]";
    agentNameActionMap.at(agentName).insert({NOFAULT, actionName});
    os << "  " << actionName << " !" << identifier << "Open -> (" << identifier << "Open'=true);\n";
    actionName = "[" + agentName + "_toggle_" + identifier + "]";
    agentNameActionMap.at(agentName).insert({NOFAULT, actionName});
    os << "  " << actionName << "  " << identifier << "Open -> (" << identifier << "Open'=false);\n";
  }

  void PrismModulesPrinter::printRobotModule(const AgentName &agentName, const coordinates &initialPosition) {
    os << "\nmodule " << agentName << std::endl;
    os << "  col"    << agentName << " : [1.." << maxBoundaries.first  << "];\n";
    os << "  row"    << agentName << " : [1.." << maxBoundaries.second << "];\n";
    os << "  view" << agentName << " : [0..3];\n";

    printTurnActionsForRobot(agentName);
    printMovementActionsForRobot(agentName);
    if(slipperyBehaviour()) printSlipperyMovementActionsForRobot(agentName);

    for(const auto &door : unlockedDoors) {
      std::string identifier = capitalize(door.getColor()) + door.getType();
      printUnlockedDoorActionsForRobot(agentName, identifier);
    }

    for(const auto &door : lockedDoors) {
      std::string identifier = capitalize(door.getColor()) + door.getType();
      std::string key = capitalize(door.getColor()) + "Key";
      printLockedDoorActionsForRobot(agentName, identifier, key);
    }

    for(const auto &key : keys) {
      std::string identifier = capitalize(key.getColor()) + key.getType();
      os << "  " << agentName << "Carrying" << identifier << " : bool;\n";
      printPortableObjectActionsForRobot(agentName, identifier);
    }

    //printNonMovementActionsForRobot(agentName);


    os << "\n" << actionStream.str();
    actionStream.str(std::string());

    if(agentNameAndPositionMap.size() > 1 && agentName == "Agent" && anyGoals) printDoneActions(agentName);
    os << "endmodule\n\n";
  }

  void PrismModulesPrinter::printPortableObjectActionsForRobot(const std::string &a, const std::string &i, const bool canBeDroped) {
    actionStream << "  [" << a << "_pickup_" << i << "] "      << " !" << a << "IsCarrying & " <<  a << "IsInFrontOf" << i << " -> (" << a << "Carrying" << i << "'=true);\n";
    if(canBeDroped) {
	    actionStream << "  [" << a << "_drop_" << i << "_north]\t" << a << "Carrying" << i << " & view" << a << "=3 & !" << a << "CannotMoveConditionally & !" << a << "CannotMoveNorthWall -> (" << a << "Carrying" << i << "'=false);\n";
	    actionStream << "  [" << a << "_drop_" << i << "_west] \t" << a << "Carrying" << i << " & view" << a << "=2 & !" << a << "CannotMoveConditionally & !" << a << "CannotMoveWestWall  -> (" << a << "Carrying" << i << "'=false);\n";
	    actionStream << "  [" << a << "_drop_" << i << "_south]\t" << a << "Carrying" << i << " & view" << a << "=1 & !" << a << "CannotMoveConditionally & !" << a << "CannotMoveSouthWall -> (" << a << "Carrying" << i << "'=false);\n";
	    actionStream << "  [" << a << "_drop_" << i << "_east] \t" << a << "Carrying" << i << " & view" << a << "=0 & !" << a << "CannotMoveConditionally & !" << a << "CannotMoveEastWall  -> (" << a << "Carrying" << i << "'=false);\n";
      actionStream << "\n";
    }
  }

  void PrismModulesPrinter::printUnlockedDoorActionsForRobot(const std::string &agentName, const std::string &identifier) {
    actionStream << "  [" << agentName << "_toggle_" << identifier  << "] " << agentName << "CannotMove" << identifier << " -> true;\n";
    actionStream << "  [" << agentName << "_toggle_" << identifier << "] " << agentName << "IsNextTo"     << identifier << " -> true;\n";
    actionStream << "\n";
  }

  void PrismModulesPrinter::printLockedDoorActionsForRobot(const std::string &agentName, const std::string &identifier, const std::string &key) {
    actionStream << "  [" << agentName << "_toggle_" << identifier  << "] " << agentName << "CannotMove" << identifier << " & " << agentName << "Carrying" << key << " -> true;\n";
    actionStream << "  [" << agentName << "_toggle_" << identifier << "] "   << agentName << "IsNextTo"   << identifier << " & " << agentName << "Carrying" << key << " -> true;\n";
    actionStream << "\n";
  }

  void PrismModulesPrinter::printTurnActionsForRobot(const AgentName &a) {
    actionStream << printTurnGuard(a, "right", RIGHT, "true") << printTurnUpdate(a, {1.0, "(view"+a+"'=mod(view"+a+"+1,4))"}, RIGHT);
    actionStream << printTurnGuard(a, "left", LEFT, "view"+a+">0") << printTurnUpdate(a, {1.0, "(view"+a+"'=view"+a+"-1)"}, LEFT);
    actionStream << printTurnGuard(a, "left", LEFT, "view"+a+"=0") << printTurnUpdate(a, {1.0, "(view"+a+"'=3)"}, LEFT);
  }

  void PrismModulesPrinter::printMovementActionsForRobot(const AgentName &a) {
    actionStream << printMovementGuard(a, "North", 3) << printMovementUpdate(a, {1.0, northUpdate(a)});
    actionStream << printMovementGuard(a, "East",  0) << printMovementUpdate(a, {1.0, eastUpdate(a)});
    actionStream << printMovementGuard(a, "South", 1) << printMovementUpdate(a, {1.0, southUpdate(a)});
    actionStream << printMovementGuard(a, "West",  2) << printMovementUpdate(a, {1.0, westUpdate(a)});
    if(faultyBehaviour()) {
      std::string actionName = "[" + a + "_stuck]";
      agentNameActionMap.at(a).insert({FORWARD, actionName});
      actionStream << "  " << actionName << " " << "previousAction" << a << "=" << std::to_string(FORWARD);
      actionStream << " & ((view" << a << "=0 & " << a << "CannotMoveEastWall) |";
      actionStream <<    " (view" << a << "=1 & " << a << "CannotMoveSouthWall) |";
      actionStream <<    " (view" << a << "=2 & " << a << "CannotMoveWestWall) |";
      actionStream <<    " (view" << a << "=3 & " << a << "CannotMoveNorthWall) ) -> true;\n";
    }
  }

  std::string PrismModulesPrinter::printMovementGuard(const AgentName &a, const std::string &direction, const size_t &viewDirection) {
    std::string actionName = "[" + a + "_move_" + direction + "]";
    agentNameActionMap.at(a).insert({FORWARD, actionName});
    std::string guard = "  " + actionName + " " + viewVariable(a, viewDirection);
    if(slipperyBehaviour())      guard += " & !" + a + "IsOnSlippery";
    if(anyLava)                  guard += " & !" + a + "IsOnLava";
    if(anyGoals && a == "Agent") guard += " & !" + a + "IsOnGoal";
    guard += " & !" + a + "CannotMove" + direction + "Wall";
    if(anyPortableObject() || !lockedDoors.empty() || !unlockedDoors.empty()) guard += " & !" + a + "CannotMoveConditionally";
    guard += " -> ";
    return guard;
  }

  std::string PrismModulesPrinter::printMovementUpdate(const AgentName &a, const update &u) const {
    return updateToString(u) + ";\n";
  }

  std::string PrismModulesPrinter::printTurnGuard(const AgentName &a, const std::string &direction, const ActionId &actionId, const std::string &cond) {
    std::string actionName = "[" + a + "_turn_" + direction + "]";
    agentNameActionMap.at(a).insert({actionId, actionName});
    std::string guard = "  " + actionName;
    if(slipperyBehaviour()) guard += " !" + a + "IsOnSlippery & ";
    if(anyLava)             guard += " !" + a + "IsOnLava &";
    guard += cond + " -> ";
    return guard;
  }

  std::string PrismModulesPrinter::printTurnUpdate(const AgentName &a, const update &u, const ActionId &actionId) const {
    return updateToString(u) + ";\n";
  }

  void PrismModulesPrinter::printNonMovementActionsForRobot(const AgentName &agentName) {
    for(auto const [actionId, action] : nonMovementActions) {
      std::string actionName = "[" + agentName + "_" + action + "]";
      agentNameActionMap.at(agentName).insert({actionId, actionName});
      actionStream << "  " << actionName << " true -> true;\n";
    }
  }

  void PrismModulesPrinter::printSlipperyMovementActionsForRobot(const AgentName &a) {
    if(!slipperyTiles.at("North").empty()) {
      printSlipperyMovementActionsForNorth(a);
      printSlipperyTurnActionsForNorth(a);
    }
    if(!slipperyTiles.at("East").empty()) {
      printSlipperyMovementActionsForEast(a) ;
      printSlipperyTurnActionsForEast(a);
    }
    if(!slipperyTiles.at("South").empty()) {
      printSlipperyMovementActionsForSouth(a);
      printSlipperyTurnActionsForSouth(a);
    }
    if(!slipperyTiles.at("West").empty()) {
      printSlipperyMovementActionsForWest(a) ;
      printSlipperyTurnActionsForWest(a);
    }
    if(!slipperyTiles.at("NorthWest").empty()) {
      printSlipperyMovementActionsForNorthWest(a);
      printSlipperyTurnActionsForNorthWest(a);
    }
    if(!slipperyTiles.at("NorthEast").empty()) {
      printSlipperyMovementActionsForNorthEast(a);
      printSlipperyTurnActionsForNorthEast(a);
    }
    if(!slipperyTiles.at("SouthWest").empty()) {
      printSlipperyMovementActionsForSouthWest(a);
      printSlipperyTurnActionsForSouthWest(a);
    }
    if(!slipperyTiles.at("SouthWest").empty()) {
      printSlipperyMovementActionsForSouthWest(a);
      printSlipperyTurnActionsForSouthWest(a);
    }
  }

  void PrismModulesPrinter::printSlipperyMovementActionsForNorth(const AgentName &a) {
    actionStream << printSlipperyMovementGuard(a, "North", 3, {"!"+a+"CannotSlipNorth", "!"+a+"CannotSlipNorthEast", "!"+a+"CannotSlipNorthWest"}) << printSlipperyMovementUpdate(a, "North", { {probIntended, northUpdate(a)}, {(1 - probIntended) * 1/2, northUpdate(a)+"&"+eastUpdate(a)}, {(1 - probIntended) * 1/2, northUpdate(a)+"&"+westUpdate(a)} }) << ";\n";
    actionStream << printSlipperyMovementGuard(a, "North", 3, {    a+"CannotSlipNorth", "!"+a+"CannotSlipNorthEast", "!"+a+"CannotSlipNorthWest"}) << printSlipperyMovementUpdate(a, "North", { {1/2, northUpdate(a)+"&"+eastUpdate(a)}, {1/2, northUpdate(a)+"&"+westUpdate(a)} }) << ";\n";
    actionStream << printSlipperyMovementGuard(a, "North", 3, {"!"+a+"CannotSlipNorth",     a+"CannotSlipNorthEast", "!"+a+"CannotSlipNorthWest"}) << printSlipperyMovementUpdate(a, "North", { {probIntended, northUpdate(a)}, {(1 - probIntended), northUpdate(a)+"&"+westUpdate(a)} }) << ";\n";
    actionStream << printSlipperyMovementGuard(a, "North", 3, {"!"+a+"CannotSlipNorth", "!"+a+"CannotSlipNorthEast",     a+"CannotSlipNorthWest"}) << printSlipperyMovementUpdate(a, "North", { {probIntended, northUpdate(a)}, {(1 - probIntended), northUpdate(a)+"&"+eastUpdate(a)} }) << ";\n";
    actionStream << printSlipperyMovementGuard(a, "North", 3, {    a+"CannotSlipNorth",     a+"CannotSlipNorthEast", "!"+a+"CannotSlipNorthWest"}) << printSlipperyMovementUpdate(a, "North", { {1, northUpdate(a)+"&"+westUpdate(a) } }) << ";\n";
    actionStream << printSlipperyMovementGuard(a, "North", 3, {"!"+a+"CannotSlipNorth",     a+"CannotSlipNorthEast",     a+"CannotSlipNorthWest"}) << printSlipperyMovementUpdate(a, "North", { {1, northUpdate(a)} }) << ";\n";
    actionStream << printSlipperyMovementGuard(a, "North", 3, {    a+"CannotSlipNorth", "!"+a+"CannotSlipNorthEast",     a+"CannotSlipNorthWest"}) << printSlipperyMovementUpdate(a, "North", { {1, northUpdate(a)+"&"+eastUpdate(a)} }) << ";\n";
    actionStream << printSlipperyMovementGuard(a, "North", 3, {    a+"CannotSlipNorth",     a+"CannotSlipNorthEast",     a+"CannotSlipNorthWest"}) << printSlipperyMovementUpdate(a, "North", {}) << ";\n";

    actionStream << printSlipperyMovementGuard(a, "North", 2, {"!"+a+"CannotSlipWest", "!"+a+"CannotSlipNorthWest"}) << printSlipperyMovementUpdate(a, "North", { {probIntended, westUpdate(a) }, {1 - probIntended, westUpdate(a)+"&"+northUpdate(a)} }) << ";\n";
    actionStream << printSlipperyMovementGuard(a, "North", 2, {    a+"CannotSlipWest", "!"+a+"CannotSlipNorthWest"}) << printSlipperyMovementUpdate(a, "North", { {1, westUpdate(a)+"&"+northUpdate(a)} }) << ";\n";
    actionStream << printSlipperyMovementGuard(a, "North", 2, {"!"+a+"CannotSlipWest",     a+"CannotSlipNorthWest"}) << printSlipperyMovementUpdate(a, "North", { {1, westUpdate(a) } }) << ";\n";
    actionStream << printSlipperyMovementGuard(a, "North", 2, {    a+"CannotSlipWest",     a+"CannotSlipNorthWest"}) << printSlipperyMovementUpdate(a, "North", {}) << ";\n";

    actionStream << printSlipperyMovementGuard(a, "North", 0, {"!"+a+"CannotSlipEast", "!"+a+"CannotSlipNorthEast"}) << printSlipperyMovementUpdate(a, "North", { {probIntended, eastUpdate(a) }, {1 - probIntended, eastUpdate(a)+"&"+northUpdate(a)} }) << ";\n";
    actionStream << printSlipperyMovementGuard(a, "North", 0, {    a+"CannotSlipEast", "!"+a+"CannotSlipNorthEast"}) << printSlipperyMovementUpdate(a, "North", { {1, eastUpdate(a)+"&"+northUpdate(a)} }) << ";\n";
    actionStream << printSlipperyMovementGuard(a, "North", 0, {"!"+a+"CannotSlipEast",     a+"CannotSlipNorthEast"}) << printSlipperyMovementUpdate(a, "North", { {1, eastUpdate(a) } }) << ";\n";
    actionStream << printSlipperyMovementGuard(a, "North", 0, {    a+"CannotSlipEast",     a+"CannotSlipNorthEast"}) << printSlipperyMovementUpdate(a, "North", {}) << ";\n";

    actionStream << printSlipperyMovementGuard(a, "North", 1, {"!"+a+"CannotSlipSouth"}) << printSlipperyMovementUpdate(a, "North", { {probIntended, southUpdate(a) }, {1 - probIntended, "true"} }) << ";\n";
    actionStream << printSlipperyMovementGuard(a, "North", 1, {    a+"CannotSlipSouth"}) << printSlipperyMovementUpdate(a, "North", { {1, "true"} }) << ";\n";
  }

  void PrismModulesPrinter::printSlipperyMovementActionsForEast(const AgentName &a) {
    actionStream << printSlipperyMovementGuard(a, "East", 0, {"!"+a+"CannotSlipEast", "!"+a+"CannotSlipSouthEast", "!"+a+"CannotSlipNorthEast"}) << printSlipperyMovementUpdate(a, "East", { {probIntended, eastUpdate(a)}, {(1 - probIntended) * 1/2, eastUpdate(a)+"&"+southUpdate(a)}, {(1 - probIntended) * 1/2, eastUpdate(a)+"&"+northUpdate(a)} }) << ";\n";
    actionStream << printSlipperyMovementGuard(a, "East", 0, {    a+"CannotSlipEast", "!"+a+"CannotSlipSouthEast", "!"+a+"CannotSlipNorthEast"}) << printSlipperyMovementUpdate(a, "East", { {1/2, eastUpdate(a)+"&"+southUpdate(a)}, {1/2, eastUpdate(a)+"&"+northUpdate(a)} }) << ";\n";
    actionStream << printSlipperyMovementGuard(a, "East", 0, {"!"+a+"CannotSlipEast",     a+"CannotSlipSouthEast", "!"+a+"CannotSlipNorthEast"}) << printSlipperyMovementUpdate(a, "East", { {probIntended, eastUpdate(a)}, {(1 - probIntended), eastUpdate(a)+"&"+northUpdate(a)} }) << ";\n";
    actionStream << printSlipperyMovementGuard(a, "East", 0, {"!"+a+"CannotSlipEast", "!"+a+"CannotSlipSouthEast",     a+"CannotSlipNorthEast"}) << printSlipperyMovementUpdate(a, "East", { {probIntended, eastUpdate(a)}, {(1 - probIntended), eastUpdate(a)+"&"+southUpdate(a)} }) << ";\n";
    actionStream << printSlipperyMovementGuard(a, "East", 0, {    a+"CannotSlipEast",     a+"CannotSlipSouthEast", "!"+a+"CannotSlipNorthEast"}) << printSlipperyMovementUpdate(a, "East", { {1, eastUpdate(a)+"&"+northUpdate(a) } }) << ";\n";
    actionStream << printSlipperyMovementGuard(a, "East", 0, {"!"+a+"CannotSlipEast",     a+"CannotSlipSouthEast",     a+"CannotSlipNorthEast"}) << printSlipperyMovementUpdate(a, "East", { {1, eastUpdate(a)} }) << ";\n";
    actionStream << printSlipperyMovementGuard(a, "East", 0, {    a+"CannotSlipEast", "!"+a+"CannotSlipSouthEast",     a+"CannotSlipNorthEast"}) << printSlipperyMovementUpdate(a, "East", { {1, eastUpdate(a)+"&"+southUpdate(a)} }) << ";\n";
    actionStream << printSlipperyMovementGuard(a, "East", 0, {    a+"CannotSlipEast",     a+"CannotSlipSouthEast",     a+"CannotSlipNorthEast"}) << printSlipperyMovementUpdate(a, "East", {}) << ";\n";

    actionStream << printSlipperyMovementGuard(a, "East", 3, {"!"+a+"CannotSlipNorth", "!"+a+"CannotSlipNorthEast"}) << printSlipperyMovementUpdate(a, "East", { {probIntended, northUpdate(a) }, {1 - probIntended, eastUpdate(a)+"&"+northUpdate(a)} }) << ";\n";
    actionStream << printSlipperyMovementGuard(a, "East", 3, {    a+"CannotSlipNorth", "!"+a+"CannotSlipNorthEast"}) << printSlipperyMovementUpdate(a, "East", { {1, eastUpdate(a)+"&"+northUpdate(a)} }) << ";\n";
    actionStream << printSlipperyMovementGuard(a, "East", 3, {"!"+a+"CannotSlipNorth",     a+"CannotSlipNorthEast"}) << printSlipperyMovementUpdate(a, "East", { {1, northUpdate(a) } }) << ";\n";
    actionStream << printSlipperyMovementGuard(a, "East", 3, {    a+"CannotSlipNorth",     a+"CannotSlipNorthEast"}) << printSlipperyMovementUpdate(a, "East", {}) << ";\n";

    actionStream << printSlipperyMovementGuard(a, "East", 1, {"!"+a+"CannotSlipSouth", "!"+a+"CannotSlipSouthEast"}) << printSlipperyMovementUpdate(a, "East", { {probIntended, southUpdate(a) }, {1 - probIntended, eastUpdate(a)+"&"+southUpdate(a)} }) << ";\n";
    actionStream << printSlipperyMovementGuard(a, "East", 1, {    a+"CannotSlipSouth", "!"+a+"CannotSlipSouthEast"}) << printSlipperyMovementUpdate(a, "East", { {1, eastUpdate(a)+"&"+southUpdate(a)} }) << ";\n";
    actionStream << printSlipperyMovementGuard(a, "East", 1, {"!"+a+"CannotSlipSouth",     a+"CannotSlipSouthEast"}) << printSlipperyMovementUpdate(a, "East", { {1, southUpdate(a) } }) << ";\n";
    actionStream << printSlipperyMovementGuard(a, "East", 1, {    a+"CannotSlipSouth",     a+"CannotSlipSouthEast"}) << printSlipperyMovementUpdate(a, "East", {}) << ";\n";

    actionStream << printSlipperyMovementGuard(a, "East", 2, {"!"+a+"CannotSlipEast"}) << printSlipperyMovementUpdate(a, "East", { {probIntended, eastUpdate(a) }, {1 - probIntended, "true"} }) << ";\n";
    actionStream << printSlipperyMovementGuard(a, "East", 2, {    a+"CannotSlipEast"}) << printSlipperyMovementUpdate(a, "East", { {1, "true"} }) << ";\n";
  }

  void PrismModulesPrinter::printSlipperyMovementActionsForSouth(const AgentName &a) {
    actionStream << printSlipperyMovementGuard(a, "South", 1, {"!"+a+"CannotSlipSouth", "!"+a+"CannotSlipSouthEast", "!"+a+"CannotSlipSouthWest"}) << printSlipperyMovementUpdate(a, "South", { {probIntended, southUpdate(a)}, {(1 - probIntended) * 1/2, southUpdate(a)+"&"+eastUpdate(a)}, {(1 - probIntended) * 1/2, southUpdate(a)+"&"+westUpdate(a)} }) << ";\n";
    actionStream << printSlipperyMovementGuard(a, "South", 1, {    a+"CannotSlipSouth", "!"+a+"CannotSlipSouthEast", "!"+a+"CannotSlipSouthWest"}) << printSlipperyMovementUpdate(a, "South", { {1/2, southUpdate(a)+"&"+eastUpdate(a)}, {1/2, southUpdate(a)+"&"+westUpdate(a)} }) << ";\n";
    actionStream << printSlipperyMovementGuard(a, "South", 1, {"!"+a+"CannotSlipSouth",     a+"CannotSlipSouthEast", "!"+a+"CannotSlipSouthWest"}) << printSlipperyMovementUpdate(a, "South", { {probIntended, southUpdate(a)}, {(1 - probIntended), southUpdate(a)+"&"+westUpdate(a)} }) << ";\n";
    actionStream << printSlipperyMovementGuard(a, "South", 1, {"!"+a+"CannotSlipSouth", "!"+a+"CannotSlipSouthEast",     a+"CannotSlipSouthWest"}) << printSlipperyMovementUpdate(a, "South", { {probIntended, southUpdate(a)}, {(1 - probIntended), southUpdate(a)+"&"+eastUpdate(a)} }) << ";\n";
    actionStream << printSlipperyMovementGuard(a, "South", 1, {    a+"CannotSlipSouth",     a+"CannotSlipSouthEast", "!"+a+"CannotSlipSouthWest"}) << printSlipperyMovementUpdate(a, "South", { {1, southUpdate(a)+"&"+westUpdate(a) } }) << ";\n";
    actionStream << printSlipperyMovementGuard(a, "South", 1, {"!"+a+"CannotSlipSouth",     a+"CannotSlipSouthEast",     a+"CannotSlipSouthWest"}) << printSlipperyMovementUpdate(a, "South", { {1, southUpdate(a)} }) << ";\n";
    actionStream << printSlipperyMovementGuard(a, "South", 1, {    a+"CannotSlipSouth", "!"+a+"CannotSlipSouthEast",     a+"CannotSlipSouthWest"}) << printSlipperyMovementUpdate(a, "South", { {1, southUpdate(a)+"&"+eastUpdate(a)} }) << ";\n";
    actionStream << printSlipperyMovementGuard(a, "South", 1, {    a+"CannotSlipSouth",     a+"CannotSlipSouthEast",     a+"CannotSlipSouthWest"}) << printSlipperyMovementUpdate(a, "South", {}) << ";\n";

    actionStream << printSlipperyMovementGuard(a, "South", 2, {"!"+a+"CannotSlipWest", "!"+a+"CannotSlipSouthWest"}) << printSlipperyMovementUpdate(a, "South", { {probIntended, westUpdate(a) }, {1 - probIntended, westUpdate(a)+"&"+southUpdate(a)} }) << ";\n";
    actionStream << printSlipperyMovementGuard(a, "South", 2, {    a+"CannotSlipWest", "!"+a+"CannotSlipSouthWest"}) << printSlipperyMovementUpdate(a, "South", { {1, westUpdate(a)+"&"+southUpdate(a)} }) << ";\n";
    actionStream << printSlipperyMovementGuard(a, "South", 2, {"!"+a+"CannotSlipWest",     a+"CannotSlipSouthWest"}) << printSlipperyMovementUpdate(a, "South", { {1, westUpdate(a) } }) << ";\n";
    actionStream << printSlipperyMovementGuard(a, "South", 2, {    a+"CannotSlipWest",     a+"CannotSlipSouthWest"}) << printSlipperyMovementUpdate(a, "South", {}) << ";\n";

    actionStream << printSlipperyMovementGuard(a, "South", 0, {"!"+a+"CannotSlipEast", "!"+a+"CannotSlipSouthEast"}) << printSlipperyMovementUpdate(a, "South", { {probIntended, eastUpdate(a) }, {1 - probIntended, eastUpdate(a)+"&"+southUpdate(a)} }) << ";\n";
    actionStream << printSlipperyMovementGuard(a, "South", 0, {    a+"CannotSlipEast", "!"+a+"CannotSlipSouthEast"}) << printSlipperyMovementUpdate(a, "South", { {1, eastUpdate(a)+"&"+southUpdate(a)} }) << ";\n";
    actionStream << printSlipperyMovementGuard(a, "South", 0, {"!"+a+"CannotSlipEast",     a+"CannotSlipSouthEast"}) << printSlipperyMovementUpdate(a, "South", { {1, eastUpdate(a) } }) << ";\n";
    actionStream << printSlipperyMovementGuard(a, "South", 0, {    a+"CannotSlipEast",     a+"CannotSlipSouthEast"}) << printSlipperyMovementUpdate(a, "South", {}) << ";\n";

    actionStream << printSlipperyMovementGuard(a, "South", 3, {"!"+a+"CannotSlipSouth"}) << printSlipperyMovementUpdate(a, "South", { {probIntended, northUpdate(a) }, {1 - probIntended, "true"} }) << ";\n";
    actionStream << printSlipperyMovementGuard(a, "South", 3, {    a+"CannotSlipSouth"}) << printSlipperyMovementUpdate(a, "South", { {1, "true"} }) << ";\n";
  }

  void PrismModulesPrinter::printSlipperyMovementActionsForWest(const AgentName &a) {
    actionStream << printSlipperyMovementGuard(a, "West", 2, {"!"+a+"CannotSlipWest", "!"+a+"CannotSlipSouthWest", "!"+a+"CannotSlipNorthWest"}) << printSlipperyMovementUpdate(a, "West", { {probIntended, westUpdate(a)}, {(1 - probIntended) * 1/2, westUpdate(a)+"&"+southUpdate(a)}, {(1 - probIntended) * 1/2, westUpdate(a)+"&"+northUpdate(a)} }) << ";\n";
    actionStream << printSlipperyMovementGuard(a, "West", 2, {    a+"CannotSlipWest", "!"+a+"CannotSlipSouthWest", "!"+a+"CannotSlipNorthWest"}) << printSlipperyMovementUpdate(a, "West", { {1/2, westUpdate(a)+"&"+southUpdate(a)}, {1/2, westUpdate(a)+"&"+northUpdate(a)} }) << ";\n";
    actionStream << printSlipperyMovementGuard(a, "West", 2, {"!"+a+"CannotSlipWest",     a+"CannotSlipSouthWest", "!"+a+"CannotSlipNorthWest"}) << printSlipperyMovementUpdate(a, "West", { {probIntended, westUpdate(a)}, {(1 - probIntended), westUpdate(a)+"&"+northUpdate(a)} }) << ";\n";
    actionStream << printSlipperyMovementGuard(a, "West", 2, {"!"+a+"CannotSlipWest", "!"+a+"CannotSlipSouthWest",    a+"CannotSlipNorthWest"}) << printSlipperyMovementUpdate(a, "West", { {probIntended, westUpdate(a)}, {(1 - probIntended), westUpdate(a)+"&"+southUpdate(a)} }) << ";\n";
    actionStream << printSlipperyMovementGuard(a, "West", 2, {    a+"CannotSlipWest",     a+"CannotSlipSouthWest", "!"+a+"CannotSlipNorthWest"}) << printSlipperyMovementUpdate(a, "West", { {1, westUpdate(a)+"&"+northUpdate(a) } }) << ";\n";
    actionStream << printSlipperyMovementGuard(a, "West", 2, {"!"+a+"CannotSlipWest",     a+"CannotSlipSouthWest",     a+"CannotSlipNorthWest"}) << printSlipperyMovementUpdate(a, "West", { {1, westUpdate(a)} }) << ";\n";
    actionStream << printSlipperyMovementGuard(a, "West", 2, {    a+"CannotSlipWest", "!"+a+"CannotSlipSouthWest",     a+"CannotSlipNorthWest"}) << printSlipperyMovementUpdate(a, "West", { {1, westUpdate(a)+"&"+southUpdate(a)} }) << ";\n";
    actionStream << printSlipperyMovementGuard(a, "West", 2, {    a+"CannotSlipWest",     a+"CannotSlipSouthWest",     a+"CannotSlipNorthWest"}) << printSlipperyMovementUpdate(a, "West", {}) << ";\n";

    actionStream << printSlipperyMovementGuard(a, "West", 3, {"!"+a+"CannotSlipNorth", "!"+a+"CannotSlipNorthWest"}) << printSlipperyMovementUpdate(a, "West", { {probIntended, northUpdate(a) }, {1 - probIntended, westUpdate(a)+"&"+northUpdate(a)} }) << ";\n";
    actionStream << printSlipperyMovementGuard(a, "West", 3, {    a+"CannotSlipNorth", "!"+a+"CannotSlipNorthWest"}) << printSlipperyMovementUpdate(a, "West", { {1, westUpdate(a)+"&"+northUpdate(a)} }) << ";\n";
    actionStream << printSlipperyMovementGuard(a, "West", 3, {"!"+a+"CannotSlipNorth",     a+"CannotSlipNorthWest"}) << printSlipperyMovementUpdate(a, "West", { {1, northUpdate(a) } }) << ";\n";
    actionStream << printSlipperyMovementGuard(a, "West", 3, {    a+"CannotSlipNorth",     a+"CannotSlipNorthWest"}) << printSlipperyMovementUpdate(a, "West", {}) << ";\n";

    actionStream << printSlipperyMovementGuard(a, "West", 1, {"!"+a+"CannotSlipSouth", "!"+a+"CannotSlipSouthWest"}) << printSlipperyMovementUpdate(a, "West", { {probIntended, southUpdate(a) }, {1 - probIntended, westUpdate(a)+"&"+southUpdate(a)} }) << ";\n";
    actionStream << printSlipperyMovementGuard(a, "West", 1, {    a+"CannotSlipSouth", "!"+a+"CannotSlipSouthWest"}) << printSlipperyMovementUpdate(a, "West", { {1, westUpdate(a)+"&"+southUpdate(a)} }) << ";\n";
    actionStream << printSlipperyMovementGuard(a, "West", 1, {"!"+a+"CannotSlipSouth",     a+"CannotSlipSouthWest"}) << printSlipperyMovementUpdate(a, "West", { {1, southUpdate(a) } }) << ";\n";
    actionStream << printSlipperyMovementGuard(a, "West", 1, {    a+"CannotSlipSouth",     a+"CannotSlipSouthWest"}) << printSlipperyMovementUpdate(a, "West", {}) << ";\n";

    actionStream << printSlipperyMovementGuard(a, "West", 0, {"!"+a+"CannotSlipWest"}) << printSlipperyMovementUpdate(a, "West", { {probIntended, westUpdate(a) }, {1 - probIntended, "true"} }) << ";\n";
    actionStream << printSlipperyMovementGuard(a, "West", 0, {    a+"CannotSlipWest"}) << printSlipperyMovementUpdate(a, "West", {{1, "true"}}) << ";\n";
  }

  void PrismModulesPrinter::printSlipperyTurnActionsForNorth(const AgentName &a) {
    actionStream << printSlipperyTurnGuard(a, "right", "North", RIGHT, {"!"+a+"CannotSlipNorth"},  "true") << printSlipperyTurnUpdate(a, { {probIntended, "(view"+a+"'=mod(view"+a+"+1,4))"}, { 1 - probIntended, northUpdate(a)} }) << ";\n";
    actionStream << printSlipperyTurnGuard(a, "right", "North", RIGHT, {    a+"CannotSlipNorth"}, "true") << printSlipperyTurnUpdate(a, { {1, "(view"+a+"'=mod(view"+a+"+1,4))"} }) << ";\n";

    actionStream << printSlipperyTurnGuard(a, "left", "North", LEFT, {"!"+a+"CannotSlipNorth"}, "view"+a+">0") << printSlipperyTurnUpdate(a, { {probIntended, "(view"+a+"'=view"+a+"-1)"}, {1 - probIntended, northUpdate(a)} }) << ";\n";
    actionStream << printSlipperyTurnGuard(a, "left", "North", LEFT, {"!"+a+"CannotSlipNorth"}, "view"+a+"=0") << printSlipperyTurnUpdate(a, { {probIntended, "(view"+a+"'=3)"},     {1 - probIntended, northUpdate(a)} }) << ";\n";
    actionStream << printSlipperyTurnGuard(a, "left", "North", LEFT, {    a+"CannotSlipNorth"}, "view"+a+">0") << printSlipperyTurnUpdate(a, { {1, "(view"+a+"'=view"+a+"-1)"} }) << ";\n";
    actionStream << printSlipperyTurnGuard(a, "left", "North", LEFT, {    a+"CannotSlipNorth"}, "view"+a+"=0") << printSlipperyTurnUpdate(a, { {1, "(view"+a+"'=3)"} }) << ";\n";
  }

  void PrismModulesPrinter::printSlipperyTurnActionsForEast(const AgentName &a) {
    actionStream << printSlipperyTurnGuard(a, "right", "East", RIGHT, {"!"+a+"CannotSlipEast"},  "true") << printSlipperyTurnUpdate(a, { {probIntended, "(view"+a+"'=mod(view"+a+"+1,4))"}, { 1 - probIntended, eastUpdate(a)} }) << ";\n";
    actionStream << printSlipperyTurnGuard(a, "right", "East", RIGHT, {    a+"CannotSlipEast"}, "true") << printSlipperyTurnUpdate(a, { {1, "(view"+a+"'=mod(view"+a+"+1,4))"} }) << ";\n";

    actionStream << printSlipperyTurnGuard(a, "left", "East", LEFT, {"!"+a+"CannotSlipEast"}, "view"+a+">0") << printSlipperyTurnUpdate(a, { {probIntended, "(view"+a+"'=view"+a+"-1)"}, {1 - probIntended, eastUpdate(a)} }) << ";\n";
    actionStream << printSlipperyTurnGuard(a, "left", "East", LEFT, {"!"+a+"CannotSlipEast"}, "view"+a+"=0") << printSlipperyTurnUpdate(a, { {probIntended, "(view"+a+"'=3)"},     {1 - probIntended, eastUpdate(a)} }) << ";\n";
    actionStream << printSlipperyTurnGuard(a, "left", "East", LEFT, {    a+"CannotSlipEast"}, "view"+a+">0") << printSlipperyTurnUpdate(a, { {1, "(view"+a+"'=view"+a+"-1)"} }) << ";\n";
    actionStream << printSlipperyTurnGuard(a, "left", "East", LEFT, {    a+"CannotSlipEast"}, "view"+a+"=0") << printSlipperyTurnUpdate(a, { {1, "(view"+a+"'=3)"} }) << ";\n";
  }

  void PrismModulesPrinter::printSlipperyTurnActionsForSouth(const AgentName &a) {
    actionStream << printSlipperyTurnGuard(a, "right", "South", RIGHT, {"!"+a+"CannotSlipSouth"},  "true") << printSlipperyTurnUpdate(a, { {probIntended, "(view"+a+"'=mod(view"+a+"+1,4))"}, { 1 - probIntended, southUpdate(a)} }) << ";\n";
    actionStream << printSlipperyTurnGuard(a, "right", "South", RIGHT, {    a+"CannotSlipSouth"}, "true") << printSlipperyTurnUpdate(a, { {1, "(view"+a+"'=mod(view"+a+"+1,4))"} }) << ";\n";

    actionStream << printSlipperyTurnGuard(a, "left", "South", LEFT, {"!"+a+"CannotSlipSouth"}, "view"+a+">0") << printSlipperyTurnUpdate(a, { {probIntended, "(view"+a+"'=view"+a+"-1)"}, {1 - probIntended, southUpdate(a)} }) << ";\n";
    actionStream << printSlipperyTurnGuard(a, "left", "South", LEFT, {"!"+a+"CannotSlipSouth"}, "view"+a+"=0") << printSlipperyTurnUpdate(a, { {probIntended, "(view"+a+"'=3)"},     {1 - probIntended, southUpdate(a)} }) << ";\n";
    actionStream << printSlipperyTurnGuard(a, "left", "South", LEFT, {    a+"CannotSlipSouth"}, "view"+a+">0") << printSlipperyTurnUpdate(a, { {1, "(view"+a+"'=view"+a+"-1)"} }) << ";\n";
    actionStream << printSlipperyTurnGuard(a, "left", "South", LEFT, {    a+"CannotSlipSouth"}, "view"+a+"=0") << printSlipperyTurnUpdate(a, { {1, "(view"+a+"'=3)"} }) << ";\n";
  }

  void PrismModulesPrinter::printSlipperyTurnActionsForWest(const AgentName &a) {
    actionStream << printSlipperyTurnGuard(a, "right", "West", RIGHT, {"!"+a+"CannotSlipWest"},  "true") << printSlipperyTurnUpdate(a, { {probIntended, "(view"+a+"'=mod(view"+a+"+1,4))"}, { 1 - probIntended, westUpdate(a)} }) << ";\n";
    actionStream << printSlipperyTurnGuard(a, "right", "West", RIGHT, {    a+"CannotSlipWest"}, "true") << printSlipperyTurnUpdate(a, { {1, "(view"+a+"'=mod(view"+a+"+1,4))"} }) << ";\n";

    actionStream << printSlipperyTurnGuard(a, "left", "West", LEFT, {"!"+a+"CannotSlipWest"}, "view"+a+">0") << printSlipperyTurnUpdate(a, { {probIntended, "(view"+a+"'=view"+a+"-1)"}, {1 - probIntended, westUpdate(a)} }) << ";\n";
    actionStream << printSlipperyTurnGuard(a, "left", "West", LEFT, {"!"+a+"CannotSlipWest"}, "view"+a+"=0") << printSlipperyTurnUpdate(a, { {probIntended, "(view"+a+"'=3)"},     {1 - probIntended, westUpdate(a)} }) << ";\n";
    actionStream << printSlipperyTurnGuard(a, "left", "West", LEFT, {    a+"CannotSlipWest"}, "view"+a+">0") << printSlipperyTurnUpdate(a, { {1, "(view"+a+"'=view"+a+"-1)"} }) << ";\n";
    actionStream << printSlipperyTurnGuard(a, "left", "West", LEFT, {    a+"CannotSlipWest"}, "view"+a+"=0") << printSlipperyTurnUpdate(a, { {1, "(view"+a+"'=3)"} }) << ";\n";
  }

  void PrismModulesPrinter::printSlipperyMovementActionsForNorthWest(const AgentName &a) { throw std::logic_error("The logic for SlipperyNorthWest tiles is not yet implemented."); }
  void PrismModulesPrinter::printSlipperyTurnActionsForNorthWest(const AgentName &a){ throw std::logic_error("The logic for SlipperyNorthWest tiles is not yet implemented."); }

  void PrismModulesPrinter::printSlipperyMovementActionsForNorthEast(const AgentName &a) {
    actionStream << printSlipperyMovementGuard(a, "NorthEast", 0, {"!"+a+"CannotSlipNorthEast", "!"+a+"CannotSlipEast"}) << printSlipperyMovementUpdate(a, "", {{probIntended, eastUpdate(a)}, {1-probIntended, northUpdate(a)+"&"+eastUpdate(a)}}) << ";\n";
    actionStream << printSlipperyMovementGuard(a, "NorthEast", 0, {"!"+a+"CannotSlipNorthEast",     a+"CannotSlipEast"}) << printSlipperyMovementUpdate(a, "", {{1, northUpdate(a)+"&"+eastUpdate(a)}}) << ";\n";
    actionStream << printSlipperyMovementGuard(a, "NorthEast", 0, {    a+"CannotSlipNorthEast", "!"+a+"CannotSlipEast"}) << printSlipperyMovementUpdate(a, "", {{1, eastUpdate(a)}}) << ";\n";
    actionStream << printSlipperyMovementGuard(a, "NorthEast", 0, {    a+"CannotSlipNorthEast",     a+"CannotSlipEast"}) << printSlipperyMovementUpdate(a, "", {{1, "true"}}) << ";\n";

    actionStream << printSlipperyMovementGuard(a, "NorthEast", 3, {"!"+a+"CannotSlipNorthEast", "!"+a+"CannotSlipNorth"}) << printSlipperyMovementUpdate(a, "", {{probIntended, northUpdate(a)}, {1-probIntended, eastUpdate(a)+"&"+northUpdate(a)}}) << ";\n";
    actionStream << printSlipperyMovementGuard(a, "NorthEast", 3, {"!"+a+"CannotSlipNorthEast",     a+"CannotSlipNorth"}) << printSlipperyMovementUpdate(a, "", {{1, northUpdate(a)+"&"+eastUpdate(a)}}) << ";\n";
    actionStream << printSlipperyMovementGuard(a, "NorthEast", 3, {    a+"CannotSlipNorthEast", "!"+a+"CannotSlipNorth"}) << printSlipperyMovementUpdate(a, "", {{1, northUpdate(a)}}) << ";\n";
    actionStream << printSlipperyMovementGuard(a, "NorthEast", 3, {    a+"CannotSlipNorthEast",     a+"CannotSlipNorth"}) << printSlipperyMovementUpdate(a, "", {{1, "true"}}) << ";\n";


    float pd3 = (1 - probIntended) / 3;
    float pi3 = probIntended;

    float sum2 = probIntended + 2 * (1 - probIntended)/3;
    float pd2 = (1 - probIntended) / sum2;
    float pi2 = probIntended / sum2;

    float sum1 = probIntended + (1 - probIntended)/3;
    float pd1 = (1 - probIntended) / sum1;
    float pi1 = probIntended / sum1;
    actionStream << printSlipperyMovementGuard(a, "NorthEast", 1, {"!"+a+"CannotSlipNorthEast", "!"+a+"CannotSlipEast", "!"+a+"CannotSlipSouthEast", "!"+a+"CannotSlipSouth"}) << printSlipperyMovementUpdate(a, "", {{pi3, southUpdate(a)}, {pd3, northUpdate(a)+"&"+eastUpdate(a)}, {pd3, eastUpdate(a)}, {pd3, southUpdate(a)+"&"+eastUpdate(a)}}) << ";\n";
    actionStream << printSlipperyMovementGuard(a, "NorthEast", 1, {"!"+a+"CannotSlipNorthEast", "!"+a+"CannotSlipEast", "!"+a+"CannotSlipSouthEast",     a+"CannotSlipSouth"}) << printSlipperyMovementUpdate(a, "", {{1/3.f, northUpdate(a)+"&"+eastUpdate(a)}, {1/3.f, eastUpdate(a)}, {1/3.f, southUpdate(a)+"&"+eastUpdate(a)}}) << ";\n";
    actionStream << printSlipperyMovementGuard(a, "NorthEast", 1, {"!"+a+"CannotSlipNorthEast", "!"+a+"CannotSlipEast",     a+"CannotSlipSouthEast", "!"+a+"CannotSlipSouth"}) << printSlipperyMovementUpdate(a, "", {{pi1, southUpdate(a)}, {pd2, northUpdate(a)+"&"+eastUpdate(a)}, {pd2, eastUpdate(a)}}) << ";\n";
    actionStream << printSlipperyMovementGuard(a, "NorthEast", 1, {"!"+a+"CannotSlipNorthEast", "!"+a+"CannotSlipEast",     a+"CannotSlipSouthEast",     a+"CannotSlipSouth"}) << printSlipperyMovementUpdate(a, "", {{1/2.f, northUpdate(a)+"&"+eastUpdate(a)}, {1/2.f, eastUpdate(a)}}) << ";\n";
    actionStream << printSlipperyMovementGuard(a, "NorthEast", 1, {"!"+a+"CannotSlipNorthEast",     a+"CannotSlipEast", "!"+a+"CannotSlipSouthEast", "!"+a+"CannotSlipSouth"}) << printSlipperyMovementUpdate(a, "", {{pi1, southUpdate(a)}, {pd2, northUpdate(a)+"&"+eastUpdate(a)}, {pd2, southUpdate(a)+"&"+eastUpdate(a)}}) << ";\n";
    actionStream << printSlipperyMovementGuard(a, "NorthEast", 1, {"!"+a+"CannotSlipNorthEast",     a+"CannotSlipEast", "!"+a+"CannotSlipSouthEast",     a+"CannotSlipSouth"}) << printSlipperyMovementUpdate(a, "", {{1/2.f, northUpdate(a)+"&"+eastUpdate(a)}, {1/2.f, southUpdate(a)+"&"+eastUpdate(a)}}) << ";\n";
    actionStream << printSlipperyMovementGuard(a, "NorthEast", 1, {"!"+a+"CannotSlipNorthEast",     a+"CannotSlipEast",     a+"CannotSlipSouthEast", "!"+a+"CannotSlipSouth"}) << printSlipperyMovementUpdate(a, "", {{pi1, southUpdate(a)}, {pd1, northUpdate(a)+"&"+eastUpdate(a) }}) << ";\n";
    actionStream << printSlipperyMovementGuard(a, "NorthEast", 1, {"!"+a+"CannotSlipNorthEast",     a+"CannotSlipEast",     a+"CannotSlipSouthEast",     a+"CannotSlipSouth"}) << printSlipperyMovementUpdate(a, "", {{1, northUpdate(a)+"&"+eastUpdate(a)}}) << ";\n";
    actionStream << printSlipperyMovementGuard(a, "NorthEast", 1, {    a+"CannotSlipNorthEast", "!"+a+"CannotSlipEast", "!"+a+"CannotSlipSouthEast", "!"+a+"CannotSlipSouth"}) << printSlipperyMovementUpdate(a, "", {{pi1, southUpdate(a)}, {pd2, eastUpdate(a)}, {pd2, southUpdate(a)+"&"+eastUpdate(a)}}) << ";\n";
    actionStream << printSlipperyMovementGuard(a, "NorthEast", 1, {    a+"CannotSlipNorthEast", "!"+a+"CannotSlipEast", "!"+a+"CannotSlipSouthEast",     a+"CannotSlipSouth"}) << printSlipperyMovementUpdate(a, "", {{1/2.f, eastUpdate(a)}, {1/2.f, southUpdate(a)+"&"+eastUpdate(a)}}) << ";\n";
    actionStream << printSlipperyMovementGuard(a, "NorthEast", 1, {    a+"CannotSlipNorthEast", "!"+a+"CannotSlipEast",     a+"CannotSlipSouthEast", "!"+a+"CannotSlipSouth"}) << printSlipperyMovementUpdate(a, "", {{pi1, southUpdate(a)}, {pd1, eastUpdate(a)}}) << ";\n";
    actionStream << printSlipperyMovementGuard(a, "NorthEast", 1, {    a+"CannotSlipNorthEast", "!"+a+"CannotSlipEast",     a+"CannotSlipSouthEast",     a+"CannotSlipSouth"}) << printSlipperyMovementUpdate(a, "", {{1, eastUpdate(a)}}) << ";\n";
    actionStream << printSlipperyMovementGuard(a, "NorthEast", 1, {    a+"CannotSlipNorthEast",     a+"CannotSlipEast", "!"+a+"CannotSlipSouthEast", "!"+a+"CannotSlipSouth"}) << printSlipperyMovementUpdate(a, "", {{pi1, southUpdate(a)}, {pd1, southUpdate(a)+"&"+eastUpdate(a)}}) << ";\n";
    actionStream << printSlipperyMovementGuard(a, "NorthEast", 1, {    a+"CannotSlipNorthEast",     a+"CannotSlipEast", "!"+a+"CannotSlipSouthEast",     a+"CannotSlipSouth"}) << printSlipperyMovementUpdate(a, "", {{1, southUpdate(a)+"&"+eastUpdate(a)}}) << ";\n";
    actionStream << printSlipperyMovementGuard(a, "NorthEast", 1, {    a+"CannotSlipNorthEast",     a+"CannotSlipEast",     a+"CannotSlipSouthEast", "!"+a+"CannotSlipSouth"}) << printSlipperyMovementUpdate(a, "", {{1, southUpdate(a)}}) << ";\n";
    actionStream << printSlipperyMovementGuard(a, "NorthEast", 1, {    a+"CannotSlipNorthEast",     a+"CannotSlipEast",     a+"CannotSlipSouthEast",     a+"CannotSlipSouth"}) << printSlipperyMovementUpdate(a, "", {{1, "true"}}) << ";\n";

    actionStream << printSlipperyMovementGuard(a, "NorthEast", 2, {"!"+a+"CannotSlipWest", "!"+a+"CannotSlipNorthWest", "!"+a+"CannotSlipNorth", "!"+a+"CannotSlipNorthEast"}) << printSlipperyMovementUpdate(a, "", {{pi3, westUpdate(a)}, {pd3, northUpdate(a)+"&"+westUpdate(a)}, {pd3, northUpdate(a)}, {pd3, northUpdate(a)+"&"+eastUpdate(a)}}) << ";\n";
    actionStream << printSlipperyMovementGuard(a, "NorthEast", 2, {"!"+a+"CannotSlipWest", "!"+a+"CannotSlipNorthWest", "!"+a+"CannotSlipNorth",     a+"CannotSlipNorthEast"}) << printSlipperyMovementUpdate(a, "", {{pi2, westUpdate(a)}, {pd2, northUpdate(a)+"&"+westUpdate(a)}, {pd2, northUpdate(a)}}) << ";\n";
    actionStream << printSlipperyMovementGuard(a, "NorthEast", 2, {"!"+a+"CannotSlipWest", "!"+a+"CannotSlipNorthWest",     a+"CannotSlipNorth", "!"+a+"CannotSlipNorthEast"}) << printSlipperyMovementUpdate(a, "", {{pi2, westUpdate(a)}, {pd2, northUpdate(a)+"&"+westUpdate(a)}, {pd2, northUpdate(a)+"&"+eastUpdate(a)}}) << ";\n";
    actionStream << printSlipperyMovementGuard(a, "NorthEast", 2, {"!"+a+"CannotSlipWest", "!"+a+"CannotSlipNorthWest",     a+"CannotSlipNorth",     a+"CannotSlipNorthEast"}) << printSlipperyMovementUpdate(a, "", {{pi1, westUpdate(a)}, {pd1, northUpdate(a)+"&"+westUpdate(a)}}) << ";\n";
    actionStream << printSlipperyMovementGuard(a, "NorthEast", 2, {"!"+a+"CannotSlipWest",     a+"CannotSlipNorthWest", "!"+a+"CannotSlipNorth", "!"+a+"CannotSlipNorthEast"}) << printSlipperyMovementUpdate(a, "", {{pi2, westUpdate(a)}, {pd2, northUpdate(a)}, {pd2, northUpdate(a)+"&"+eastUpdate(a)}}) << ";\n";
    actionStream << printSlipperyMovementGuard(a, "NorthEast", 2, {"!"+a+"CannotSlipWest",     a+"CannotSlipNorthWest", "!"+a+"CannotSlipNorth",     a+"CannotSlipNorthEast"}) << printSlipperyMovementUpdate(a, "", {{pi1, westUpdate(a)}, {pd1, northUpdate(a)}}) << ";\n";
    actionStream << printSlipperyMovementGuard(a, "NorthEast", 2, {"!"+a+"CannotSlipWest",     a+"CannotSlipNorthWest",     a+"CannotSlipNorth", "!"+a+"CannotSlipNorthEast"}) << printSlipperyMovementUpdate(a, "", {{pi1, westUpdate(a)}, {pd1, northUpdate(a)+"&"+eastUpdate(a)}}) << ";\n";
    actionStream << printSlipperyMovementGuard(a, "NorthEast", 2, {"!"+a+"CannotSlipWest",     a+"CannotSlipNorthWest",     a+"CannotSlipNorth",     a+"CannotSlipNorthEast"}) << printSlipperyMovementUpdate(a, "", {{1, westUpdate(a)}}) << ";\n";
    actionStream << printSlipperyMovementGuard(a, "NorthEast", 2, {    a+"CannotSlipWest", "!"+a+"CannotSlipNorthWest", "!"+a+"CannotSlipNorth", "!"+a+"CannotSlipNorthEast"}) << printSlipperyMovementUpdate(a, "", {{1/3.f, northUpdate(a)+"&"+westUpdate(a)}, {1/3.f, northUpdate(a)}, {1/3.f, northUpdate(a)+"&"+eastUpdate(a)}}) << ";\n";
    actionStream << printSlipperyMovementGuard(a, "NorthEast", 2, {    a+"CannotSlipWest", "!"+a+"CannotSlipNorthWest", "!"+a+"CannotSlipNorth",     a+"CannotSlipNorthEast"}) << printSlipperyMovementUpdate(a, "", {{1/2.f, northUpdate(a)+"&"+westUpdate(a)}, {1/2.f, northUpdate(a)}}) << ";\n";
    actionStream << printSlipperyMovementGuard(a, "NorthEast", 2, {    a+"CannotSlipWest", "!"+a+"CannotSlipNorthWest",     a+"CannotSlipNorth", "!"+a+"CannotSlipNorthEast"}) << printSlipperyMovementUpdate(a, "", {{1/2.f, northUpdate(a)+"&"+westUpdate(a)}, {1/2.f, northUpdate(a)+"&"+eastUpdate(a)}}) << ";\n";
    actionStream << printSlipperyMovementGuard(a, "NorthEast", 2, {    a+"CannotSlipWest", "!"+a+"CannotSlipNorthWest",     a+"CannotSlipNorth",     a+"CannotSlipNorthEast"}) << printSlipperyMovementUpdate(a, "", {{1, northUpdate(a)+"&"+westUpdate(a)}}) << ";\n";
    actionStream << printSlipperyMovementGuard(a, "NorthEast", 2, {    a+"CannotSlipWest",     a+"CannotSlipNorthWest", "!"+a+"CannotSlipNorth", "!"+a+"CannotSlipNorthEast"}) << printSlipperyMovementUpdate(a, "", {{1/2.f, northUpdate(a)}, {1/2.f, northUpdate(a)+"&"+eastUpdate(a)}}) << ";\n";
    actionStream << printSlipperyMovementGuard(a, "NorthEast", 2, {    a+"CannotSlipWest",     a+"CannotSlipNorthWest", "!"+a+"CannotSlipNorth",     a+"CannotSlipNorthEast"}) << printSlipperyMovementUpdate(a, "", {{1, northUpdate(a)}}) << ";\n";
    actionStream << printSlipperyMovementGuard(a, "NorthEast", 2, {    a+"CannotSlipWest",     a+"CannotSlipNorthWest",     a+"CannotSlipNorth", "!"+a+"CannotSlipNorthEast"}) << printSlipperyMovementUpdate(a, "", {{1, northUpdate(a)+"&"+eastUpdate(a)}}) << ";\n";
    actionStream << printSlipperyMovementGuard(a, "NorthEast", 2, {    a+"CannotSlipWest",     a+"CannotSlipNorthWest",     a+"CannotSlipNorth",     a+"CannotSlipNorthEast"}) << printSlipperyMovementUpdate(a, "", {{1, "true"}}) << ";\n";
  }

  void PrismModulesPrinter::printSlipperyTurnActionsForNorthEast(const AgentName &a) {
    actionStream << printSlipperyTurnGuard(a, "right", "NorthEast", RIGHT, {},  "true")     << printSlipperyTurnUpdate(a, { {1, "(view"+a+"'=mod(view"+a+"+1,4))"} }) << ";\n";
    actionStream << printSlipperyTurnGuard(a, "left", "NorthEast", LEFT, {}, "view"+a+">0") << printSlipperyTurnUpdate(a, { {1, "(view"+a+"'=view"+a+"-1)"} }) << ";\n";
    actionStream << printSlipperyTurnGuard(a, "left", "NorthEast", LEFT, {}, "view"+a+"=0") << printSlipperyTurnUpdate(a, { {1, "(view"+a+"'=3)"} }) << ";\n";

  }

  void PrismModulesPrinter::printSlipperyMovementActionsForSouthWest(const AgentName &a){ throw std::logic_error("The logic for SlipperySouthWest tiles is not yet implemented."); }
  void PrismModulesPrinter::printSlipperyTurnActionsForSouthWest(const AgentName &a){ throw std::logic_error("The logic for SlipperySouthWest tiles is not yet implemented."); }
  void PrismModulesPrinter::printSlipperyMovementActionsForSouthEast(const AgentName &a){ throw std::logic_error("The logic for SlipperySouthEast tiles is not yet implemented."); }
  void PrismModulesPrinter::printSlipperyTurnActionsForSouthEast(const AgentName &a){ throw std::logic_error("The logic for SlipperySouthEast tiles is not yet implemented."); }


  std::string PrismModulesPrinter::printSlipperyMovementGuard(const AgentName &a, const std::string &direction, const ViewDirection &viewDirection, const std::vector<std::string> &guards) {
    std::string actionName = "[" + a + "_move_" + viewDirectionToString.at(viewDirection) + "]";
    agentNameActionMap.at(a).insert({FORWARD, actionName});
    return "  " + actionName + " " + viewVariable(a, viewDirection) + " & " + a + "IsOnSlippery" + direction + " & " + buildConjunction(a, guards) + " -> ";
  }

  std::string PrismModulesPrinter::printSlipperyMovementUpdate(const AgentName &a, const std::string &direction, const updates &u) const {
    return updatesToString(u);
  }

  std::string PrismModulesPrinter::printSlipperyTurnGuard(const AgentName &a, const std::string &direction, const std::string &tiltDirection, const ActionId &actionId, const std::vector<std::string> &guards, const std::string &cond) {
    std::string actionName = "[" + a + "_turn_" + direction + "]";
    agentNameActionMap.at(a).insert({actionId, actionName});
    return "  " + actionName + " " + a + "IsOnSlippery" + tiltDirection + " & " + buildConjunction(a, guards) + " & " + cond + " -> ";
  }

  std::string PrismModulesPrinter::printSlipperyTurnUpdate(const AgentName &a, const updates &u) {
    return updatesToString(u);
  }

  void PrismModulesPrinter::printFaultyMovementModule(const AgentName &a) {
    os << "\nmodule " << a << "FaultyBehaviour" << std::endl;
    os << "  previousAction" << a << " : [0.." + std::to_string(NOFAULT) + "];\n";

    std::set<size_t> exclude = {PICKUP, DROP, TOGGLE, DONE};
    for(const auto [actionId, actionName] : agentNameActionMap.at(a)) {
      if(exclude.count(actionId) > 0) continue;
      os << "  " << actionName << faultyBehaviourGuard(a, actionId) << " -> " << faultyBehaviourUpdate(a, actionId) << ";\n";
    }
    os << "endmodule\n\n";
  }

  void PrismModulesPrinter::printMoveModule() {
    os << "\nmodule " << "Arbiter" << std::endl;
    os << "  clock : [0.." << agentIndexMap.size() - 1 << "];\n";

    for(const auto [agentName, actions] : agentNameActionMap) {
      for(const auto [actionId, actionName] : actions) {
        os << "  " << actionName << " " << moveGuard(agentName) << " -> " << moveUpdate(agentName) << ";\n";
      }
    }
    os << "endmodule\n\n";
  }

  void PrismModulesPrinter::printConfiguredActions(const AgentName &agentName) {
    for (auto& config : configuration) {
      if (config.type_ == ConfigType::Module && !config.overwrite_ && agentName == config.module_) {
        os << config.expression_ ;
      }
    }

    os << "\n";
  }

  void PrismModulesPrinter::printDoneActions(const AgentName &agentName) {
    os << "  [" << agentName << "_on_goal]" << agentName << "IsOnGoal & clock=0 -> true;\n";
  }

  void PrismModulesPrinter::printPlayerStruct(const AgentName &agentName) {
    os << "player " << agentName << "\n\t";
    bool first = true;
    for(const auto [actionId, actionName] : agentNameActionMap.at(agentName)) {
      if(first) first = false;
      else os << ", ";
      os << actionName;
    }
    if(agentName == "Agent" && anyGoals) os << ", [Agent_on_goal]";
    os << "\nendplayer\n";
  }

  void PrismModulesPrinter::printRewards(const AgentName &agentName, const std::map<coordinates, float> &stateRewards, const cells &lava, const cells &goals, const std::map<Color, cells> &backgroundTiles) {
    if(lava.size() != 0) {
      os << "rewards \"" << agentName << "SafetyNoBFS\"\n";
      os << "\t" <<agentName << "IsInLavaAndNotDone: -100;\n";
      os << "endrewards\n";
    }

    if (!goals.empty() || !lava.empty())  {
      os << "rewards \"" << agentName << "SafetyNoBFSAndGoal\"\n";
      if(goals.size() != 0) os << "\t" << agentName << "IsInGoalAndNotDone:  100;\n";
      if(lava.size() != 0) os << "\t" << agentName << "IsInLavaAndNotDone: -100;\n";
      os << "endrewards\n";
    }

    os << "rewards \"" << agentName << "Time\"\n";
    os << "\t!" << agentName << "IsInGoal : -1;\n";
    if(goals.size() != 0) os << "\t" << agentName << "IsInGoalAndNotDone:  100;\n";
    if(lava.size() != 0) os << "\t" << agentName << "IsInLavaAndNotDone: -100;\n";
    os << "endrewards\n";

    if(stateRewards.size() > 0) {
      os << "rewards \"" << agentName << "SafetyWithBFS\"\n";
      if(lava.size() != 0) os << "\t" << agentName << "IsInLavaAndNotDone: -100;\n";
      for(auto const [coordinates, reward] : stateRewards) {
        os << "\txAgent=" << coordinates.first << "&yAgent=" << coordinates.second << " : " << reward << ";\n";
      }
      os << "endrewards\n";
    }
    if(stateRewards.size() > 0) {
      os << "rewards \"" << agentName << "SafetyWithBFSAndGoal\"\n";
      if(goals.size() != 0) os << "\tAgentIsInGoalAndNotDone:  100;\n";
      if(lava.size() != 0) os << "\tAgentIsInLavaAndNotDone: -100;\n";
      for(auto const [coordinates, reward] : stateRewards) {
        os << "\txAgent=" << coordinates.first << "&yAgent=" << coordinates.second << " : " << reward << ";\n";
      }
      os << "endrewards\n";
    }

    for(auto const entry : backgroundTiles)
    {
      std::cout << getColor(entry.first) << " ";
      for(auto const cell : entry.second){
        std::cout << cell.getCoordinates().first << " " << cell.getCoordinates().second << std::endl;
      }
    }
    if(backgroundTiles.size() > 0) {
      os << "rewards \"TaxiReward\"\n";
      os << "\t!AgentIsInGoal : -1;\n";
      std::string allPassengersPickedUp = "";
      bool first = true;
      for(auto const [color, cells] : backgroundTiles) {
        if(cells.size() == 0) continue;
        if(first) first = false; else allPassengersPickedUp += "&";
        std::string c = getColor(color);
        c.at(0) = std::toupper(c.at(0));
        std::string visitedLabel = agentName + "_picked_up_" + c;
        allPassengersPickedUp += visitedLabel;
        os << "[" << agentName << "_pickup_" << c << "] true : 100;\n";
      }
      if(goals.size() != 0) os << "\tAgentIsInGoalAndNotDone & " << allPassengersPickedUp << " :  100;\n";
      if(goals.size() != 0) os << "\tAgentIsInGoalAndNotDone & !(" << allPassengersPickedUp << ") :  -100;\n";
      os << "endrewards";
    }
  }

  std::string PrismModulesPrinter::faultyBehaviourGuard(const AgentName &agentName, const ActionId &actionId) const {
    if(faultyBehaviour()) {
      if(actionId == NOFAULT) {
        return "(previousAction" + agentName + "=" + std::to_string(NOFAULT) + ") ";
      } else {
        return "(previousAction" + agentName + "=" + std::to_string(NOFAULT) + " | previousAction" + agentName + "=" + std::to_string(actionId) + ") ";
      }
    } else {
      return "";
    }
  }

  std::string PrismModulesPrinter::faultyBehaviourUpdate(const AgentName &agentName, const ActionId &actionId) const {
    if(actionId != NOFAULT) {
      return updatesToString({ {1 - faultyProbability, "(previousAction" + agentName + "'=" + std::to_string(NOFAULT) + ")"},  {faultyProbability, "(previousAction" + agentName + "'=" + std::to_string(actionId) + ")" } });
    } else {
      return "true";
    }
  }

  std::string PrismModulesPrinter::moveGuard(const AgentName &agentName) const {
    return "clock=" + std::to_string(agentIndexMap.at(agentName));
  }

  std::string PrismModulesPrinter::moveUpdate(const AgentName &agentName) const {
    size_t agentIndex = agentIndexMap.at(agentName);
    return (agentIndex == numberOfPlayer - 1) ? "(clock'=0) " : "(clock'=" + std::to_string(agentIndex + 1) + ") ";

  }

  std::string PrismModulesPrinter::updatesToString(const updates &updates) const {
    if(updates.empty()) return "true";
    std::string updatesString = "";
    bool first = true;
    for(auto const update : updates) {
      if(first) first = false;
      else updatesString += " + ";
      updatesString += updateToString(update);
    }
    return updatesString;
  }

  std::string PrismModulesPrinter::updateToString(const update &u) const {
    return std::to_string(u.first) + ": " + u.second;
  }

  std::string PrismModulesPrinter::viewVariable(const AgentName &agentName, const size_t &agentDirection) const {
    return "view" + agentName + "=" + std::to_string(agentDirection);
  }

  bool PrismModulesPrinter::anyPortableObject() const {
    return !keys.empty();
  }

  bool PrismModulesPrinter::faultyBehaviour() const {
    return faultyProbability > 0.0f;
  }

  bool PrismModulesPrinter::slipperyBehaviour() const {
    return !slipperyTiles.at("North").empty() || !slipperyTiles.at("East").empty() || !slipperyTiles.at("South").empty() || !slipperyTiles.at("West").empty();
  }

  bool PrismModulesPrinter::isGame() const {
    return modelType == ModelType::SMG;
  }

  std::string PrismModulesPrinter::buildConjunction(const AgentName &a, std::vector<std::string> formulae) const {
    if(formulae.empty()) return "true";
    std::string conjunction = "";
    bool first = true;
    for(auto const formula : formulae) {
      if(first) first = false;
      else conjunction += " & ";
      conjunction += formula;
    }
    return conjunction;
  }

}
