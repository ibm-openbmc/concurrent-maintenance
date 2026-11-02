#pragma once

#include <gpiod.hpp>

#include <string>

class SiblingBmcReset
{
  public:
    explicit SiblingBmcReset(const std::string& lineName);

    void assertReset();    
    void deassertReset(); 
    
    ~SiblingBmcReset();

  private:
    std::string lineName;
    gpiod::line line;
};