
#!/usr/bin/env bash

if [ -f ./build/bin/clarice ]; then
    echo "[INFO]: Running...\n"
   	
    ./build/bin/clarice # ./main.c
    
else
    echo  "[ERROR]: Could not run application: Executable does not exit or has some error. \n"
fi
