#pragma once

#include <iostream>
#include <utility>
#include <vector>

typedef std::pair<int, int> coordinates;

class Grid;

enum class Type : char {
  Wall       = 'W',
  Floor      = ' ',
  Door       = 'D',
  LockedDoor = 'L',
  Key        = 'K',
  Ball       = 'A',
  Box        = 'B',
  Goal       = 'G',
  Lava       = 'V',
  Agent      = 'X',
  Adversary  = 'Z',
  SlipperyNorth = 'n',
  SlipperySouth = 's',
  SlipperyEast  = 'e',
  SlipperyWest  = 'w',
  SlipperyNorthWest = 'a',
  SlipperyNorthEast = 'b',
  SlipperySouthWest = 'c',
  SlipperySouthEast = 'd'
};
enum class Color : char {
  Red    = 'R',
  Green  = 'G',
  Blue   = 'B',
  Purple = 'P',
  Yellow = 'Y',
  White  = 'W',
  //Grey   = 'G',
  None   = ' '
};

constexpr std::initializer_list<Color> allColors = {Color::Red, Color::Green, Color::Blue, Color::Purple, Color::Yellow};
std::string getColor(Color color);

class cell {
  public:
    coordinates getNorth() const { return std::make_pair(column, row - 1); }
    coordinates getSouth() const { return std::make_pair(column, row + 1); }
    coordinates getEast()  const { return std::make_pair(column + 1, row); }
    coordinates getWest()  const { return std::make_pair(column - 1, row); }

    cell getNorth(const std::vector<cell> &grid) const;
    cell getEast(const std::vector<cell> &grid) const;
    cell getSouth(const std::vector<cell> &grid) const;
    cell getWest(const std::vector<cell> &grid) const;

    friend std::ostream& operator<<(std::ostream& os, const cell& cell);

    coordinates getCoordinates() const;
    std::string getColor() const;
    std::string getType() const;

    int row;
    int column;
    Type type;
    Color color;
};
