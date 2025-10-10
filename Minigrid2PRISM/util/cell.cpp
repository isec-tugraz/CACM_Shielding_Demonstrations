#include "cell.h"

#include <stdexcept>
#include <algorithm>

std::ostream &operator<<(std::ostream &os, const cell &c) {
  os << static_cast<char>(c.type) << static_cast<char>(c.color);
  os <<  " at (" << c.column << "," << c.row << ")";
  return os;
}

coordinates cell::getCoordinates() const {
  return std::make_pair(column, row);
}

cell cell::getNorth(const std::vector<cell> &grid) const {
  auto north = std::find_if(grid.begin(), grid.end(), [this](const cell &c) {
        return this->row - 1 == c.row && this->column == c.column;
      });
  if(north == grid.end()) {
    throw std::logic_error{ "Cannot get cell north of (" + std::to_string(row) + "," + std::to_string(column) + ")"};
    std::exit(EXIT_FAILURE);
  }
  return *north;
}

cell cell::getEast(const std::vector<cell> &grid) const {
  auto east = std::find_if(grid.begin(), grid.end(), [this](const cell &c) {
        return this->row == c.row && this->column + 1 == c.column;
      });
  if(east == grid.end()) {
    throw std::logic_error{ "Cannot get cell east of (" + std::to_string(row) + "," + std::to_string(column) + ")"};
    std::exit(EXIT_FAILURE);
  }
  return *east;
}

cell cell::getSouth(const std::vector<cell> &grid) const {
  auto south = std::find_if(grid.begin(), grid.end(), [this](const cell &c) {
        return this->row + 1 == c.row && this->column == c.column;
      });
  if(south == grid.end()) {
    throw std::logic_error{ "Cannot get cell south of (" + std::to_string(row) + "," + std::to_string(column) + ")"};
    std::exit(EXIT_FAILURE);
  }
  return *south;
}

cell cell::getWest(const std::vector<cell> &grid) const {
  auto west = std::find_if(grid.begin(), grid.end(), [this](const cell &c) {
        return this->row == c.row && this->column - 1 == c.column;
      });
  if(west == grid.end()) {
    throw std::logic_error{ "Cannot get cell west of (" + std::to_string(row) + "," + std::to_string(column) + ")"};
    std::exit(EXIT_FAILURE);
  }
  return *west;
}

std::string cell::getColor() const {
  switch(color) {
    case Color::Red:    return "red";
    case Color::Green:  return "green";
    case Color::Blue:   return "blue";
    case Color::Purple: return "purple";
    case Color::Yellow: return "yellow";
    case Color::None:   return "transparent";
    default: return "";
    //case Color::Grey   = 'G',
  }
}

std::string cell::getType() const {
  switch(type) {
    case Type::Wall:         return "Wall";
    case Type::Floor:        return "Floor";
    case Type::Door:         return "Door";
    case Type::LockedDoor:    return "LockedDoor";
    case Type::Key:          return "Key";
    case Type::Ball:          return "Ball";
    case Type::Box:           return "Box";
    case Type::Goal:          return "Goal";
    case Type::Lava:          return "Lava";
    case Type::Agent:         return "Agent";
    case Type::Adversary:     return "Adversary";
    case Type::SlipperyNorth: return "SlipperyNorth";
    case Type::SlipperySouth: return "SlipperySouth";
    case Type::SlipperyEast:  return "SlipperyEast";
    case Type::SlipperyWest:  return "SlipperyWest";
    default: return "";
  }
}

std::string getColor(Color color) {
  switch(color) {
    case Color::Red:    return "red";
    case Color::Green:  return "green";
    case Color::Blue:   return "blue";
    case Color::Purple: return "purple";
    case Color::Yellow: return "yellow";
    case Color::None:   return "transparent";
    default: return "";
    //case Color::Grey   = 'G',
  }
}
