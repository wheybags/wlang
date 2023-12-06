#include "OutputString.hpp"

void OutputString::appendLine(std::string_view line)
{
  if (line == "}" || line == "};")
    this->outdent();

  for (int32_t i = 0; i < this->indentCount; i++)
    this->str += ' ';

  this->str += line;
  this->str += '\n';

  if (line == "{")
    this->indent();
}