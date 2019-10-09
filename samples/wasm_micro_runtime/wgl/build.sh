mkdir -p build
cd build
source ../../../../zephyr-env.sh
cmake -GNinja -DBOARD=nucleo_f746zg ..
ninja
