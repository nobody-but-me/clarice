
#!/usr/bin/env bash

# if [ "$(type -t ./build/bin/mol)" = "file" ]; then
if [ -f ./build/bin/prototype ]; then
    echo "[INFO]: RUNNING...\n"
    
    cd ./build/bin/
    
    ./prototype $1
    
    cd ..
else
    echo "[ERROR]: COULD NOT RUN ENGINE: EXECUTABLE DOES NOT EXIST OR HAS SOME ERROR. \n"
fi
