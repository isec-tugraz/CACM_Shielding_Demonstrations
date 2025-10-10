#pragma once

#include <string>
#include <map>

#include "cell.h"

#define NOFAULT 7
#define LEFT 0
#define RIGHT 1
#define FORWARD 2
#define PICKUP 3
#define DROP 4
#define TOGGLE 5
#define DONE 6


typedef std::string AgentName;
typedef size_t ViewDirection;
typedef std::pair<std::string, coordinates> AgentNameAndPosition;
typedef AgentNameAndPosition KeyNameAndPosition;
typedef std::map<AgentNameAndPosition::first_type, AgentNameAndPosition::second_type> AgentNameAndPositionMap;
typedef std::map<KeyNameAndPosition::first_type, KeyNameAndPosition::second_type> KeyNameAndPositionMap;
typedef std::pair<cell, std::string> CellAndCondition;
typedef std::pair<float, std::string> update;
typedef std::vector<update> updates;
typedef int8_t ActionId;

std::string capitalize(std::string string);

namespace prism {
  enum class ModelType {
    MDP, SMG
  };
}
