/* from bscan_s6_spi_isf.v */
    assign      CSB = !(CS_GO && !CS_STOP);

   assign      RAM_DI = MISO;
   assign      TDO1 = RAM_DO;

   wire        rst = CAPTURE || RESET || UPDATE || !SEL1;

   always @(negedge DRCK1 or posedge rst)
     if (rst)
       begin
	  have_header <= 0;
	  CS_GO_PREP <= 0;
	  CS_STOP <= 0;
       end
     else
       begin
	  CS_STOP <= CS_STOP_PREP;
	  if (!have_header)
	    begin
	       if (header[46:15] == 32'h59a659a6)
		 begin
		    len <= {header [14:0],1'b0};
		    have_header <= 1;
		    if ({header [14:0],1'b0} != 0)
		      begin
			 CS_GO_PREP <= 1;
		      end
		 end
	    end
	  else if (len != 0)
	    begin
	       len <= len -1;
	    end // if (!have_header)
       end // else: !if(CAPTRE || RESET || UPDATE || !SEL1)

   reg         reset_header = 0;
   always @(posedge DRCK1 or posedge rst)
     if (rst)
       begin
	  CS_GO <= 0;
	  CS_STOP_PREP <= 0;
	  RAM_WADDR <= 0;
	  RAM_RADDR <=0;
	  RAM_WE <= 0;
          reset_header <= 1;
       end
     else
       begin
	  RAM_RADDR <= RAM_RADDR + 1;
	  RAM_WE <= !CSB;
	  if(RAM_WE)
	    RAM_WADDR <= RAM_WADDR + 1;

          reset_header <=0;
          //For the next if, the value of reset_header is probed at rising
          //clock, before "reset_header<=0" is executed:
          if(reset_header)
             header <={47'h000000000000, TDI};
          else
             header <= {header[46:0], TDI};
	  CS_GO <= CS_GO_PREP;
	  if (CS_GO && (len == 0))
	    CS_STOP_PREP <= 1;
       end // else: !if(CAPTURE || RESET || UPDATE || !SEL1)
