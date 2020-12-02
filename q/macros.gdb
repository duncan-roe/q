b scrdit.c:1622
commands
silent
printf "starting macro %o\n", thisch
end
cond 2 (thisch < 0101 || thisch > 0132) && thisch < 04000 && thisch != 255
b scrdit.c:1224
commands
silent
echo ^NI invoked\n
end
b scrdit.c:865
commands
silent
printf "resuming macro %o\n", i
end
cond 4 verb != 'I' && (curmac < FIRST_PSEUDO || curmac > LAST_PSEUDO)
