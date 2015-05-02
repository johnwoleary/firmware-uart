TOP= rtfSimpleUart
VERILOG_FILES= $(TOP).v rtfSimpleUart.v rtfSimpleUartTx.v rtfSimpleUartRx.v edge_det.v
C_FILES= main.c
BOUND=30

verify: $(C_FILES) $(VERILOG_FILES) $(TOP).h
	/bin/rm -f $(TOP).vcd
	hw-cbmc $(C_FILES) $(VERILOG_FILES) --module $(TOP) --bound $(BOUND) --vcd $(TOP).vcd

debug: $(TOP).vcd
	gtkwave $(TOP).vcd

$(TOP).h: $(TOP).v
	hw-cbmc $(VERILOG_FILES) --module $(TOP) --gen-interface | sed -n '/Unwinding Bound/,$$p' > $(TOP).h

clean:
	rm -f $(TOP).h $(TOP).vcd
