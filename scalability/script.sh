xenSource=$1
cd $xenSource/scalability
rank=$(cat configure | cut -d' ' -f1)
mem=$(cat configure | cut -d' ' -f2)
nr_vcpu=$(cat configure | cut -d' ' -f3)
nr_idle=$(cat configure | cut -d' ' -f4)
resultFile="results/$3/boot.$rank.$mem.$nr_vcpu.$nr_idle"
mkdir -p results/$3
rm $resultFile
if [ "1" == "$rank" ]
then
	mem=32
fi
xl destroy Mini-OS
sleep 2
cd $xenSource/extras/mini-os/
echo 'kernel = "mini-os.gz"' > $xenSource/extras/mini-os/domain_config
echo "memory = $mem" >> $xenSource/extras/mini-os/domain_config
echo "vcpus = $nr_vcpu" >> $xenSource/extras/mini-os/domain_config
echo 'name = "Mini-OS"' >> $xenSource/extras/mini-os/domain_config
echo "on_crash = 'destroy'" >> $xenSource/extras/mini-os/domain_config

if [ "1" == "$rank" ]
then
	#Avoid the first boot, which takes a lot of time due to freemem
        xl create $xenSource/extras/mini-os/domain_config 2> $xenSource/scalability/logCreation
	cd -
        newRank=`expr $rank + 1`
	mem=$(cat configure | cut -d' ' -f2)
        echo "$newRank $mem $nr_vcpu $nr_idle" > configure
	exit
fi

xl create $xenSource/extras/mini-os/domain_config 2> $xenSource/scalability/logCreation

cd $xenSource/scalability
xl list Mini-OS > toto
toto=$(wc -l toto |cut -d' ' -f1)
if [ "$toto" == "0" ] 
then
	echo "VM not created=$toto="
	exit
fi
instantDebut=$(grep -w main_create logCreation |grep "Enter" |cut -d' ' -f 4)
echo "Time-$instantDebut function startTime endTime duration" > $resultFile

for fonction in `cat targetFunctions`
do
	debut=$(grep -w $fonction logCreation |grep "Enter" |cut -d' ' -f 4)
	fin=$(grep -w $fonction logCreation |grep "Exit" |cut -d' ' -f 4)
	duree=`expr $fin - $debut`
	instantPassage=`expr $debut - $instantDebut`
        echo "$instantPassage $fonction $debut $fin $duree" >> $resultFile

done
if [ "$2" == "$rank" ]
then
	echo "End of experiment"
	echo "1 $mem $nr_vcpu $nr_idle" > configure
else
	newRank=`expr $rank + 1`
	echo "$newRank $mem $nr_vcpu $nr_idle" > configure
#	reboot
fi
#java ComputeEachDuration targetFunctions
#grep -w main_create results/boot.*/expe1/boot.2* |cut -d'.' -f2,8 |cut -d' ' -f1,5