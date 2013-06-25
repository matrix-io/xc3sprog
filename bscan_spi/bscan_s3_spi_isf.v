module top
  (
   input gnd
   );
   wire   CAPTURE;
   wire   UPDATE;
   wire   DRCK1;
   wire   TDI;
   wire   TDO1;
   wire   CSB;
   reg [47:0] header;
   reg [15:0]  len;
   reg 	       have_header = 0;
   wire        MISO;
   wire        MOSI = TDI;
   wire        SEL1;
   wire        SHIFT;
   wire        RESET;
   reg 	       CS_GO = 0;
   reg 	       CS_GO_PREP = 0;
   reg 	       CS_STOP = 0;
   reg 	       CS_STOP_PREP = 0;
   reg [13:0] RAM_RADDR;
   reg [13:0] RAM_WADDR;
   wire        DRCK1_INV = !DRCK1;
   wire        RAM_DO;
   wire        RAM_DI;
   reg 	       RAM_WE = 0;

   RAMB16_S1_S1 RAMB16_S1_S1_inst
     (
      .DOA(RAM_DO),
      .DOB(),
      .ADDRA(RAM_RADDR),
      .ADDRB(RAM_WADDR),
      .CLKA(DRCK1_INV),
      .CLKB(DRCK1),
      .DIA(1'b0),
      .DIB(RAM_DI),
      .ENA(1'b1),
      .ENB(1'b1),
      .SSRA(1'b0),
      .SSRB(1'b0),
      .WEA(1'b0),
      .WEB(RAM_WE)
      );

   BSCAN_SPARTAN3A BSCAN_SPARTAN3A_inst
     (
      .CAPTURE(CAPTURE),
      .DRCK1(DRCK1),
      .DRCK2(),
      .RESET(RESET),
      .SEL1(SEL1),
      .SEL2(),
      .SHIFT(SHIFT),
      .TCK(),
      .TDI(TDI),
      .TMS(),
      .UPDATE(UPDATE),
      .TDO1(TDO1),
      .TDO2(1'b0)
      );
   SPI_ACCESS
     #(.SIM_DEVICE("3S50AN")
       )
       SPI_ACCESS_inst
	 (
	  .MISO(MISO),
	  .CLK(DRCK1),
	  .CSB(CSB),
	  .MOSI(MOSI)
	  );
`include "bscan_common.v"
endmodule
