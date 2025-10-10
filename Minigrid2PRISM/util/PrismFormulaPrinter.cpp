#include "PrismFormulaPrinter.h"

#include <map>
#include <string>
#include <algorithm>


std::string oneOffToString(const int &offset) {
  return offset != 0 ? ( offset == 1 ? "+1" : "-1" )  : "";
}

std::string vectorToDisjunction(const std::vector<std::string> &formulae) {
  bool first = true;
  std::string disjunction = "";
  for(const auto &formula : formulae) {
    if(first) first = false;
    else disjunction += " | ";
    disjunction += formula;
  }
  return disjunction;
}

std::string cellToConjunction(const AgentName &agentName, const cell &c) {
  return "col" + agentName + "=" + std::to_string(c.column) + "&row" + agentName + "=" + std::to_string(c.row);
}

std::string cellToConjunctionWithOffset(const AgentName &agentName, const cell &c, const std::string &xOffset, const std::string &yOffset){
  return "col" + agentName + xOffset + "=" + std::to_string(c.column) + "&row" + agentName + yOffset + "=" + std::to_string(c.row);
}

std::string coordinatesToConjunction(const AgentName &agentName, const coordinates &c, const ViewDirection viewDirection) {
  return "col" + agentName + "=" + std::to_string(c.first) + "&row" + agentName + "=" + std::to_string(c.second) + "&view" + agentName + "=" + std::to_string(viewDirection);
}

std::string objectPositionToConjunction(const AgentName &agentName, const std::string &identifier, const std::pair<int, int> &relativePosition) {
  std::string xOffset = oneOffToString(relativePosition.first);
  std::string yOffset = oneOffToString(relativePosition.second);
  return "col" + agentName + xOffset + "=col" + identifier + "&row" + agentName + yOffset + "=row" + identifier;
}
std::string objectPositionToConjunction(const AgentName &agentName, const std::string &identifier, const std::pair<int, int> &relativePosition, const ViewDirection viewDirection) {
  std::string xOffset = oneOffToString(relativePosition.first);
  std::string yOffset = oneOffToString(relativePosition.second);
  return "col" + agentName + xOffset + "=col" + identifier + "&row" + agentName + yOffset + "=row" + identifier + "&view" + agentName + "=" + std::to_string(viewDirection);
}

std::map<ViewDirection, coordinates> getAdjacentCells(const cell &c) {
  return {{1, c.getNorth()}, {2, c.getEast()}, {3, c.getSouth()}, {0, c.getWest()}};
}

std::map<ViewDirection, std::pair<int, int>> getRelativeAdjacentCells() {
  return { {1, {0,+1}}, {2, {-1,0}}, {3, {0,-1}}, {0, {+1,0}} };
}

std::map<std::string, std::pair<int, int>> getRelativeSurroundingCells() {
  return { {"NorthWest", {-1,-1}}, {"North", { 0,-1}}, {"NorthEast", {+1,-1}},
           {"West",      {-1, 0}},                     {"East",      {+1, 0}},
           {"SouthWest", {-1,+1}}, {"South", { 0,+1}}, {"SouthEast", {+1,+1}} };
}

namespace prism {
  PrismFormulaPrinter::PrismFormulaPrinter(std::ostream &os, const std::map<std::string, cells> &restrictions, const cells &walls, const cells &lockedDoors, const cells &unlockedDoors, const cells &keys, const std::map<std::string, cells> &slipperyTiles, const cells &lava, const cells &goals, const AgentNameAndPositionMap &agentNameAndPositionMap, const bool faulty)
    : os(os),  restrictions(restrictions), walls(walls), lockedDoors(lockedDoors), unlockedDoors(unlockedDoors), keys(keys), slipperyTiles(slipperyTiles), lava(lava), goals(goals), agentNameAndPositionMap(agentNameAndPositionMap), faulty(faulty)
  { }

  void PrismFormulaPrinter::print(const AgentName &agentName) {
    conditionalMovementRestrictions.clear();
    for(const auto& [direction, cells] : restrictions) {
      printRestrictionFormula(agentName, direction, cells);
    }

    if(slipperyBehaviour()) {
      for(const auto& [direction, cells] : slipperyTiles) {
        printIsOnFormula(agentName, "Slippery", cells, direction);
      }
      std::vector<std::string> allSlipperyDirections;
      for(const auto &[slipperyType, _] : slipperyTiles) {
        allSlipperyDirections.push_back(agentName + "IsOnSlippery" + slipperyType);
      }
      os << buildFormula(agentName + "IsOnSlippery", vectorToDisjunction(allSlipperyDirections));

      for(const auto& [direction, relativePosition] : getRelativeSurroundingCells()) {
        printSlipRestrictionFormula(agentName, direction);
      }
    } else {
      os << buildFormula(agentName + "IsOnSlippery", "false");
    }
    if(!lava.empty())  printIsOnFormula(agentName, "Lava", lava);
    if(!goals.empty()) printIsOnFormula(agentName, "Goal", goals);


    for(const auto& key : keys) {
      std::string identifier = capitalize(key.getColor()) + key.getType();
      printRelativeIsInFrontOfFormulaWithCondition(agentName, identifier, "!" + identifier + "PickedUp");
      portableObjects.push_back(agentName + "Carrying" + identifier);
    }

    for(const auto& door : unlockedDoors) {
      std::string identifier = capitalize(door.getColor()) + door.getType();
      printRestrictionFormulaWithCondition(agentName, identifier, getAdjacentCells(door), "!" + identifier + "Open");
      printIsNextToFormula(agentName, identifier, getAdjacentCells(door));
    }

    for(const auto& door : lockedDoors) {
      std::string identifier = capitalize(door.getColor()) + door.getType();
      printRestrictionFormulaWithCondition(agentName, identifier, getAdjacentCells(door), "!" + identifier + "Open");
      printIsNextToFormula(agentName, identifier, getAdjacentCells(door));
    }

    if(conditionalMovementRestrictions.size() > 0) {
      os << buildFormula(agentName + "CannotMoveConditionally", vectorToDisjunction(conditionalMovementRestrictions));
    }
    if(portableObjects.size() > 0) {
      os << buildFormula(agentName + "IsCarrying", vectorToDisjunction(portableObjects));
    }
  }

  void PrismFormulaPrinter::printRestrictionFormula(const AgentName &agentName, const std::string &direction, const cells &grid_cells) {
    os << buildFormula(agentName + "CannotMove" + direction + "Wall", buildDisjunction(agentName, grid_cells));
  }

  void PrismFormulaPrinter::printIsOnFormula(const AgentName &agentName, const std::string &type, const cells &grid_cells, const std::string &direction) {
    os << buildFormula(agentName + "IsOn" + type + direction, buildDisjunction(agentName, grid_cells));
  }

  void PrismFormulaPrinter::printIsNextToFormula(const AgentName &agentName, const std::string &type, const std::map<ViewDirection, coordinates> &coordinates) {
    os << buildFormula(agentName + "IsNextTo" + type, buildDisjunction(agentName, coordinates));
  }

  void PrismFormulaPrinter::printRestrictionFormulaWithCondition(const AgentName &agentName, const std::string &reason, const std::map<ViewDirection, coordinates> &coordinates, const std::string &condition) {
    os << buildFormula(agentName + "CannotMove" + reason, "(" + buildDisjunction(agentName, coordinates) + ") & " + condition);
    conditionalMovementRestrictions.push_back(agentName + "CannotMove" + reason);
  }

  void PrismFormulaPrinter::printRelativeIsInFrontOfFormulaWithCondition(const AgentName &agentName, const std::string &reason, const std::string &condition) {
    os << buildFormula(agentName + "IsInFrontOf" + reason, "(" + buildDisjunction(agentName, reason) + ") & " + condition);
  }

  void PrismFormulaPrinter::printSlipRestrictionFormula(const AgentName &agentName, const std::string &direction) {
    std::pair<int, int> slipCell = getRelativeSurroundingCells().at(direction);
    bool semicolon = anyPortableObject() ? false : true;
    os << buildFormula(agentName + "CannotSlip" + direction, buildDisjunction(agentName, walls, slipCell), semicolon);
    if(!semicolon) os << ";\n";
  }

  void PrismFormulaPrinter::printCollisionFormula(const AgentName &agentName) {
    if(!agentNameAndPositionMap.empty()) {
      os << "formula collision = ";
      bool first = true;
      for(auto const [name, coordinates] : agentNameAndPositionMap) {
        if(name == agentName) continue;
        if(first) first = false;
        else os << " | ";
        os << "(col"+agentName+"=col"+name+"&row"+agentName+"=row"+name+")";
      }
      os << ";\n";
      printCollisionLabel();
    }
  }

  void PrismFormulaPrinter::printCollisionLabel() {
    if(!agentNameAndPositionMap.empty()) {
      os << "label \"collision\" = collision;\n";
    }
  }

  void PrismFormulaPrinter::printInitStruct() {
    os << "init\n  true\n";
    //bool first = true;
    //for(auto const [a, coordinates] : agentNameAndPositionMap) {
    //  if(first) first = false;
    //  else os << " & ";
    //  os << "(col"+a+"="+std::to_string(coordinates.first)+"&row"+a+"="+std::to_string(coordinates.second)+" & ";
    //  os << "(view"+a+"=0|view"+a+"=1|view"+a+"=2|view"+a+"=3) ";
    //  if(faulty) os << " & previousAction"+a+"="+std::to_string(NOFAULT);
    //  os << ")";
    //}
    //for(auto const key : keys) {
    //  std::string identifier = capitalize(key.getColor()) + key.getType();
    //  os << " & (col"+identifier+"="+std::to_string(key.column)+"&row"+identifier+"="+std::to_string(key.row)+"&"+identifier+"PickedUp=false) ";
    //}
    os << "endinit\n\n";
  }


  std::string PrismFormulaPrinter::buildFormula(const std::string &formulaName, const std::string &formula, const bool semicolon) {
    return "formula " + formulaName + " = " + formula + (semicolon ? ";\n": "");
  }

  std::string PrismFormulaPrinter::buildDisjunction(const AgentName &agentName, const std::map<ViewDirection, coordinates> &cells) {
    if(cells.size() == 0) return "false";
    bool first = true;
    std::string disjunction = "";
    for(const auto [viewDirection, coordinates] : cells) {
      if(first) first = false;
      else disjunction += " | ";
      disjunction += "(" + coordinatesToConjunction(agentName, coordinates, viewDirection) + ")";
    }
    return disjunction;
  }

  std::string PrismFormulaPrinter::buildDisjunction(const AgentName &agentName, const cells &cells) {
    if(cells.size() == 0) return "false";
    bool first = true;
    std::string disjunction = "";
    for(auto const cell : cells) {
      if(first) first = false;
      else disjunction += " | ";
      disjunction += "(" + cellToConjunction(agentName, cell) + ")";
    }
    return disjunction;
  }

  std::string PrismFormulaPrinter::buildDisjunction(const AgentName &agentName, const std::string &reason) {
    std::string disjunction = "";
    bool first = true;
    for(auto const [viewDirection, relativePosition] : getRelativeAdjacentCells()) {
      if(first) first = false;
      else disjunction += " | ";
      disjunction += "(" + objectPositionToConjunction(agentName, reason, relativePosition, viewDirection) + ")";
    }
    return disjunction;
  }

  std::string PrismFormulaPrinter::buildDisjunction(const AgentName &agentName, const cells &cells, const std::pair<int, int> &offset) {
    std::string disjunction = "";
    bool first = true;
    std::string xOffset = oneOffToString(offset.first);
    std::string yOffset = oneOffToString(offset.second);
    for(auto const cell : cells) {
      if(first) first = false;
      else disjunction += " | ";
      disjunction += "(" + cellToConjunctionWithOffset(agentName, cell, xOffset, yOffset) + ")";
    }
    return disjunction;
  }

  bool PrismFormulaPrinter::slipperyBehaviour() const {
    return !slipperyTiles.at("North").empty() || !slipperyTiles.at("East").empty() || !slipperyTiles.at("South").empty() || !slipperyTiles.at("West").empty();
  }
  bool PrismFormulaPrinter::anyPortableObject() const {
    return !keys.empty();
  }
}
