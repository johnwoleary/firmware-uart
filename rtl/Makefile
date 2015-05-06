TOP= rtfSimpleUart
VERILOG_FILES= $(TOP).v rtfSimpleUart.v rtfSimpleUartTx.v rtfSimpleUartRx.v edge_det.v
BOUND=1000

loopback_int: loopback_int.c $(VERILOG_FILES) $(TOP).h
	/bin/rm -f $(TOP).vcd
	hw-cbmc loopback_int.c $(VERILOG_FILES) --module $(TOP) --bound $(BOUND) --vcd $(TOP).vcd

loopback: loopback.c $(VERILOG_FILES) $(TOP).h
	/bin/rm -f $(TOP).vcd
	hw-cbmc loopback.c $(VERILOG_FILES) --module $(TOP) --bound $(BOUND) --vcd $(TOP).vcd

tx_two_bytes: tx_two_bytes.c $(VERILOG_FILES) $(TOP).h
	/bin/rm -f $(TOP).vcd
	hw-cbmc tx_two_bytes.c $(VERILOG_FILES) --module $(TOP) --bound $(BOUND) --vcd $(TOP).vcd

debug: $(TOP).vcd
	gtkwave $(TOP).vcd waves.gtkw

$(TOP).h: $(TOP).v
	hw-cbmc $(VERILOG_FILES) --module $(TOP) --gen-interface | sed -n '/Unwinding Bound/,$$p' > $(TOP).h

clean:
	rm -f $(TOP).h $(TOP).vcd
