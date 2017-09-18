#!/bin/bash
#Backup Script

#This part makes sure inputs are valid
if [ $# -ne 4 ]; then
	echo "Please input 4 arguments, file backupdir interval-secs max-backups"
	exit
fi
if [ ! -f "$1" ]; then
	echo "First argument is not a file or file doesn't exist"
	exit
fi
if [ ! -d "$2" ]; then
	echo "Second argument is not a directory or directory doesn't exist"
	exit
fi

#File Backup
NUMBACKUPFILES=`ls $2 | wc -l`
#if max num of backups already used
if [ $NUMBACKUPFILES -ge $4 ]; then
	NUMFILESTODEL=`expr $NUMBACKUPFILES - $4`
	let NUMFILESTODEL=NUMFILESTODEL+1
	while [ $NUMFILESTODEL -gt 0 ]; do
		rm $2/"`ls $2 | head -n 1`"
		let NUMFILESTODEL=NUMFILESTODEL-1
	done
fi
#copy file into backup dir
cp $1 $2/"`date +%Y-%m-%d-%H-%M-%S`-$1"
while [ 1 ]; do
	sleep $3
	#stores the difference between the input file and most recent backup file
	DIFF=$(diff $1 ./$2/"`ls -r $2 | head -n 1`")
	if [ "$DIFF" != "" ]; then
		NUMBACKUPFILES=`ls $2 | wc -l`
		#if you're already using the max num of backups
		if [ $NUMBACKUPFILES -ge $4 ]; then
			NUMFILESTODEL=`expr $NUMBACKUPFILES - $4`
			let NUMFILESTODEL=NUMFILESTODEL+1
			while [ $NUMFILESTODEL -gt 0 ]; do
				rm $2/"`ls $2 | head -n 1`"
				let NUMFILESTODEL=NUMFILESTODEL-1
			done
		fi
		#copy file into backup dir because there is a change
		cp $1 $2/"`date +%Y-%m-%d-%H-%M-%S`-$1"
		#send email
		echo "Your file was updated, here are the changes:" > tmpmsg
		echo "$DIFF" >> tmpmsg
		/usr/bin/mailx -s "Updated File" $USER < tmpmsg
		rm tmpmsg
	fi
done
