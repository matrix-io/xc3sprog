namespace eval bscan_spi {
  variable part
  variable package
  variable part_full
  
  variable path_script [file dirname [file normalize [info script]]]
  variable path_root $path_script
  variable path_hdl [file join $path_script ../]
  variable path_constraints [file join $path_script ../]
  variable path_output [file join $path_script ../]
  variable path_work
  
  variable bitstream
  
  variable hdl_files {  \
    bscan_xc7_spi.vhd \
  }
}
  
proc LOG { loglevel msg } { puts [format "%s: %s" $loglevel $msg] }
proc INFO  { msg } { LOG "INFO" $msg}
proc WARN  { msg } { LOG "WARN" $msg}
proc ERROR { msg } { LOG "ERR" $msg}

proc bscan_spi::init { { part "xc7a35t" } { package "cpg236" } } {
  INFO "Part is $part - Package is $package"
  set bscan_spi::part $part
  set bscan_spi::package $package
  set bscan_spi::part_full [format {%s%s} $bscan_spi::part $bscan_spi::package]
  
  set bscan_spi::path_work [file join $bscan_spi::path_root work $bscan_spi::part_full]
  INFO "Creating work directory $bscan_spi::path_work"
  file mkdir $bscan_spi::path_work
  cd "$bscan_spi::path_work"
  
  set bscan_spi::bitstream [format {%s%s} $bscan_spi::part_full ".bit"]
  INFO "Bitstream will be $bscan_spi::bitstream"
  
  close_design -quiet 
  close_project -quiet

  INFO "Creating in-memory project"
  create_project -part $bscan_spi::part_full -in_memory -verbose
  set_property target_language VHDL [current_project]
  set_property vhdl_version vhdl_2k [current_fileset]

  foreach { filename } $bscan_spi::hdl_files {
    set full_path [file join $bscan_spi::path_hdl $filename]
    INFO "Adding $full_path"
    read_vhdl $full_path
  }
  set full_path [file join $bscan_spi::path_constraints [format {%s%s} $bscan_spi::part_full ".xdc"]]
  INFO "Adding constraints file $full_path"
  read_xdc $full_path 
}

proc bscan_spi::compile { } {
  INFO "Synthesizing top module"
  synth_design -name smartglass -top top
  INFO "Optimizing design"
  opt_design
  INFO "Placing design"
  place_design
  INFO "Optimising design"
  phys_opt_design
  INFO "Routing design"
  route_design
  INFO "Writing bitstream"
  write_bitstream -force -verbose [file join $bscan_spi::path_output $bscan_spi::bitstream]
}
