// Copyright (c) 2015, 2017 The ComPWA Team.
// This file is part of the ComPWA framework, check
// https://github.com/ComPWA/ComPWA/license.txt for details.

#ifndef LOGGING_HPP_
#define LOGGING_HPP_

#include "easylogging++.h"

namespace ComPWA {

///
/// \class Logging
/// Logging class privides an interface for logging all over the framework.
/// Behind the scenes boost::log is currently used which allows a detailed
/// on logging format and log levels
///

class Logging {
public:
  Logging(std::string outFileName = "output.log",
          std::string minLevel = "DEBUG");

  void setLogLevel(std::string minLevel);
  
};

} // namespace ComPWA

#endif
