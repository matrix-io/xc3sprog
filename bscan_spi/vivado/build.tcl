if { [llength $argv] != 2 } {
  puts "Usage: [info script] <part> <package>"
  puts " "
  puts "Where "
  puts "   part        : FPGA part to compile for, i.e. xc7a200t"
  puts "   pacakge     : FPGA package, i.e. fbg676"
  puts " "
  error "Insufficient arguments";
}
set part [lindex $argv 0]
set package [lindex $argv 1]

set path_script [file dirname [file normalize [info script]]]
source -quiet [file join $path_script bscan_spi.tcl]

bscan_spi::init $part $package
bscan_spi::compile
