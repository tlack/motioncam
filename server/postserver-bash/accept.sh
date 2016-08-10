. ../CONFIG.sh
out=$DIR/`date +%Y%m%d-%H%M%S%N`.jpg
nc -vvvvvvv -l $HOST $POSTPORT >$out.tmp && \
				test -s $out.tmp && \
				convert $out.tmp $IMAGEMAGICKOPTS $out && \
				rm $out.tmp
date
ls -l $out

