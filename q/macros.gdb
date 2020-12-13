b scrdit.c:1576
commands
silent
printf "starting macro %o\n", thisch
end
b scrdit.c:1232
commands
silent
echo ^NI invoked\n
end
b scrdit.c:864
commands
silent
printf "resuming macro %o\n", i
end
cond 4 verb != 'I' && (curmac < FIRST_PSEUDO || curmac > LAST_PSEUDO)
b scrdit.c:906
commands
silent
printf "jumping back %d characters\n", -(int16_t)thisch
end
cond 5 (int16_t)thisch < 0
