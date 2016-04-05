PART=xc7a200t
PACKAGE=fbg676

SCRIPT_PATH=`dirname $0`
VIVADO_EXEC=~/Xilinx/Vivado/2015.4/bin/vivado
${VIVADO_EXEC} -mode batch -source ${SCRIPT_PATH}/build.tcl -tclargs ${PART} ${PACKAGE}