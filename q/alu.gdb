b exec_alu_opcode if cmd_state == RUNNING
commands
silent
printf "opcode %s\n", opcode_defs[alu_table_index[opcode - FIRST_ALU_OP]].name
end
b scrdit.c:768 if cmd_state == RUNNING && effaddr >= 0
commands
silent
printf "pop %ld to %o\n", *val, effaddr
end
b scrdit.c:785 if cmd_state == RUNNING
commands
silent
printf "popf %g to %o\n", *val, effaddr
end
b scrdit.c:733 if cmd_state == RUNNING && effaddr >= 0
commands
silent
printf "push %ld from %o\n", val, effaddr
end
b scrdit.c:751 if cmd_state == RUNNING
commands
silent
printf "pushf %g from %o\n", val, effaddr
end
b scrdit.c:733 if cmd_state == RUNNING && effaddr < 0
commands
silent
printf "pshtab %ld from t%c\n", val+1, tab_name[thisch-FIRST_ALU_OP-num_ops]
end
b scrdit.c:768 if cmd_state == RUNNING && effaddr < 0
commands
silent
printf "poptab %ld to t%c\n", *val+1, tab_name[thisch-FIRST_ALU_OP-num_ops-NUM_TABS]
end
