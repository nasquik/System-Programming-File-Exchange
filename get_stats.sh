#!/bin/bash

var=$(cat)
#echo "$var"
filesSent=0
filesReceived=0
minClient=9999999
maxClient=0
totalClients=0
exitedClients=0
bytesSent=0
bytesReceived=0

declare -a ListOfClients

while read -r word1 word2 word3 word4
do
    if [ "$word1" == "Client" ]; then

        if [ "$word3" == "exited" ]; then
            exitedClients=$(( exitedClients + 1 ))
        elif [ "$word3" == "created" ]; then
            ListOfClients+=("$word2")
            totalClients=$(( totalClients + 1 ))

            if [ "$word2" -lt "$minClient" ]; then 
                minClient="$word2"
            fi

            if [ "$word2" -gt "$maxClient" ]; then 
                maxClient="$word2"
            fi
        fi

    elif [ "$word1" == "s" ]; then
        if [ "$word2" == "f" ]; then
            filesSent=$(( filesSent + 1 ))
        fi

        bytesSent=$(( bytesSent + word4 ))

    elif [ "$word1" == "r" ]; then
        if [ "$word2" == "f" ]; then
            filesReceived=$(( filesReceived + 1 ))
        fi

        bytesReceived=$(( bytesReceived + word4 ))
    fi

done <<< "$var"

echo "Number of Clients: $totalClients"
echo "List Of Clients:"
for client in "${ListOfClients[@]}"; do
    echo "$client"
done
echo "Min Client: $minClient"
echo "Max Client: $maxClient"
echo "Bytes Sent: $bytesSent"
echo "Bytes Received: $bytesReceived"
echo "Files Sent: $filesSent"
echo "Files Received: $filesReceived"
echo "Clients that exited: $exitedClients"
