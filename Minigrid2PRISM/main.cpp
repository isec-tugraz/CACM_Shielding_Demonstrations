#include "util/OptionParser.h"
#include "util/MinigridGrammar.h"
#include "util/Grid.h"
#include "util/ConfigYaml.h"

#include <iostream>
#include <fstream>
#include <filesystem>
#include <sstream>


std::vector<std::string> parseCommaSeparatedString(std::string const& str) {
  std::vector<std::string> result;
  std::stringstream stream(str);
  while(stream.good()) {
    std::string substr;
    getline(stream, substr, ',');
    substr.at(0) = std::toupper(substr.at(0));
    result.push_back(substr);
  }
  return result;
}


struct printer {
  typedef boost::spirit::utf8_string string;

  void element(string const& tag, string const& value, int depth) const {
    for (int i = 0; i < (depth*4); ++i) std::cout << ' ';

    std::cout << "tag: " << tag;
    if (value != "") std::cout << ", value: " << value;
    std::cout << std::endl;
  }
};

void print_info(boost::spirit::info const& what) {
  using boost::spirit::basic_info_walker;

  printer pr;
  basic_info_walker<printer> walker(pr, what.tag, 0);
  boost::apply_visitor(walker, what.value);
}

void setProbability(const std::string& gridProperties, const std::vector<Property> configProperties, const std::string& identifier, float& prop) {
  auto start_pos = gridProperties.find(identifier);
  std::string seperator = ";";

  if (start_pos != std::string::npos) {
    auto end_pos = gridProperties.find('\n', start_pos);
    auto value = gridProperties.substr(start_pos + identifier.length()  + seperator.size(), end_pos - start_pos - identifier.length());
    prop = std::stod(value);
  }

  auto yaml_config_prop = std::find_if(configProperties.begin(), configProperties.end(), [&identifier](const Property&  obj) -> bool {return obj.property == identifier;} );

  if (yaml_config_prop != configProperties.end()) {
    prop = (*yaml_config_prop).value_;
  }
}


int main(int argc, char* argv[]) {
  popl::OptionParser optionParser("Allowed options");

  auto helpOption = optionParser.add<popl::Switch>("h", "help", "Print this help message.");
  auto inputFilename = optionParser.add<popl::Value<std::string>>("i", "input-file", "Filename of the input file.");
  auto outputFilename = optionParser.add<popl::Value<std::string>>("o", "output-file", "Filename for the output file.");
  auto configFilename = optionParser.add<popl::Value<std::string>, popl::Attribute::optional>("c", "config-file", "Filename of the predicate configuration file.");


  try {
    optionParser.parse(argc, argv);

    if(helpOption->count() > 0) {
      std::cout << optionParser << std::endl;
      return EXIT_SUCCESS;
    }
  } catch (const popl::invalid_option &e) {
    return io::printPoplException(e);
  } catch (const std::exception &e) {
		std::cerr << "Exception: " << e.what() << "\n";
		return EXIT_FAILURE;
	}

  std::fstream file {outputFilename->value(0), file.trunc | file.out};
  std::fstream infile {inputFilename->value(0), infile.in};
  std::string line, content, background, rewards, properties;
  std::cout << "\n";
  bool parsingBackground = false;
  bool parsingStateRewards = false;
  bool parsingEnvironmentProperties = false;
  while (std::getline(infile, line) && !line.empty()) {
    if(line.at(0) == '-' && line.at(line.size() - 1) == '-' && parsingBackground) {
      parsingStateRewards = true;
      parsingBackground = false;
      continue;
    } else if (line.at(0) == '-' && line.at(line.size() - 1 ) == '-' && parsingStateRewards) {
      parsingStateRewards = false;
      parsingEnvironmentProperties = true;
      continue;
    } else if(line.at(0) == '-' && line.at(line.size() - 1) == '-') {
      parsingBackground = true;
      continue;
    }
    if(!parsingBackground && !parsingStateRewards && !parsingEnvironmentProperties) {
      content += line + "\n";
    } else if (parsingBackground) {
      background += line + "\n";
    } else if(parsingStateRewards) {
      rewards += line + "\n";
    } else if (parsingEnvironmentProperties) {
      properties += line + "\n";
    }
  }
  std::cout << "\n";

  pos_iterator_t contentFirst(content.begin());
  pos_iterator_t contentIter = contentFirst;
  pos_iterator_t contentLast(content.end());
  MinigridParser<pos_iterator_t> contentParser(contentFirst);
  pos_iterator_t backgroundFirst(background.begin());
  pos_iterator_t backgroundIter = backgroundFirst;
  pos_iterator_t backgroundLast(background.end());
  MinigridParser<pos_iterator_t> backgroundParser(backgroundFirst);

  cells contentCells;
  cells backgroundCells;
  std::vector<Configuration> configurations;
  std::vector<Property> parsed_properties;
  std::map<coordinates, float> stateRewards;
  float faultyProbability = 0.0;
  float probIntended = 1.0;
  float probTurnIntended = 1.0;

  try {
    bool ok = phrase_parse(contentIter, contentLast, contentParser, qi::space, contentCells);
    // TODO if(background is not empty) {
    ok     &= phrase_parse(backgroundIter, backgroundLast, backgroundParser, qi::space, backgroundCells);
    // TODO }
    if (configFilename->is_set()) {
      YamlConfigParser parser(configFilename->value(0));
      auto parseResult = parser.parseConfiguration();
      configurations = parseResult.configurations_;
      parsed_properties = parseResult.properties_;
    }

    boost::escaped_list_separator<char> seps('\\', ';', '\n');
    Tokenizer csvParser(rewards, seps);
    for(auto iter = csvParser.begin(); iter != csvParser.end(); ++iter) {
      int x = std::stoi(*iter);
      int y = std::stoi(*(++iter));
      float reward = std::stof(*(++iter));
      stateRewards[std::make_pair(x,y)] = reward;
    }
    if (!properties.empty()) {
      auto faultProbabilityIdentifier = std::string("FaultProbability");
      auto probForwardIntendedIdentifier = std::string("ProbForwardIntended");
      auto probTurnIntendedIdentifier = std::string("ProbTurnIntended");

      setProbability(properties, parsed_properties, faultProbabilityIdentifier, faultyProbability);
      setProbability(properties, parsed_properties, probForwardIntendedIdentifier, probIntended);
      setProbability(properties, parsed_properties, probTurnIntendedIdentifier, probTurnIntended);
    }
    if(ok) {
      Grid grid(contentCells, backgroundCells, stateRewards, probIntended, faultyProbability);

      auto modelTypeIter = std::find_if(parsed_properties.begin(), parsed_properties.end(), [](const Property&  obj) -> bool {return obj.property == "modeltype";});
      prism::ModelType modelType = prism::ModelType::MDP;;
      if (modelTypeIter != parsed_properties.end()) {
        if ((*modelTypeIter).value_str_ == "smg") {
          modelType = prism::ModelType::SMG;
        } else {
          modelType = prism::ModelType::MDP;
        }

        grid.setModelType(modelType);
      }
  


      //grid.printToPrism(std::cout, configurations);
      std::stringstream ss;
      grid.printToPrism(ss, configurations);
      std::string str = ss.str();
      grid.applyOverwrites(str, configurations);
      file << str;
    }
  } catch(qi::expectation_failure<pos_iterator_t> const& e) {
    std::cout << "expected: "; print_info(e.what_);
    std::cout << "got: \"" << std::string(e.first, e.last) << '"' << std::endl;
    std::cout << "Expectation failure: " << e.what() << " at '" << std::string(e.first,e.last) << "'\n";
  } catch(const std::exception& e) {
    std::cerr << "Exception '" << typeid(e).name() << "' caught:" << e.what() << std::endl;
    std::cerr << "\t" << e.what() << std::endl;
    std::exit(EXIT_FAILURE);
  }

  return 0;
}
