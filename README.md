How to run:  
cmake -S . -B build -DPICO_BOARD=waveshare_rp2040_one  
cmake --build build --target hello_world &&  picotool load build/hello_world.uf2 -f && picotool reboot


