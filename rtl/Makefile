TOP= rtfSimpleUart
VERILOG_FILES= $(TOP).v rtfSimpleUart.v rtfSimpleUartTx.v rtfSimpleUartRx.v edge_det.v

flowcontrol: flowcontrol.c $(VERILOG_FILES) $(TOP).h
	/bin/rm -f $(TOP).vcd
	hw-cbmc flowcontrol.c $(VERILOG_FILES) --module $(TOP) --bound 800 --vcd $(TOP).vcd

loopback_int: loopback_int.c $(VERILOG_FILES) $(TOP).h
	/bin/rm -f $(TOP).vcd
	hw-cbmc loopback_int.c $(VERILOG_FILES) --module $(TOP) --bound 4000 --vcd $(TOP).vcd

loopback: loopback.c $(VERILOG_FILES) $(TOP).h
	/bin/rm -f $(TOP).vcd
	hw-cbmc loopback.c $(VERILOG_FILES) --module $(TOP) --bound 800 --vcd $(TOP).vcd

tx_two_bytes: tx_two_bytes.c $(VERILOG_FILES) $(TOP).h
	/bin/rm -f $(TOP).vcd
	hw-cbmc tx_two_bytes.c $(VERILOG_FILES) --module $(TOP) --bound 800 --vcd $(TOP).vcd

debug: $(TOP).vcd
	gtkwave $(TOP).vcd waves.gtkw

$(TOP).h: $(TOP).v
	hw-cbmc $(VERILOG_FILES) --module $(TOP) --gen-interface | sed -n '/Unwinding Bound/,$$p' > $(TOP).h

clean:
	rm -f $(TOP).h $(TOP).vcd
