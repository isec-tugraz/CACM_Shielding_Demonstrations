#include "Grid.h"
#include <boost/algorithm/string/find.hpp>

#include <algorithm>

Grid::Grid(cells gridCells, cells background, const std::map<coordinates, float> &stateRewards, const float probIntended, const float faultyProbability)
  : allGridCells(gridCells), background(background), stateRewards(stateRewards), probIntended(probIntended), faultyProbability(faultyProbability)
{
  cell max = allGridCells.at(allGridCells.size() - 1);
  maxBoundaries = std::make_pair(max.column - 1, max.row - 1);
  std::copy_if(gridCells.begin(),  gridCells.end(),  std::back_inserter(walls),         [](cell c) { return c.type == Type::Wall; });
  std::copy_if(gridCells.begin(),  gridCells.end(),  std::back_inserter(lava),          [](cell c) { return c.type == Type::Lava; });
  std::copy_if(gridCells.begin(),  gridCells.end(),  std::back_inserter(floor),         [](cell c) { return c.type == Type::Floor; }); // TODO CHECK IF ALL AGENTS ARE ADDED TO FLOOR
  std::copy_if(background.begin(), background.end(), std::back_inserter(slipperyNorth), [](cell c) { return c.type == Type::SlipperyNorth; });
  std::copy_if(background.begin(), background.end(), std::back_inserter(slipperyEast),  [](cell c) { return c.type == Type::SlipperyEast; });
  std::copy_if(background.begin(), background.end(), std::back_inserter(slipperySouth), [](cell c) { return c.type == Type::SlipperySouth; });
  std::copy_if(background.begin(), background.end(), std::back_inserter(slipperyWest),  [](cell c) { return c.type == Type::SlipperyWest; });
  std::copy_if(background.begin(), background.end(), std::back_inserter(slipperyNorthWest), [](cell c) { return c.type == Type::SlipperyNorthWest; });
  std::copy_if(background.begin(), background.end(), std::back_inserter(slipperyNorthEast), [](cell c) { return c.type == Type::SlipperyNorthEast; });
  std::copy_if(background.begin(), background.end(), std::back_inserter(slipperySouthWest), [](cell c) { return c.type == Type::SlipperySouthWest; });
  std::copy_if(background.begin(), background.end(), std::back_inserter(slipperySouthEast), [](cell c) { return c.type == Type::SlipperySouthEast; });
  std::copy_if(gridCells.begin(),  gridCells.end(),  std::back_inserter(lockedDoors),   [](cell c) { return c.type == Type::LockedDoor; });
  std::copy_if(gridCells.begin(),  gridCells.end(),  std::back_inserter(unlockedDoors), [](cell c) { return c.type == Type::Door; });
  std::copy_if(gridCells.begin(),  gridCells.end(),  std::back_inserter(goals),         [](cell c) { return c.type == Type::Goal; });
  std::copy_if(gridCells.begin(),  gridCells.end(),  std::back_inserter(keys),          [](cell c) { return c.type == Type::Key; });
  std::copy_if(gridCells.begin(),  gridCells.end(),  std::back_inserter(boxes),         [](cell c) { return c.type == Type::Box; });
  std::copy_if(gridCells.begin(),  gridCells.end(),  std::back_inserter(balls),         [](cell c) { return c.type == Type::Ball; });
  std::copy_if(gridCells.begin(),  gridCells.end(),  std::back_inserter(adversaries),   [](cell c) { return c.type == Type::Adversary; });
  agent = *std::find_if(gridCells.begin(), gridCells.end(), [](cell c) { return c.type == Type::Agent; });
  floor.push_back(agent);

  agentNameAndPositionMap.insert({ "Agent", agent.getCoordinates() });
  for(auto const& adversary : adversaries) {
    std::string color = adversary.getColor();
    color.at(0) = std::toupper(color.at(0));
    try {
      auto success = agentNameAndPositionMap.insert({ color, adversary.getCoordinates() });
      floor.push_back(adversary);
      if(!success.second) {
        throw std::logic_error("Agent with " + color + " already present\n");
      }
    } catch(const std::logic_error& e) {
      std::cerr << "Expected agents colors to be different. Agent with color : '" << color << "' already present." << std::endl;
      throw;
    }
  }
  for(auto const& key : keys) {
    std::string color = key.getColor();
    try {
      auto success = keyNameAndPositionMap.insert({color, key.getCoordinates() });
      if (!success.second) {
        throw std::logic_error("Multiple keys with same color not supported " + color + "\n");
      }
    } catch(const std::logic_error& e) {
      std::cerr << "Expected key colors to be different. Key with color : '" << color << "' already present." << std::endl;
      throw;
    }
  }
  for(auto const& color : allColors) {
    cells cellsOfColor;
    std::copy_if(background.begin(), background.end(), std::back_inserter(cellsOfColor), [&color](cell c) {
        return c.type == Type::Floor && c.color == color;
    });
    if(cellsOfColor.size() > 0) {
      backgroundTiles.emplace(color, cellsOfColor);
    }
  }
  
  if (adversaries.empty()) {
    modelType = prism::ModelType::MDP;
  } else {
    modelType = prism::ModelType::SMG;
  }
}

std::ostream& operator<<(std::ostream& os, const Grid& grid) {
  int lastRow = 1;
  for(auto const& cell : grid.allGridCells) {
    if(lastRow != cell.row)
      os << std::endl;
    os << static_cast<char>(cell.type) << static_cast<char>(cell.color);
    lastRow = cell.row;
  }
  return os;
}

cells Grid::getGridCells() {
  return allGridCells;
}

bool Grid::isBlocked(coordinates p) {
  return isWall(p);
}

bool Grid::isWall(coordinates p) {
  return std::find_if(walls.begin(), walls.end(),
      [p](cell cell) {
        return cell.row == p.second && cell.column == p.first;
      }) != walls.end();
}


void Grid::applyOverwrites(std::string& str, std::vector<Configuration>& configuration) {
  for (auto& config : configuration) {
    if (!config.overwrite_) {
      continue;
    }
      for (auto& index : config.indexes_) {
        size_t start_pos;
        std::string search;

        if (config.type_ == ConfigType::Formula) {
          search = "formula " + config.identifier_;
        } else if (config.type_ == ConfigType::Label) {
          search = "label " + config.identifier_;
        } else if (config.type_ == ConfigType::Module) {
          search = config.identifier_;
        } else if (config.type_ == ConfigType::UpdateOnly) {
          search = config.identifier_;
        } else if (config.type_ == ConfigType::GuardOnly) {
          search = config.identifier_;
        }
        else if (config.type_ == ConfigType::Constant) {
          search = config.identifier_;
        }

        auto iter = boost::find_nth(str, search, index);
        auto end_identifier = config.end_identifier_;

        start_pos = std::distance(str.begin(), iter.begin());
        size_t end_pos = str.find(end_identifier, start_pos);

        if (config.type_ == ConfigType::GuardOnly || config.type_ == ConfigType::Module) {
          start_pos += search.length();
        } else if (config.type_ == ConfigType::UpdateOnly) {
          start_pos = str.find("->", start_pos) + 2;
        }

        if (end_pos != std::string::npos && end_pos != 0) {
          std::string expression = config.expression_;
          str.replace(start_pos, end_pos - start_pos , expression);
        }
      }
  }
}
void Grid::printToPrism(std::ostream& os, std::vector<Configuration>& configuration) {
  cells northRestriction, eastRestriction, southRestriction, westRestriction;
  cells walkable = floor;
  walkable.insert(walkable.end(), goals.begin(), goals.end());
  walkable.insert(walkable.end(), boxes.begin(), boxes.end());
  walkable.insert(walkable.end(), lava.begin(), lava.end());
  walkable.insert(walkable.end(), keys.begin(), keys.end());
  walkable.insert(walkable.end(), balls.begin(), balls.end());

  for(auto const& c : walkable) {
    if(isWall(c.getNorth())) northRestriction.push_back(c);
    if(isWall(c.getEast()))   eastRestriction.push_back(c);
    if(isWall(c.getSouth())) southRestriction.push_back(c);
    if(isWall(c.getWest()))   westRestriction.push_back(c);
  }


  std::map<std::string, cells> wallRestrictions = {{"North", northRestriction}, {"East", eastRestriction}, {"South", southRestriction}, {"West", westRestriction}};
  std::map<std::string, cells> slipperyTiles    = {{"North", slipperyNorth}, {"East", slipperyEast}, {"South", slipperySouth}, {"West", slipperyWest}, {"NorthWest", slipperyNorthWest}, {"NorthEast", slipperyNorthEast},{"SouthWest", slipperySouthWest},{"SouthEast", slipperySouthEast}};

  std::vector<AgentName> agentNames;
  std::transform(agentNameAndPositionMap.begin(),
                 agentNameAndPositionMap.end(),
                 std::back_inserter(agentNames),
                 [](const std::map<AgentNameAndPosition::first_type,AgentNameAndPosition::second_type>::value_type &pair){return pair.first;});
  std::string agentName = agentNames.at(0);

  prism::PrismFormulaPrinter formulas(os, wallRestrictions, walls, lockedDoors, unlockedDoors, keys, slipperyTiles, lava, goals, agentNameAndPositionMap, faultyProbability > 0.0);
  prism::PrismModulesPrinter modules(os, modelType, maxBoundaries, lockedDoors, unlockedDoors, keys, slipperyTiles, agentNameAndPositionMap, configuration, probIntended, faultyProbability, !lava.empty(), !goals.empty());

  modules.printModelType(modelType);
  for(const auto &agentName : agentNames) {
    formulas.print(agentName);
  }
  if(agentNameAndPositionMap.size() > 1) formulas.printCollisionFormula(agentName);
  formulas.printInitStruct();

  modules.print();




  //if(!stateRewards.empty()) {
  //  modules.printRewards(os, agentName, stateRewards, lava, goals, backgroundTiles);
  //}

  //if (!configuration.empty()) {
  //  modules.printConfiguration(os, configuration);
  //}
}

void Grid::setModelType(prism::ModelType type)
{
  modelType = type;
}