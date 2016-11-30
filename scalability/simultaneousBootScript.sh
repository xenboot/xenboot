xenSource=$1
cd $xenSource/scalability
rank=$(cat configure | cut -d' ' -f1)
mem=$(cat configure | cut -d' ' -f2)
nr_vcpu=$(cat configure | cut -d' ' -f3)
nr_idle=$(cat configure | cut -d' ' -f4)
nr_vm=$(cat configure | cut -d' ' -f5)
resultFile="$xenSource/scalability/results/boot.$mem.$nr_vcpu.$nr_idle.$nr_vm/$3/boot.$rank.$mem.$nr_vcpu.$nr_idle.$nr_vm"
mkdir -p $xenSource/scalability/results/boot.$mem.$nr_vcpu.$nr_idle.$nr_vm/$3
rm $resultFile
rm $xenSource/extras/mini-os/logTh*

if [ "1" == "$rank" ]
then
	mem=32
fi
cmd=""
i=0

while [ "$i" != "$nr_vm" ]
do
	i=`expr $i + 1`
	cmd="Mini-OS$i "$cmd  
done
xl create-client destroy $cmd
sleep $(($nr_vm * 2))
cd $xenSource/extras/mini-os/

cmd=""
i=0

while [ "$i" != "$nr_vm" ]
do
	i=`expr $i + 1`
	echo 'kernel = "mini-os.gz"' > $xenSource/extras/mini-os/domain_config$i
	echo "memory = $mem" >> $xenSource/extras/mini-os/domain_config$i
	echo "vcpus = $nr_vcpu" >> $xenSource/extras/mini-os/domain_config$i
	echo 'name = "Mini-OS'$i'"' >> $xenSource/extras/mini-os/domain_config$i
	echo "on_crash = 'destroy'" >> $xenSource/extras/mini-os/domain_config$i
	cmd="$xenSource/extras/mini-os/domain_config$i "$cmd
done

#NOT NECESARY IN 4.7
#Avoid the first boot, which takes a lot of time due to freemem
#if [ "1" == "$rank" ]
#then
#	xl multiple-create $xenSource/extras/mini-os/domain_config1 > $xenSource/scalability/logCreation
#	cd -
#   	newRank=`expr $rank + 1`
#	mem=$(cat configure | cut -d' ' -f2)
#	echo "$newRank $mem $nr_vcpu $nr_idle $nr_vm" > configure
#	exit
#fi

xl create-client create $cmd 
sleep $(($nr_vm * 5))
#cd $xenSource/scalability
xl list > toto
toto=$(wc -l toto | cut -d' ' -f1)
nbLine=`expr $nr_vm + 2`

if [ $toto -ne $nbLine ] 
then
	echo "VMs not created $toto!=$nbLine"
	exit
fi

#awk -F " " '{if ($0 ~ /^[0-9]+/) print $0 >> ("logTh" $1)}' logCreation

instantDebut=-1

for new_instantDebut in `grep -w main_create logTh* |grep "Enter" |cut -d' ' -f 5`
do
	if [ $instantDebut -eq -1 ]
	then
		instantDebut=$new_instantDebut
	elif [ $new_instantDebut -lt $instantDebut ]
	then
		instantDebut=$new_instantDebut
	fi
done

echo "Time-$instantDebut function startTime endTime duration" > $resultFile

for fonction in `cat targetFunctions`
do
	min=-1
	max=-1

	for f in logTh*; do
		debut=$(grep -w $fonction $f |grep "Enter" | cut -d' ' -f 5)
		fin=$(grep -w $fonction $f |grep "Exit" | cut -d' ' -f 5)
		duree=`expr $fin - $debut`
		instantPassage=`expr $debut - $instantDebut`
		echo "$instantPassage $fonction $debut $fin $duree" >> $resultFile
		if [[ $min -eq -1 ]]
                then
                	min=$duree
                	max=$min
                elif [[ $duree -lt $min ]]
                then
                	min=$duree
                elif [[ $max -lt $duree ]]
                then
                	max=$duree
               	fi
		#echo "----------------------------------$fin---------------------$debut----$fonction"	
	done
	echo "$instantPassage $fonction min_max $min $max" >> $resultFile

done

if [ "$2" == "$rank" ]
then
	echo "End of experiment"
	echo "1 $mem $nr_vcpu $nr_idle $nr_vm" > $xenSource/scalability/configure
else
	echo "newRank:$newRank"
	newRank=`expr $rank + 1`
	echo "$newRank $mem $nr_vcpu $nr_idle $nr_vm" > $xenSource/scalability/configure
#	reboot
fi

#java ComputeEachDuration targetFunctions
#grep -w main_create results/boot.*/expe1/boot.2* |cut -d'.' -f2,8 |cut -d' ' -f1,5
