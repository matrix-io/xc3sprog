library IEEE;
use IEEE.STD_LOGIC_1164.ALL;
use IEEE.STD_LOGIC_ARITH.ALL;
use IEEE.STD_LOGIC_UNSIGNED.ALL;

library UNISIM;
use UNISIM.VComponents.all;

entity top is
	port (
		MOSI_ext	: out std_logic;
		CSB_ext		: out std_logic
	);
end top;

architecture Behavioral of top is
	signal CAPTURE: std_logic;
	signal UPDATE: std_logic;
	signal DRCK1: std_logic;
	signal TDI: std_logic;
	signal TDO1: std_logic;
	signal CSB: std_logic := '1';
	signal header: std_logic_vector(47 downto 0);
	signal len: std_logic_vector(15 downto 0);
	signal have_header : std_logic := '0';
	signal MISO: std_logic;
	signal MOSI: std_logic;
	signal SEL1: std_logic;
	signal SHIFT: std_logic;
	signal RESET: std_logic;
	signal CS_GO: std_logic := '0';
	signal CS_GO_PREP: std_logic := '0';
	signal CS_STOP: std_logic := '0';
	signal CS_STOP_PREP: std_logic := '0';
	signal RAM_RADDR: std_logic_vector(13 downto 0);
	signal RAM_WADDR: std_logic_vector(13 downto 0);
	signal DRCK1_INV : std_logic;
	signal RAM_DO: std_logic_vector(0 downto 0);
	signal RAM_DI: std_logic_vector(0 downto 0);
	signal RAM_WE: std_logic := '0';
begin

	MOSI_ext	<= MOSI;
	CSB_ext		<= CSB;

	DRCK1_INV <= not DRCK1;

   RAMB16_S1_S1_inst : RAMB16_S1_S1
   port map (
	   DOA => RAM_DO,      -- Port A 1-bit Data Output
      DOB => open,      -- Port B 1-bit Data Output
      ADDRA => RAM_RADDR,  -- Port A 14-bit Address Input
      ADDRB => RAM_WADDR,  -- Port B 14-bit Address Input
      CLKA => DRCK1_inv,    -- Port A Clock
      CLKB => DRCK1,    -- Port B Clock
      DIA => "0",      -- Port A 1-bit Data Input
      DIB => RAM_DI,      -- Port B 1-bit Data Input
      ENA => '1',      -- Port A RAM Enable Input
      ENB => '1',      -- PortB RAM Enable Input
      SSRA => '0',    -- Port A Synchronous Set/Reset Input
      SSRB => '0',    -- Port B Synchronous Set/Reset Input
      WEA => '0',      -- Port A Write Enable Input
      WEB => RAM_WE       -- Port B Write Enable Input
   );

	BSCAN_VIRTEX5_inst : BSCAN_VIRTEX5
	generic map (
		JTAG_CHAIN => 1			-- Value for USER command. Possible values: (1,2,3 or 4)
		) 
	port map (
		CAPTURE 	=> CAPTURE, 	-- CAPTURE output from TAP controller
		DRCK 		=> DRCK1, 		-- Data register output for USER functions
		RESET 	=> RESET,		-- Reset output from TAP controller
		SEL 		=> SEL1, 		-- USER active output
		SHIFT 	=> SHIFT, 		-- SHIFT output from TAP controller
		TDI 		=> TDI, 			-- TDI output from TAP controller
		UPDATE 	=> UPDATE,		-- UPDATE output from TAP controller
		TDO 		=> TDO1 			-- Data input for USER function
	);

	-- see XAPP1020
	STARTUP_VIRTEX5_inst : STARTUP_VIRTEX5
	port map (
		CFGCLK 		=> open, -- Config logic clock 1-bit output
		CFGMCLK 		=> open, -- Config internal osc clock 1-bit output
		DINSPI 		=> MISO, -- DIN SPI PROM access 1-bit output
		EOS 			=> open, -- End of Startup 1-bit output
		TCKSPI 		=> open, -- TCK SPI PROM access 1-bit output
		CLK 			=> open, -- Clock input for start-up sequence
		GSR 			=> '0', -- Global Set/Reset input (GSR cannot be used for the port name)
		GTS 			=> '0', -- Global 3-state input (GTS cannot be used for the port name)
		USRCCLKO 	=> DRCK1, -- User CCLK 1-bit input
		USRCCLKTS 	=> '0', -- User CCLK 3-state, 1-bit input
		USRDONEO 	=> open, -- User Done 1-bit input
		USRDONETS 	=> open -- User Done 3-state, 1-bit input
	);

	MOSI <= TDI;
	
	CSB <= '0' when CS_GO = '1' and CS_STOP = '0' else '1';

	RAM_DI <= MISO & "";

	TDO1 <= RAM_DO(0);

	-- falling edges
	process(DRCK1, CAPTURE, RESET, UPDATE, SEL1)
	begin
	
		if CAPTURE = '1' or RESET='1' or UPDATE='1' or SEL1='0' then
		
			have_header <= '0';
			
			-- disable CSB
			CS_GO_PREP <= '0';
			CS_STOP <= '0';
									
		elsif falling_edge(DRCK1) then
					
			-- disable CSB?
			CS_STOP <= CS_STOP_PREP;
			
			-- waiting for header?
			if have_header='0' then
				
				-- got magic + len
				if header(46 downto 15) = x"59a659a6" then
					len <= header(14 downto 0) & "0";
					have_header <= '1';
										
					-- enable CSB on rising edge (if len > 0?)
					if (header(14 downto 0) & "0") /= x"0000" then
						CS_GO_PREP <= '1';
					end if;
					
				end if;

			elsif len /= x"0000" then
				len <= len - 1;
			
			end if;
			
		end if;
		
	end process;
	
	-- rising edges
	process(DRCK1, CAPTURE, RESET, UPDATE, SEL1)
	begin
	
		if CAPTURE = '1' or RESET='1' or UPDATE='1' or SEL1='0' then
	
			-- disable CSB
			CS_GO <= '0';
			CS_STOP_PREP <= '0';
			
			RAM_WADDR <= (others => '0');
			RAM_RADDR <= (others => '0');
			RAM_WE <= '0';
				
		elsif rising_edge(DRCK1) then
					
			RAM_RADDR <= RAM_RADDR + 1;
			
			RAM_WE <= not CSB;
			
			if RAM_WE='1' then
				RAM_WADDR <= RAM_WADDR + 1;
			end if;
			
			header <= header(46 downto 0) & TDI;
			
			-- enable CSB?
			CS_GO <= CS_GO_PREP;
			
			-- disable CSB on falling edge
			if CS_GO = '1' and len = x"0000" then
				CS_STOP_PREP <= '1';
			end if;
			
		end if;
	
	end process;
	
end Behavioral;
