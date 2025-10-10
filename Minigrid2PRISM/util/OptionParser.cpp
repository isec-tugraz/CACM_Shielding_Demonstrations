#include <iostream>

#include "popl.hpp"
#include "OptionParser.h"

namespace io {
  int printPoplException(const popl::invalid_option &e) {
    std::cerr << "Invalid Option Exception: " << e.what() << "\n";
		std::cerr << "error:  ";
		if (e.error() == popl::invalid_option::Error::missing_argument) {
			std::cerr << "missing_argument\n";
    } else if (e.error() == popl::invalid_option::Error::invalid_argument) {
			std::cerr << "invalid_argument\n";
    } else if (e.error() == popl::invalid_option::Error::too_many_arguments) {
			std::cerr << "too_many_arguments\n";
    } else if (e.error() == popl::invalid_option::Error::missing_option) {
			std::cerr << "missing_option\n";
    }

		if (e.error() == popl::invalid_option::Error::missing_option) {
      std::string option_name(e.option()->name(popl::OptionName::short_name, true));
			if (option_name.empty())
				option_name = e.option()->name(popl::OptionName::long_name, true);
			std::cerr << "option: " << option_name << "\n";
		}
		else {
			std::cerr << "option: " << e.option()->name(e.what_name()) << "\n";
			std::cerr << "value:  " << e.value() << "\n";
		}
		return EXIT_FAILURE;
  }
}
