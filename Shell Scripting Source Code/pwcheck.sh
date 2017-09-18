#!/bin/bash
#Password Strength Checker
#Get password from file and store as a variable
password=$(<$1)
if [ ${#password} -lt 6 ] || [ ${#password} -gt 32 ]; then
	echo "Error: Password length invalid."
else
	SCORE=${#password}	#num char in string
	if egrep -q ["#""$""+""%""@"] $1; then
		let SCORE=SCORE+5
	fi
	if egrep -q [0-9] $1; then
		let SCORE=SCORE+5
	fi
	if egrep -q [A-Za-z] $1; then
		let SCORE=SCORE+5
	fi
	if egrep -q "([A-Za-z0-9])\1+" $1; then
		let SCORE=SCORE-10
	fi
	if egrep -q [a-z][a-z][a-z] $1; then
		let SCORE=SCORE-3
	fi
	if egrep -q [A-Z][A-Z][A-Z] $1; then
		let SCORE=SCORE-3
	fi
	if egrep -q [0-9][0-9][0-9] $1; then
		let SCORE=SCORE-3
	fi
	echo "Password Score:" $SCORE
fi
