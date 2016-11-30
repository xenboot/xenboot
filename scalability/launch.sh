nr_of_expes=$2

for process in `ps -e | awk '{print $1}' | sed '1d'`; do taskset -pc 0 $process; done

for i in `seq 1 $1`; do
	rank=0
	awk -v i=$i '$5=i' configure > configure.new && mv configure.new configure
	while [ $rank -ne $nr_of_expes ]
	do
		./simultaneousBootScript.sh /home/toulouse/xen $nr_of_expes expe1
		rank=$(cat configure | cut -d' ' -f1)	
	done
	
	#replace the first position in configure file with 1
	awk '$1="1"' configure > configure.new && mv configure.new configure
done
