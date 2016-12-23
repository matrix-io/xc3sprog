#include "xc3sprog.h"


int main(int argc, char **args) {
  //  detect_chain();
  fpga_program(std::string(args[1]));
}

