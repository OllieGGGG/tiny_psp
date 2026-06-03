How to run:  
git submodule update --init --recursive
cmake -S . -B build -DPICO_BOARD=waveshare_rp2040_one  
cmake --build build --target tiny_psp &&  picotool load build/tiny_psp.uf2 -f && picotool reboot

