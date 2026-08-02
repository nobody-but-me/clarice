
#!/usr/bin/env bash

if [ -f ./build/bin/pessoa ]; then
    echo "[INFO]: Running...\n"
   	
    ./build/bin/pessoa
    
else
    echo  "[ERROR]: Could not run application: Executable does not exit or has some error. \n"
fi
