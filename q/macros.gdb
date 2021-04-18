b scrdit.c:1587
commands
silent
printf "starting macro %o\n", thisch
end
b scrdit.c:1236
commands
silent
echo ^NI invoked\n
end
b scrdit.c:864 if verb != 'I' && (curmac < FIRST_PSEUDO || curmac > LAST_PSEUDO)
commands
silent
printf "resuming macro %o\n", i
end
b scrdit.c:906 if (int16_t)thisch < 0
commands
silent
printf "jumping back %d characters\n", -(int16_t)thisch
end
