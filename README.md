How to run:  
git submodule update --init --recursive
cmake -S . -B build -DPICO_BOARD=waveshare_rp2040_one  
cmake --build build --target tiny_psp &&  picotool load build/tiny_psp.uf2 -f --ser {SERIAL_NUMBER} && picotool reboot

Board 1 serial number: E464C401431C5328
Board 2 serial number: E464C40143501721 

