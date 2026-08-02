
#!/usr/bin/env bash

echo "[CONFIG]: Are you generating ninja files? (y or n)(defaults: n)"
read -p " > " build
echo "[CONFIG]: Do you want to run the project right after building it? (y or n)(defaults: n)"
read -p " > " run

echo "[INFO]: Creating build folder..."

mkdir -p ./build && cd ./build

if [ "$build" = "y" ]; then
	echo "[INFO]: Generating Ninja files..."
	cmake .. -G Ninja -DCMAKE_C_COMPILER=gcc
fi

echo "[INFO]: Building..."
ninja

cd ..

if  [ "$run" = "y" ]; then
    echo "\n--------------------------------------------------\n"
    ./run.sh
fi

