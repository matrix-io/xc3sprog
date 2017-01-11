PART=xc7a35t
PACKAGE=cpg236

SCRIPT_PATH=`dirname $0`
VIVADO_EXEC=/opt/Xilinx/Vivado/2016.2/bin/vivado
${VIVADO_EXEC} -mode batch -source ${SCRIPT_PATH}/build.tcl -tclargs ${PART} ${PACKAGE}
