b exec_alu_opcode if cmd_state == RUNNING
commands
silent
printf "opcode %s\n", opcode_defs[alu_table_index[opcode - FIRST_ALU_OP]].name
end
