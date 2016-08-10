. ../CONFIG.sh
echo $HOST $POSTPORT $DIR | tee -a $LOG
while :;
do
	sh accept.sh 2>&1 | tee -a $LOG
done
