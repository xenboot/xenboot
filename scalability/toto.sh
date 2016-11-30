nr_vm=2
instantDebut=-1
for new_instantDebut in `grep -w main_create logCreation |grep "Enter" |cut -d' ' -f 5`
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
        i=0
        while [ "$i" != "$nr_vm" ]
        do
                i=`expr $i + 1`
                j=0
                for debut in `grep -w $fonction logCreation |grep "Enter" |grep "domain_config$i" |cut -d' ' -f 5`
                do
                        j=`expr $j + 1`
                        fin=$(grep -w $fonction logCreation |grep "Exit" |grep "domain_config$i" |cut -d' ' -f 5 |sed "${j}q;d")
                        echo "----------------------------------$fin---------------------$debut----$fonction"
                        duree=`expr $fin - $debut`
                        instantPassage=`expr $debut - $instantDebut`
                done
        done
done
