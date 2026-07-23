b exec_alu_opcode if cmd_state == RUNNING
commands
silent
printf "opcode %s\n", opcode_defs[alu_table_index[opcode - FIRST_ALU_OP]].name
end
b scrdit.c:768 if cmd_state == RUNNING
commands
silent
printf "<POP %ld to %lo>\n", *val, ((long)val - (long)ALU_memory)/sizeof *ALU_memory
end
