#include "ConfigYaml.h"
#include <iostream>

std::ostream& operator <<(std::ostream &os, const Label& label) {
    os << "\"" << label.label_ << "\"" << "=" << label.text_;
    return os;
}

std::ostream& operator << (std::ostream &os, const Formula& formula) {
    os << formula.formula_ << "=" << formula.content_;
    return os;
}

std::ostream& operator << (std::ostream& os, const Command& command) {
    os << command.action_;
    return os;
}

std::ostream& operator << (std::ostream& os, const Constant& constant) {
    os << "const " << constant.type_ << " " << constant.constant_ << " = " << constant.value_;
    return os;
}

std::ostream& operator << (std::ostream& os, const Module& module) {
    os << "Module: " << module.module_ << std::endl;
    for (auto& command : module.commands_) {
      os << command << std::endl;
    }
    return os;
}

std::string Label::createExpression() const {
    if (overwrite_) {
        return "label \"" + label_ + "\" = " + text_ + Configuration::overwrite_identifier_;
    } 

    return "label \"" + label_ + "\" = " + text_ + Configuration::configuration_identifier_;
}

std::string Formula::createExpression() const {
    if (overwrite_) {
        return "formula " + formula_ + " = " + content_ + Configuration::overwrite_identifier_;
    }

    return "formula " + formula_ + " = " + content_ + Configuration::configuration_identifier_;
}

std::string Command::createExpression() const {
    if (overwrite_) {
        return action_  + "\t" + guard_ + " -> " + update_  + Configuration::overwrite_identifier_;
    }
    
    return "\t" + action_  + "\t" + guard_ + " -> " + update_+ Configuration::configuration_identifier_;
}

std::string Constant::createExpression() const {
    if (overwrite_) {
        return "const " + type_ + " " + constant_ +  " = " + value_  + Configuration::overwrite_identifier_;
    }
    
    return "const " + type_ + " " + constant_ +  " = " + value_  + Configuration::configuration_identifier_;
}

YAML::Node YAML::convert<Module>::encode(const Module& rhs) {
    YAML::Node node;
    
    node.push_back(rhs.module_);
    node.push_back(rhs.commands_);

    return node;
}

bool YAML::convert<Module>::decode(const YAML::Node& node, Module& rhs) {
    if (!node.Type() == NodeType::Map) {
      return false;
    }


    rhs.module_ = node["module"].as<std::string>();
    
    if (node["commands"]) {
        rhs.commands_ = node["commands"].as<std::vector<Command>>();
    }

    if (node["module_text"]) {
        rhs.module_text_ = node["module_text"].as<std::string>();
    }
    if (node["overwrite"]) {
        rhs.overwrite_module = node["overwrite"].as<bool>();
    }
    return true;
}

YAML::Node YAML::convert<Command>::encode(const Command& rhs) {
    YAML::Node node;

    node.push_back(rhs.action_);
    node.push_back(rhs.guard_);
    node.push_back(rhs.overwrite_);
    node.push_back(rhs.update_);

    return node;
}

bool YAML::convert<Command>::decode(const YAML::Node& node, Command& rhs) {
    if (!node.Type() == NodeType::Map) {
        return false;
    }

    rhs.action_ = node["action"].as<std::string>();
    if (node["guard"]) {
        rhs.guard_ = node["guard"].as<std::string>();
    }

    if (node["update"]) {
        rhs.update_ = node["update"].as<std::string>();
    }

    if (node["overwrite"]) {
        rhs.overwrite_ = node["overwrite"].as<bool>();
    }
    if (node["index"]) {
        try {
            rhs.indexes_ = node["index"].as<std::vector<int>>();
        }
        catch(const std::exception& e) {
            rhs.indexes_ = {node["index"].as<int>()};
        }   
    }

    return true;
}


YAML::Node YAML::convert<Label>::encode(const Label& rhs) {
    YAML::Node node;

    node.push_back(rhs.label_);
    node.push_back(rhs.text_);

    return node;
}

bool YAML::convert<Label>::decode(const YAML::Node& node, Label& rhs) {
    if (!node.Type() == NodeType::Map || !node["label"] || !node["text"]) {
        return false;
    }
    rhs.label_ = node["label"].as<std::string>();
    rhs.text_ = node["text"].as<std::string>();

    if (node["overwrite"]) {
        rhs.overwrite_ = node["overwrite"].as<bool>();
    }

    return true;
}

YAML::Node YAML::convert<Formula>::encode(const Formula& rhs) {
    YAML::Node node;

    node.push_back(rhs.content_);
    node.push_back(rhs.formula_);
    node.push_back(rhs.overwrite_);

    return node;
}

bool YAML::convert<Formula>::decode(const YAML::Node& node, Formula& rhs) {
    if (!node.IsDefined() || !node.Type() == NodeType::Map || !node["formula"] || !node["content"]) {
      return false;
    }

    rhs.formula_ = node["formula"].as<std::string>();
    rhs.content_ = node["content"].as<std::string>();

    if(node["overwrite"]) {
      rhs.overwrite_ = node["overwrite"].as<bool>();
    }

    return true;
}

YAML::Node YAML::convert<Constant>::encode(const Constant& rhs) {
    YAML::Node node;

    node.push_back(rhs.constant_);
    node.push_back(rhs.value_);
    node.push_back(rhs.type_);
    node.push_back(rhs.overwrite_);

    return node;
}

bool YAML::convert<Constant>::decode(const YAML::Node& node, Constant& rhs) {
   if (!node.IsDefined() || !node.Type() == NodeType::Map || !node["constant"] || !node["type"] || !node["value"]) {
      return false;
    }

    rhs.constant_ = node["constant"].as<std::string>();
    rhs.type_ = node["type"].as<std::string>();
    rhs.value_ = node["value"].as<std::string>();

    if(node["overwrite"]) {
      rhs.overwrite_ = node["overwrite"].as<bool>();
    }

    return true;
}

YAML::Node YAML::convert<Property>::encode(const Property& rhs) {
    YAML::Node node;
    
    node.push_back(rhs.property);
    node.push_back(rhs.value_);

    return node;
}

bool YAML::convert<Property>::decode(const YAML::Node& node, Property& rhs) {
    if (!node.IsDefined() || !node["property"] || !node["value"]) {
        return false;
    }

    rhs.property = node["property"].as<std::string>();
    try {
        rhs.value_ = node["value"].as<double>();
    }
    catch(const std::exception& e) {
        rhs.value_str_ = node["value"].as<std::string>();
    }   

    return true;
}

const std::string Configuration::configuration_identifier_ { "; // created through configuration"};
const std::string Configuration::overwrite_identifier_{"; // Overwritten through configuration"};

YamlConfigParseResult YamlConfigParser::parseConfiguration() {
        std::vector<Configuration> configuration;
        std::vector<Property> properties;

        try {
            YAML::Node config = YAML::LoadFile(file_);  
            std::vector<Label> labels;
            std::vector<Formula> formulas;
            std::vector<Module> modules;
            std::vector<Constant> constants;

            if (config["labels"]) {
                labels = config["labels"].as<std::vector<Label>>();
            }
            if (config["formulas"]) {
                formulas = config["formulas"].as<std::vector<Formula>>();
            }
            if (config["modules"]) {
                modules = config["modules"].as<std::vector<Module>>();
            }

            if (config["constants"]) {
                constants = config["constants"].as<std::vector<Constant>>();
            }

            if (config["properties"]) {
                properties = config["properties"].as<std::vector<Property>>();
            }
        
            for (auto& label : labels) {
                configuration.push_back({label.createExpression(), label.label_ , ConfigType::Label, label.overwrite_});
            }
            for (auto& formula : formulas) {
                configuration.push_back({formula.createExpression(), formula.formula_ ,ConfigType::Formula, formula.overwrite_});
            }
            for (auto& module : modules) {
                if (module.overwrite_module) {
                    Configuration config = Configuration(module.module_text_, "module " + module.module_ + "\n", ConfigType::Module, true, module.module_, {0}, "endmodule");
                    configuration.push_back(config);
                    continue;
                }
                for (auto& command : module.commands_) {
                    Configuration config;
                    if (!command.guard_.empty() && !command.action_.empty() && command.update_.empty()) {
                        config = Configuration(" " + command.guard_, command.action_, ConfigType::GuardOnly, true, module.module_, command.indexes_, "->");
                    } else if (!command.update_.empty() && !command.action_.empty() && command.guard_.empty()) {
                        config = Configuration( " " + command.update_, command.action_, ConfigType::UpdateOnly, true, module.module_,  command.indexes_, ";");
                    } else {
                        config = Configuration(command.createExpression(), command.action_, ConfigType::Module, command.overwrite_, module.module_, command.indexes_); 
                    }

                    configuration.push_back(config);
                }
            }
            for (auto& constant : constants) {
                // std::cout << constant.constant_ << std::endl;
                configuration.push_back({constant.createExpression(), "const " + constant.type_ + " " + constant.constant_, ConfigType::Constant, constant.overwrite_});
            }            
        }
        catch(const std::exception& e) {
            std::cout << "Exception '" << typeid(e).name() << "' caught:" << std::endl;
            std::cout << "\t" << e.what() << std::endl;
            std::cout << "while parsing configuration " << file_ << std::endl;
        }

        return YamlConfigParseResult(configuration, properties);
}